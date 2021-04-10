# ECE 49022 Senior Design Team 43: EZ Door Lock
This is an **ESP-IDF** project developed for the **esp32** to operate the EZ Door Lock.

A ECE 49022 senior design project to be submitted to College of Electrical and Computer Engineering, Purdue University. This is a collaborative effort by authors Sung-Woo Jang, Benjamin Oh, and Joel Taina. Each major subsystem was designed by the corresponding author.

## Synopsis
The EZ Door Lock is an electronic access control system, to be installed on doors, that adopts
fingerprint recognition to provide speedy, personalized authorization for users.

### Mode of operation
EZ Door Lock will detect and evaluate a finger pattern in less than a
second, providing speedy entry for allowed users. Upon a successful entry, the door will unlock;
it will lock automatically once the door is shut. PIN entry is used both as a means to access
manually as well as access to admin control, which allows admins to add and remove profiles.

### Requirements
* Software must be capable of managing at least 16 users
* Scanner must recognize valid fingerprints faster than entering a PIN
* Lock system must accept PINs as backup access
* Software must enforce identity management, restricting admin control to privileged users
* Lock system must fit in standard residential entry door
* Lock system must be capable of operating with battery power for at least 6 months
* Software must preserve profile data when power is cut

## Major subsystems
EZ Door Lock is controlled by the ESP32-WROOM-32 Wi-Fi module. It consists of the following subsystems:
* Profile Recognition
* User Interface and Interaction of Things
* Wireless Communication

### Profile Recognition
*Authored by Joel Taina*

The system is composed of the following hardware:
* SD card
* R503 fingerprint module

ESP32 development boards need to be connected to the SD card as follows:

ESP32 pin        | SD card pin | SPI pin  | Notes
-----------------|-------------|----------|------------
GPIO18 (VSPICLK) | CLK         | SCK      | 10k pullup (on SD reader schematic)
GPIO23 (VSPID)   | CMD         | MOSI     | 10k pullup (on SD reader schematic)
GPIO19 (VSPIQ)   | D0          | MISO     | 10k pullup (on SD reader schematic)
GPIO5 (VSPICS0)  | D3          | CS       | 

ESP32 development boards need to be connected to the R503 module as follows:

ESP32 pin        | R503 pin | Notes
-----------------|----------|------------
GPIO16 (U2RXD)   | TXD      | Data output. TTL logical level
GPIO17 (U2TXD)   | RXD      | Data input. TTL logical level
GPIO4            | WAKEUP   | Finger Detection Signal. 1nF debouncing capacitor to GND

**Note about GPIO2**

GPIO2 pin is used as a bootstrapping pin, and should be low to enter UART download mode. One way to do this is to connect GPIO0 and GPIO2 using a jumper, and then the auto-reset circuit on most development boards will pull GPIO2 low along with GPIO0, when entering download mode.

- Some boards have pulldown and/or LED on GPIO2. LED is usually ok, but pulldown will interfere with D0 signals and must be removed. Check the schematic of your development board for anything connected to GPIO2.

### User Interface and Integration of Things
*Authored by Sung-Woo Jang*

The system is composed of the following hardware:
* CFAL1602 OLED display
* 4x4 keypad
* Power Relay module
* Solenoid lock
* Push Button


ESP32 development boards need to be connected to the 4x4 Keypad as follows:

ESP32 PIN  |   pin   | Notes
-----------|---------|------------
GPIO25     |   COL1  | Output
GPIO26     |   COL2  | Output
GPIO27     |   COL3  | Output
GPIO33     |   COL4  | Output
GPIO34     |   ROW1  | Input, 10k pullup
GPIO35     |   ROW2  | Input, 10k pullup
GPIO36     |   ROW3  | Input, 10k pullup
GPIO39     |   ROW4  | Input, 10k pullup


ESP32 development boards need to be connected to the Relay and Push Button as follows:

ESP32 PIN |      pin      | Notes
----------|---------------|--------
GPIO22    |  Push Button  | Input
GPIO21    |     Relay     | Output


ESP32 development boards need to be connected to the CFAL1602 OLED Display as follows:

ESP32 PIN  | LCD pin | Notes
-----------|---------|--------
GPIO14     |   SDK   |
GPIO15     |   MOSI  |
GPIO13     |    CS   |

*Joel will help with!*


### Wireless Communication (requires editing)
*Authored by Benjamin Oh*

This subsystem doesn't require any special hardware, and can run on any ESP32 development board.

## Other information

### Code Style
The source code is developed following the ESP-IDF Development Framework Style Guide: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/contribute/style-guide.html#espressif-iot-development-framework-style-guide

### Unit Tests
The `tests/` directory contains all unit tests for the project, using the Unity test framework provided by ESP-IDF. For examples on how to run the tests see the ESP-IDF unit test sample code: https://github.com/espressif/esp-idf/tree/master/examples/system/unit_test

### References
* ESP-IDF example code: https://github.com/espressif/esp-idf/tree/master/examples
* R502-interface: https://github.com/JasonFevang/R502-interface
