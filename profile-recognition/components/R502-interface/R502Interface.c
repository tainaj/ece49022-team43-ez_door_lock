#include "R502Interface.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

uint8_t data_cb_buffer[R502_MAX_DATA_LEN];

esp_err_t R502_init(R502Interface *this, uart_port_t _uart_num, gpio_num_t _pin_txd, 
    gpio_num_t _pin_rxd, gpio_num_t _pin_irq, 
    R502_baud_t _baud)
{
    if(this->initialized){
        return ESP_OK;
    }
    this->cur_baud = _baud;
    this->pin_txd = _pin_txd;
    this->pin_rxd = _pin_rxd;
    this->pin_irq = _pin_irq;
    this->uart_num = _uart_num;
    this->pin_rts = (gpio_num_t)UART_PIN_NO_CHANGE;
    this->pin_cts = (gpio_num_t)UART_PIN_NO_CHANGE;

    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = 9600*(this->cur_baud),
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .use_ref_tick = false
    };
    esp_err_t err = uart_param_config(this->uart_num, &uart_config);
    if(err) return err;
    err = uart_set_pin(this->uart_num, this->pin_txd, this->pin_rxd, this->pin_rts, this->pin_cts);
    if(err) return err;
    err = uart_driver_install(this->uart_num, 
        MAX(sizeof(R502_DataPkg_t), this->min_uart_buffer_size), 0, 0, NULL, 0);
    if(err) return err;

    /* Configure parameters of a pin_isr interrupt,
     * communication pins and install the driver */
    gpio_config_t io_conf = {
        .intr_type = GPIO_PIN_INTR_NEGEDGE,
        .pin_bit_mask = 1ULL<<(this->pin_irq),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 0,
        .pull_down_en = 0
    };
    err = gpio_config(&io_conf);
    if(err) return err;

    //hook isr handler for specific gpio pin (first requires gpi_iser_install_service(0))
    gpio_isr_handler_add(this->pin_irq, gpio_isr_handler, (void*) this->pin_irq);
    if(err) return err;

    // wait for R502 to prepare itself
    vTaskDelay(200 / portTICK_PERIOD_MS);
    this->initialized = true;
    this->interrupt = 0;
    return ESP_OK;
}

esp_err_t R502_deinit(R502Interface *this)
{
    if(this->initialized){
        this->initialized = false;
        esp_err_t err_uart_driver = uart_driver_delete(this->uart_num);
        esp_err_t err_isr_remove = gpio_isr_handler_remove(this->pin_irq);
        gpio_uninstall_isr_service();
        if(err_uart_driver) return err_uart_driver;
        if(err_isr_remove) return err_isr_remove;

        // reset up_image_cb
        this->up_image_cb = NULL;
        // reset up_char_cb
        this->up_char_cb = NULL;
    }
    return ESP_OK;
}

/*static void IRAM_ATTR irq_intr(void *arg)
{
    R502Interface *me = (R502Interface *)arg;
    me->interrupt++;
    //printf("Interrupt count: %d\n", me->interrupt);
    //ESP_LOGI(me->TAG, "interrupt count %d", me->interrupt);
}*/

uint8_t *R502_get_module_address(R502Interface *this){
    return this->adder;
}

void R502_set_up_image_cb(R502Interface *this, up_image_cb_t _up_image_cb){
    this->up_image_cb = _up_image_cb;
}

void R502_set_up_char_cb(R502Interface *this, up_char_cb_t _up_char_cb){
    this->up_char_cb = _up_char_cb;
}

esp_err_t R502_vfy_pass(R502Interface *this, const uint8_t pass[4], 
    R502_conf_code_t *res)
{
    R502_DataPkg_t pkg;
    R502_VfyPwd_t *data = &pkg.data.vfy_pwd;

    // Fill package
    set_headers(this, &pkg, R502_pid_command, sizeof(R502_VfyPwd_t));
    data->instr_code = R502_ic_vfy_pwd;
    for(int i = 0; i < 4; i++){
        data->password[i] = pass[i];
    }
    fill_checksum(&pkg);

    // Send package, get response
    R502_DataPkg_t receive_pkg;
    R502_GeneralAck_t *receive_data = &receive_pkg.data.general_ack;
    esp_err_t err = send_command_package(this, &pkg, &receive_pkg, 
        sizeof(*receive_data), this->default_read_delay);
    if(err) return err;
    
    // Return result
    *res = (R502_conf_code_t)receive_data->conf_code;
    return ESP_OK;
}

static esp_err_t set_sys_para(R502Interface *this, R502_para_num parameter_num, int value, 
    R502_conf_code_t *res)
{
    // validate input data
    esp_err_t err = ESP_OK;
    switch(parameter_num){
        case R502_para_num_baud_control:{
            if(value != 1 && value != 2 && value != 4 && value != 6 && 
                value != 12)
            {
                err = ESP_ERR_INVALID_ARG;
            }
            break;
        }
        case R502_para_num_security_level:{
            if(value != 1 && value != 2 && value != 3 && value != 4 && 
                value != 5)
            {
                err = ESP_ERR_INVALID_ARG;
            }
            break;
        }
        case R502_para_num_data_pkg_len:{
            if(value != 0 && value != 1 && value != 2 && value != 3){
                err = ESP_ERR_INVALID_ARG;
            }
            break;
        }
        default:{
            ESP_LOGE(this->TAG, "invalid parameter number, %d", parameter_num);
            return ESP_ERR_INVALID_ARG;
        }
    };
    if(err){
        ESP_LOGE(this->TAG, "invalid parameter value, %d. Param_num %d", value, 
            parameter_num);
        return err;
    }

    R502_DataPkg_t pkg;
    R502_SetSysPara_t *data = &pkg.data.set_sys_para;

    // Fill package
    set_headers(this, &pkg, R502_pid_command, sizeof(R502_SetSysPara_t));
    data->instr_code = R502_ic_set_sys_para;
    data->parameter_number = parameter_num;
    data->contents = value;
    fill_checksum(&pkg);

    // Send package, get response
    R502_DataPkg_t receive_pkg;
    R502_GeneralAck_t *receive_data = &receive_pkg.data.general_ack;
    err = send_command_package(this, &pkg, &receive_pkg, 
        sizeof(*receive_data), this->default_read_delay);
    if(err) return err;

    // Return result
    *res = (R502_conf_code_t)receive_data->conf_code;
    return ESP_OK;
}

esp_err_t R502_set_baud_rate(R502Interface *this, R502_baud_t baud, R502_conf_code_t *res)
{
    esp_err_t err = set_sys_para(this, R502_para_num_baud_control, baud, res);
    if(err){
        ESP_LOGE(this->TAG, "set_sys_para err %s", esp_err_to_name(err));
        return err;
    }
    // remember the current baud rate
    this->cur_baud = baud;

    esp_err_t err_uart_driver = uart_driver_delete(this->uart_num);
    if(err_uart_driver){
        ESP_LOGE(this->TAG, "error deleting uart driver: %s",  
            esp_err_to_name(err));
    }
    // Reinitialize with the new baud rate
    uart_config_t uart_config = {
        .baud_rate = 9600*baud,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .use_ref_tick = false
    };
    err = uart_param_config(this->uart_num, &uart_config);
    if(err){
        ESP_LOGE(this->TAG, "error reconfiguring uart baud rate: %s",  
            esp_err_to_name(err));
    }

    err = uart_driver_install(this->uart_num, 
        MAX(sizeof(R502_DataPkg_t), this->min_uart_buffer_size), 0, 0, NULL,
        0);
    if(err){
        ESP_LOGE(this->TAG, "error installing uart driver: %s",  
            esp_err_to_name(err));
    }
    return ESP_OK;
}

esp_err_t R502_set_security_level(R502Interface *this, uint8_t security_level,
    R502_conf_code_t *res)
{
    esp_err_t err = set_sys_para(this, R502_para_num_security_level, 
        security_level, res);
    return err;
}

esp_err_t R502_set_data_package_length(R502Interface *this, R502_data_len_t data_length,
    R502_conf_code_t *res)
{
    esp_err_t err = set_sys_para(this, R502_para_num_data_pkg_len, data_length, res);
    if(err) return err;
    return ESP_OK;
}

esp_err_t R502_read_sys_para(R502Interface *this, R502_conf_code_t *res, 
    R502_sys_para_t *sys_para)
{
    R502_DataPkg_t pkg;
    R502_GeneralCommand_t *data = &pkg.data.general;

    // Fill package
    set_headers(this, &pkg, R502_pid_command, sizeof(R502_GeneralCommand_t));
    data->instr_code = R502_ic_read_sys_para;
    fill_checksum(&pkg);

    // Send package, get response
    R502_DataPkg_t receive_pkg;
    R502_ReadSysParaAck_t *receive_data = &receive_pkg.data.read_sys_para_ack;
    esp_err_t err = send_command_package(this, &pkg, &receive_pkg, 
        sizeof(*receive_data), this->default_read_delay);
    if(err) return err;

    // Return result
    *res = (R502_conf_code_t)receive_data->conf_code;

    if(sys_para->system_identifier_code != this->system_identifier_code){
        ESP_LOGW(this->TAG, "sys_para system identifier is %d not %d", 
            sys_para->system_identifier_code, this->system_identifier_code);
    }

    sys_para->status_register = conv_8_to_16(receive_data->data + 0);
    sys_para->system_identifier_code = conv_8_to_16(receive_data->data + 2);
    sys_para->finger_library_size = conv_8_to_16(receive_data->data + 4);
    sys_para->security_level = conv_8_to_16(receive_data->data + 6);
    memcpy(sys_para->device_address, receive_data->data+8, 4);
    sys_para->data_package_length = (R502_data_len_t)conv_8_to_16(receive_data->data + 12);
    sys_para->baud_setting = (R502_baud_t)conv_8_to_16(receive_data->data + 14);

    return ESP_OK;
}

esp_err_t R502_template_num(R502Interface *this, R502_conf_code_t *res, 
    uint16_t *template_num)
{
    R502_DataPkg_t pkg;
    R502_GeneralCommand_t *data = &pkg.data.general;

    // Fill package
    set_headers(this, &pkg, R502_pid_command, sizeof(R502_GeneralCommand_t));
    data->instr_code = R502_ic_template_num;
    fill_checksum(&pkg);

    // Send package, get response
    R502_DataPkg_t receive_pkg;
    R502_TemplateNumAck_t *receive_data = &receive_pkg.data.template_num_ack;
    esp_err_t err = send_command_package(this, &pkg, &receive_pkg, 
        sizeof(*receive_data), this->default_read_delay);
    if(err) return err;

    // Return result
    *res = (R502_conf_code_t)receive_data->conf_code;
    *template_num = conv_8_to_16(receive_data->template_num);

    return ESP_OK;
}

esp_err_t R502_gen_image(R502Interface *this, R502_conf_code_t *res)
{
    R502_DataPkg_t pkg;
    R502_GeneralCommand_t *data = &pkg.data.general;

    // Fill package
    set_headers(this, &pkg, R502_pid_command, sizeof(R502_GeneralCommand_t));
    data->instr_code = R502_ic_gen_img;
    fill_checksum(&pkg);

    // Send package, get response
    R502_DataPkg_t receive_pkg;
    R502_GeneralAck_t *receive_data = &receive_pkg.data.general_ack;
    esp_err_t err = send_command_package(this, &pkg, &receive_pkg, 
        sizeof(*receive_data), this->read_delay_gen_image);
    if(err) return err;

    // Return result
    *res = (R502_conf_code_t)receive_data->conf_code;
    return ESP_OK;
}

/*esp_err_t R502_up_image(R502Interface *this, R502_data_len_t data_len, 
    R502_conf_code_t *res)
{
    R502_DataPkg_t pkg;
    R502_GeneralCommand_t *data = &pkg.data.general;

    if(!(this->up_image_cb)){
        ESP_LOGW(this->TAG, "up_image callback not set");
        return ESP_ERR_INVALID_STATE;
    }
    
    // TODO: Check stored parameters to see if the R502 has an image ready
    // to send. If not, still perform the transfer, but send a warning

    // Fill package
    set_headers(this, &pkg, R502_pid_command, sizeof(R502_GeneralCommand_t));
    data->instr_code = R502_ic_up_image;
    fill_checksum(&pkg);

    // Send package, get response
    R502_DataPkg_t receive_pkg;
    R502_GeneralAck_t *receive_data = &receive_pkg.data.general_ack;
    esp_err_t err = send_command_package(this, &pkg, &receive_pkg, 
        sizeof(*receive_data), this->read_delay_gen_image);
    if(err) return err;

    *res = (R502_conf_code_t)receive_data->conf_code;
    if(*res != R502_ok){ 
        // The esp side of things is ok, but the module isn't ready to send
        return ESP_OK;
    }
    int data_len_i = 0;
    switch(data_len){
        case R502_data_len_32:
            data_len_i = 32;
            break;
        case R502_data_len_64:
            data_len_i = 64;
            break;
        case R502_data_len_128:
            data_len_i = 128;
            break;
        case R502_data_len_256:
            data_len_i = 256;
            break;
        default:
            ESP_LOGE(this->TAG, "invalid data length, use enum");
            return ESP_ERR_INVALID_ARG;
    }

    // receive data packages
    R502_pid_t pid = R502_pid_data;
    uint8_t data_cb_buffer[R502_max_data_len * 2];
    uint8_t *rec_data = receive_pkg.data.data.content;
    int bytes_received = 0;
    while(pid == R502_pid_data){
        err = receive_package(this, &receive_pkg, 
            data_len_i + R502_cs_len + this->header_size, this->default_read_delay);
        if(err) return err;
        bytes_received += data_len_i;

        pid = (R502_pid_t)receive_pkg.pid;

        // convert 4bit bytes to 8bit in an expanded buffer
        for(int i = 0; i < data_len_i; i++){
            // Low four bytes
            data_cb_buffer[i*2] = (rec_data[i] & 0xf) << 4;
            // High four bytes
            data_cb_buffer[i*2+1] = rec_data[i] & 0xf0;
        }

        // call callback
        this->up_image_cb(data_cb_buffer, data_len_i * 2);
    }
    ESP_LOGI(this->TAG, "bytes received %d", bytes_received);

    return ESP_OK;
}*/

esp_err_t R502_img_2_tz(R502Interface *this, uint8_t buffer_id, R502_conf_code_t *res)
{
    R502_DataPkg_t pkg;
    R502_Img2Tz_t *data = &pkg.data.img_2_tz;

    // Fill package
    set_headers(this, &pkg, R502_pid_command, sizeof(R502_Img2Tz_t));
    data->instr_code = R502_ic_img_2_tz;
    data->buffer_id = buffer_id;
    fill_checksum(&pkg);

    // Send package, get response
    R502_DataPkg_t receive_pkg;
    R502_GeneralAck_t *receive_data = &receive_pkg.data.general_ack;
    esp_err_t err = send_command_package(this, &pkg, &receive_pkg, 
        sizeof(*receive_data), this->read_delay_gen_image);
    if(err) return err;

    // Return result
    *res = (R502_conf_code_t)receive_data->conf_code;
    return ESP_OK;
}

esp_err_t R502_reg_model(R502Interface *this, R502_conf_code_t *res)
{
    R502_DataPkg_t pkg;
    R502_GeneralCommand_t *data = &pkg.data.general;

    // Fill package
    set_headers(this, &pkg, R502_pid_command, sizeof(R502_GeneralCommand_t));
    data->instr_code = R502_ic_reg_model;
    fill_checksum(&pkg);

    // Send package, get response
    R502_DataPkg_t receive_pkg;
    R502_GeneralAck_t *receive_data = &receive_pkg.data.general_ack;
    esp_err_t err = send_command_package(this, &pkg, &receive_pkg, 
        sizeof(*receive_data), this->read_delay_gen_image);
    if(err) return err;

    // Return result
    *res = (R502_conf_code_t)receive_data->conf_code;
    return ESP_OK;
}

esp_err_t R502_up_char(R502Interface *this, R502_data_len_t data_len, uint8_t buffer_id, 
    R502_conf_code_t *res)
{
    R502_DataPkg_t pkg;
    R502_UpChar_t *data = &pkg.data.up_char;

    if(!(this->up_char_cb)){
        ESP_LOGW(this->TAG, "up_char callback not set");
        return ESP_ERR_INVALID_STATE;
    }
    
    // TODO: Check stored parameters to see if the R502 has an image ready
    // to send. If not, still perform the transfer, but send a warning

    // Fill package
    set_headers(this, &pkg, R502_pid_command, sizeof(R502_UpChar_t));
    data->instr_code = R502_ic_up_char;
    data->buffer_id = buffer_id;
    fill_checksum(&pkg);

    // Send package, get response
    R502_DataPkg_t receive_pkg;
    R502_GeneralAck_t *receive_data = &receive_pkg.data.general_ack;
    esp_err_t err = send_command_package(this, &pkg, &receive_pkg, 
        sizeof(*receive_data), this->read_delay_gen_image);
    if(err) return err;

    *res = (R502_conf_code_t)receive_data->conf_code;
    if(*res != R502_ok){ 
        // The esp side of things is ok, but the module isn't ready to send
        return ESP_OK;
    }
    int data_len_i = 0;
    switch(data_len){
        case R502_data_len_32:
            data_len_i = 32;
            break;
        case R502_data_len_64:
            data_len_i = 64;
            break;
        case R502_data_len_128:
            data_len_i = 128;
            break;
        case R502_data_len_256:
            data_len_i = 256;
            break;
        default:
            ESP_LOGE(this->TAG, "invalid data length, use enum");
            return ESP_ERR_INVALID_ARG;
    }

    // receive data packages
    R502_pid_t pid = R502_pid_data;
    //uint8_t data_cb_buffer[R502_max_data_len];
    uint8_t *rec_data = receive_pkg.data.data.content;
    int bytes_received = 0;
    while(pid == R502_pid_data){
        err = receive_package(this, &receive_pkg, 
            data_len_i + R502_cs_len + this->header_size, this->default_read_delay);
        if(err) return err;
        bytes_received += data_len_i;

        pid = (R502_pid_t)receive_pkg.pid;

        // transcribe receive data into data buffer
        for(int i = 0; i < data_len_i; i++){
            // Full byte
            data_cb_buffer[i] = rec_data[i];
        }

        // call callback
        this->up_char_cb(data_cb_buffer, data_len_i);
    }
    ESP_LOGI(this->TAG, "bytes received %d", bytes_received);

    return ESP_OK;
}

esp_err_t R502_down_char(R502Interface *this, R502_data_len_t data_len, uint8_t buffer_id, 
    uint8_t * char_data, R502_conf_code_t *res)
{
    R502_DataPkg_t pkg;
    R502_DownChar_t *data = &pkg.data.down_char;

    if(!char_data){
        ESP_LOGW(this->TAG, "down_char data not supplied");
        return ESP_ERR_INVALID_STATE;
    }
    
    // TODO: Check stored parameters to see if the R502 has an image ready
    // to send. If not, still perform the transfer, but send a warning

    // Fill package
    set_headers(this, &pkg, R502_pid_command, sizeof(R502_DownChar_t));
    data->instr_code = R502_ic_down_char;
    data->buffer_id = buffer_id;
    fill_checksum(&pkg);

    // Send package, get response
    R502_DataPkg_t receive_pkg;
    R502_GeneralAck_t *receive_data = &receive_pkg.data.general_ack;
    esp_err_t err = send_command_package(this, &pkg, &receive_pkg, 
        sizeof(*receive_data), this->read_delay_gen_image);
    if(err) return err;

    *res = (R502_conf_code_t)receive_data->conf_code;
    if(*res != R502_ok){ 
        // The esp side of things is ok, but the module isn't ready to send
        return ESP_OK;
    }

    int data_len_i = 0;
    uint16_t length = 0;

    switch(data_len){
        case R502_data_len_32:
            data_len_i = 32;
            length = sizeof(R502_Data32_t);
            break;
        case R502_data_len_64:
            data_len_i = 64;
            length = sizeof(R502_Data64_t);
            break;
        case R502_data_len_128:
            data_len_i = 128;
            length = sizeof(R502_Data128_t);
            break;
        case R502_data_len_256:
            data_len_i = 256;
            length = sizeof(R502_Data_t);
            break;
        default:
            ESP_LOGE(this->TAG, "invalid data length, use enum");
            return ESP_ERR_INVALID_ARG;
    }

    // transmit data packages
    R502_DataPkg_t pkg2;
    R502_Data_t *data2 = &pkg2.data.data;

    int char_data_length = R502_template_size;
    int packet_num = char_data_length / data_len_i;

    printf("packet_num: %d", packet_num);
    printf("char_data_length: %d", char_data_length);

    int i = 1;
    while (i <= packet_num) {
        // Fill data package
        set_headers(this, &pkg2, R502_pid_data, length);
        for (int j = 0; j < data_len_i; j++) {
            (data2->content)[j] = char_data[data_len_i*(i-1) + j];
        }
        fill_checksum(&pkg2);

        // Send package, get response
        err = send_package(this, &pkg2);
        if(err) return err;

        i++;
    }

    // Fill end of data package
    set_headers(this, &pkg2, R502_pid_end_of_data, length);
    for (int j = 0; j < data_len_i; j++) {
        (data2->content)[j] = 0xfa;
    }
    fill_checksum(&pkg2);

    // Send package, get response
    err = send_package(this, &pkg2);
    if(err) return err;

    ESP_LOGI(this->TAG, "bytes sent %d", char_data_length);

    return ESP_OK;
}

esp_err_t R502_store(R502Interface *this, uint8_t buffer_id, uint16_t page_id,
    R502_conf_code_t *res)
{
    R502_DataPkg_t pkg;
    R502_Store_t *data = &pkg.data.store;

    uint8_t page_id_i[2];

    // Fill package
    set_headers(this, &pkg, R502_pid_command, sizeof(R502_Store_t));
    data->instr_code = R502_ic_store;
    data->buffer_id = buffer_id;
    conv_16_to_8(page_id, page_id_i);
    for(int i = 0; i < 2; i++){
        data->page_id[i] = page_id_i[i];
    }
    fill_checksum(&pkg);

    // Send package, get response
    R502_DataPkg_t receive_pkg;
    R502_GeneralAck_t *receive_data = &receive_pkg.data.general_ack;
    esp_err_t err = send_command_package(this, &pkg, &receive_pkg, 
        sizeof(*receive_data), this->read_delay_gen_image);
    if(err) return err;

    // Return result
    *res = (R502_conf_code_t)receive_data->conf_code;
    return ESP_OK;
}

esp_err_t R502_delet_char(R502Interface *this, uint16_t page_id,
    uint16_t num_of_templates, R502_conf_code_t *res)
{
    R502_DataPkg_t pkg;
    R502_DeletChar_t *data = &pkg.data.delet_char;

    uint8_t page_id_i[2];
    uint8_t n_i[2];

    // Fill package
    set_headers(this, &pkg, R502_pid_command, sizeof(R502_DeletChar_t));
    data->instr_code = R502_ic_delet_char;
    conv_16_to_8(page_id, page_id_i);
    conv_16_to_8(num_of_templates, n_i);
    for(int i = 0; i < 2; i++){
        data->page_id[i] = page_id_i[i];
        data->num_of_templates[i] = n_i[i];
    }
    fill_checksum(&pkg);

    // Send package, get response
    R502_DataPkg_t receive_pkg;
    R502_GeneralAck_t *receive_data = &receive_pkg.data.general_ack;
    esp_err_t err = send_command_package(this, &pkg, &receive_pkg, 
        sizeof(*receive_data), this->default_read_delay);
    if(err) return err;

    // Return result
    *res = (R502_conf_code_t)receive_data->conf_code;
    return ESP_OK;
}

esp_err_t R502_empty(R502Interface *this, R502_conf_code_t *res)
{
    R502_DataPkg_t pkg;
    R502_GeneralCommand_t *data = &pkg.data.general;

    // Fill package
    set_headers(this, &pkg, R502_pid_command, sizeof(R502_GeneralCommand_t));
    data->instr_code = R502_ic_empty;
    fill_checksum(&pkg);

    // Send package, get response
    R502_DataPkg_t receive_pkg;
    R502_GeneralAck_t *receive_data = &receive_pkg.data.general_ack;
    esp_err_t err = send_command_package(this, &pkg, &receive_pkg, 
        sizeof(*receive_data), this->default_read_delay);
    if(err) return err;

    // Return result
    *res = (R502_conf_code_t)receive_data->conf_code;
    return ESP_OK;
}

esp_err_t R502_search(R502Interface *this, uint8_t buffer_id, uint16_t start_page,
    uint16_t page_num, R502_conf_code_t *res, uint16_t *page_id,
    uint16_t *match_score)
{
    R502_DataPkg_t pkg;
    R502_Search_t *data = &pkg.data.search;

    uint8_t start_page_i[2];
    uint8_t page_num_i[2];

    // Fill package
    set_headers(this, &pkg, R502_pid_command, sizeof(R502_Search_t));
    data->instr_code = R502_ic_search;
    data->buffer_id = buffer_id;
    conv_16_to_8(start_page, start_page_i);
    conv_16_to_8(page_num, page_num_i);
    for(int i = 0; i < 2; i++){
        data->start_page[i] = start_page_i[i];
        data->page_num[i] = page_num_i[i];
    }
    fill_checksum(&pkg);

    // Send package, get response
    R502_DataPkg_t receive_pkg;
    R502_SearchAck_t *receive_data = &receive_pkg.data.search_ack;
    esp_err_t err = send_command_package(this, &pkg, &receive_pkg, 
        sizeof(*receive_data), this->read_delay_gen_image);
    if(err) return err;

    // Return result
    *res = (R502_conf_code_t)receive_data->conf_code;
    *page_id = conv_8_to_16(receive_data->page_id);
    *match_score = conv_8_to_16(receive_data->match_score);
    return ESP_OK;
}

esp_err_t R502_led_config(R502Interface *this, R502_led_ctrl_t ctrl, uint8_t speed,
    R502_led_color_t color_index, uint8_t count, R502_conf_code_t *res)
{
    R502_DataPkg_t pkg;
    R502_LedConfig_t *data = &pkg.data.led_config;


    // Fill package
    set_headers(this, &pkg, R502_pid_command, sizeof(R502_LedConfig_t));
    data->instr_code = R502_ic_led_config;
    data->ctrl = ctrl;
    data->speed = speed;
    data->color_index = color_index;
    data->count = count;

    fill_checksum(&pkg);

    // Send package, get response
    R502_DataPkg_t receive_pkg;
    R502_GeneralAck_t *receive_data = &receive_pkg.data.general_ack;
    esp_err_t err = send_command_package(this, &pkg, &receive_pkg, 
        sizeof(*receive_data), this->default_read_delay);
    if(err) return err;

    // Return result
    *res = (R502_conf_code_t)receive_data->conf_code;
    return ESP_OK;
}


static esp_err_t send_command_package(R502Interface *this, const R502_DataPkg_t *pkg,
    R502_DataPkg_t *receive_pkg, int data_rec_length, int read_delay_ms)
{
    esp_err_t err = send_package(this, pkg);
    if(err) return err;
    return receive_package(this, receive_pkg, data_rec_length + this->header_size, 
        read_delay_ms);
}


static esp_err_t send_package(R502Interface *this, const R502_DataPkg_t *pkg)
{
    int pkg_len = package_length(pkg);
    if(pkg_len < this->header_size + sizeof(R502_GeneralCommand_t) || 
        pkg_len > sizeof(R502_DataPkg_t))
    {
        ESP_LOGE(this->TAG, "package length not set correctly");
        return ESP_ERR_INVALID_ARG;
    }

    //printf("Send Data\n");
    //int printed = 0;
    //while(printed < pkg_len){
        //for(int i = 0; i < 8 && printed < pkg_len; i++){
            //printf("0x%02X ", *((uint8_t *)&pkg+printed));
            //printed++;
        //}
        //printf("\n");
    //}

    //printf("freak out?\n");
    int len = uart_write_bytes(this->uart_num, (char *)pkg, pkg_len);
    if(len == -1){
        ESP_LOGE(this->TAG, "uart write error, parameter error");
        return ESP_ERR_INVALID_STATE;
    }
    else if(len != package_length(pkg)){
        // not all data transferred
        ESP_LOGE(this->TAG, "uart write error, wrong number of bytes written");
        return ESP_ERR_INVALID_SIZE;
    }
    return ESP_OK;
}

static esp_err_t receive_package(R502Interface *this, const R502_DataPkg_t *rec_pkg,
    int data_length, int read_delay_ms)
{
    int len = uart_read_bytes(this->uart_num, (uint8_t *)rec_pkg, data_length,
        read_delay_ms / portTICK_RATE_MS);
    
    //ESP_LOGI(this->TAG, "received %d bytes", len);

    if(len == -1){
        ESP_LOGE(this->TAG, "uart read error, parameter error");
        return ESP_ERR_INVALID_STATE;
    }
    else if(len == 0){
        ESP_LOGE(this->TAG, "uart read error, R502 not found");
        return ESP_ERR_NOT_FOUND;
    }
    // This check is uncesseccary, it'll probably fail the crc cause it doesn't
    // write data into the last crc byte if this is the case
    else if(len < data_length){
        ESP_LOGE(this->TAG, "uart read error, not enough bytes read, %d < %d", 
            len, data_length);
        uart_flush(this->uart_num);
        return ESP_ERR_INVALID_RESPONSE;
    }

    //printf("response Data\n");
    //int printed = 0;
    //while(printed < len){
        //for(int i = 0; i < 8 && printed < len; i++){
            //printf("0x%02X ", *((uint8_t *)&rec_pkg+printed));
            //printed++;
        //}
        //printf("\n");
    //}

    // Verify response
    if(!verify_checksum(rec_pkg)){
        ESP_LOGE(this->TAG, "uart read error, invalid CRC"); 
        uart_flush(this->uart_num);
        return ESP_ERR_INVALID_CRC;
    }
    esp_err_t err = verify_headers(this, rec_pkg, len - this->header_size);
    if(err){
        uart_flush(this->uart_num);
    }
    return err;
}

static void busy_delay(int64_t microseconds)
{
    // wait
    int64_t time_start = esp_timer_get_time();
    while(esp_timer_get_time() < time_start + microseconds);
}

static void fill_checksum(R502_DataPkg_t *package)
{
    int data_length = conv_8_to_16(package->length) - 2; // -2 for the 2 byte CS
    int sum = package->pid + package->length[0] + package->length[1];
    uint8_t *itr = (uint8_t *)&(package->data);
    for(int i = 0; i < data_length; i++){
        sum += *itr++;
    }
    // Now itr is pointing to the first checksum byte
    *itr = (sum >> 8) & 0xff;
    *++itr = sum & 0xff;
}

static bool verify_checksum(const R502_DataPkg_t *package)
{
    int data_length = conv_8_to_16(package->length) - 2; // -2 for the 2 byte CS
    int sum = package->pid + package->length[0] + package->length[1];
    uint8_t *itr = (uint8_t *)&(package->data);
    for(int i = 0; i < data_length; i++){
        sum += *itr++;
    }
    int checksum = *itr << 8;
    checksum += *++itr;

    return (sum == checksum);
}

static esp_err_t verify_headers(R502Interface *this, const R502_DataPkg_t *pkg, 
    uint16_t length)
{
    // start
    if(memcmp(pkg->start, this->start, sizeof(this->start)) != 0){
        ESP_LOGE(this->TAG, "Response has invalid start");
        return ESP_ERR_INVALID_RESPONSE;
    }

    // module address
    if(memcmp(pkg->adder, this->adder, sizeof(this->adder)) != 0){
        ESP_LOGE(this->TAG, "Response has invalid adder");
        return ESP_ERR_INVALID_RESPONSE;
    }

    // pid
    if(pkg->pid != R502_pid_command && pkg->pid != R502_pid_data && 
        pkg->pid != R502_pid_ack && pkg->pid != R502_pid_end_of_data){
        ESP_LOGE(this->TAG, "Response has invalid pid, %d", pkg->pid);
        return ESP_ERR_INVALID_RESPONSE;
    }

    // length
    if(conv_8_to_16(pkg->length) != length){
        ESP_LOGE(this->TAG, "Response has invalid length, %d vs %dB received", 
            conv_8_to_16(pkg->length), length);
        return ESP_ERR_INVALID_RESPONSE;
    }

    return ESP_OK;
}

static void set_headers(R502Interface *this, R502_DataPkg_t *package, R502_pid_t pid,
    uint16_t length)
{
    memcpy(package->start, this->start, sizeof(this->start));
    memcpy(package->adder, this->adder, sizeof(this->adder));

    package->pid = pid;

    package->length[0] = (length >> 8) & 0xff;
    package->length[1] = length & 0xff;
}


static uint16_t conv_8_to_16(const uint8_t in[2])
{
    return (in[0] << 8) + in[1];
}

static void conv_16_to_8(const uint16_t in, uint8_t out[2])
{
    out[0] = (in >> 8) & 0xff;
    out[1] = in & 0xff;
}

static uint16_t package_length(const R502_DataPkg_t *pkg){
    return sizeof(*pkg) - sizeof(pkg->data) + conv_8_to_16(pkg->length);
}