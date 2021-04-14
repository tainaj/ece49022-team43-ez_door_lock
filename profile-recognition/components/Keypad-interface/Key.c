/*
|| @file Key.cpp
|| @version 1.0
|| @author Mark Stanley, Joel Taina
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
#include "Key.h"


// default constructor
void Key_init(Key *this) {
	this->kchar = NO_KEY;
	this->kstate = IDLE;
	this->stateChanged = false;
}

// constructor
void Key_init_c(Key *this, char userKeyChar) {
	this->kchar = userKeyChar;
	this->kcode = -1;
	this->kstate = IDLE;
	this->stateChanged = false;
}

void Key_key_update (Key *this, char userKeyChar, KeyState userState, bool userStatus) {
	this->kchar = userKeyChar;
	this->kstate = userState;
	this->stateChanged = userStatus;
}



/*
|| @changelog
|| | 1.0 2012-06-04 - Mark Stanley : Initial Release
|| #
*/
