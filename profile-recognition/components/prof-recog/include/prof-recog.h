#ifndef PROF_RECOG_H_
#define PROF_RECOG_H_

#include "R502Interface.h"
#include "SD-Interface.h"

//#include "CFAL1602.h"

/**
 * @mainpage Profile recognition submodule
 * The profile recognition submodule is a major subsystem of EZ Door Lock.
 * 
 * This is an ESP-IDF component developed for the esp32 to interface with the
 * SD card reader via SPI, the R503 module via UART, and main by
 * the activity diagrams
 */

/**
 * \brief Provides command-level api to interact with the profile recognition
 * subsystem
 */

// TEMP: May need to move to its own header
#define FL_INPUT_READY      0x01    // if 0, reject all input entries (finger, ENTER key)

#define FL_PROFILEID        0x02    // profile id (deleteProfile)
#define FL_FP_0             0x04    // finger entry 1 (verifyUser, addProfile)
#define FL_FP_1             0x08    // finger entry 2 (addProfile)
#define FL_FP_01            0x0C    // both finger entries
#define FL_PIN              0x10    // PIN entry (verifyUser, addProfile)
#define FL_PRIVILEGE        0x20    // privilege entry (addProfile)

#define FL_FSM              0xC0
#define FL_IDLESTATE        0x00    // fsm = 0
#define FL_VERIFYUSER       0x40    // fsm = 1
#define FL_DELETEPROFILE    0x80    // fsm = 2
#define FL_ADDPROFILE       0xC0    // fsm = 3

#define MAX_PROFILES 200

/**
 * \brief Profile buffer array containing non-fingerprint data
 */
typedef struct profile_t {
    uint8_t isUsed;
    uint8_t PIN[4];
    uint8_t privilege;
} profile_t; // index of array = position

/**
 * @brief externally defined CFAL1602 struct, defined in main
 */
//extern CFAL1602Interface CFAL1602;

// END TEMP

#define PIN_TXD  (GPIO_NUM_17)
#define PIN_RXD  (GPIO_NUM_16)
#define PIN_IRQ  (GPIO_NUM_4)
#define PIN_RTS  (UART_PIN_NO_CHANGE)
#define PIN_CTS  (UART_PIN_NO_CHANGE)

/**
 * \brief Initialize profile recognition subsystem
 * \retval See vfy_pass for description of all possible return values
 */
esp_err_t profileRecog_init();

/**
 * \brief Search the library for profile with matching fingerprint
 * \param flags status flags
 * \param ret_code OUT for the return code
 * \param privilege OUT for the privilege level of verified users
 * \retval See vfy_pass for description of all possible return values
 */
esp_err_t verifyUser_fingerprint(uint8_t *flags, uint8_t *ret_code, uint8_t *privilege);

/**
 * \brief Search the library for profile with matching input pin
 * \param flags status flags
 * \param pin_input input PIN
 * \param ret_code OUT for the return code
 * \param privilege OUT for the privilege level of verified users
 * \retval See vfy_pass for description of all possible return values
 */
esp_err_t verifyUser_PIN(uint8_t *flags, uint8_t *pin_input, uint8_t *ret_code, uint8_t *privilege);

/**
 * \brief Add one of two fingerprints required to form a template
 * \param flags status flags
 * \param ret_code OUT for the return code
 * \retval See vfy_pass for description of all possible return values
 */
esp_err_t addProfile_fingerprint(uint8_t *flags, uint8_t *ret_code);

/**
 * \brief Add a PIN and verify that it is currently unused
 * \param flags status flags
 * \param pin_input input PIN
 * \param ret_code OUT for the return code
 * \retval See vfy_pass for description of all possible return values
 */
esp_err_t addProfile_PIN(uint8_t *flags, uint8_t *pin_input, uint8_t *ret_code);

/**
 * \brief Add a privilege level (occurs after adding a PIN)
 * \param flags status flags
 * \param privilege input privilege level
 * \param ret_code OUT for the return code
 * \retval See vfy_pass for description of all possible return values
 */
esp_err_t addProfile_privilege(uint8_t *flags, uint8_t privilege, uint8_t *ret_code);

/**
 * \brief Compile and save profile to FP module and SD card flash (occurs after PIN, privilege, and fingerprint inserted)
 * \param flags status flags
 * \param ret_code OUT for the return code
 * \retval See vfy_pass for description of all possible return values
 */
esp_err_t addProfile_compile(uint8_t *flags, uint8_t *ret_code);

/**
 * \brief Compile and save profile to FP module and SD card flash (occurs after PIN, privilege, and fingerprint inserted)
 * \param flags status flags
 * \param prof_id input profile id
 * \param ret_code OUT for the return code
 * \retval See vfy_pass for description of all possible return values
 */
esp_err_t deleteProfile_remove(uint8_t *flags, int prof_id, uint8_t *ret_code);

#endif /* PROF_RECOG_H_ */