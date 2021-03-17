#include <limits.h>
#include "unity.h"
#include "R502Interface.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp32/rom/uart.h"

#define PIN_TXD  (GPIO_NUM_17)
#define PIN_NC_1 (GPIO_NUM_25)
#define PIN_NC_2 (GPIO_NUM_26)
#define PIN_NC_3 (GPIO_NUM_27)
#define PIN_RXD  (GPIO_NUM_16)
#define PIN_IRQ  (GPIO_NUM_4)
#define PIN_RTS  (UART_PIN_NO_CHANGE)
#define PIN_CTS  (UART_PIN_NO_CHANGE)

// The object under test
R502Interface R502 = {
    .up_image_cb = NULL,
    .up_char_cb = NULL,
    .TAG = "R502",
    .adder = {0xFF, 0xFF, 0xFF, 0xFF},
    .cur_data_pkg_len = 128,
    .initialized = false,
    .interrupt = 0,
    .start = {0xEF, 0x01},
    .system_identifier_code = 0,
    .default_read_delay = 200,
    .read_delay_gen_image = 2000,
    .min_uart_buffer_size = 256,
    .header_size = offsetof(R502_DataPkg_t, data)
};

// Template buffer
uint8_t character_buffer[R502_TEMPLATE_SIZE];

void tearDown(){
    R502_deinit(&R502);
}

void wait_with_message(char *message){
    printf(message);
    uart_rx_one_char_block();
}

void populate_buffer() {
    for (int i = 0; i < R502_template_size; i++) {
        character_buffer[i] = (uint8_t)(i % 256);
    }
}

static int up_image_size = 0;
void up_image_callback(uint8_t data[R502_max_data_len * 2], 
    int data_len)
{
    // this is where you would store or otherwise do something with the image
    up_image_size += data_len;

    //int box_width = 16;
    //int total = 0;
    //while(total < data_len){
        //for(int i = 0; i < box_width; i++){
            //printf("0x%02X ", data[total]);
            //total++;
        //}
        //printf("\n");
    //}
    // Check that all bytes have the lower four bytes 0
    for(int i = 0; i < data_len; i++){
        TEST_ASSERT_EQUAL(0, data[i] & 0x0f);
    }
}

static int up_char_size = 0;
void up_char_callback(uint8_t data[R502_max_data_len], 
    int data_len)
{
    // this is where you would store or otherwise do something with the image
    up_char_size += data_len;

    //int box_width = 16;
    //int total = 0;
    //while(total < data_len){
        //for(int i = 0; i < box_width; i++){
            //printf("0x%02X ", data[total]);
            //total++;
        //}
        //printf("\n");
    //}
}

TEST_CASE("Not connected", "[initialization]")
{
    esp_err_t err = R502_init(&R502, UART_NUM_1, PIN_NC_1, PIN_NC_2, PIN_NC_3, R502_baud_115200);
    TEST_ESP_OK(err);
    uint8_t pass[4] = {0x00, 0x00, 0x00, 0x00};
    R502_conf_code_t conf_code = R502_fail;
    err = R502_vfy_pass(&R502, pass, &conf_code);
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, err);
}

TEST_CASE("Reinitialize", "[initialization]")
{
    esp_err_t err = R502_init(&R502, UART_NUM_1, PIN_NC_1, PIN_NC_2, PIN_NC_3, R502_baud_115200);
    TEST_ESP_OK(err);
    printf("safe!\n");
    err = R502_deinit(&R502);
    TEST_ESP_OK(err);
    printf("safe2!\n");
    err = R502_init(&R502, UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ, R502_baud_115200);
    TEST_ESP_OK(err);
    printf("safe3!\n");
    uint8_t pass[4] = {0x00, 0x00, 0x00, 0x00};
    R502_conf_code_t conf_code = R502_fail;
    printf("safe3.5\n");
    err = R502_vfy_pass(&R502, pass, &conf_code);
    TEST_ESP_OK(err);
    printf("safe4!\n");
    TEST_ASSERT_EQUAL(conf_code, R502_ok);
}

TEST_CASE("VfyPwd", "[system]")
{
    esp_err_t err = R502_init(&R502, UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ, R502_baud_115200);
    TEST_ESP_OK(err);
    uint8_t pass[4] = {0x00, 0x00, 0x00, 0x00};
    R502_conf_code_t conf_code = R502_fail;
    err = R502_vfy_pass(&R502, pass, &conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    pass[3] = 0x01;
    err = R502_vfy_pass(&R502, pass, &conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_err_wrong_pass, conf_code);
    err = R502_deinit(&R502);
    TEST_ESP_OK(err);
}

TEST_CASE("ReadSysPara", "[system]")
{
    esp_err_t err = R502_init(&R502, UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ, R502_baud_115200);
    TEST_ESP_OK(err);
    R502_sys_para_t sys_para;
    R502_conf_code_t conf_code;
    err = R502_read_sys_para(&R502, &conf_code, &sys_para);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(R502_get_module_address(&R502), 
        sys_para.device_address, 4);
    printf("status_register %d\n", sys_para.status_register);
    printf("system_identifier_code %d\n", sys_para.system_identifier_code);
    printf("finger_library_size %d\n", sys_para.finger_library_size);
    printf("security_level %d\n", sys_para.security_level);
    printf("device_address %x%x%x%x\n", sys_para.device_address[0], 
        sys_para.device_address[1], sys_para.device_address[2], 
        sys_para.device_address[3]);
    printf("data_package_length %d\n", sys_para.data_package_length);
    printf("baud_setting %d\n", sys_para.baud_setting);
}

TEST_CASE("SetBaudRate", "[system]")
{
    esp_err_t err = R502_init(&R502, UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ, R502_baud_115200);
    TEST_ESP_OK(err);

    R502_conf_code_t conf_code = R502_fail;
    R502_baud_t current_baud = R502_baud_115200;
    R502_sys_para_t sys_para;
    err = R502_read_sys_para(&R502, &conf_code, &sys_para);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    current_baud = sys_para.baud_setting;

    // 9600
    err = R502_set_baud_rate(&R502, R502_baud_9600, &conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);

    // 19200
    err = R502_set_baud_rate(&R502, R502_baud_19200, &conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);

    // 38400
    err = R502_set_baud_rate(&R502, R502_baud_38400, &conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);

    // 57600
    err = R502_set_baud_rate(&R502, R502_baud_57600, &conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);

    // 115200
    err = R502_set_baud_rate(&R502, R502_baud_115200, &conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);

    // error baud rate
    conf_code = R502_fail;
    err = R502_set_baud_rate(&R502, (R502_baud_t)9600, &conf_code);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
    TEST_ASSERT_EQUAL(R502_fail, conf_code);

    // rest baud rate to what it was before testing
    err = R502_set_baud_rate(&R502, current_baud, &conf_code); // normal
    //err = R502.set_baud_rate(R502_baud_115200, conf_code); // change baud rate to this
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
}

TEST_CASE("SetSecurityLevel", "[system]")
{
    esp_err_t err = R502_init(&R502, UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ, R502_baud_115200);
    TEST_ESP_OK(err);

    R502_conf_code_t conf_code = R502_fail;
    int current_security_level = 0;
    R502_sys_para_t sys_para;
    err = R502_read_sys_para(&R502, &conf_code, &sys_para);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    current_security_level = sys_para.security_level;

    for(int sec_lev = 1; sec_lev <= 5; sec_lev++){
        err = R502_set_security_level(&R502, sec_lev, &conf_code);
        TEST_ESP_OK(err);
        TEST_ASSERT_EQUAL(R502_ok, conf_code);
        err = R502_read_sys_para(&R502, &conf_code, &sys_para);
        TEST_ESP_OK(err);
        TEST_ASSERT_EQUAL(R502_ok, conf_code);
        TEST_ASSERT_EQUAL(sys_para.security_level, sec_lev);
    }

    conf_code = R502_fail;
    err = R502_set_security_level(&R502, 6, &conf_code);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
    TEST_ASSERT_EQUAL(R502_fail, conf_code);

    err = R502_set_security_level(&R502, current_security_level, &conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
}

TEST_CASE("SetDataPackageLength", "[system]")
{
    // Can only set to 128 and 256 bytes, not 32 or 64
    esp_err_t err = R502_init(&R502, UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ, R502_baud_115200);
    TEST_ESP_OK(err);

    R502_conf_code_t conf_code = R502_fail;
    R502_data_len_t current_data_length = R502_data_len_32;
    R502_sys_para_t sys_para;
    err = R502_read_sys_para(&R502, &conf_code, &sys_para);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    current_data_length = sys_para.data_package_length;

    err = R502_set_data_package_length(&R502, R502_data_len_64, &conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    err = R502_read_sys_para(&R502, &conf_code, &sys_para);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    // this should be 64bytes, but my module only goes down to 128
    // Leaving assert in so if it changes with another module I will know
    TEST_ASSERT_EQUAL(R502_data_len_128, sys_para.data_package_length);

    err = R502_set_data_package_length(&R502, R502_data_len_256, &conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    err = R502_read_sys_para(&R502, &conf_code, &sys_para);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    TEST_ASSERT_EQUAL(R502_data_len_256, sys_para.data_package_length);

    err = R502_set_data_package_length(&R502, R502_data_len_128, &conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    err = R502_read_sys_para(&R502, &conf_code, &sys_para);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    TEST_ASSERT_EQUAL(R502_data_len_128, sys_para.data_package_length);

    // reset to starting data length
    err = R502_set_data_package_length(&R502, current_data_length, &conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
}

TEST_CASE("TemplateNum", "[system]")
{
    esp_err_t err = R502_init(&R502, UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ, R502_baud_115200);
    TEST_ESP_OK(err);
    R502_conf_code_t conf_code;
    uint16_t template_num = 65535;
    err = R502_template_num(&R502, &conf_code, &template_num);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
}

TEST_CASE("GenImage", "[fingerprint processing]")
{
    esp_err_t err = R502_init(&R502, UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ, R502_baud_115200);
    TEST_ESP_OK(err);
    R502_conf_code_t conf_code;
    err = R502_gen_image(&R502, &conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_err_no_finger, conf_code);
}

TEST_CASE("GenImage-Success", "[fingerprint processing][userInput]")
{
    esp_err_t err = R502_init(&R502, UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ, R502_baud_115200);
    TEST_ESP_OK(err);
    R502_conf_code_t conf_code;

    wait_with_message("Place finger on sensor, then press enter\n");

    err = R502_gen_image(&R502, &conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
}

/*TEST_CASE("UpImage", "[fingerprint processing][dataExchange]")
{
    esp_err_t err = R502_init(&R502, UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ, R502_baud_115200);
    TEST_ESP_OK(err);

    // read and save current data_package_length and baud rate
    R502_sys_para_t sys_para;
    R502_conf_code_t conf_code;
    err = R502_read_sys_para(&R502, &conf_code, &sys_para);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    R502_data_len_t starting_data_len = sys_para.data_package_length;

    // Attempt up_image without setting the callback
    err = R502_up_image(&R502, starting_data_len, &conf_code);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, err);

    // reset counter to measure size of the uploaded image
    R502_set_up_image_cb(&R502, up_image_callback);

    up_image_size = 0;

    err = R502_up_image(&R502, starting_data_len, &conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    TEST_ASSERT_EQUAL(R502_image_size, up_image_size);
}

TEST_CASE("UpImage-Advanced", "[fingerprint processing][dataExchange]")
{
    esp_err_t err = R502_init(&R502, UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ, R502_baud_115200);
    TEST_ESP_OK(err);
    R502_set_up_image_cb(&R502, up_image_callback);

    // read and save current data_package_length and baud rate
    R502_sys_para_t sys_para;
    R502_conf_code_t conf_code;
    err = R502_read_sys_para(&R502, &conf_code, &sys_para);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    R502_data_len_t starting_data_len = sys_para.data_package_length;
    R502_baud_t starting_baud = sys_para.baud_setting;

    
    R502_baud_t test_bauds[5] = { R502_baud_9600, R502_baud_19200, 
        R502_baud_38400, R502_baud_57600, R502_baud_115200 };
    R502_data_len_t test_data_lens[2] = { R502_data_len_128, 
        R502_data_len_256 };

    // Loop through all bauds and data lengths to test all combinations
    for(int baud_i = 0; baud_i < 5; baud_i++){
        ESP_LOGI(R502.TAG, "Test baud %d", test_bauds[baud_i]);
        err = R502_set_baud_rate(&R502, test_bauds[baud_i], &conf_code);
        TEST_ESP_OK(err);
        TEST_ASSERT_EQUAL(R502_ok, conf_code);

        for(int data_i = 0; data_i < 2; data_i++){
            ESP_LOGI(R502.TAG, "Test data len %d", test_data_lens[data_i]);
            // reset counter to measure size of the uploaded image
            up_image_size = 0;

            err = R502_set_data_package_length(&R502, test_data_lens[data_i], 
                &conf_code);
            TEST_ESP_OK(err);
            TEST_ASSERT_EQUAL(R502_ok, conf_code);

            err = R502_up_image(&R502, test_data_lens[data_i], &conf_code);
            TEST_ESP_OK(err);
            TEST_ASSERT_EQUAL(R502_ok, conf_code);
            TEST_ASSERT_EQUAL(R502_image_size, up_image_size);
        }
    }

    // reset the data package length and baud settings to default
    err = R502_set_data_package_length(&R502, starting_data_len, &conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    err = R502_set_baud_rate(&R502, starting_baud, &conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
}*/

TEST_CASE("Img2Tz-Input", "[fingerprint processing][userInput]")
{
    esp_err_t err = R502_init(&R502, UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ, R502_baud_115200);
    TEST_ESP_OK(err);
    R502_conf_code_t conf_code;

    // generate image from fingerprint
    wait_with_message("Place finger on sensor, then press enter\n");

    err = R502_gen_image(&R502, &conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);

    // main test
    uint8_t buffer_id = 1;
    err = R502_img_2_tz(&R502, buffer_id, &conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
}

TEST_CASE("RegModel-Input", "[fingerprint processing command][userInput]")
{
    esp_err_t err = R502_init(&R502, UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ, R502_baud_115200);
    TEST_ESP_OK(err);
    R502_conf_code_t conf_code;

    // 1st fingerprint: generate image from fingerprint
    wait_with_message("Place finger on sensor, then press enter\n");

    err = R502_gen_image(&R502, &conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);

    // 1st fingerprint: generate character file
    uint8_t buffer_id = 1;
    err = R502_img_2_tz(&R502, buffer_id, &conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);

    // Repeat for 2nd fingerprint
    wait_with_message("Place finger on sensor again, then press enter\n");

    err = R502_gen_image(&R502, &conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);

    buffer_id = 2;
    err = R502_img_2_tz(&R502, buffer_id, &conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);

    // main test
    err = R502_reg_model(&R502, &conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
}

TEST_CASE("UpChar", "[fingerprint processing command][dataExchange]")
{
    esp_err_t err = R502_init(&R502, UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ, R502_baud_115200);
    TEST_ESP_OK(err);
    R502_conf_code_t conf_code;

    // main test
    // read and save current data_package_length and baud rate
    R502_sys_para_t sys_para;
    //R502_conf_code_t conf_code;
    err = R502_read_sys_para(&R502, &conf_code, &sys_para);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    R502_data_len_t starting_data_len = sys_para.data_package_length;

    // Attempt up_char without setting the callback
    uint8_t buffer_id = 1;
    err = R502_up_char(&R502, starting_data_len, buffer_id, &conf_code);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, err);

    // reset counter to measure size of the uploaded image
    R502_set_up_char_cb(&R502, up_char_callback);

    up_char_size = 0;

    err = R502_up_char(&R502, starting_data_len, buffer_id, &conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    TEST_ASSERT_EQUAL(R502_template_size, up_char_size);
}

TEST_CASE("UpChar-Advanced", "[fingerprint processing][dataExchange]")
{
    esp_err_t err = R502_init(&R502, UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ, R502_baud_115200);
    TEST_ESP_OK(err);
    R502_set_up_char_cb(&R502, up_char_callback);

    // read and save current data_package_length and baud rate
    R502_sys_para_t sys_para;
    R502_conf_code_t conf_code;
    err = R502_read_sys_para(&R502, &conf_code, &sys_para);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    R502_data_len_t starting_data_len = sys_para.data_package_length;
    R502_baud_t starting_baud = sys_para.baud_setting;

    
    R502_baud_t test_bauds[5] = { R502_baud_9600, R502_baud_19200, 
        R502_baud_38400, R502_baud_57600, R502_baud_115200 };
    R502_data_len_t test_data_lens[2] = { R502_data_len_128, 
        R502_data_len_256 };
    int buffer_id = 1;

    // Loop through all bauds and data lengths to test all combinations
    for(int baud_i = 0; baud_i < 5; baud_i++){
        ESP_LOGI(R502.TAG, "Test baud %d", test_bauds[baud_i]);
        err = R502_set_baud_rate(&R502, test_bauds[baud_i], &conf_code);
        TEST_ESP_OK(err);
        TEST_ASSERT_EQUAL(R502_ok, conf_code);

        for(int data_i = 0; data_i < 2; data_i++){
            ESP_LOGI(R502.TAG, "Test data len %d", test_data_lens[data_i]);
            // reset counter to measure size of the uploaded character file
            up_char_size = 0;

            err = R502_set_data_package_length(&R502, test_data_lens[data_i], 
                &conf_code);
            TEST_ESP_OK(err);
            TEST_ASSERT_EQUAL(R502_ok, conf_code);

            err = R502_up_char(&R502, test_data_lens[data_i], buffer_id, &conf_code);
            TEST_ESP_OK(err);
            TEST_ASSERT_EQUAL(R502_ok, conf_code);
            TEST_ASSERT_EQUAL(R502_template_size, up_char_size);
        }
    }

    // reset the data package length and baud settings to default
    err = R502_set_data_package_length(&R502, starting_data_len, &conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    err = R502_set_baud_rate(&R502, starting_baud, &conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
}

TEST_CASE("DownChar", "[fingerprint processing][dataExchange]")
{
    populate_buffer();

    esp_err_t err = R502_init(&R502, UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ, R502_baud_115200);
    TEST_ESP_OK(err);
    R502_conf_code_t conf_code;

    // main test
    // read and save current data_package_length and baud rate
    R502_sys_para_t sys_para;
    //R502_conf_code_t conf_code;
    err = R502_read_sys_para(&R502, &conf_code, &sys_para);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    R502_data_len_t starting_data_len = sys_para.data_package_length;

    // Attempt up_char without setting the callback
    uint8_t buffer_id = 1;
    err = R502_down_char(&R502, starting_data_len, buffer_id, character_buffer, &conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
}

TEST_CASE("Store", "[fingerprint processing]")
{
    esp_err_t err = R502_init(&R502, UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ, R502_baud_115200);
    TEST_ESP_OK(err);
    R502_conf_code_t conf_code;

    // main test
    uint8_t buffer_id = 1;
    uint16_t page_id = 36;
    err = R502_store(&R502, buffer_id, page_id, &conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
}

TEST_CASE("DeletChar", "[fingerprint processing]")
{
    esp_err_t err = R502_init(&R502, UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ, R502_baud_115200);
    TEST_ESP_OK(err);
    R502_conf_code_t conf_code;

    // main test
    uint16_t page_id = 36;
    uint16_t n = 1;
    err = R502_delet_char(&R502, page_id, n, &conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
}

TEST_CASE("Empty", "[fingerprint processing]")
{
    esp_err_t err = R502_init(&R502, UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ, R502_baud_115200);
    TEST_ESP_OK(err);
    R502_conf_code_t conf_code;

    // main test
    err = R502_empty(&R502, &conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
}

TEST_CASE("Search", "[fingerprint processing]")
{
    esp_err_t err = R502_init(&R502, UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ, R502_baud_115200);
    TEST_ESP_OK(err);
    R502_conf_code_t conf_code;

    // main test
    uint8_t buffer_id = 1;
    uint16_t start_page = 0;
    uint16_t page_num = 0xffff;
    uint16_t page_id = 0xeeee;
    uint16_t match_score = 0xdddd;
    err = R502_search(&R502, buffer_id, start_page, page_num, &conf_code, &page_id, &match_score);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    TEST_ASSERT_EQUAL(page_id, 36);
}

TEST_CASE("LedConfig-control", "[other]")
{
    esp_err_t err = R502_init(&R502, UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ, R502_baud_115200);
    TEST_ESP_OK(err);
    R502_conf_code_t conf_code;

    R502_led_ctrl_t control_codes[6] = { R502_led_ctrl_breathing,
        R502_led_ctrl_flashing, R502_led_ctrl_always_on,
        R502_led_ctrl_always_off, R502_led_ctrl_grad_on,
        R502_led_ctrl_grad_off };

    R502_led_color_t color = R502_led_color_blue;
    uint8_t speed = 0x7f;
    uint8_t cycles = 0x5;

    // Loop through all control codes
    for(int ctrl_i = 0; ctrl_i < 6; ctrl_i++){
        ESP_LOGI(R502.TAG, "Test led ctrl %d", control_codes[ctrl_i]);

        err = R502_led_config(&R502, control_codes[ctrl_i], speed, color, cycles, &conf_code);
        TEST_ESP_OK(err);
        TEST_ASSERT_EQUAL(R502_ok, conf_code);

        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }

    // turn off
    err = R502_led_config(&R502, R502_led_ctrl_always_off, speed, color, cycles, &conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
}

TEST_CASE("LedConfig-speed", "[other]")
{
    esp_err_t err = R502_init(&R502, UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ, R502_baud_115200);
    TEST_ESP_OK(err);
    R502_conf_code_t conf_code;

    R502_led_ctrl_t ctrl = R502_led_ctrl_breathing;
    R502_led_color_t color = R502_led_color_purple;
    uint8_t cycles = 0x2;

    // Loop through all speeds to test the fastest speeds
    for(int speed_i = 256; speed_i > 0; speed_i /= 2){
        ESP_LOGI(R502.TAG, "Test led speed %d", speed_i-1);

        err = R502_led_config(&R502, ctrl, speed_i-1, color, cycles, &conf_code);
        TEST_ESP_OK(err);
        TEST_ASSERT_EQUAL(R502_ok, conf_code);

        vTaskDelay(40 * speed_i / portTICK_PERIOD_MS);
    }

    // turn off
    ctrl = R502_led_ctrl_always_off;
    err = R502_led_config(&R502, ctrl, 1, color, cycles, &conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
}