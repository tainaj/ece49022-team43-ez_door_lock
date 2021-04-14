/*
||
|| @file Keypad.cpp
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
#include "Keypad.h"

#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<25) | (1ULL<<26) | (1ULL<<27) | (1ULL<<33))
#define GPIO_INPUT_PIN_SEL  ((1ULL<<34) | (1ULL<<35) | (1ULL<<36) | (1ULL<<39))

/**
 * -------------------------------------------------
 * Private stuff
 * -------------------------------------------------
 */

static unsigned long Keypad_millis() {
	return (unsigned long) (esp_timer_get_time() / 1000ULL);
}

static void Keypad_transitionTo(Keypad *this, uint8_t idx, KeyState nextState) {
	(this->key)[idx].kstate = nextState;
	(this->key)[idx].stateChanged = true;

	// Sketch used the getKey() function.
	// Calls keypadEventListener only when the first key in slot 0 changes state.
	if (this->single_key)  {
	  	if ( (this->keypadEventListener!=NULL) && (idx==0) )  {
			(this->keypadEventListener)((this->key)[0].kchar);
		}
	}
	// Sketch used the getKeys() function.
	// Calls keypadEventListener on any key that changes state.
	else {
	  	if (this->keypadEventListener!=NULL)  {
			(this->keypadEventListener)((this->key)[idx].kchar);
		}
	}
}

// Private
// This function is a state machine but is also used for debouncing the keys.
static void Keypad_nextKeyState(Keypad *this, uint8_t idx, bool button) {

	(this->key)[idx].stateChanged = false;

	switch ((this->key)[idx].kstate) {
		case IDLE:
			if (button==CLOSED) {
				Keypad_transitionTo (this, idx, PRESSED);
				this->holdTimer = Keypad_millis(); }		// Get ready for next HOLD state.
			break;
		case PRESSED:
			if ((Keypad_millis() - (this->holdTimer)) > this->holdTime)	// Waiting for a key HOLD...
				Keypad_transitionTo (this, idx, HOLD);
			else if (button==OPEN)				// or for a key to be RELEASED.
				Keypad_transitionTo (this, idx, RELEASED);
			break;
		case HOLD:
			if (button==OPEN)
				Keypad_transitionTo (this, idx, RELEASED);
			break;
		case RELEASED:
			Keypad_transitionTo (this, idx, IDLE);
			break;
	}
}

// Manage the list without rearranging the keys. Returns true if any keys on the list changed state.
static bool Keypad_updateList(Keypad *this) {

	bool anyActivity = false;

	// Delete any IDLE keys
	for (uint8_t i=0; i<LIST_MAX; i++) {
		if ((this->key)[i].kstate==IDLE) {
			(this->key)[i].kchar = NO_KEY;
			(this->key)[i].kcode = -1;
			(this->key)[i].stateChanged = false;
		}
	}

	// Add new keys to empty slots in the key list.
	for (uint8_t r=0; r<(this->sizeKpd).rows; r++) {
		for (uint8_t c=0; c<(this->sizeKpd).columns; c++) {

			// CHANGE THIS
			//bool button = bitRead(bitMap[r],c);
			bool button = (bool)((this->bitMap)[r] & (1ULL << c));

			char keyChar = (this->keymap)[r * (this->sizeKpd).columns + c];
			int keyCode = r * (this->sizeKpd).columns + c;
			int idx = Keypad_findInList (this, keyCode);
			// Key is already on the list so set its next state.
			if (idx > -1)	{
				Keypad_nextKeyState(this, idx, button);
			}
			// Key is NOT on the list so add it.
			if ((idx == -1) && button) {
				for (uint8_t i=0; i<LIST_MAX; i++) {
					if ((this->key)[i].kchar==NO_KEY) {		// Find an empty slot or don't add key to list.
						(this->key)[i].kchar = keyChar;
						(this->key)[i].kcode = keyCode;
						(this->key)[i].kstate = IDLE;		// Keys NOT on the list have an initial state of IDLE.
						Keypad_nextKeyState (this, i, button);
						break;	// Don't fill all the empty slots with the same key.
					}
				}
			}
		}
	}

	// Report if the user changed the state of any key.
	for (uint8_t i=0; i<LIST_MAX; i++) {
		if ((this->key)[i].stateChanged) anyActivity = true;
	}

	return anyActivity;
}

// Private : Hardware scan
static void Keypad_scanKeys(Keypad *this) {
	// Re-intialize the row pins. Allows sharing these pins with other hardware.

	// bitMap stores ALL the keys that are being pressed.
	for (uint8_t c=0; c<(this->sizeKpd).columns; c++) {

		gpio_set_direction((gpio_num_t)((this->columnPins)[c]), GPIO_MODE_OUTPUT);

		//pin_write(columnPins[c], LOW);	// Begin column pulse output.
		gpio_set_level((gpio_num_t)((this->columnPins)[c]), 0);


		for (uint8_t r=0; r<(this->sizeKpd).rows; r++) {
			//bitWrite(bitMap[r], c, !pin_read(rowPins[r]));  // keypress is active low so invert to high.
			if (!gpio_get_level((gpio_num_t)((this->rowPins)[r]))) {
				(this->bitMap)[r] |= 1ULL << c;	// set bit
			} else {
				(this->bitMap)[r] &= ~(1ULL << c);	// clear bit
			}

		}
		// Set pin to high impedance input. Effectively ends column pulse.
		//pin_write(columnPins[c],HIGH);
		gpio_set_level((gpio_num_t)((this->columnPins)[c]), 1);

		//pin_mode(columnPins[c],INPUT);
		gpio_set_direction((gpio_num_t)((this->columnPins)[c]), GPIO_MODE_INPUT);
	}
}

/**
 * -------------------------------------------------
 * Public API
 * -------------------------------------------------
 */

// <<constructor>> Allows custom keymap, pin configuration, and keypad sizes.
void Keypad_init(Keypad *this, char *userKeymap, uint8_t *row, uint8_t *col,
	uint8_t numRows, uint8_t numCols)
{
	this->rowPins = row;
	this->columnPins = col;
	(this->sizeKpd).rows = numRows;
	(this->sizeKpd).columns = numCols;

	Keypad_begin(this, userKeymap);

	Keypad_setDebounceTime(this, 10);
	Keypad_setHoldTime(this, 500);
	this->keypadEventListener = 0;

	this->startTime = 0;
	this->single_key = false;

	// NEW: Populate Key keys[LIST_MAX]
	for (int i=0; i<LIST_MAX; i++) {
		Key_init(&(this->key[i]));
	}

	// Initialize pin GPIO - rows (input)
	gpio_config_t io_conf;
	io_conf.intr_type = (gpio_int_type_t)GPIO_PIN_INTR_DISABLE;
	io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
	io_conf.mode = GPIO_MODE_INPUT;
	//io_conf.pull_up_en = (gpio_pullup_t)1;
	//io_conf.pull_down_en = (gpio_pulldown_t)0;
	gpio_config(&io_conf);

	// Initialize pin GPIO - cols (output - start input, turn output when checking)
	gpio_config_t io_conf2;
    io_conf2.intr_type = (gpio_int_type_t)GPIO_PIN_INTR_DISABLE;
    io_conf2.mode = GPIO_MODE_INPUT;
    io_conf2.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    io_conf2.pull_up_en = (gpio_pullup_t)1;
    gpio_config(&io_conf2);
}

// Returns a single key only. Retained for backwards compatibility.
char Keypad_getKey(Keypad *this) {
	this->single_key = true;

	if (Keypad_getKeys(this) && (this->key)[0].stateChanged && ((this->key)[0].kstate==PRESSED))
		return (this->key)[0].kchar;
	
	this->single_key = false;

	return NO_KEY;
}

// Populate the key list.
bool Keypad_getKeys(Keypad *this) {
	bool keyActivity = false;

	// Limit how often the keypad is scanned. This makes the loop() run 10 times as fast.
	if ( (Keypad_millis() - this->startTime) > this->debounceTime ) {
		Keypad_scanKeys(this);
		keyActivity = Keypad_updateList(this);
		this->startTime = Keypad_millis();
	}

	return keyActivity;
}

// Backwards compatibility function.
KeyState Keypad_getState(Keypad *this) {
	return (this->key)[0].kstate;
}

// Let the user define a keymap - assume the same row/column count as defined in constructor
void Keypad_begin(Keypad *this, char *userKeymap) {
    this->keymap = userKeymap;
}

// New in 2.1
bool Keypad_isPressed(Keypad *this, char keyChar) {
	for (uint8_t i=0; i<LIST_MAX; i++) {
		if ( (this->key)[i].kchar == keyChar ) {
			if ( ((this->key)[i].kstate == PRESSED) && (this->key)[i].stateChanged )
				return true;
		}
	}
	return false;	// Not pressed.
}

// Minimum debounceTime is 1 mS. Any lower *will* slow down the loop().
void Keypad_setDebounceTime(Keypad *this, uint debounce) {
	this->debounceTime = (debounce<1) ? 1 : debounce;
}

void Keypad_setHoldTime(Keypad *this, uint hold) {
    this->holdTime = hold;
}

void Keypad_addEventListener(Keypad *this, void (*listener)(char)){
	this->keypadEventListener = listener;
}

// Search by character for a key in the list of active keys.
// Returns -1 if not found or the index into the list of active keys.
int Keypad_findInList_c (Keypad *this, char keyChar) {
	for (uint8_t i=0; i<LIST_MAX; i++) {
		if ((this->key)[i].kchar == keyChar) {
			return i;
		}
	}
	return -1;
}

// Search by code for a key in the list of active keys.
// Returns -1 if not found or the index into the list of active keys.
int Keypad_findInList (Keypad *this, int keyCode) {
	for (uint8_t i=0; i<LIST_MAX; i++) {
		if ((this->key)[i].kcode == keyCode) {
			return i;
		}
	}
	return -1;
}

// New in 2.0
char Keypad_waitForKey(Keypad *this) {
	char waitKey = NO_KEY;
	while( (waitKey = Keypad_getKey(this)) == NO_KEY );	// Block everything while waiting for a keypress.
	return waitKey;
}

// The end user can test for any changes in state before deciding
// if any variables, etc. needs to be updated in their code.
bool Keypad_keyStateChanged(Keypad *this) {
	return (this->key)[0].stateChanged;
}

// The number of keys on the key list, key[LIST_MAX], equals the number
// of bytes in the key list divided by the number of bytes in a Key object.
uint8_t Keypad_numKeys(Keypad *this) {
	return sizeof(this->key)/sizeof(Key);
}

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
|| | 1.8 2011-11-21 - Mark Stanley     : Added decision logic to compile WProgram.h or Arduino.h
|| | 1.8 2009-07-08 - Alexander Brevig : No longer uses arrays
|| | 1.7 2009-06-18 - Alexander Brevig : Every time a state changes the keypadEventListener will trigger, if set.
|| | 1.7 2009-06-18 - Alexander Brevig : Added setDebounceTime. setHoldTime specifies the amount of
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
