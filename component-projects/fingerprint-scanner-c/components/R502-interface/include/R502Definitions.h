#ifndef R502DEFINITIONS_H_
#define R502DEFINITIONS_H_

/**
 * \file R502Definitions.h
 * \brief Defines data types and enums to faciliate communication with the R502 
 * module
 */

#pragma once
#include <stdint.h>

/// Constants ///

// DO NOT USE ANYWHERE ELSE! //
#define R502_TEMPLATE_SIZE 384 * 4 // Why isn't this 2 according to docs?
#define R502_IMAGE_SIZE 36 * 1024 // Why isn't this 72 according to docs?
#define R502_CS_LEN 2
#define R502_MAX_DATA_LEN 256

// Use these instead //
static const int R502_template_size = R502_TEMPLATE_SIZE;
static const int R502_image_size = R502_IMAGE_SIZE;
static const int R502_cs_len = R502_CS_LEN;
static const int R502_max_data_len = R502_MAX_DATA_LEN;

/**
 * \brief Package identifiers
 */
typedef enum {
    R502_pid_command = 0x01,
    R502_pid_data = 0x02,
    R502_pid_ack = 0x07,
    R502_pid_end_of_data = 0x08
} R502_pid_t;

/**
 * \brief Instruction codes sent with each command
 */
typedef enum {
    R502_ic_gen_img = 0x1,
    R502_ic_img_2_tz = 0x2,
    R502_ic_match = 0x3,
    R502_ic_search = 0x4,
    R502_ic_reg_model = 0x5,
    R502_ic_store = 0x6,
    R502_ic_load_char = 0x7,
    R502_ic_up_char = 0x8,
    R502_ic_down_char = 0x9,
    R502_ic_up_image = 0xA,
    R502_ic_down_image = 0xB,
    R502_ic_delet_char = 0xC,
    R502_ic_empty = 0xD,
    R502_ic_set_sys_para = 0xE,
    R502_ic_read_sys_para = 0xF,
    R502_ic_set_pwd = 0x12,
    R502_ic_vfy_pwd = 0x13,
    R502_ic_get_random_code = 0x14,
    R502_ic_set_adder = 0x15,
    R502_ic_control = 0x17,
    R502_ic_write_notepad = 0x18,
    R502_ic_read_notepad = 0x19,
    R502_ic_template_num = 0x1D,
    R502_ic_led_config = 0x35
} R502_instr_code_t;

/**
 * \brief Confirmation codes returned by the R502 module in acknowledge packages
 */
typedef enum {
    R502_fail = -1,
    R502_ok = 0x0,
    R502_err_receive = 0x1,
    R502_err_no_finger = 0x2,
    R502_err_fail_to_enroll = 0x3,
    R502_err_disorderly_image = 0x6,
    R502_err_no_character_pointer = 0x7,
    R502_err_no_match = 0x8,
    R502_err_not_found = 0x9,
    R502_err_combine = 0xA,
    R502_err_page_id_out_of_range = 0xB,
    R502_err_reading_template = 0xC,
    R502_err_uploading_template = 0xD,
    R502_err_receiving_data = 0xE,
    R502_err_uploading_image = 0xF,
    R502_err_deleting_template = 0x10,
    R502_err_clear_library = 0x11,
    R502_err_wrong_pass = 0x13,
    R502_err_no_valid_primary_image = 0x15,
    R502_err_writing_flash = 0x18,
    R502_err_no_definition = 0x19,
    R502_err_invalid_reg_num = 0x1A,
    R502_err_wrong_reg_config = 0x1B,
    R502_err_wrong_page_num = 0x1C,
    R502_err_comm_port = 0x1D,
} R502_conf_code_t;

/**
 * \brief parameter numbers to identity which system parameter is being modified
 */
typedef enum {
    R502_para_num_baud_control = 4,
    R502_para_num_security_level = 5,
    R502_para_num_data_pkg_len = 6,
} R502_para_num;

/**
 * \brief Enum defining which parameter value maps to which baud rate
 */
typedef enum {
    R502_baud_9600 = 1,
    R502_baud_19200 = 2,
    R502_baud_38400 = 4,
    R502_baud_57600 = 6,
    R502_baud_115200 = 12,
} R502_baud_t;

/**
 * \brief Enum defining which parameter value maps to which data package length
 */
typedef enum {
    R502_data_len_32 = 0,
    R502_data_len_64 = 1,
    R502_data_len_128 = 2,
    R502_data_len_256 = 3,
} R502_data_len_t;

/**
 * \brief Enum defining led configuration control codes 
 */
typedef enum {
    R502_led_ctrl_breathing = 1,
    R502_led_ctrl_flashing = 2,
    R502_led_ctrl_always_on = 3,
    R502_led_ctrl_always_off = 4,
    R502_led_ctrl_grad_on = 5,
    R502_led_ctrl_grad_off = 6,
} R502_led_ctrl_t;

/**
 * \brief Enum defining led configuration color index 
 */
typedef enum {
    R502_led_color_red = 1,
    R502_led_color_blue = 2,
    R502_led_color_purple = 3,
} R502_led_color_t;

///// Return Data Structures /////

/**
 * \brief Data structure to return system parameters to caller
 */
typedef struct R502_sys_para_t {
    uint16_t status_register;
    uint16_t system_identifier_code;
    uint16_t finger_library_size;
    uint16_t security_level;
    uint8_t device_address[4];
    R502_data_len_t data_package_length;
    R502_baud_t baud_setting;
} R502_sys_para_t;

///// Command Packages /////

/// System Commands ///

/**
 * \brief Data section of a general command package
 * 
 * Many commands have this same format, reuse this
 */
typedef struct R502_GeneralCommand_t {
    uint8_t instr_code; //!< instruction code
    uint8_t checksum[R502_CS_LEN]; //!< checksum
} R502_GeneralCommand_t;

/**
 * \brief Data section of the VfyPwd command
 */
typedef struct R502_VfyPwd_t {
    uint8_t instr_code; //!< instruction code
    uint8_t password[4]; //!< User-provided password
    uint8_t checksum[R502_CS_LEN]; //!< checksum
} R502_VfyPwd_t;

/**
 * \brief Data section of the SetPwd command
 */
typedef struct R502_SetPwd_t {
    uint8_t instr_code; //!< instruction code
    uint8_t password[4]; //!< User-provided password
    uint8_t checksum[R502_CS_LEN]; //!< checksum
} R502_SetPwd_t;

/**
 * \brief Data section of the SetSysPara command
 */
typedef struct R502_SetSysPara_t {
    uint8_t instr_code; //!< instruction code
    uint8_t parameter_number; //!< Number to indicate which parameter to set
    uint8_t contents; //!< value to set the parameter to
    uint8_t checksum[R502_CS_LEN]; //!< checksum
} R502_SetSysPara_t;

/**
 * \brief Data section of the Img2Tz command
 */
typedef struct R502_Img2Tz_t {
    uint8_t instr_code; //!< instruction code
    uint8_t buffer_id; //!< character file buffer number
    uint8_t checksum[R502_CS_LEN]; //!< checksum
} R502_Img2Tz_t;

/**
 * \brief Data section of the UpChar command
 */
typedef struct R502_UpChar_t {
    uint8_t instr_code; //!< instruction code
    uint8_t buffer_id; //!< character file buffer number
    uint8_t checksum[R502_CS_LEN]; //!< checksum
} R502_UpChar_t;

/**
 * \brief Data section of the DownChar command
 */
typedef struct R502_DownChar_t {
    uint8_t instr_code; //!< instruction code
    uint8_t buffer_id; //!< character file buffer number
    uint8_t checksum[R502_CS_LEN]; //!< checksum
} R502_DownChar_t;

/**
 * \brief Data section of the Store command
 */
typedef struct R502_Store_t {
    uint8_t instr_code; //!< instruction code
    uint8_t buffer_id; //!< character file buffer number
    uint8_t page_id[2]; //!< flash location of the template
    uint8_t checksum[R502_CS_LEN]; //!< checksum
} R502_Store_t;

/**
 * \brief Data section of the DeletChar command
 */
typedef struct R502_DeletChar_t {
    uint8_t instr_code; //!< instruction code
    uint8_t page_id[2]; //!< flash location of the template
    uint8_t num_of_templates[2]; //!< number of templates to be deleted
    uint8_t checksum[R502_CS_LEN]; //!< checksum
} R502_DeletChar_t;

/**
 * \brief Data section of the Search command
 */
typedef struct R502_Search_t {
    uint8_t instr_code; //!< instruction code
    uint8_t buffer_id; //!< character file buffer number
    uint8_t start_page[2]; //!< searching start address
    uint8_t page_num[2]; //!< searching numbers
    uint8_t checksum[R502_CS_LEN]; //!< checksum
} R502_Search_t;

/**
 * \brief Data section of the (Aura)LedConfig command
 */
typedef struct R502_LedConfig_t {
    uint8_t instr_code; //!< instruction code
    uint8_t ctrl; //!< control code
    uint8_t speed; //!< speed: breathing, flashing, gradual light
    uint8_t color_index; //!< red, blue, purple
    uint8_t count; //!< number of cycles: breathing, flashing light
    uint8_t checksum[R502_CS_LEN]; //!< checksum
} R502_LedConfig_t;

///// Acknowledgement Packages /////

/**
 * \brief Data section of a general acknowledge package from R502
 * 
 * Many commands have this same acknowledge format, reuse this
 */
typedef struct R502_GeneralAck_t {
    uint8_t conf_code; //!< confirmation code
    uint8_t checksum[R502_CS_LEN]; //!< checksum
} R502_GeneralAck_t;

/**
 * \brief Data section of a ReadSysPara acknowledge package from R502
 */
typedef struct R502_ReadSysParaAck_t {
    uint8_t conf_code; //!< confirmation code
    uint8_t data[16]; //!< Data section containing all system parameters
    uint8_t checksum[R502_CS_LEN]; //!< checksum
} R502_ReadSysParaAck_t;


/**
 * \brief Data section of a template_num acknowledge package from R502
 */
typedef struct R502_TemplateNumAck_t {
    uint8_t conf_code; //!< confirmation code
    uint8_t template_num[2]; //!< Data section containing all system parameters
    uint8_t checksum[R502_CS_LEN]; //!< checksum
} R502_TemplateNumAck_t;

/**
 * \brief Data section of a search acknowledge package from R502
 */
typedef struct R502_SearchAck_t {
    uint8_t conf_code; //!< confirmation code
    uint8_t page_id[2]; //!< matching templates location
    uint8_t match_score[2]; //!< matching score
    uint8_t checksum[R502_CS_LEN]; //!< checksum
} R502_SearchAck_t;

///// Data Packages /////

/**
 * \brief Data section of a data package with 256 bytes of data
 */
typedef struct R502_Data_t {
    uint8_t content[R502_MAX_DATA_LEN];
    uint8_t checksum[R502_CS_LEN]; //!< checksum
} R502_Data_t;

/**
 * \brief Data section of a data package with 128 bytes of data
 */
typedef struct R502_Data128_t {
    uint8_t content[128];
    uint8_t checksum[R502_CS_LEN]; //!< checksum
} R502_Data128_t;

/**
 * \brief Data section of a data package with 64 bytes of data
 */
typedef struct R502_Data64_t {
    uint8_t content[64];
    uint8_t checksum[R502_CS_LEN]; //!< checksum
} R502_Data64_t;

/**
 * \brief Data section of a data package with 32 bytes of data
 */
typedef struct R502_Data32_t {
    uint8_t content[32];
    uint8_t checksum[R502_CS_LEN]; //!< checksum
} R502_Data32_t;



/**
 * \brief General data structure storing any type of package
 */
typedef struct R502_DataPkg_t {
    uint8_t start[2]; //!< Two byte header, all packages are the same
    uint8_t adder[4]; //!< Module address, default 0xff, 0xff, 0xff, 0xff
    uint8_t pid; //!< Package id
    uint8_t length[2]; //!< length in bytes of data section of this package
    union {
        R502_GeneralCommand_t general;
        R502_VfyPwd_t vfy_pwd;
        R502_SetPwd_t set_pwd;
        R502_SetSysPara_t set_sys_para;
        R502_Img2Tz_t img_2_tz;
        R502_UpChar_t up_char;
        R502_DownChar_t down_char;
        R502_Store_t store;
        R502_DeletChar_t delet_char;
        R502_Search_t search;
        R502_LedConfig_t led_config;
        R502_GeneralAck_t general_ack;
        R502_ReadSysParaAck_t read_sys_para_ack;
        R502_TemplateNumAck_t template_num_ack;
        R502_SearchAck_t search_ack;

        R502_Data_t data;
        R502_Data128_t data128;
        R502_Data64_t data64;
        R502_Data32_t data32;
    } data; //!< Data and checksum of the package
} R502_DataPkg_t;

#endif /* R502DEFINITIONS_H_ */