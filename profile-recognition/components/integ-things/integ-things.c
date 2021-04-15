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
extern bool isHelp;

extern int accessAdmin; // currently seeking admin mode (by pressing admin query button)
extern volatile uint8_t flags; // used to track inputs

extern uint8_t ret_code;
extern uint8_t privilege;



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

// aquire lock INPUT_READY
bool my_acquire_lock(bool checkForHelp) {
    if (!(flags & FL_INPUT_READY)) {
        printf("Wait\n");
        return false;
    }
    // blocked if help is activated
    if (checkForHelp && isHelp) {
        printf("Turn off help before trying again\n");
        return false;
    }
    // successful acquire
    flags &= ~FL_INPUT_READY;
    return true;
}

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

void verifyUser_outcome_handler() {
    // case 1: bad fingerprint, denied
    if (ret_code != 0x0) {
        // reset system to verifyUser initial state.
        restore_to_verifyUser();
    }
    // case 2: open door, enter admin, sorry not admin
    else {
        if (accessAdmin) {
            // case a: admin access, privileged
            if (privilege) {
                // Print 0: Access granted (0.5 seconds)
                // Print 1:
                WS2_msg_print(&CFAL1602, access_granted, 0, false);
                WS2_msg_clear(&CFAL1602, 1);
                printf("Access granted\n");
                vTaskDelay(500 / portTICK_PERIOD_MS);

                // Print 0: Entering admin.. (1 second)
                WS2_msg_print(&CFAL1602, entering_admin, 0, false);
                printf("Entering admin..\n");
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                
                // reset system to idleState initial state.
                restore_to_idleState();

                // return
            }
            // case b: admin access, not privileged
            else {
                // Print 0: Access denied (1 second)
                // Print 1:
                WS2_msg_print(&CFAL1602, access_denied, 0, false);
                WS2_msg_clear(&CFAL1602, 1);
                printf("Sorry, not admin\n");
                vTaskDelay(1000 / portTICK_PERIOD_MS);

                // reset system to verifyUser initial state.
                restore_to_verifyUser();

                // return
            }
        }
        // case c: door open
        else {
            // Print 0: Access granted (0.5 second)
            // Print 1:
            WS2_msg_print(&CFAL1602, access_granted, 0, false);
            WS2_msg_clear(&CFAL1602, 1);
            printf("Access granted\n");
            vTaskDelay(500 / portTICK_PERIOD_MS);

            // open door.
            open_door();

            // reset system to verifyUser initial state
            restore_to_verifyUser();
        }
    }
}

void idleState_toggle_menu(bool increasing) {
    // determine direction of toggle
    if (increasing) {
        accessAdmin = (accessAdmin == 2) ? 0 : accessAdmin+1;
    } else {
        accessAdmin = (accessAdmin == 0) ? 2 : accessAdmin-1;
    }

    // print hovering menu item
    if (accessAdmin == 0) {
        // Print 1: Add Profile
        WS2_msg_print(&CFAL1602, item1, 1, false);
        printf("Add profile. Press ENTER to start\n");
    } else if (accessAdmin == 1) {
        // Print 1: Delete Profile
        WS2_msg_print(&CFAL1602, item2, 1, false);
        printf("Delete profile. Press ENTER to start\n");
    } else if (accessAdmin == 2) {
        // Print 1: Exit Admin
        WS2_msg_print(&CFAL1602, item3, 1, false);
        printf("Exit admin mode. Press ENTER to start\n");
    }
}