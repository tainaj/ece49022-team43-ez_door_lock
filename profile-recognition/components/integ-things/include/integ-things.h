#ifndef INTEG_THINGS_H_
#define INTEG_THINGS_H_

#include "prof-recog.h"

//#include "CFAL1602.h"

/**
 * @mainpage Integration of things submodule
 * The profile recognition submodule is a major subsystem of EZ Door Lock.
 * 
 * This is an ESP-IDF component developed for the esp32 to interface with the
 * SD card reader via SPI, the R503 module via UART, and main by
 * the activity diagrams
 */

/**
 * \brief Provides command-level api to interact with the integration of things
 * subsystem
 */

#define RELAY_OUTPUT 21
#define DOOR_INPUT 22

/**
 * @brief reset system to verifyUser initial state.
 *        - restore flags to verifyUser init
 *        - set admin to 0 (door open select)
 *        - init screen for verifyUser
 * @return none
 */
void restore_to_verifyUser();

/**
 * @brief reset system to idleState initial state.
 *        - restore flags to idleState init
 *        - set admin to 0 (addProfile select)
 *        - init screen for idleState. Show item 1 (0-exit, 1-add, 2-delete)
 * @return none
 */
void restore_to_idleState();

/**
 * @brief open door.
 *        - Toggle GPIO21 on (relay output)
 *        - print "door open"
 *        - Toggle GPIO21 off (relay output)
 * @return none
 */
void open_door();

#endif /* INTEG_THINGS_H_ */