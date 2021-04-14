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

char* help_save1;
char* help_save2;
bool isHelp = false;

bool doorOpen = false;

int accessAdmin; // currently seeking admin mode (by pressing admin query button)
volatile uint8_t flags = 0; // used to track inputs

//#define RELAY_OUTPUT 21
//#define DOOR_INPUT 22

// ------TEST ONLY-------------

// Sample input PIN and privilege
//pin1[4] = {1, 2, 3, 4}; // default admin
uint8_t pinEnter[4] = {0};
uint8_t privEnter = 0;          // user-type
int profileIdEnter = 2;

//uint8_t pinTemp[4] = {0}; // NEW:
char * pinCharTemp;
char pinChar[17] = {0}; // NEW: message string to print to WS2
int pin_idx = 0;

static int is_pressed;
uint8_t privilege = 0;
uint8_t ret_code = 0;

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

// NEW: CFAL1602
#define CFAL1602_MISO -1
#define CFAL1602_MOSI 15
#define CFAL1602_CLK  14
#define CFAL1602_CS   13

// CFAL1602 this object
CFAL1602Interface CFAL1602 = {
    .TAG = "CFAL1602C-PB",
    .initialized = false,
    .line_len = 16,
    .line_count = 2,
    .timer_period = 500000
};

// gpio event queue: all interrupts lead here
xQueueHandle gpio_evt_queue = NULL;

void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

// Event loop
static void gpio_keypad_loop(void *arg)
{
    static char key;
    //static int pin_idx = 0;
    for (;;) {
        vTaskDelay(10 / portTICK_PERIOD_MS);
        key = Keypad_getKey(&keypad);
        if (key == '\0') {
            continue;
        }
        switch (key) {
            case ('1') : // add other function too
            case ('2') : // add other function too
            case ('3') :
            case ('4') :
            case ('5') :
            case ('6') :
            case ('7') :
            case ('8') :
            case ('9') :
            case ('0') :
                // attempt to get lock
                if (!(flags & FL_INPUT_READY)) {
                    printf("Wait\n");
                    break;
                }
                // blocked if help is activated
                if (isHelp) {
                    printf("Turn off help before trying again\n");
                    break;
                }
                flags &= ~FL_INPUT_READY;
                if ((flags & FL_FSM) == FL_VERIFYUSER) {
                    // enter PIN digit (any time)

                    // 1st press: show PIN prescript
                    if (pin_idx == 0) {
                        // Print 1: PIN: (edit pinChar, set pinEnter to +5)
                        sprintf(pinChar, "%s", "PIN: \0\0\0\0\0\0\0\0\0\0\0");
                        pinCharTemp = pinChar + 5;
                        WS2_msg_print(&CFAL1602, pinChar, 1, false);
                    } 

                    // Enter digit up to allowed number of chars (4)
                    if (pin_idx < 4) {
                        // Print 1: PIN: %%%% (edit pinChar, increment for each digit)
                        pinCharTemp[pin_idx] = key;
                        pinEnter[pin_idx] = (int)(key - '0'); // convert char to int
                        pin_idx++;
                        WS2_msg_print(&CFAL1602, pinChar, 1, false);
                    }
                    // return
                }
                else if ((flags & FL_FSM) == FL_ADDPROFILE) {
                    // enter PIN digit (only for PIN or PRIV set)
                    // further: 1,2 for PIN or PRIV. 0,3-9 for PIN AND PRIV only. May recommend moving to seperate cases 1 and 2 once integ-things is built

                    // PIN mode: all digits 0-9
                    if (flags & FL_PIN) {
                        // do thing
                        // Update pinChar

                        // Enter digit up to allowed number of chars (4)
                        if (pin_idx < 4) {
                            // Print 1: PIN: %%%% (edit pinChar, increment for each digit)
                            pinCharTemp[pin_idx] = key;
                            pinEnter[pin_idx] = (int)(key - '0'); // convert char to int
                            pin_idx++;
                            WS2_msg_print(&CFAL1602, pinChar, 1, false);
                        }
                        // return
                    }
                    // PRIV mode: digits 1,2
                    else if (flags & FL_PRIVILEGE) {
                        if ((key == '1') || (key == '2')) {
                            // do thing


                            
                        }
                    }
                }
                // release lock
                flags |= FL_INPUT_READY;
                break;

            case ('*') :
                // attempt to get lock
                if (!(flags & FL_INPUT_READY)) {
                    printf("Wait\n");
                    break;
                }
                // blocked if help is activated
                if (isHelp) {
                    printf("Turn off help before trying again\n");
                    break;
                }
                flags &= ~FL_INPUT_READY;

                if ((flags & FL_FSM) == FL_VERIFYUSER) {
                    // backspace PIN digit (any time)

                    // Backspace existing digits
                    if (pin_idx > 0) {
                        // Print 1: PIN: %%%% (edit pinChar, decrement for each digit)
                        pin_idx--;
                        pinCharTemp[pin_idx] = '\0';
                        WS2_msg_print(&CFAL1602, pinChar, 1, false);
                    }
                    // return
                }
                else if ((flags & FL_FSM) == FL_ADDPROFILE) {
                    // backspace PIN digit (only for PIN or PRIV set. Technically, it will not affect for all cases.)
                    if (pin_idx > 0) {
                        pin_idx--;
                        pinCharTemp[pin_idx] = '\0';
                        WS2_msg_print(&CFAL1602, pinChar, 1, false);
                        // WS2_print line 1
                    }
                    // return
                }
                else if ((flags & FL_FSM) == FL_DELETEPROFILE) {
                    // reject profile deletion during confirmation (only for PROFILE cleared)
                }
                // release lock
                flags |= FL_INPUT_READY;
                break;

            case ('#') :
                // attempt to get lock
                if (!(flags & FL_INPUT_READY)) {
                    printf("Wait\n");
                    break;
                }
                // blocked if help is activated
                if (isHelp) {
                    printf("Turn off help before trying again\n");
                    break;
                }
                flags &= ~FL_INPUT_READY;

                if ((flags & FL_FSM) == FL_IDLESTATE) {
                    // select admin control option (any time)

                    // Print 0: Selected (1 second)
                    WS2_msg_print(&CFAL1602, selected_this, 0, false);
                    printf("Starting Add Profile...\n");
                    vTaskDelay(1000 / portTICK_PERIOD_MS);

                    if (accessAdmin == 0) {
                        // case 1: addProfile
                        // Print 0: Admin: Add
                        WS2_msg_print(&CFAL1602, admin_add, 0, false);

                        // Print 1: PIN (variable). Show PIN prescript
                        if (pin_idx == 0) {
                            // Print 1: PIN: (edit pinChar, set pinEnter to +5)
                            sprintf(pinChar, "%s", "PIN: \0\0\0\0\0\0\0\0\0\0\0");
                            pinCharTemp = pinChar + 5;
                            WS2_msg_print(&CFAL1602, pinChar, 1, false);
                        } else {
                            ESP_LOGE("me", "bad bad bad");
                        }
                        printf("Enter PIN...\n");

                        // Set flags to AddProfile init
                        flags = FL_ADDPROFILE | FL_PRIVILEGE | FL_PIN | FL_FP_01;

                        // return (to AddProfile)
                    } else if (accessAdmin == 1) {
                        // case 2: deleteProfile
                        // Print 0: Admin: Delete
                        /*WS2_msg_print(&CFAL1602, admin_delete, 0, false);

                        // Print 1: Profile (variable). Show Profile prescript
                        if (pin_idx == 0) {
                            // Print 1: Profile: (edit pinChar, set pinEnter to +5)
                            sprintf(pinChar, "%s", "Profile: \0\0\0\0\0\0\0");
                            pinCharTemp = pinChar + 5;
                            WS2_msg_print(&CFAL1602, pinChar, 1, false);
                        } else {
                            ESP_LOGE("me", "bad bad bad");
                        }
                        printf("Enter PIN...\n");*/

                        // WORK ON BOTTOM AFTER ADD_PROFILE
                        // Enter digit up to allowed number of chars (4)
                        /*if (pin_idx < 4) {
                            // Print 1: PIN: %%%% (edit pinChar, increment for each digit)
                            pinCharTemp[pin_idx] = key;
                            pinEnter[pin_idx] = (int)(key - '0'); // convert char to int
                            pin_idx++;
                            WS2_msg_print(&CFAL1602, pinChar, 1, false);
                        }


                        flags = FL_DELETEPROFILE | FL_PROFILEID;
                        printf("Starting Delete Profile...\n");
                        vTaskDelay(1000 / portTICK_PERIOD_MS); // 1 second delay: need time to read the print statement
                        printf("Enter profile id...\n");*/    
                    } else if (accessAdmin == 2) {
                        // case 3: Exit Admin
                        // Print 0: Leaving Admin... (1 second)
                        // Print 1: (1 second)
                        WS2_msg_print(&CFAL1602, leaving_admin, 0, false);
                        WS2_msg_clear(&CFAL1602, 1);
                        printf("Leaving admin control...\n");
                        vTaskDelay(1000 / portTICK_PERIOD_MS);

                        // reset system to verifyUser initial state
                        restore_to_verifyUser();
                        printf("Reset to Verify User\n");

                        // return (verifyUser)
                    }
                }
                else if ((flags & FL_FSM) == FL_VERIFYUSER) {
                    // enter PIN for access (any time)

                    bool isPinGood = (pin_idx < 4) ? false : true;

                    // Clear PIN display and contents
                    WS2_msg_clear(&CFAL1602, 1);
                    for (int i=0; i < 16; i++) {
                        pinChar[i] = '\0';
                    }
                    pin_idx = 0;

                    // case 1: PIN too short
                    if (!isPinGood) {
                        // Print 0: Invalid PIN (1 second)
                        // Print 1: Must be 4 chars (1 second)
                        WS2_msg_print(&CFAL1602, invalid_pin, 0, false);
                        WS2_msg_print(&CFAL1602, must_be_4_chars, 1, false);
                        vTaskDelay(1000 / portTICK_PERIOD_MS);
                        
                        // restore flags to verifyUser init
                        flags = FL_VERIFYUSER | FL_PIN | FL_FP_0;

                        // set admin to 0 (door open select)
                        accessAdmin = 0;
                        printf("Leaving admin verification\n");

                        // init screen for verifyUser
                        WS2_msg_clear(&CFAL1602, 0);
                        WS2_msg_clear(&CFAL1602, 1);
                    } 
                    // case 2: PIN long enough: call verifyUser_PIN
                    else {
                        // call verifyUser_PIN
                        verifyUser_PIN(&flags, pinEnter, &ret_code, &privilege);

                        // handle all 5 verifyUser outcomes
                        verifyUser_outcome_handler();

                        // return
                    }
                }
                else if ((flags & FL_FSM) == FL_ADDPROFILE) {
                    // enter PIN, priv, compile (any time)

                    bool isPinGood = (pin_idx < 4) ? false : true;

                    // PIN mode: submit PIN
                    if (flags & FL_PIN) {
                        // Clear PIN display and contents
                        WS2_msg_clear(&CFAL1602, 1);
                        for (int i=0; i < 16; i++) {
                            pinChar[i] = '\0';
                        }
                        pin_idx = 0;

                        // case 1: PIN too short
                        if (!isPinGood) {
                            // Print 0: Invalid PIN (1 second)
                            // Print 1: Must be 4 chars (1 second)
                            WS2_msg_print(&CFAL1602, invalid_pin, 0, false);
                            WS2_msg_print(&CFAL1602, must_be_4_chars, 1, false);
                            vTaskDelay(1000 / portTICK_PERIOD_MS);

                            // NOTE BEGIN: OPTIONAL?
                            // Print 0: Admin: Add
                            WS2_msg_print(&CFAL1602, admin_add, 0, false);
                            // NOTE END

                            // Print 1: PIN (variable). Show PIN prescript
                            if (pin_idx == 0) {
                                // Print 1: PIN: (edit pinChar, set pinEnter to +5)
                                sprintf(pinChar, "%s", "PIN: \0\0\0\0\0\0\0\0\0\0\0");
                                pinCharTemp = pinChar + 5;
                                WS2_msg_print(&CFAL1602, pinChar, 1, false);
                            } else {
                                ESP_LOGE("me", "bad bad bad");
                            }
                            printf("Enter PIN...\n");

                            // return. Still in PIN mode
                        } 
                        // case 2: PIN good
                        else {
                            // call verifyUser_PIN
                            addProfile_PIN(&flags, pinEnter, &ret_code);

                            // if retcode == 0
                            // case 1: PIN already used
                            if (ret_code != 0x0) {
                                // NOTE BEGIN: OPTIONAL?
                                // Print 0: Admin: Add
                                WS2_msg_print(&CFAL1602, admin_add, 0, false);
                                // NOTE END

                                // Print 1: PIN (variable). Show PIN prescript
                                if (pin_idx == 0) {
                                    // Print 1: PIN: (edit pinChar, set pinEnter to +5)
                                    sprintf(pinChar, "%s", "PIN: \0\0\0\0\0\0\0\0\0\0\0");
                                    pinCharTemp = pinChar + 5;
                                    WS2_msg_print(&CFAL1602, pinChar, 1, false);
                                } else {
                                    ESP_LOGE("me", "bad bad bad");
                                }
                                printf("Enter PIN...\n");

                                // return. Still in PIN mode
                            }
                            // case 2: PIN good for use
                            else {
                                // Print 0: PIN good for you (1 second)
                                WS2_msg_print(&CFAL1602, pin_good_for_use, 0, false);
                                vTaskDelay(1000 / portTICK_PERIOD_MS);

                                // Print 0: Admin Add
                                WS2_msg_print(&CFAL1602, admin_add, 0, false);

                                // Print 1: Privilege (variable). Show Privilege prescript
                                if (pin_idx == 0) {
                                    // Print 1: Privilege: (edit pinChar, set pinEnter to +11)
                                    sprintf(pinChar, "%s", "Privilege: \0\0\0\0\0");
                                    pinCharTemp = pinChar + 11;
                                    WS2_msg_print(&CFAL1602, pinChar, 1, false);
                                } else {
                                    ESP_LOGE("me", "bad bad bad");
                                }
                                printf("Enter Privilege...\n");

                                // return
                            }
                        }
                    }

                    // TEST: comment out the bottom for now
                    /*if (flags & FL_PIN) {
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
                    }*/
                }
                else if ((flags & FL_FSM) == FL_DELETEPROFILE) {
                    // select profile option from list (only for PROFILE set)
                    // accept profile deletion during confirmation (only for PROFILE cleared)

                    // TEST: comment out bottom for now. Wait for implementation of list select.
                    /*if (flags & FL_PROFILEID) {
                        //profileEnter = 
                        deleteProfile_remove(&flags, profileIdEnter, &ret_code);
                        if (ret_code == 0) {
                            printf("profile deleted!\n");
                        }
                    }*/
                }
                // release lock
                flags |= FL_INPUT_READY;
                break;

            case ('A') : // special character A : up arrow
                // attempt to get lock
                if (!(flags & FL_INPUT_READY)) {
                    printf("Wait\n");
                    break;
                }
                // blocked if help is activated
                if (isHelp) {
                    printf("Turn off help before trying again\n");
                    break;
                }
                flags &= ~FL_INPUT_READY;

                if ((flags & FL_FSM) == FL_IDLESTATE) {
                    // toggle next (up) admin control option 0-2 (any time)

                    // toggle next admin control option 0-2. Choose direction
                    idleState_toggle_menu(true);

                    // return
                }
                else if ((flags & FL_FSM) == FL_VERIFYUSER) {
                    // toggle next admin access option 0-1 (any time)

                    // modulo increment accessAdmin (2)
                    accessAdmin = (accessAdmin+1) % 2;
                    if (accessAdmin == 1) {
                        // Print 0: Admin: Verify
                        WS2_msg_print(&CFAL1602, admin_verify, 0, false);
                        printf("Entering admin verification\n");
                    } else if (accessAdmin == 0){
                        // Print 0: 
                        WS2_msg_clear(&CFAL1602, 0);
                        printf("Leaving admin verification\n");
                    }
                }
                else if ((flags & FL_FSM) == FL_DELETEPROFILE) {
                    // toggle next (up) profile option from list (only for PROFILE set. IMPLEMENT THIS)

                }
                // release lock
                flags |= FL_INPUT_READY;
                break;

            case ('B') : // special character B : down arrow
                // attempt to get lock
                if (!(flags & FL_INPUT_READY)) {
                    printf("Wait\n");
                    break;
                }
                // blocked if help is activated
                if (isHelp) {
                    printf("Turn off help before trying again\n");
                    break;
                }
                flags &= ~FL_INPUT_READY;

                if ((flags & FL_FSM) == FL_IDLESTATE) {
                    // toggle prev (down) admin control option 0-2 (any time)

                    // toggle next admin control option 0-2. Choose direction
                    idleState_toggle_menu(false);

                    // return
                }
                else if ((flags & FL_FSM) == FL_DELETEPROFILE) {
                    // toggle prev (down) profile option from list (only for PROFILE set. IMPLEMENT THIS)

                }
                // release lock
                flags |= FL_INPUT_READY;
                break;

            case ('C') : // special character C : help button
                // attempt to get lock
                if (!(flags & FL_INPUT_READY)) {
                    printf("Wait\n");
                    break;
                }
                flags &= ~FL_INPUT_READY;

                // toggle help (any time)

                // case 1: enter help
                if (!isHelp) {
                    // toggle help
                    isHelp = true;

                    // store previous text into save buffers
                    help_save1 = WS2_get_string(&CFAL1602, 0);
                    help_save2 = WS2_get_string(&CFAL1602, 1);

                    // print help strings (TEST: currently junk help)
                    WS2_msg_print(&CFAL1602, help1, 0, true);
                    WS2_msg_print(&CFAL1602, help2, 1, true);
                // case 2: leave help
                } else {
                    // toggle help
                    isHelp = false;

                    // load/print previous text from save buffers
                    WS2_msg_print(&CFAL1602, help_save1, 0, true);
                    WS2_msg_print(&CFAL1602, help_save2, 1, true);
                }
                // release lock
                flags |= FL_INPUT_READY;
                break;
            
            case ('D') : // special character D : abort button
                // attempt to get lock
                if (!(flags & FL_INPUT_READY)) {
                    printf("Wait\n");
                    break;
                }
                // blocked if help is activated
                if (isHelp) {
                    printf("Turn off help before trying again\n");
                    break;
                }
                flags &= ~FL_INPUT_READY;

                // abort to verifyUser or idleState-admin mode (any time)

                // Clear PIN display and contents
                WS2_msg_clear(&CFAL1602, 1);
                for (int i=0; i < 16; i++) {
                    pinChar[i] = '\0';
                }
                pin_idx = 0;

                // Print 0: Canceled (1 second)
                // Print 1:
                WS2_msg_print(&CFAL1602, canceled, 0, false);
                WS2_msg_clear(&CFAL1602, 1);
                printf("Canceling...\n");
                vTaskDelay(1000 / portTICK_PERIOD_MS);

                // case 1: verifyUser abort : abort PIN entry
                if ((flags & FL_FSM) == FL_VERIFYUSER) {

                    // reset system to verifyUser initial state.
                    restore_to_verifyUser();

                    // return
                }
                // case 2: idleState, addProfile, deleteProfile abort:
                else {
                    // Print 0: Returning to (1 second)
                    // Print 1: menu (1 second)
                    WS2_msg_print(&CFAL1602, return_to_menu_0, 0, false);
                    WS2_msg_print(&CFAL1602, return_to_menu_1, 1, false);
                    printf("Returning to menu...\n");
                    vTaskDelay(1000 / portTICK_PERIOD_MS);

                    // reset system to idleState initial state.
                    restore_to_idleState();

                    // return
                }
                // release lock
                flags |= FL_INPUT_READY;
                break;

            default :
                printf("Not valid character\n");
        }
    }
}

// gpio_task_example: event handler for all gpio-related events
// Includes: fingerprint entry, PIN entry
// Missing: Web application update
static void gpio_task_example(void* arg)
{
    static uint32_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            is_pressed = gpio_get_level(io_num);
            ESP_LOGI("main", "GPIO[%d] intr, val: %d", io_num, is_pressed);
            // NEW: Do action for GPIO4: Add Profile
            //if ((flags & FL_INPUT_READY) != FL_INPUT_READY) {
              //  continue;
            //}

            switch(io_num) {
                case (GPIO_NUM_22) : // door open button from back
                    if (!is_pressed) {
                        break;
                    }
                    // attempt to get lock
                    if (!(flags & FL_INPUT_READY)) {
                        printf("Wait\n");
                        break;
                    }
                    flags &= ~FL_INPUT_READY;

                    if ((flags & FL_FSM) == FL_VERIFYUSER) {
                        // Clear PIN display and contents
                        WS2_msg_clear(&CFAL1602, 1);
                        for (int i=0; i < 16; i++) {
                            pinChar[i] = '\0';
                        }
                        pin_idx = 0;

                        // exception to INPUT_READY: autotoggle off help
                        if (isHelp) {
                            // toggle help
                            isHelp = false;
                        }

                        // open door.
                        open_door();

                        // reset system to verifyUser initial state.
                        restore_to_verifyUser();

                        // return
                    }
                    // release lock
                    flags |= FL_INPUT_READY;
                    break;

                case (GPIO_NUM_4) :
                    if (is_pressed) {
                        break;
                    }
                    // attempt to get lock
                    if (!(flags & FL_INPUT_READY)) {
                        printf("Wait\n");
                        break;
                    }
                    // blocked if help is activated
                    if (isHelp) {
                        printf("Turn off help before trying again\n");
                        break;
                    }
                    flags &= ~FL_INPUT_READY;

                    if ((flags & FL_FSM) == FL_VERIFYUSER) {
                        // Clear PIN display and contents
                        WS2_msg_clear(&CFAL1602, 1);
                        for (int i=0; i < 16; i++) {
                            pinChar[i] = '\0';
                        }
                        pin_idx = 0;

                        // Call verifyUser_fingerprint()
                        verifyUser_fingerprint(&flags, &ret_code, &privilege);

                        // handle all 5 verifyUser outcomes
                        verifyUser_outcome_handler();

                        // return
                    }
                    else if ((flags & FL_FSM) == FL_ADDPROFILE) {
                        addProfile_fingerprint(&flags, &ret_code);
                        if (ret_code != 0) {
                            // release lock
                            flags |= FL_INPUT_READY;
                            break;
                        }
                        if ((flags & (FL_PRIVILEGE | FL_PIN | FL_FP_01)) == 0) {
                            printf("PIN, privilege, fingerprint completed. Press ENTER to complete ADD Profile\n");
                        }
                    }

                    // release lock
                    flags |= FL_INPUT_READY;

                    break;
                default :
                    ESP_LOGE("GPIO", "invalid GPIO number, %d", io_num);
            }
        }
        //esp_task_wdt_reset();
        //printf("loop0\n");
    }
}

void app_main(void)
{
    // 1: Initialize the main project loop
    // a) Install GPIO ISR service
    // GPIO23: 
    // GPIO5: enter button
    // GPIO4: FP scanner input 
    gpio_install_isr_service(0);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // 2: create a queue to handle gpio events from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(gpio_task_example, "gpio_task_example", 4096, NULL, 10, NULL);

    // NEW: initialize CFAL1602!
    WS2_init(&CFAL1602, CFAL1602_MISO, CFAL1602_MOSI, CFAL1602_CLK, CFAL1602_CS);

    // NEW: initialize Keypad!
    Keypad_init(&keypad, makeKeymap(keys), rowPins, colPins, ROWS, COLS);
    xTaskCreate(gpio_keypad_loop, "gpio_keypad_loop", 4096, NULL, 12, NULL);
    // SKIP BOTTOM UNTIL ABOVE WORK SUCCESSFULLY!


    // TEST: Wathcog stuff
    //vTaskDelay();
    //(unsigned long) (esp_timer_get_time() / 1000ULL)
    
    // 3: Init profile recognition

    WS2_msg_print(&CFAL1602, initializing_0, 0, false);
    profileRecog_init();
    WS2_msg_clear(&CFAL1602, 0);
    WS2_msg_clear(&CFAL1602, 1);

    // 4: Init other subsystems

    // 5: configure push-buttons for simulating PIN entry and mode (TEST)
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.pin_bit_mask = 1ULL << RELAY_OUTPUT;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = 0;
    io_conf.pull_down_en = 0;
    gpio_config(&io_conf);

    gpio_config_t io_conf2;
    io_conf2.intr_type = GPIO_PIN_INTR_POSEDGE;
    io_conf2.pin_bit_mask = 1ULL << DOOR_INPUT;
    io_conf2.mode = GPIO_MODE_INPUT;
    io_conf2.pull_up_en = 1;
    gpio_config(&io_conf2);
    gpio_isr_handler_add(GPIO_NUM_22, gpio_isr_handler, (void*) GPIO_NUM_22);

    // 6: start at VerifyUser (FSM = 01). Start in Open Door mode (isAdmin = 0)
    flags = FL_VERIFYUSER | FL_PIN | FL_FP_0 | FL_INPUT_READY;

    // inf: await the push buttons (in gpio_task_example thread)
    if (esp_task_wdt_delete(NULL) != ESP_OK) {
        ESP_LOGW("main", "failure to unsubscribe main loop from task watchdog");
    }

    return;
}
// BLALFGLSGSDGS
