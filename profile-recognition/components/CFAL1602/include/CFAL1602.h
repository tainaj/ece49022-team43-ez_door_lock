/**
 * @mainpage CFAL1602 OLED display code
 * This code is an abstraction interface for use by the overall system.
 * 
 * This is an ESP-IDF component developed for the esp32 to interface with the
 * OLED display via SPI
 */

/**
 * \brief Provides command-level api to interact with the OLED display
 */

#ifndef CFAL1602_H_
#define CFAL1602_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "event_source.h"
#include "messages.h"

#ifdef CONFIG_IDF_TARGET_ESP32
#define LCD_HOST    VSPI_HOST
#define DMA_CHAN    2
#endif

/*#define PIN_NUM_MISO -1
#define PIN_NUM_MOSI 15
#define PIN_NUM_CLK  14
#define PIN_NUM_CS   13
#endif*/

/**
 * @brief Command code leader bit
 */
typedef enum {
    WS2_clear     = 0x01,
    WS2_home      = 0x02,
    WS2_entry     = 0x04,
    WS2_display   = 0x08,
    WS2_cursor    = 0x10,
    WS2_function  = 0x20,
    WS2_setCGRAM  = 0x40, // unused
    WS2_setDDRAM  = 0x80,
    WS2_busy      = 0x100, // unused
    WS2_data_write = 0x200,
    WS2_data_read = 0x300 // unused
} WS0010_cmd_t;

/**
 * @brief Command code parameters (see datasheet)
 */
typedef enum {
    WS2_entry_S = 0x01,
    WS2_entry_I_D = 0x02,
    WS2_display_B = 0x01,
    WS2_display_C = 0x02,
    WS2_display_D = 0x04,
    WS2_cursor_R_L = 0x04,
    WS2_cursor_S_C = 0x08,
    WS2_function_FT00 = 0x00, // English-Japanese
    WS2_function_FT01 = 0x01, // Western European 1
    WS2_function_FT10 = 0x10, // English-Russian
    WS2_function_FT11 = 0x11, // Western European 2
    WS2_function_F = 0x04,
    WS2_function_N = 0x08,
    WS2_function_DL = 0x10,
} WS0010_cmd_param_t;

// NEW: Add CFAL1602 this object
// WARNING: move to CFAL1602.h when done
typedef struct CFAL1602Interface {
    // public variables - NULL

    // identifier tag
    const char *TAG;

    // initialized bool
    bool initialized;

    // callbacks - NULL

    // spi handle
    spi_device_handle_t spi;

    // parameters - line 0
    const char* msg_0;       // used for print
    bool isAutoScroll_0;     // used for print
    int count_0;             // used to keep track of str position

    // parameters - line 1
    const char* msg_1;
    bool isAutoScroll_1;
    int count_1;

    // GPIO pins
    gpio_num_t pin_miso;
    gpio_num_t pin_mosi;
    gpio_num_t pin_sck;
    gpio_num_t pin_cs;

    // private constants
    const int line_len;
    const int line_count;
    const uint64_t timer_period;
} CFAL1602Interface;

void WS2_init(CFAL1602Interface *this, gpio_num_t _pin_miso,
    gpio_num_t _pin_mosi, gpio_num_t _pin_sck, gpio_num_t _pin_cs);

/**
 * @brief Print a message to the CFAL1602 (WS0010) OLED text display
 * @param msg           char string to print
 * @param line          line number, 0 (TOP) or 1 (BOTTOM)
 * @param isAutoScroll  bool for enabling auto-scroll text
 * @return none
 */
void WS2_msg_print(CFAL1602Interface *this, const char* msg, int line,
    bool isAutoScroll);

/**
 * @brief Clear a line on the CFAL1602 (WS0010) OLED text display
 * @param line          line number, 0 (TOP) or 1 (BOTTOM)
 * @return none
 */
void WS2_msg_clear(CFAL1602Interface *this, int line);

#endif /* CFAL1602_H */