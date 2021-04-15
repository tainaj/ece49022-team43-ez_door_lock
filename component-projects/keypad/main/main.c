/* SPI Master example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "Keypad.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

// Inputs

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
 
// initialize the library with the numbers of the interface pins

void app_main(void) {

  // SETUP
  Keypad_init(&keypad, makeKeymap(keys), rowPins, colPins, ROWS, COLS);

  gpio_config_t io_conf;
  io_conf.intr_type = (gpio_int_type_t)GPIO_PIN_INTR_DISABLE; // pedantic
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = 1ULL << GPIO_NUM_22;
  io_conf.pull_down_en = (gpio_pulldown_t)0; // pedaitic
  io_conf.pull_up_en = (gpio_pullup_t)0; // pedantic
  gpio_config(&io_conf);

  gpio_set_level(GPIO_NUM_22, 1);

  vTaskDelay(3000 / portTICK_PERIOD_MS);

  gpio_set_level(GPIO_NUM_22, 0);

  printf("Welcome!\n");
  printf("Enter PIN or FP.\n");

  // LOOP
  while(1) {
    char key = Keypad_getKey(&keypad);
    if (key != '\0') {
      printf("hi char %c\n", key);
    }
    if (key == '*') {
      printf("asterisk\n");
    }
    else if (key == '#') {
      printf("hashtag\n");
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}