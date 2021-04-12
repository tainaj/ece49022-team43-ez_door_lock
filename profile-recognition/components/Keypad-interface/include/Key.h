/*
||
|| @file Key.h
|| @version 1.0
|| @author Mark Stanley
|| @contact mstanley@technologist.com
||
|| @description
|| | Key class provides an abstract definition of a key or button
|| | and was initially designed to be used in conjunction with a
|| | state-machine.
|| #
||
|| @license
|| | This library is free software; you can redistribute it and/or
|| | modify it under the terms of the GNU Lesser General Public
|| | License as published by the Free Software Foundation; version
|| | 2.1 of the License.
|| |
|| | This library is distributed in the hope that it will be useful,
|| | but WITHOUT ANY WARRANTY; without even the implied warranty of
|| | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
|| | Lesser General Public License for more details.
|| |
|| | You should have received a copy of the GNU Lesser General Public
|| | License along with this library; if not, write to the Free Software
|| | Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
|| #
||
*/

#ifndef Keypadlib_KEY_H_
#define Keypadlib_KEY_H_

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

#define OPEN false // previously LOW
#define CLOSED true // previously HIGH

typedef unsigned int uint;
typedef enum{ IDLE, PRESSED, HOLD, RELEASED } KeyState;

#define NO_KEY '\0' // temp

typedef struct Key {
	// public variables
	char kchar;
	int kcode;
	KeyState kstate;
	bool stateChanged;

    // private variables - NULL
} Key;

/**
 * @brief default constructor (no parameters)
 */
void Key_init(Key *this);

/**
 * @brief constructor
 * @param userKeyChar input character
 */
void Key_init_c(Key *this, char userKeyChar);

/**
 * @brief default constructor (no parameters)
 * @param userKeyChar input character
 * @param userState initial state
 * @param userStatus initial status
 */
void Key_key_update(Key *this, char userKeyChar, KeyState userState, bool userStatus);

#endif

/*
|| @changelog
|| | 1.0 2012-06-04 - Mark Stanley : Initial Release
|| #
*/
