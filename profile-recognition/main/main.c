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


bool isAdmin; // currently in admin mode (from successin verifying admin PIN/FP)
bool accessAdmin; // currently seeking admin mode (by pressing admin query button)
uint8_t flags = 0; // used to track inputs

// ------TEST ONLY-------------

// Sample input PIN and privilege
uint8_t pin1[4] = {1, 2, 3, 4}; // user
uint8_t pin2[4] = {5, 6, 7, 8}; // admin
uint8_t pin3[4] = {1, 2, 4, 8}; // invalid
uint8_t *pinEnter = pin3;
uint8_t privEnter = 1;          // user-type

// --------------END TEST-----------------



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
