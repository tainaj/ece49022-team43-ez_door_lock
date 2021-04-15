/* SPI Master example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "CFAL1602.h"

static const char* TAG = "default_event_loop";

// CFAL1602 this object
CFAL1602Interface CFAL1602 = {
    .TAG = "CFAL1602C-PB",
    .initialized = false,
    .line_len = 16,
    .line_count = 2,
    .timer_period = 1000000
};

// Example text messages
DRAM_ATTR static const char* message1 =
    "This is a long  "
    "statement.      "
    "Please feel free"
    "to edit. No     "
    "chance.         "
    "JK not dead";

DRAM_ATTR static const char* message2 = 
    "Space-time!";

DRAM_ATTR static const char* message3 =
    "I have a bad feeling about this. "
    "Shut up, Obi-Wan.      ";

// main function
void app_main(void)
{
    WS2_init(&CFAL1602, PIN_NUM_MISO, PIN_NUM_MOSI, PIN_NUM_CLK, PIN_NUM_CS);

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // Begin event loops!

    ESP_LOGI(TAG, "setting up");

    // Start of function "print message"

    // Use WS2_mag_print function
    WS2_msg_print(&CFAL1602, message1, 0, false);

    // NEW: print a new message after 10 seconds
    vTaskDelay(9800 / portTICK_PERIOD_MS);
    WS2_msg_print(&CFAL1602, message2, 0, false);

    // NEW: print a new message after 10 seconds
    vTaskDelay(9500 / portTICK_PERIOD_MS);
    WS2_msg_print(&CFAL1602, message3, 0, true);

    // NEW: print a new message after 10 seconds
    vTaskDelay(9500 / portTICK_PERIOD_MS);
    WS2_msg_print(&CFAL1602, message1, 1, true);

    // NEW: print a new message after 10 seconds
    vTaskDelay(19500 / portTICK_PERIOD_MS);
    WS2_msg_print(&CFAL1602, message2, 1, true);

    vTaskDelay(9500 / portTICK_PERIOD_MS);
    WS2_msg_clear(&CFAL1602, 0);
}