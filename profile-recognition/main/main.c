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

#include "R502Interface.h"

#define PIN_TXD  (GPIO_NUM_17)
#define PIN_NC_1 (GPIO_NUM_25)
#define PIN_NC_2 (GPIO_NUM_26)
#define PIN_NC_3 (GPIO_NUM_27)
#define PIN_RXD  (GPIO_NUM_16)
#define PIN_IRQ  (GPIO_NUM_4)
#define PIN_RTS  (UART_PIN_NO_CHANGE)
#define PIN_CTS  (UART_PIN_NO_CHANGE)

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

uint8_t flags;

// The object under test
R502Interface R502 = {
    //.gpio_evt_queue = gpio_evt_queue;
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

void tearDown(){
    R502_deinit(&R502);
}

// NEW: Move ISR handling to main.c
xQueueHandle gpio_evt_queue = NULL;

int count = 0;

R502_data_len_t starting_data_len;

static void gpio_task_example(void* arg)
{
    static uint32_t io_num;
    static R502_conf_code_t conf_code;
    static int is_pressed;
    static uint16_t page_id = 0xeeee;
    static uint16_t match_score = 0xeeee;

    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            is_pressed = gpio_get_level(io_num);
            ESP_LOGI("main", "GPIO[%d] intr, val: %d", io_num, is_pressed);
            // NEW: Do action for GPIO4: Add Profile
            //if ((flags & FL_INPUT_READY) != FL_INPUT_READY) {
            //    continue;
            //}

            switch(io_num) {
                case (GPIO_NUM_4) :
                    // Case GPIO4: press finger on scanner

                    /*if (flags & (FL_FP_01)) {
                        flags &= ~FL_INPUT_READY;

                        ESP_LOGI("main", "count: %d", count);

                        // 1: Wait for fingerprint. When received, wait for 500ms.
                        printf("Scanning fingerprint...\n");
                        vTaskDelay(20 / portTICK_PERIOD_MS);
                        
                        // 2: Action. GenImg(). Abort if fail
                        R502_gen_image(&R502, &conf_code);
                        ESP_LOGI("main", "genImg res: %d", (int)conf_code);
                        if (conf_code != R502_ok) {
                            printf("Bad finger entry: try again\n");
                            flags |= FL_INPUT_READY;
                            break;
                        } 

                        // 3: Action: Img2Tz(). Abort if fail
                        R502_img_2_tz(&R502, count+1, &conf_code);
                        ESP_LOGI("main", "Img2Tz res: %d", (int)conf_code);
                        if (conf_code != R502_ok) {
                            printf("Bad finger entry: try again\n");
                            flags |= FL_INPUT_READY;
                            break;
                        }

                        flags &= ~FL_FP_0;

                        // 4: Decision.
                        // A: VerifyUser (01): search up char file against library and return resuit
                        if ((flags & FL_FSM) == FL_VERIFYUSER) {
                            R502_search(&R502, 1, 0, 0xffff, &conf_code, &page_id, &match_score);
                            ESP_LOGI("main", "Search res: %d", (int)conf_code);
                            if (conf_code != R502_ok) {
                                printf("Access denied\n");
                                flags |= FL_FP_0;
                                flags |= FL_INPUT_READY;
                                break;
                            }
                            flags &= ~FL_PIN;
                            flags |= FL_SUCCESS;
                            printf("Accepted: %d\n", page_id);
                            flags &= ~FL_SUCCESS;
                            flags |= FL_FP_0;
                        }
                    }*/

                    if(is_pressed) {
                        printf("Release finger\n");
                    }
                    else {
                        ESP_LOGI("main", "count: %d", count);

                        // 1: Wait for fingerprint. When received, wait for 500ms.
                        printf("Scanning fingerprint...\n");
                        vTaskDelay(20 / portTICK_PERIOD_MS);
                        
                        // 2: Action. GenImg(). Abort if fail
                        R502_gen_image(&R502, &conf_code);
                        ESP_LOGI("main", "genImg res: %d", (int)conf_code);
                        if (conf_code != R502_ok) {
                            printf("Bad finger entry: try again\n");
                            break;
                        }

                        // 3: Action: Img2Tz(). Abort if fail
                        R502_img_2_tz(&R502, count+1, &conf_code);
                        ESP_LOGI("main", "Img2Tz res: %d", (int)conf_code);
                        if (conf_code != R502_ok) {
                            printf("Bad finger entry: try again\n");
                            break;
                        }

                        if (count == 1) {
                            count = 0;
                            // 4: Action: RegModel()
                            //while(R502_reg_model(&R502, &conf_code) != R502_);
                            R502_reg_model(&R502, &conf_code);
                            ESP_LOGI("main", "regModel res: %d", (int)conf_code);
                            if (conf_code != R502_ok) {
                                printf("Bad finger entry: try again\n");
                                break;
                            }
                            
                            // 5: Action: UpChar()
                            R502_up_char(&R502, starting_data_len, 1, &conf_code);
                            ESP_LOGI("main", "upChar res: %d", (int)conf_code);
                            if (conf_code != R502_ok) {
                                printf("Failed to upload template\n");
                                break;
                            }
                            
                            printf("Successful upload!\n");
                        }
                        else {
                            count = 1;
                        }
                        
                    }

                    break;
                default :
                    ESP_LOGE("GPIO", "invalid GPIO number, %d", io_num);
            }
        }
    }
}

static int up_char_size = 0;
void up_char_callback(uint8_t data[R502_max_data_len], 
    int data_len)
{
    // this is where you would store or otherwise do something with the image
    up_char_size += data_len;

    //int box_width = 16;
    //int total = 0;
    //while(total < data_len){
        //for(int i = 0; i < box_width; i++){
            //printf("0x%02X ", data[total]);
            //total++;
        //}
        //printf("\n");
    //}
}

void app_main(void)
{
    // 1: hello world
    flags = 0;

    printf("Hello world!\n");

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // 2: create a queue to handle gpio events from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(gpio_task_example, "gpio_task_example", 4096, NULL, 10, NULL);

    // 3: init UART
    R502_init(&R502, UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ, R502_baud_115200);

    R502_conf_code_t conf_code;
    R502_sys_para_t sys_para;

    // 4: collect current data package length (128?)
    R502_read_sys_para(&R502, &conf_code, &sys_para);
    starting_data_len = sys_para.data_package_length;

    // 5: set up upChar collector code
    R502_set_up_char_cb(&R502, up_char_callback);
    up_char_size = 0;

    // 6: start at VerifyUser (FSM = 01)
    flags |= FL_VERIFYUSER | FL_PIN | FL_FP_0 | FL_INPUT_READY;

    // inf: await the push buttons (in gpio_task_example thread
}
