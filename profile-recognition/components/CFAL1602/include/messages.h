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

DRAM_ATTR static const char* init_error =
    "ERROR: Init Fail";

// Verify User messages
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
DRAM_ATTR static const char* help_0_idlestate =
    "Admin menu HELP";
DRAM_ATTR static const char* help_1_idlestate =
    "                There are 3 options: Add Profile, Delete Profile, Exit Admin. "
    "To toggle menu options, press A or B. "
    "To select option, press #.                ";

DRAM_ATTR static const char* help_0_verifyuser =
    "Verify User HELP";
DRAM_ATTR static const char* help_1_verifyuser =
    "                Open the door, or access admin menu (admin privilege only). "
    "Press finger on scanner, or enter 4-digit PIN with #. "
    "Press * to backspace. "
    "Press A to toggle admin verify. "
    "Press C to exit help mode. "
    "Press D to return to initial state.                ";

DRAM_ATTR static const char* help_0_addprofile =
    "Add Profile HELP";
DRAM_ATTR static const char* help_1_addprofile_pin =
    "                Enter 4-digit PIN, then press # to confirm. "
    "If PIN is already used by another profile, enter different PIN. "
    "Press * to backspace. "
    "Press C to exit help mode. "
    "Press D to abort Add Profile.                ";
DRAM_ATTR static const char* help_1_addprofile_priv =
    "                Enter 1 for user, or 2 for admin, then press # to confirm. "
    "Press * to backspace. "
    "Press C to exit help mode. "
    "Press D to abort Add Profile.                ";
DRAM_ATTR static const char* help_1_addprofile_fp =
    "                Enter finger two times to register prints. "
    "If fingers do not match, repeat the procedure. "
    "Press C to exit help mode. "
    "Press D to abort Add Profile.                ";
DRAM_ATTR static const char* help_1_addprofile_compile =
    "                Confirmation step. Press # to complete Add Profile. "
    "Press C to exit help mode. "
    "Press D to abort Add Profile.                ";

DRAM_ATTR static const char* help_0_delprofile =
    "Del Profile HELP";
DRAM_ATTR static const char* help_1_delprofile_menu =
    "                Select an existing profile to delete. "
    "To toggle menu options, press A or B. "
    "To select option, press #. "
    "Press C to exit help mode. "
    "Press D to abort Delete Profile.                ";
DRAM_ATTR static const char* help_1_delprofile_confirm =
    "                Confirmation step. Press # to complete Delete Profile. "
    "Press * to return to profile menu. "
    "Press C to exit help mode. "
    "Press D to abort Delete Profile.                ";

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

DRAM_ATTR static const char* return_to_menu_0 =
    "Returning to";
DRAM_ATTR static const char* return_to_menu_1 =
    "menu...";

// Add Profile messages
DRAM_ATTR static const char* admin_add =
    "Admin: Add";

DRAM_ATTR static const char* pin_already_used =
    "PIN already used";

DRAM_ATTR static const char* pin_accepted =
    "PIN accepted";

DRAM_ATTR static const char* invalid_priv =
    "Invalid priv";
DRAM_ATTR static const char* must_be_1_or_2 =
    "Must be 1 or 2";

DRAM_ATTR static const char* priv_accepted =
    "Priv accepted";

DRAM_ATTR static const char* awaiting_2_fp =
    "Awaiting 2 FP";

DRAM_ATTR static const char* awaiting_1_fp =
    "Awaiting 1 FP";

DRAM_ATTR static const char* reenter_fps =
    "Reenter FPs";

DRAM_ATTR static const char* fps_dont_match_0 =
    "Fingerprints";
DRAM_ATTR static const char* fps_dont_match_1 =
    "don't match";

DRAM_ATTR static const char* fp_accepted =
    "FP accepted";

DRAM_ATTR static const char* create_profile =
    "Create profile?";

DRAM_ATTR static const char* creating_profile =
    "Creating profile";

DRAM_ATTR static const char* slots_full_0 =
    "Error:          "
    "Delete a profile";
DRAM_ATTR static const char* slots_full_1 =
    "Slots full      "
    "to free slot    ";

DRAM_ATTR static const char* profile_created =
    "Profile created:";

// Delete Profile messages
DRAM_ATTR static const char* admin_delete =
    "Admin: Delete";

// Delete Profile messages
DRAM_ATTR static const char* leaving_admin =
    "Leaving Admin...";

DRAM_ATTR static const char* are_you_sure =
    "Are you sure?";

DRAM_ATTR static const char* return_to_menu_2 =
    "profile menu...";

DRAM_ATTR static const char* deleting_profile =
    "Deleting profile";

DRAM_ATTR static const char* profile_deleted =
    "Profile deleted";

DRAM_ATTR static const char* p0_cannot_be_deleted_0 =
    "Profile 0 cannot";

DRAM_ATTR static const char* p0_cannot_be_deleted_1 =
    "be deleted";

#endif /* MESSAGES_H */