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
/*DRAM_ATTR static const char* enter_pin =
    "PIN: Press for.."
    "PIN: #     ENTER"
    "PIN: * backspace"
    "PIN: A     admin";*/

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

DRAM_ATTR static const char* entering_admin =
    "Entering admin..";

DRAM_ATTR static const char* invalid_pin =
    "Invalid PIN";
DRAM_ATTR static const char* must_be_4_chars =
    "Must be 4 chars";

DRAM_ATTR static const char* admin_verify =
    "Admin: Verify";

DRAM_ATTR static const char* canceled =
    "Canceled";

// NEW: help thing
DRAM_ATTR static const char* help1 =
    "The purpose is to experience test, "
    "in the face of certain test, "
    "to accept that test, "
    "to maintain control of oneself in one's test."
    "It is a subroutine expected of every Starfleet captain.    -Spock, [YTP] Spock is Emotionally Compromised";

DRAM_ATTR static const char* help2 =
    "I have a new hearing aid Emperor. "
    "Or should I call you, Darth Idiot? "
    "Or should I call you, Darth Arthur? "
    "Or should I call you, Darth Darthius? ";

// Idle State messages
DRAM_ATTR static const char* admin_menu =
    "Admin: Menu";

DRAM_ATTR static const char* item1 =
    "1: Add Profile";

DRAM_ATTR static const char* item2 =
    "2: Del Profile";

DRAM_ATTR static const char* item3 =
    "3: Exit Admin";

DRAM_ATTR static const char* selected_this =
    "Selected";

// Add Profile messages
DRAM_ATTR static const char* admin_add =
    "Admin: Add";

// Delete Profile messages
DRAM_ATTR static const char* admin_delete =
    "Admin: Delete";

// Delete Profile messages
DRAM_ATTR static const char* leaving_admin =
    "Leaving Admin...";

#endif /* MESSAGES_H */