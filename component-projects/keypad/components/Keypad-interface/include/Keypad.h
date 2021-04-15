/*
||
|| @file Keypad.h
|| @version 3.1
|| @author Mark Stanley, Alexander Brevig, Joel Taina
|| @contact mstanley@technologist.com, alexanderbrevig@gmail.com
||
|| @description
|| | This library provides a simple interface for using matrix
|| | keypads. It supports multiple keypresses while maintaining
|| | backwards compatibility with the old single key library.
|| | It also supports user selectable pins and definable keymaps.
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

#ifndef KEYPAD_H
#define KEYPAD_H

#include "Key.h"

#define OPEN false
#define CLOSED true

typedef char KeypadEvent;
typedef unsigned int uint;
typedef unsigned long ulong;

// Made changes according to this post http://arduino.cc/forum/index.php?topic=58337.0
// by Nick Gammon. Thanks for the input Nick. It actually saved 78 bytes for me. :)
typedef struct {
    uint8_t rows;
    uint8_t columns;
} KeypadSize;

#define LIST_MAX 10		// Max number of keys on the active list.
#define MAPSIZE 10		// MAPSIZE is the number of rows (times 16 columns)
#define makeKeymap(x) ((char*)x)

// NEW: Create dtruct typedef as template for a C++ object class. Public and private variables
typedef struct Keypad {
	// public variables
	uint bitMap[MAPSIZE];	// 10 row x 16 column array of bits. Except Due which has 32 columns.
	Key key[LIST_MAX];
	unsigned long holdTimer;

    // private variables
	unsigned long startTime;
	char *keymap;
    uint8_t *rowPins;
    uint8_t *columnPins;
	KeypadSize sizeKpd;
	uint debounceTime;
	uint holdTime;
	bool single_key;
	void (*keypadEventListener)(char);
} Keypad;

/**
 * @brief initialize interface, must call first
 * @param userKeymap ROW x COL array of keypad characters
 * @param row GPIO row pins
 * @param col GPIO col pins
 * @param numRows count of row pins
 * @param numCols count of col pins
 */
void Keypad_init(Keypad *this, char *userKeymap, uint8_t *row, uint8_t *col,
	uint8_t numRows, uint8_t numCols);

/**
 * @brief Returns a single key only. Retained for backwards compatibility.
 * @retval character pressed
 */
char Keypad_getKey(Keypad *this);

/**
 * @brief Populate the key list.
 * @retval success in getting a key press (debounce)
 */
bool Keypad_getKeys(Keypad *this);

//KeyState getState();

/**
 * @brief Backwards compatibility function.
 * @retval state of button press
 */
KeyState Keypad_getState(Keypad *this);

/**
 * @brief Let the user define a keymap - assume the same row/column count as defined in constructor
 * @param userKeymap ROW x COL array of keypad characters
 */
void Keypad_begin(Keypad *this, char *userKeymap);

/**
 * @brief New in 2.1
 * @param keyChar Character to check
 * @retval is button currently pressed?
 */
bool Keypad_isPressed(Keypad *this, char keyChar);

/**
 * @brief Let the user define a keymap - assume the same row/column count as defined in constructor
 * @param debounce number of milliseconds to debounce keys
 */
void Keypad_setDebounceTime(Keypad *this, uint debounce);

/**
 * @brief Let the user define a keymap - assume the same row/column count as defined in constructor
 * @param hold number of milliseconds to register a hold press
 */
void Keypad_setHoldTime(Keypad *this, uint hold);

/**
 * @brief Self-explanatory
 * @param listener event listener (function pointer)
 */
void Keypad_addEventListener(Keypad *this, void (*listener)(char));

/**
 * @brief Search by character for a key in the list of active keys.
 * @param keyChar character for a key
 * @retval Returns -1 if not found or the index into the list of active keys.
 */
int Keypad_findInList_c(Keypad *this, char keyChar);


/**
 * @brief Search by code for a key in the list of active keys.
 * @param keyCode code for a key
 * @retval Returns -1 if not found or the index into the list of active keys.
 */
int Keypad_findInList(Keypad *this, int keyCode);

/**
 * @brief New in 2.0
 * @retval Key character retrieved
 */
char Keypad_waitForKey(Keypad *this);

/**
 * @brief New in 2.0
 * @retval Has key state changed?
 */
bool Keypad_keyStateChanged(Keypad *this);

/**
 * @brief New in 2.0
 * @retval number of keys on the key list
 */
uint8_t Keypad_numKeys(Keypad *this);

#endif

/*
|| @changelog
|| | 3.X 2021-04-10 - Joel Taina       : Converted libraries from C++ to C
|| | 3.X 2021-04-09 - Joel Taina       : Revised libraries to work directly with ESP32 (instead of Arduino)
|| | 3.1 2013-01-15 - Mark Stanley     : Fixed missing RELEASED & IDLE status when using a single key.
|| | 3.0 2012-07-12 - Mark Stanley     : Made library multi-keypress by default. (Backwards compatible)
|| | 3.0 2012-07-12 - Mark Stanley     : Modified pin functions to support Keypad_I2C
|| | 3.0 2012-07-12 - Stanley & Young  : Removed static variables. Fix for multiple keypad objects.
|| | 3.0 2012-07-12 - Mark Stanley     : Fixed bug that caused shorted pins when pressing multiple keys.
|| | 2.0 2011-12-29 - Mark Stanley     : Added waitForKey().
|| | 2.0 2011-12-23 - Mark Stanley     : Added the public function keyStateChanged().
|| | 2.0 2011-12-23 - Mark Stanley     : Added the private function scanKeys().
|| | 2.0 2011-12-23 - Mark Stanley     : Moved the Finite State Machine into the function getKeyState().
|| | 2.0 2011-12-23 - Mark Stanley     : Removed the member variable lastUdate. Not needed after rewrite.
|| | 1.8 2011-11-21 - Mark Stanley     : Added test to determine which header file to compile,
|| |                                          WProgram.h or Arduino.h.
|| | 1.8 2009-07-08 - Alexander Brevig : No longer uses arrays
|| | 1.7 2009-06-18 - Alexander Brevig : This library is a Finite State Machine every time a state changes
|| |                                          the keypadEventListener will trigger, if set
|| | 1.7 2009-06-18 - Alexander Brevig : Added setDebounceTime setHoldTime specifies the amount of
|| |                                          microseconds before a HOLD state triggers
|| | 1.7 2009-06-18 - Alexander Brevig : Added transitionTo
|| | 1.6 2009-06-15 - Alexander Brevig : Added getState() and state variable
|| | 1.5 2009-05-19 - Alexander Brevig : Added setHoldTime()
|| | 1.4 2009-05-15 - Alexander Brevig : Added addEventListener
|| | 1.3 2009-05-12 - Alexander Brevig : Added lastUdate, in order to do simple debouncing
|| | 1.2 2009-05-09 - Alexander Brevig : Changed getKey()
|| | 1.1 2009-04-28 - Alexander Brevig : Modified API, and made variables private
|| | 1.0 2007-XX-XX - Mark Stanley : Initial Release
|| #
*/
