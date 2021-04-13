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

// Example text messages
DRAM_ATTR static const char* message1 =
    "This is a long  "
    "statement.      "
    "Please feel free"
    "to edit. No     "
    "chance.         "
    "JK not dead"; // use block

DRAM_ATTR static const char* message2 = 
    "Space-time!";

DRAM_ATTR static const char* message3 =
    "I have a bad feeling about this. "
    "Shut up, Obi-Wan.      ";

// Verify User messages
DRAM_ATTR static const char* welcome =
    "Welcome, weary traveller!                ";

DRAM_ATTR static const char* enter_pin =
    "PIN. Press # to ENTER.     ";

DRAM_ATTR static const char* pin_too_small =
    "Invalid PIN:    "
    "Must be 4 chars."; // use block

DRAM_ATTR static const char* pin_entered =
    "PIN entered...";

#endif /* MESSAGES_H */