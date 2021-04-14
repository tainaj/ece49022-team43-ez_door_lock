#include "integ-things.h"

// NEW
#include "CFAL1602.h" // NEW: PRIV_REQUIRES
extern CFAL1602Interface CFAL1602;


/** --------------------------------------------------------------------------
 * SUBSYSTEM    : integThings
 * Author       : Joel Taina
 * Components   : 
 *      - Profile recognition
 * Description  : Integration of Things provides commonly used subfunctions in
 *      main loop.
 * 
 * Functions    :
 *      - verify_user
 *      - add_profile
 *      - delete_profile
 * --------------------------------------------------------------------------
 */

// externs to main function:
extern int accessAdmin; // currently seeking admin mode (by pressing admin query button)
extern volatile uint8_t flags; // used to track inputs



// private functions
/*static esp_err_t genImg_Img2Tz(uint8_t buffer_id) {

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
}*/

// public functions

void restore_to_verifyUser() {
    // restore flags to verifyUser init
    flags = FL_VERIFYUSER | FL_PIN | FL_FP_0;

    // set admin to 0 (door open select)
    accessAdmin = 0;

    // init screen for verifyUser
    WS2_msg_clear(&CFAL1602, 0);
    WS2_msg_clear(&CFAL1602, 1);
}

void restore_to_idleState() {
    // restore flags to idleState init
    flags = FL_IDLESTATE;

    // set admin to 0 (addProfile select)
    accessAdmin = 0;

    // init screen for idleState. Show item 1 (0-exit, 1-add, 2-delete)
    WS2_msg_print(&CFAL1602, admin_menu, 0, false);
    WS2_msg_print(&CFAL1602, item1, 1, false);
}

void open_door() {
    // Toggle GPIO21 on (relay output)
    gpio_set_level((gpio_num_t)RELAY_OUTPUT, 1); // turn on relay (GPIO21)

    // Print 0: Door open (1 second)
    // Print 1: 
    WS2_msg_print(&CFAL1602, door_open, 0, false);
    WS2_msg_clear(&CFAL1602, 1);
    printf("Hello there, opening door\n");
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // Toggle GPIO21 off (relay output)
    gpio_set_level((gpio_num_t)RELAY_OUTPUT, 0); // turn off relay (GPIO21)
}