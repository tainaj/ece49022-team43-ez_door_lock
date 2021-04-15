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

void app_main(void)
{
    // 1st test: hello world
    printf("Hello world!\n");

    // 2nd test: init UART
    esp_err_t err = R502_init(&R502, UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ, R502_baud_115200);
    //TEST_ESP_OK(err);
    R502_conf_code_t conf_code;

    R502_led_ctrl_t control_codes[6] = { R502_led_ctrl_breathing,
        R502_led_ctrl_flashing, R502_led_ctrl_always_on,
        R502_led_ctrl_always_off, R502_led_ctrl_grad_on,
        R502_led_ctrl_grad_off };

    R502_led_color_t color = R502_led_color_blue;
    uint8_t speed = 0x7f;
    uint8_t cycles = 0x5;

    // Loop through all control codes
    for(int ctrl_i = 0; ctrl_i < 6; ctrl_i++){
        //ESP_LOGI(R502.TAG, "Test led ctrl %d", control_codes[ctrl_i]);

        err = R502_led_config(&R502, control_codes[ctrl_i], speed, color, cycles, &conf_code);
        //TEST_ESP_OK(err);
        //TEST_ASSERT_EQUAL(R502_ok, conf_code);

        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }

    // turn off
    err = R502_led_config(&R502, R502_led_ctrl_always_off, speed, color, cycles, &conf_code);
    //TEST_ESP_OK(err);
    //TEST_ASSERT_EQUAL(R502_ok, conf_code);
}
