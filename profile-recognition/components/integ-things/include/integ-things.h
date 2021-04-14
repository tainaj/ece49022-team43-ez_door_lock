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

/**
 * @brief reset system to verifyUser initial state.
 *        - restore flags to verifyUser init
 *        - set admin to 0 (door open select)
 *        - init screen for verifyUser
 * @return See vfy_pass for description of all possible return values
 */
void restore_to_verifyUser();

#endif /* INTEG_THINGS_H_ */