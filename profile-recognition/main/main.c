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
#include "esp_task_wdt.h"

#include "main.h"

#include "CFAL1602.h" // NEW PRIV_REQUIRE

// NEW
#include "Keypad.h" // NEW PRIV_REQUIRE

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

//typedef int project_mode_t;

int accessAdmin; // currently seeking admin mode (by pressing admin query button)
uint8_t flags = 0; // used to track inputs

// ------TEST ONLY-------------

// Sample input PIN and privilege
uint8_t pin1[4] = {1, 2, 3, 4}; // default admin
uint8_t pin2[4] = {5, 6, 7, 8}; // invalid
uint8_t pin3[4] = {1, 2, 4, 8}; // invalid
uint8_t *pinEnter = pin1;
uint8_t privEnter = 0;          // user-type
int profileIdEnter = 2;

// --------------END TEST-----------------

// NEW: Keypad
#define ROWS 4
#define COLS 4

char keys[ROWS][COLS] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

uint8_t rowPins[ROWS] = {34, 35, 36, 39};
uint8_t colPins[COLS] = {25, 26, 27, 33};

// The Keypad object
Keypad keypad;

// Event loop
/*static void gpio_keypad_loop(void *arg)
{
    for (;;) {
        char key = Keypad_getKey(&keypad);
        if (key != '\0') {
            printf("hi char %c\n", key);
        }
        if (key == '*') {
            printf("asterisk\n");
        } else if (key == '#') {
            printf("hashtag\n");
        }
    }
}*/

// NEW: CFAL1602

#ifdef CONFIG_IDF_TARGET_ESP32

#define CFAL1602_MISO -1
#define CFAL1602_MOSI 15
#define CFAL1602_CLK  14
#define CFAL1602_CS   13
#endif

//#define PUSH_BUTTON 22 // input
//#define RELAY       21 // output

// CFAL1602 this object
CFAL1602Interface CFAL1602 = {
    .TAG = "CFAL1602C-PB",
    .initialized = false,
    .line_len = 16,
    .line_count = 2,
    .timer_period = 1000000
};

// gpio event queue: all interrupts lead here
xQueueHandle gpio_evt_queue = NULL;

void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

// Event loop
/*static void gpio_keypad_loop(void *arg)
{
    for (;;) {
        char key = Keypad_getKey(&keypad);
        if (key != '\0') {
            printf("hi char %c\n", key);
        }
        if (key == '*') {
            printf("asterisk\n");
        } else if (key == '#') {
            printf("hashtag\n");
        }
    }
}*/

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
                case (GPIO_NUM_22) : // admin button
                    if (!is_pressed) {
                        break;
                    }
                    //accessAdmin = !accessAdmin;

                    if ((flags & FL_FSM) == FL_IDLESTATE) {
                        accessAdmin = (accessAdmin+1) % 3;
                        if (accessAdmin == 1) {
                            printf("Add profile. Press ENTER to start\n");
                        } else if (accessAdmin == 2) {
                            printf("Delete profile. Press ENTER to start\n");
                        } else if (accessAdmin == 0) {
                            printf("Exit admin mode. Press ENTER to start\n");
                        }
                    }
                    else if ((flags & FL_FSM) == FL_VERIFYUSER) {
                        accessAdmin = (accessAdmin+1) % 2;
                        //accessAdmin = !accessAdmin;
                        if (accessAdmin == 1) {
                            printf("Entering admin verification\n");
                        } else if (accessAdmin == 0){
                            printf("Leaving admin verification\n");
                        }
                    }
                    break;

                case (GPIO_NUM_21) : // enter button
                    if (!is_pressed) {
                        break;
                    }
                    if ((flags & FL_FSM) == FL_IDLESTATE) {
                        if (accessAdmin == 1) {
                            flags = FL_ADDPROFILE | FL_PRIVILEGE | FL_PIN | FL_FP_01 | FL_INPUT_READY;
                            printf("Starting Add Profile...\n");
                            vTaskDelay(1000 / portTICK_PERIOD_MS); // 1 second delay: need time to read the print statement
                            printf("Enter PIN...\n");
                        } else if (accessAdmin == 2) {
                            flags = FL_DELETEPROFILE | FL_PROFILEID | FL_INPUT_READY;
                            printf("Starting Delete Profile...\n");
                            vTaskDelay(1000 / portTICK_PERIOD_MS); // 1 second delay: need time to read the print statement
                            printf("Enter profile id...\n");    
                        } else if (accessAdmin == 0) {
                            flags |= FL_VERIFYUSER | FL_PIN | FL_FP_0 | FL_INPUT_READY;
                            printf("Leaving admin control...\n");
                            vTaskDelay(1000 / portTICK_PERIOD_MS); // 1 second delay: need time to read the print statement
                            printf("Reset to Verify User\n");
                        }
                    }
                    else if ((flags & FL_FSM) == FL_VERIFYUSER) {
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
                                // TEST ONLY
                                flags = FL_IDLESTATE | FL_INPUT_READY;
                                //flags = FL_ADDPROFILE | FL_PRIVILEGE | FL_PIN | FL_FP_01 | FL_INPUT_READY;
                                // AND TEST
                            } else {
                                printf("Sorry, not admin\n");
                                flags |= FL_VERIFYUSER | FL_PIN | FL_FP_0 | FL_INPUT_READY;
                            }
                        } else {
                            printf("Hello there, opening door\n");
                            vTaskDelay(1000 / portTICK_PERIOD_MS); // 1 second delay: need time to read the print statement
                            printf("Closing door\n");

                            flags |= FL_VERIFYUSER | FL_PIN | FL_FP_0 | FL_INPUT_READY;
                        }
                    }
                    else if ((flags & FL_FSM) == FL_ADDPROFILE) {
                        if (flags & FL_PIN) {
                            pinEnter = pin2; // TEST ONLY
                            addProfile_PIN(&flags, pinEnter, &ret_code);
                            if (ret_code == 0) {
                                printf("Enter privilege level...\n");
                            }
                        }
                        else if (flags & FL_PRIVILEGE) {
                            addProfile_privilege(&flags, privEnter, &ret_code);
                            printf("PIN and privilege completed.\n");
                            if ((flags & FL_FP_01) == 0) {
                                printf("Fingerprint completed. Press ENTER to complete Add Profile\n");
                            } else {
                                printf("Awaiting fingerprint...\n");
                            }
                        }
                        else if (flags & FL_FP_01) {
                            printf("No fingerprint loaded. Awaiting fingerprint...\n");
                        }
                        else {
                            printf("PIN, privilege, fingerprint completed. Creating profile...\n");
                            addProfile_compile(&flags, &ret_code);

                            flags = FL_IDLESTATE | FL_INPUT_READY;
                        }
                        /*if (ret_code != 0x0) {
                            break;
                        }*/
                    }
                    else if ((flags & FL_FSM) == FL_DELETEPROFILE) {
                        if (flags & FL_PROFILEID) {
                            //profileEnter = 
                            deleteProfile_remove(&flags, profileIdEnter, &ret_code);
                            if (ret_code == 0) {
                                printf("profile deleted!\n");
                            }
                        }
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
                                // TEST ONLY (Idle state)
                                flags = FL_IDLESTATE | FL_INPUT_READY;

                                //flags = FL_ADDPROFILE | FL_PRIVILEGE | FL_PIN | FL_FP_01 | FL_INPUT_READY;
                                // AND TEST
                            } else {
                                printf("Sorry, not admin\n");
                                flags |= FL_VERIFYUSER | FL_PIN | FL_FP_0 | FL_INPUT_READY;
                            }
                        } else {
                            printf("Hello there, opening door\n");

                            vTaskDelay(1000 / portTICK_PERIOD_MS); // 1 second delay: need time to read the print statement
                            printf("Closing door\n");

                            flags |= FL_VERIFYUSER | FL_PIN | FL_FP_0 | FL_INPUT_READY;
                        }
                    }
                    else if ((flags & FL_FSM) == FL_ADDPROFILE) {
                        addProfile_fingerprint(&flags, &ret_code);
                        if (ret_code != 0) {
                            break;
                        }
                        if ((flags & (FL_PRIVILEGE | FL_PIN | FL_FP_01)) == 0) {
                            printf("PIN, privilege, fingerprint completed. Press ENTER to complete ADD Profile\n");
                        }
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
    xTaskCreate(gpio_task_example, "gpio_task_example", 4096, NULL, 12, NULL);

    // NEW: initialize CFAL1602!
    WS2_init(&CFAL1602, CFAL1602_MISO, CFAL1602_MOSI, CFAL1602_CLK, CFAL1602_CS);

    WS2_msg_print(&CFAL1602, message1, 0, false);

    // NEW: initialize Keypad!
    Keypad_init(&keypad, makeKeymap(keys), rowPins, colPins, ROWS, COLS);
    //xTaskCreate(gpio_keypad_loop, "gpio_keypad_loop", 1024, NULL, 10, NULL);
    // SKIP BOTTOM UNTIL ABOVE WORK SUCCESSFULLY!

    
    // 3: Init profile recognition
    profileRecog_init();

    // 4: Init other subsystems

    // 5: configure push-buttons for simulating PIN entry and mode (TEST)
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    io_conf.pin_bit_mask = ((1ULL << GPIO_NUM_22) | (1ULL << GPIO_NUM_21));
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    io_conf.pull_down_en = 0;
    gpio_config(&io_conf);
    gpio_isr_handler_add(GPIO_NUM_22, gpio_isr_handler, (void*) GPIO_NUM_22);
    gpio_isr_handler_add(GPIO_NUM_21, gpio_isr_handler, (void*) GPIO_NUM_21);

    // 6: start at VerifyUser (FSM = 01). Start in Open Door mode (isAdmin = 0)
    flags |= FL_VERIFYUSER | FL_PIN | FL_FP_0 | FL_INPUT_READY;

    // inf: await the push buttons (in gpio_task_example thread)
    if (esp_task_wdt_delete(NULL) != ESP_OK) {
        ESP_LOGW("main", "failure to unsubscribe main loop from task watchdog");
    }

    return;
}
