/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_task_wdt.h"

#include "R502Interface.h"

#define FL_INPUT_READY      0x01
#define FL_SUCCESS          0x02

#define FL_FP_0             0x04
#define FL_FP_1             0x08
#define FL_FP_01            0x0C
#define FL_PIN              0x10
#define FL_PRIVILEGE        0x20

#define FL_FSM              0xC0
#define FL_VERIFYUSER       0x40
#define FL_DELETEPROFILE    0x80
#define FL_ADDPROFILE       0xC0

#define MAX_PROFILES 200

typedef struct profile_t {
    uint8_t isUsed;
    uint8_t PIN[4];
    uint8_t privilege;
} profile_t; // index of array = position

bool isAdmin; // currently in admin mode (from successin verifying admin PIN/FP)
bool accessAdmin; // currently seeking admin mode (by pressing admin query button)
uint8_t flags = 0; // used to track inputs

// ------TEST ONLY-------------
profile_t profiles[MAX_PROFILES] = {
    {
        .isUsed = 1,    // Slot 0. Stored profile
        .PIN = {1, 2, 3, 4},
        .privilege = 1,
    },
    {
        .isUsed = 0,    // Slot 1. Deallocated. Stuff remains
        .PIN = {5, 6, 7, 8},
        .privilege = 0,
    },                  /// Slots 2-199. Deallocated. Not seen
};

// Sample input PIN and privilege
uint8_t pin1[4] = {1, 2, 3, 4}; // user
uint8_t pin2[4] = {5, 6, 7, 8}; // admin
uint8_t pin3[4] = {1, 2, 4, 8}; // invalid
uint8_t *pinEnter = pin3;
uint8_t privEnter = 1;          // user-type

// Temp storage for profile creation
uint8_t fingerprintBuffer[R502_TEMPLATE_SIZE];    // temp storage for fingerprint template (addProfile, init)
uint8_t pinBuffer[4];           // temp storage for PIN (addProfile)
uint8_t privBuffer;             // temp storage for privilege (addProfile)
// --------------END TEST-----------------

/**----------------------------------------------------------------------
 * SUBSYSTEM    : profileRecog
 * Author       : Joel Taina
 * Components   : 
 *      - R502-interface (R503 scanner)
 *      - SD card flash memory
 * Description  : Profile Recognition fetches user profile information to verify
 *      an inserted fingerprint or PIN.
 *      In admin mode, the functionalities of addProfile and deleteProfile
 *      allow the admin to add users to the system by requesting their
 *      fingerprint, PIN, and privilege level.
 * 
 * Functions    :
 *      - verify_user
 *      - add_profile
 *      - delete_profile
 * ----------------------------------------------------------------------
 */

/** ------------------------------------------
 * @file ProfileRecog.h
 * @author Joel Taina
 * @version 1.1 3/17/21
 * -------------------------------------------
 */

#define PIN_TXD  (GPIO_NUM_17)
#define PIN_RXD  (GPIO_NUM_16)
#define PIN_IRQ  (GPIO_NUM_4)
#define PIN_RTS  (UART_PIN_NO_CHANGE)
#define PIN_CTS  (UART_PIN_NO_CHANGE)



/**
 * \brief Initialize profile recognition subsystem
 * \retval See vfy_pass for description of all possible return values
 */
esp_err_t profileRecog_init();

/**
 * \brief Search the library for profile with matching fingerprint
 * \param flags status flags
 * \param ret_code OUT for the return code
 * \param privilege OUT for the privilege level of verified users
 * \retval See vfy_pass for description of all possible return values
 */
esp_err_t verifyUser_fingerprint(uint8_t *flags, uint8_t *ret_code, uint8_t *privilege);

/**
 * \brief Search the library for profile with matching input pin
 * \param flags status flags
 * \param pin_input input PIN
 * \param ret_code OUT for the return code
 * \param privilege OUT for the privilege level of verified users
 * \retval See vfy_pass for description of all possible return values
 */
esp_err_t verifyUser_PIN(uint8_t *flags, uint8_t *pin_input, uint8_t *ret_code, uint8_t *privilege);

/**
 * \brief Add one of two fingerprints required to form a template
 * \param flags status flags
 * \param ret_code OUT for the return code
 * \retval See vfy_pass for description of all possible return values
 */
esp_err_t addProfile_fingerprint(uint8_t *flags, uint8_t *ret_code);

/**
 * \brief Add a PIN and verify that it is currently unused
 * \param flags status flags
 * \param pin_input input PIN
 * \param ret_code OUT for the return code
 * \retval See vfy_pass for description of all possible return values
 */
esp_err_t addProfile_PIN(uint8_t *flags, uint8_t *pin_input, uint8_t *ret_code);

/**
 * \brief Add a privilege level (occurs after adding a PIN)
 * \param flags status flags
 * \param privilege input privilege level
 * \param ret_code OUT for the return code
 * \retval See vfy_pass for description of all possible return values
 */
esp_err_t addProfile_privilege(uint8_t *flags, uint8_t privilege, uint8_t *ret_code);

/**
 * \brief Compile and save profile to FP module and SD card flash (occurs after PIN, privilege, and fingerprint inserted)
 * \param flags status flags
 * \param ret_code OUT for the return code
 * \retval See vfy_pass for description of all possible return values
 */
esp_err_t addProfile_compile(uint8_t *flags, uint8_t *ret_code);

// Private functions

/**
 * \brief Generate fingerprint from entry and create character file
 * \param buffer_id char buffer id
 * \retval See vfy_pass for description of all possible return values
 */
static esp_err_t genImg_Img2Tz(uint8_t buffer_id);

/** -----------------------------------------
 * @file ProfileRecog.c
 * @author Joel Taina
 * @version 1.1 3/17/21
 * ------------------------------------------
 */

//extern uint8_t flags;

R502_data_len_t starting_data_len;
static R502_conf_code_t conf_code;
static R502_sys_para_t sys_para;
static uint16_t page_id;
static uint16_t match_score;
static int up_char_size = 0;

// The object under test
R502Interface R502 = {
    .up_image_cb = NULL,
    .up_char_cb = NULL,
    .TAG = "R502",
    .adder = {0xFF, 0xFF, 0xFF, 0xFF},
    .cur_data_pkg_len = 128,
    .initialized = false,
    .interrupt = 0,
    .start = {0xEF, 0x01},
    .system_identifier_code = 0,
    .default_read_delay = 200,
    .read_delay_gen_image = 2000,
    .min_uart_buffer_size = 256,
    .header_size = offsetof(R502_DataPkg_t, data)
};


void up_char_callback(uint8_t data[R502_max_data_len], 
    int data_len)
{
    // this is where you would store or otherwise do something with the image
    uint8_t *buffer_ptr = &fingerprintBuffer[up_char_size];
    int total = 0;
    while (total < data_len) {
        buffer_ptr[total] = data[total];
        total++;
    }

    up_char_size += data_len;
}

esp_err_t profileRecog_init() {
    R502_init(&R502, UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ, R502_baud_115200);

    R502_read_sys_para(&R502, &conf_code, &sys_para);
    starting_data_len = sys_para.data_package_length;

    R502_set_up_char_cb(&R502, up_char_callback);
    up_char_size = 0;
    printf("starting_data_len: %d\n", starting_data_len);

    return ESP_OK;
}

esp_err_t verifyUser_fingerprint(uint8_t *flags, uint8_t *ret_code, uint8_t *privilege) {
    *ret_code = 1;
    if (*flags & FL_FP_0) {

        printf("Scanning fingerprint...\n");
        if (genImg_Img2Tz(1) != ESP_OK) {
            printf("Bad fingerprint entry\n");
            return ESP_FAIL;
        }

        // 4: search up char file against library and return resuit
        R502_search(&R502, 1, 0, 0xffff, &conf_code, &page_id, &match_score);
        ESP_LOGI("main", "Search res: %d", (int)conf_code);
        if (conf_code != R502_ok) {
            printf("Access denied\n");
            //flags |= FL_INPUT_READY;
            return ESP_FAIL;
        }
        *flags &= ~(FL_PIN | FL_FP_0);
        *ret_code = 0; // 0 = SUCCESS
        *privilege = profiles[page_id].privilege;
        printf("Accepted: %d\n", page_id);
    }

    return ESP_OK;
}

esp_err_t verifyUser_PIN(uint8_t *flags, uint8_t *pin_input, uint8_t *ret_code, uint8_t *privilege) {
    bool isMatch = false;

    *ret_code = 1;
    if (*flags & FL_PIN) {

        // 1: Wait for fingerprint. When received, wait for 500ms.
        printf("Checking PIN\n");
        vTaskDelay(20 / portTICK_PERIOD_MS);
                        

        // 2: Import a PIN from a profile in database (implement later)
        // 3: Compare input PIN to that of the profile
        for (int i = 0; i < MAX_PROFILES; i++) {
            // Skip unused slots
            if (profiles[i].isUsed == 0) {
                continue;
            }

            ESP_LOGI("verifyUser_PIN", "comparing profile: %d", i);
            isMatch = true;

            // Retrieve a PIN from a profile. If mismatch found, end comparison and move on to next one
            for (int j = 0; j < 4; j++) {
                if (pin_input[j] != profiles[i].PIN[j]) {
                    ESP_LOGI("verifyUser_PIN", "mismatch %d and %d\n", pin_input[j], profiles[i].PIN[j]);
                    isMatch = false;
                    break;
                }
            }

            if (isMatch) {
                printf("Accepted PIN from index %d\n", i);
                ESP_LOGI("verifyUser_PIN", "Privilege level: %d\n", profiles[i].privilege);
                *flags &= ~(FL_PIN | FL_FP_0);
                *ret_code = 0; // 0 = SUCCESS
                *privilege = profiles[i].privilege;
                return ESP_OK;
            }
        }
        printf("Invalid PIN\n");
        *ret_code = 1; // 1 = FAIL
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t addProfile_fingerprint(uint8_t *flags, uint8_t *ret_code) {
    *ret_code = 1;

    if (*flags & FL_FP_0) {
        // 1: Wait for fingerprint. When received, wait for 500ms.
        printf("Scanning fingerprint 1...\n");
        //vTaskDelay(20 / portTICK_PERIOD_MS);
        
        if (genImg_Img2Tz(1) != ESP_OK) {
            printf("Bad fingerprint entry\n");
            return ESP_FAIL;
        }
        
        // 4: Clear fingerprint 0 flag
        *flags &= ~FL_FP_0;
    }
    else if (*flags & FL_FP_1) {
        // 1: Wait for fingerprint. When received, wait for 500ms.
        printf("Scanning fingerprint 2...\n");

        if (genImg_Img2Tz(2) != ESP_OK) {
            printf("Bad fingerprint entry\n");
            return ESP_FAIL;
        }

        // 4: Action: RegModel()
        R502_reg_model(&R502, &conf_code);
        ESP_LOGI("main", "regModel res: %d", (int)conf_code);
        if (conf_code != R502_ok) {
            printf("Finger entries do not match\n");
            return ESP_FAIL;
        }
        
        // 6: Clear fingerprint 2 flag
        *flags &= ~FL_FP_1;
        printf("Fingerprint template created!\n");
    }

    // Wrap up
    *ret_code = 0; // 0 = SUCCESS

    return ESP_OK;
}

esp_err_t addProfile_PIN(uint8_t *flags, uint8_t *pin_input, uint8_t *ret_code) {
    static bool isMatch;

    *ret_code = 1;

    if (*flags & FL_PIN) {
        // 1: Wait for PIN entry. When recieved, wait for 500ms. (no reason)
        printf("Verifying PIN uniqueness\n");
        vTaskDelay(20 / portTICK_PERIOD_MS);

        // 2: Import a PIN from a profile in database
        // 3: Compare input PIN to that of the profile (check for uniqueness)
        for (int i = 0; i < MAX_PROFILES; i++) {
            // Skip unused slots
            if (profiles[i].isUsed == 0) {
                continue;
            }

            // Profile is there. Start comparing
            ESP_LOGI("profileRecog", "comparing profile: %d", i);
            isMatch = true;

            // Retrieve a PIN from a profile. If mismatch found, end comparison and move on to next one
            for (int j = 0; j < 4; j++) {
                if (pin_input[j] != profiles[i].PIN[j]) {
                    printf("mismatch %d and %d\n", pin_input[j], profiles[i].PIN[j]);
                    isMatch = false;
                    break;
                }
            }

            if (isMatch) {
                printf("PIN already being used by profile %d\n", i);
                printf("Select different PIN\n");
                return ESP_FAIL;
            }
        }
        printf("Accepted PIN ");
        for (int j = 0; j < 4; j++) {
            pinBuffer[j] = pin_input[j];
            printf("%d", (int)pinBuffer[j]);
        }
        printf("\n");
        *flags &= ~FL_PIN;
        *ret_code = 0; // 0 = SUCCESS
        return ESP_OK;
    }

    return ESP_OK;
}

esp_err_t addProfile_privilege(uint8_t *flags, uint8_t privilege, uint8_t *ret_code) {
    *ret_code = 1;

    if (*flags & FL_PRIVILEGE) {
        // 1: Wait for PIN entry. When recieved, wait for 500ms. (no reason)
        printf("Updating privilege to %d\n", (int)privilege);
        vTaskDelay(20 / portTICK_PERIOD_MS);
        privBuffer = privilege;
        *flags &= ~FL_PRIVILEGE;
        *ret_code = 0; // 0 = SUCCESS
        return ESP_OK;
    }

    return ESP_OK;
}

esp_err_t addProfile_compile(uint8_t *flags, uint8_t *ret_code) {
    *ret_code = 1;

    // Check for inputs
    if (*flags & (FL_PRIVILEGE | FL_PIN | FL_FP_01)) {
        ESP_LOGE("addProfile_compile", "Profile components not accounted for. Abort.");
        return ESP_FAIL;
    }

    // Store fingerprint template to R503 flash memory banks
    // A: Store to next available slot (on buffer)
    int page_id = 0;
    while (profiles[page_id].isUsed) {
        page_id++;
        if (page_id >= MAX_PROFILES) {
            printf("Slots full. Delete profile to add space\n");
            return ESP_FAIL;
        }
    }
    ESP_LOGI("addProfile_compile", "Profile slot to fill: %d", page_id);

    R502_store(&R502, 1, page_id, &conf_code);
    ESP_LOGI("main", "store res: %d", (int)conf_code);
    if (conf_code != R502_ok) {
        printf("Failed to store\n");
        return ESP_FAIL;
    }

    printf("Uploading char file to ESP32\n");

    // 5: Action: UpChar(). Will upload template to ESP32 char buffer
    R502_up_char(&R502, starting_data_len, 1, &conf_code);
    ESP_LOGI("main", "upChar res: %d", (int)conf_code);
    if (conf_code != R502_ok) {
        printf("Failed to upload template\n");
        return ESP_FAIL;
    }

    // 6: Compilation: Prepare to write to SD card...
    printf("Upload successful. Now sending to buffers...\n");

    // TEST: print contents
    printf("-----------------------------------------------\n");
    printf("Profile to be sent to SD card\n");
    // a) PIN
    printf("PIN: ");
    for (int i = 0; i < 4; i++) {
        printf("%d ", pinBuffer[i]);
    }
    printf("\n");
    // b) privilege
    printf("Privilege: %d\n", (int)privBuffer);
    // c) char template (fingerprint). Get from example test.
    printf("Template hex: \n");
    uint8_t *data = &fingerprintBuffer[0];
    int box_width = 16;
    int package_size = 128;

    while (up_char_size > 0) {
        printf("\nPackage %d:\n", up_char_size / package_size);
        int total = 0;
        while(total < package_size){
            for(int i = 0; i < box_width; i++){
                printf("0x%02X ", data[total]);
                total++;
            }
            printf("\n");
        }

        //CONFIG_ESP_TASK_WDT

        up_char_size -= package_size;
        data = data + package_size;
    }

    // Update the profile slot that is open...
    printf("SD card needed to progress forward\n");

    return ESP_OK;
}

// private functions
static esp_err_t genImg_Img2Tz(uint8_t buffer_id) {

    // 1: Wait for fingerprint. When received, wait for 500ms.
    vTaskDelay(20 / portTICK_PERIOD_MS);
                    
    // 2: Action. GenImg(). Abort if fail
    R502_gen_image(&R502, &conf_code);
    ESP_LOGI("main", "genImg res: %d", (int)conf_code);
    if (conf_code != R502_ok) {
        return ESP_FAIL;
    } 

    // 3: Action: Img2Tz(). Abort if fail
    R502_img_2_tz(&R502, buffer_id, &conf_code);
    ESP_LOGI("main", "Img2Tz res: %d", (int)conf_code);
    if (conf_code != R502_ok) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

/** --------------------------------------------------------------------
 * END SUBSYSTEM    : profileRecog
 * ---------------------------------------------------------------------
 */

/** --------------------------------------------------------------------
 * SUBSYSTEM    : main
 * Author       : Joel Taina
 * Components   : 
 *      - profileRecog
 *      - uiIntegration
 *      - webDevelop
 * Description  : Main code
 * ----------------------------------------------------------------------
 */

/** ------------------------------------------
 * @file main.c
 * @author Joel Taina
 * @version 1.1 3/17/21
 * -------------------------------------------
 */

// gpio event queue: all interrupts lead here
xQueueHandle gpio_evt_queue = NULL;

void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}


// gpio_task_example: event handler for all gpio-related events
// Includes: fingerprint entry, PIN entry
// Missing: Web application update
static void gpio_task_example(void* arg)
{
    static uint32_t io_num;
    static R502_conf_code_t conf_code;
    static int is_pressed;
    static uint16_t page_id = 0xeeee;
    static uint16_t match_score = 0xeeee;
    static uint8_t privilege = 0;
    static uint8_t ret_code = 0;

    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            is_pressed = gpio_get_level(io_num);
            ESP_LOGI("main", "GPIO[%d] intr, val: %d", io_num, is_pressed);
            // NEW: Do action for GPIO4: Add Profile
            //if ((flags & FL_INPUT_READY) != FL_INPUT_READY) {
              //  continue;
            //}

            switch(io_num) {
                case (GPIO_NUM_23) : // admin button
                    if (!is_pressed) {
                        break;
                    }
                    accessAdmin = !accessAdmin;
                    if (accessAdmin) {
                        printf("Entering admin verification\n");
                    } else {
                        printf("Leaving admin verification\n");
                    }
                    break;

                case (GPIO_NUM_5) : // enter button
                    if (!is_pressed) {
                        break;
                    }

                    if ((flags & FL_FSM) == FL_VERIFYUSER) {
                        // Echo PIN entered
                        for (int i = 0; i < 4; i++) {
                            printf("%d ", pinEnter[i]);
                        }
                        printf("\n");

                        verifyUser_PIN(&flags, pinEnter, &ret_code, &privilege);
                        if (ret_code != 0x0) {
                            break;
                        }
                        if (accessAdmin) {
                            if (privilege) {
                                printf("Accessing admin...\n");
                                isAdmin = 1;
                                // TEST ONLY
                                flags = FL_ADDPROFILE | FL_PRIVILEGE | FL_PIN | FL_FP_01 | FL_INPUT_READY;
                                // AND TEST
                            } else {
                                printf("Sorry, not admin\n");
                            }
                        } else {
                            printf("Hello there, opening door\n");
                        }
                    }
                    else if ((flags & FL_FSM) == FL_ADDPROFILE) {
                        if (flags & FL_PIN) {
                            addProfile_PIN(&flags, pinEnter, &ret_code);
                        }
                        else if (flags & FL_PRIVILEGE) {
                            addProfile_privilege(&flags, privEnter, &ret_code);
                        }
                        else if (flags & FL_FP_01) {
                            printf("PIN and privilege completed. Awaiting fingerprint.\n");
                        }
                        else {
                            printf("PIN, privilege, fingerprint completed. Creating profile...\n");
                            addProfile_compile(&flags, &ret_code);
                        }
                        /*if (ret_code != 0x0) {
                            break;
                        }*/
                    }
                    break;
                case (GPIO_NUM_4) :
                    // Case GPIO4: press finger on scanner

                    if (is_pressed) {
                        break;
                    }

                    if ((flags & FL_FSM) == FL_VERIFYUSER) {
                        verifyUser_fingerprint(&flags, &ret_code, &privilege);
                        if (ret_code != 0x0) {
                            break;
                        }
                        if (accessAdmin) {
                            if (privilege) {
                                printf("Accessing admin...\n");
                                isAdmin = 1;
                                // TEST ONLY
                                flags = FL_ADDPROFILE | FL_PRIVILEGE | FL_PIN | FL_FP_01 | FL_INPUT_READY;
                                // AND TEST
                            } else {
                                printf("Sorry, not admin\n");
                                flags |= FL_PIN | FL_FP_0;
                            }
                        } else {
                            printf("Hello there, opening door\n");
                        }
                    }
                    else if ((flags & FL_FSM) == FL_ADDPROFILE) {
                        addProfile_fingerprint(&flags, &ret_code);
                    }
                    else {
                        printf("unknown state\n");
                    }

                    break;
                default :
                    ESP_LOGE("GPIO", "invalid GPIO number, %d", io_num);
            }
        }
    }
}

void app_main(void)
{
    // 1: Initialize the main project loop
    // a) Install GPIO ISR service
    // GPIO23: mode button
    // GPIO5: enter button
    // GPIO4: FP scanner input 
    gpio_install_isr_service(0);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // 2: create a queue to handle gpio events from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(gpio_task_example, "gpio_task_example", 4096, NULL, 10, NULL);

    // 3: Init profile recognition
    profileRecog_init();

    // 4: Init other subsystems

    // 5: configure push-buttons for simulating PIN entry and mode (TEST)
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    io_conf.pin_bit_mask = ((1ULL << GPIO_NUM_23) | (1ULL << GPIO_NUM_5));
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    io_conf.pull_down_en = 0;
    gpio_config(&io_conf);
    gpio_isr_handler_add(GPIO_NUM_23, gpio_isr_handler, (void*) GPIO_NUM_23);
    gpio_isr_handler_add(GPIO_NUM_5, gpio_isr_handler, (void*) GPIO_NUM_5);

    // 6: start at VerifyUser (FSM = 01). Start in Open Door mode (isAdmin = 0)
    flags |= FL_VERIFYUSER | FL_PIN | FL_FP_0 | FL_INPUT_READY;
    isAdmin = 0;

    // inf: await the push buttons (in gpio_task_example thread)
    if (esp_task_wdt_delete(NULL) != ESP_OK) {
        ESP_LOGW("main", "failure to unsubscribe main loop from task watchdog");
    }
    return;
}
