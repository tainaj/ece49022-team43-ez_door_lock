#include "prof-recog.h"

/** --------------------------------------------------------------------------
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
 * --------------------------------------------------------------------------
 */

// User profile buffers
static uint8_t fingerprintBuffer[R502_TEMPLATE_SIZE];    // temp storage for fingerprint template (addProfile, init)
static uint8_t pinBuffer[4];           // temp storage for PIN (addProfile)
static uint8_t privBuffer;             // temp storage for privilege (addProfile)

profile_t profiles[MAX_PROFILES] = {
    {
        .isUsed = 1,    // Slot 0. Default admin profile (no fingerprint)
        .PIN = {1, 2, 3, 4},
        .privilege = 1,
    },                  /// Slots 1-199. Deallocated. Not seen
};

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


static void print_buffer() {
    // a) PIN
    printf("PIN: ");
    for (int i = 0; i < 4; i++) {
        printf("%d ", pinBuffer[i]);
    }
    printf("\n");

    // b) privilege
    printf("Privilege: %d\n", (int)privBuffer);

    // c) char template (fingerprint)
    printf("Template hex: \n");
    uint8_t *data = &fingerprintBuffer[0];
    int box_width = 16;
    int package_size = 128;
    int data_left = R502_TEMPLATE_SIZE;

    while (data_left > 0) {
        printf("\nPackage %d:\n", 12 - (data_left / package_size));
        int total = 0;
        while(total < package_size){
            for(int i = 0; i < box_width; i++){
                printf("0x%02X ", data[total]);
                total++;
            }
            printf("\n");
        }

        data_left -= package_size;
        data = data + package_size;
    }
}

// private functions
static esp_err_t genImg_Img2Tz(uint8_t buffer_id) {

    // 1: Wait for fingerprint. When received, wait for 500ms.
    vTaskDelay(20 / portTICK_PERIOD_MS);
                    
    // 2: Action. GenImg(). Abort if fail
    R502_gen_image(&R502, &conf_code);
    ESP_LOGI("genImg_Img2Tz", "genImg res: %d", (int)conf_code);
    if (conf_code != R502_ok) {
        return ESP_FAIL;
    } 

    // 3: Action: Img2Tz(). Abort if fail
    R502_img_2_tz(&R502, buffer_id, &conf_code);
    ESP_LOGI("genImg_Img2Tz", "Img2Tz res: %d", (int)conf_code);
    if (conf_code != R502_ok) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

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

// public functions
esp_err_t profileRecog_init() {
    R502_init(&R502, UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ, R502_baud_115200);

    R502_read_sys_para(&R502, &conf_code, &sys_para);
    starting_data_len = sys_para.data_package_length;

    R502_set_up_char_cb(&R502, up_char_callback);
    up_char_size = 0;
    ESP_LOGI("profileRecog_init", "starting_data_len: %d", starting_data_len);

    SD_init();

    // Clear R503 module (empty)
    R502_empty(&R502, &conf_code);
    ESP_LOGI("profileRecog_init", "Empty res: %d", (int)conf_code);
    if (conf_code != R502_ok) {
        return ESP_FAIL;
    }

    // Import each file from SD "/sdcard/profiles/profile%d.bin"
    // Update the profile slot that is open...
    printf("SD card needed to progress forward\n");

    int i = 1;
    const int count = 200;

    for (int i = 0; i < count; i++) {
        // NEW
        esp_err_t err = SD_readProfile(i, pinBuffer, 4, &privBuffer, 1, fingerprintBuffer, R502_TEMPLATE_SIZE);
        if (err == ESP_ERR_NOT_FOUND) {
            continue;
        } else if (err != ESP_OK) {
            return ESP_FAIL;
        }

        // Print buffers
        print_buffer();

        // Download template to buffers (ESP32: PIN, priv; R503: fingerprint)
        printf("Downloading template into main buffers...\n");

        profiles[i].isUsed = 1;
        profiles[i].privilege = privBuffer;
        for (int j = 0; j < 3; j++) {
            profiles[i].PIN[j] = pinBuffer[j];
        }
        
        ESP_LOGI("profileRecog_init", "Profile slot to init: %d", i);

        // 5: Action: downChar(). Will upload template to ESP32 char buffer
        printf("Downloading char file from ESP32\n");

        R502_down_char(&R502, starting_data_len, 1, fingerprintBuffer, &conf_code);
        ESP_LOGI("main", "downChar res: %d", (int)conf_code);
        if (conf_code != R502_ok) {
            ESP_LOGE("profileRecog_init", "Failed to download template");
            return ESP_FAIL;
        }

        //vTaskDelay(1000 / portTICK_PERIOD_MS);

        R502_store(&R502, 1, (uint16_t)i, &conf_code);
        ESP_LOGI("profileRecog_init", "store res: %d", (int)conf_code);
        if (conf_code != R502_ok) {
            ESP_LOGE("profleRecog_init", "Failed to store");
            return ESP_FAIL;
        }

        ESP_LOGI("profileRecog_init", "Successful R502 import");
    }

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
    printf("Profile to be sent to SD card: %d\n", page_id);
    print_buffer();
    up_char_size = 0;

    // Update the profile slot that is open...
    printf("SD card needed to progress forward\n");

    const char* directory = "/sdcard/profiles/";
    const char* fileName = "profile";
    const char* fileType = ".bin";

    char name_buffer[512];
    int i = page_id;

    const int count = 200;

    sprintf(name_buffer, "%s%s%d%s", directory, fileName, i, fileType);
    printf("%s\n", name_buffer);
    FILE* f = fopen(name_buffer, "w");
    if (f == NULL) {
        ESP_LOGE("main", "Failed to open file for writing");
        return ESP_FAIL;
    }

    // Write 4-byte PIN to file
    if (fwrite(pinBuffer, sizeof(uint8_t), 4, f) != 4) {
        ESP_LOGE("profileRecog_init", "PIN not fully written");
        return ESP_FAIL;
    }

    // Write 1-byte privilege to file
    if (fwrite(&privBuffer, sizeof(uint8_t), 1, f) != 1) {
        ESP_LOGE("profileRecog_init", "Privilege not fully written");
        return ESP_FAIL;
    }

    // Write 384x4 (1536) byte fingerprint template to file
    if (fwrite(fingerprintBuffer, sizeof(uint8_t), R502_TEMPLATE_SIZE, f) != R502_TEMPLATE_SIZE) {
        ESP_LOGE("profileRecog_init", "Fingerprint not fully written");
        return ESP_FAIL;
    }

    fclose(f);

    return ESP_OK;
}