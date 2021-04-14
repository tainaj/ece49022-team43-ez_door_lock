/**
 * @mainpage Message buffer file for CFAL1602 OLED text display
 * 
 * This is an ESP-IDF component developed for the esp32 to interface with the
 * OLED display via SPI
 */

/**
 * \brief Provides command-level api to interact with the OLED display
 */

#ifndef MESSAGES_H_
#define MESSAGES_H_

// Initialization messages
DRAM_ATTR static const char* initializing_0 =
    "Initializing... ";

// Verify User messages
DRAM_ATTR static const char* enter_pin =
    "PIN: Press for.."
    "PIN: #     ENTER"
    "PIN: * backspace"
    "PIN: A     admin";

DRAM_ATTR static const char* invalid_pin =
    "Invalid PIN";

DRAM_ATTR static const char* must_be_4_chars =
    "Must be 4 chars";

DRAM_ATTR static const char* pin_entered =
    "PIN entered...";

DRAM_ATTR static const char* scanning =
    "Scanning...";

DRAM_ATTR static const char* bad_fingerprint_entry_0 =
    "Bad fingerprint";

DRAM_ATTR static const char* bad_fingerprint_entry_1 =
    "entry";

DRAM_ATTR static const char* access_denied =
    "Access denied";

DRAM_ATTR static const char* access_granted =
    "Access granted";

DRAM_ATTR static const char* door_open =
    "Door open";

DRAM_ATTR static const char* checking_pin =
    "Checking PIN...";

DRAM_ATTR static const char* entering_admin =
    "Entering admin..";

DRAM_ATTR static const char* admin_verify =
    "Admin: Verify";

DRAM_ATTR static const char* canceled =
    "Canceled";


// Idle State messages
DRAM_ATTR static const char* admin_menu =
    "Admin: Menu";

DRAM_ATTR static const char* item1 =
    "1: Add Profile";

DRAM_ATTR static const char* item2 =
    "2: Del Profile";

DRAM_ATTR static const char* item3 =
    "3: Exit Admin";

#endif /* MESSAGES_H */