# Guide to ESP-IDF for VS Code
**Based on Espressif's GitHub guide**

## Link to Espressif's GitHub setup guide (contains video)
https://github.com/espressif/vscode-esp-idf-extension

## Link to Blinking an LED
https://www.instructables.com/Blinking-an-LED-With-ESP32/

## Tips
- 1st link: Read the guide on GitHub, but skip the "Configure json" step
  - This guide is outlined by the video.
  - Follow this guide as you watch the video.
- 2nd link: Implement on the breadboard

## Outline
1. Intro
   - Basic intro
2. Installation
   - Install VS Code, Git, Python 3.5+
   - Install VS Code Extension: Espressif IDF
3. Onboarding
   - Create workspace folder. Just do File->Open Folder. Choose a folder to work in.
   - Configure ESP-IDF extension
     - *NOTE: Setup may look different on your screen than on the video. Click on the SETUP hyperlink in Espressif's setup guide under "How to use" for details.*
4. Open Example Projects
   - View ESP-IDF project examples
     - *NOTE: The "show esp-idf examples" command is this: "ESP-IDF: Show Examples Projects"*
   - Select the blink example : get_started->blink
5. Build, Flash, Monitor
   - When doing Flash, you may be asked to select JTAG or UART. Choose UART, it worked for me.
6. Configuration menu
   - Not required to get the "blink" example running!
   - But useful as a starting place to understand Bluetooth and other functionalities!

## Note
- The blink example uses GPIO5, which corresponds to pin D5 on the development board.
- The GPIO number is shown by hovering the cursor over *CONFIG_BLINK_GPIO* in blink->main->blink.c
