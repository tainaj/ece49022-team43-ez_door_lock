#include "CFAL1602.h"

/**
 * Private variables (put on CFAL1602 object)
 * 
 */

typedef struct my_struct_t {
    CFAL1602Interface *this;
    const char* msg;
    int count;
    bool isAutoScroll;
    bool isEnabled;
    const int line;
} my_struct_t;

my_struct_t line0 = {
    .count = 0,
    .isEnabled = false,
    .line = 0
};

my_struct_t line1 = {
    .count = 0,
    .isEnabled = false,
    .line = 1
};

static const char* TAG = "cfal1602";

static const char* clear_string = "                "; // used to clear line

static esp_event_handler_instance_t s_instance1_1;
static esp_event_handler_instance_t s_instance1_2;
static esp_event_handler_instance_t s_instance2_1;
static esp_event_handler_instance_t s_instance2_2;

static esp_event_handler_instance_t s_instanceS_1;
static esp_event_handler_instance_t s_instanceS_2;

/**
 * In 16-char string, replace instances of \0 (0x00) with SPACE (0x20)
 * Note: String s actually 17-char, including \0 end byte
 */
static void replace_null_with_spaces(char *s) {
    for (int i=0; i < 16; i++) {
        if (s[i] == 0x0) {
            s[i] = ' ';
        }
    }
}

static char* get_id_string(esp_event_base_t base, int32_t id) {
    char* event = "";
    if (base == TIMER_EVENTS) {
        switch(id) {
            case TIMER_EVENT_STARTED:
            event = "TIMER_EVENT_STARTED";
            break;
            case TIMER_EVENT_EXPIRY:
            event = "TIMER_EVENT_EXPIRY";
            break;
            case TIMER_EVENT_STOPPED:
            event = "TIMER_EVENT_STOPPED";
            break;
        }
    } else {
        event = "TASK_ITERATION_EVENT";
    }
    return event;
}

/* Send a command to the LCD. Uses spi_device_polling_transmit, which waits
 * until the transfer is complete.
 *
 * Since command transactions are usually small, they are handled in polling
 * mode for higher speed. The overhead of interrupt transactions is more than
 * just waiting for the transaction to complete.
 */
static void lcd_cmd(spi_device_handle_t spi, const uint8_t cmd)
{
    esp_err_t ret;
    spi_transaction_t t;

    uint16_t cmd2 = SPI_SWAP_DATA_TX(cmd, 10);

    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length=10;                     //Command is 8 bits
    t.tx_buffer=&cmd2;               //The data is the cmd itself
    t.user=(void*)0;                //D/C needs to be set to 0
    ret=spi_device_polling_transmit(spi, &t);  //Transmit!
    assert(ret==ESP_OK);            //Should have had no issues.
}

/* Send data to the LCD. Uses spi_device_polling_transmit, which waits until the
 * transfer is complete.
 *
 * Since data transactions are usually small, they are handled in polling
 * mode for higher speed. The overhead of interrupt transactions is more than
 * just waiting for the transaction to complete.
 */
static void lcd_data(spi_device_handle_t spi, const char cmd)
{
    esp_err_t ret;
    spi_transaction_t t;

    uint16_t tmp = (uint16_t)(WS2_data_write | cmd);

    uint16_t cmd2 = SPI_SWAP_DATA_TX(tmp, 10);

    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length=10;                     //Command is 8 bits
    t.tx_buffer=&cmd2;               //The data is the cmd itself
    t.user=(void*)0;                //D/C needs to be set to 0
    ret=spi_device_polling_transmit(spi, &t);  //Transmit!
    assert(ret==ESP_OK);            //Should have had no issues.
}

/* Print a string on one line to the LCD.
 *
 * In case of 0x0 (NULL) bytes in string, specify length of string.
 * Line number is 0 (top) or 1 (bottom)
 */
static void lcd_print(spi_device_handle_t spi, int line, const char *s) {
    /* (1) Send command moving cursor to start of selected line */
    /* (2) Write each character of text */
	/* (3) Write remaining slots with spaces */

    int n = 16; // limit string length
    int length = strlen(s);

    lcd_cmd(spi, (int)(WS2_setDDRAM | (line << 6))); /* (1) */
    while ((--length >= 0) && (--n >= 0)) { /* (2) */
        if (*s == 0x00) {
            lcd_data(spi, ' ');
            s++;
        } else {
            lcd_data(spi, *s++);
        }
    }
    while (--n >= 0) {                      /* (3) */
        lcd_data(spi, ' ');
    }
}

//Initialize the display
static void lcd_init(spi_device_handle_t spi)
{
    // set function and display
    vTaskDelay(100 / portTICK_PERIOD_MS);
    lcd_cmd(spi, WS2_function | WS2_function_DL | WS2_function_N | WS2_function_FT01);
    lcd_cmd(spi, WS2_display | WS2_display_D);

    // clear screen
    lcd_cmd(spi, WS2_clear);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    // set cursor and return home
    lcd_cmd(spi, WS2_entry | WS2_entry_I_D);
    lcd_cmd(spi, WS2_home);
}

/* Event source periodic timer related definitions */
ESP_EVENT_DEFINE_BASE(TIMER_EVENTS);

esp_timer_handle_t TIMER;

// Callback that will be executed when the timer period lapses. Posts the timer expiry event
// to the default event loop.
static void timer_callback(void* arg)
{
    ESP_ERROR_CHECK(esp_event_post(TIMER_EVENTS, TIMER_EVENT_EXPIRY, NULL, 0, portMAX_DELAY));
}

// Handler which executes when the timer started event gets executed by the initial posting.
static void timer_started_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    // local vars
    char print_string[17] = {0};
    my_struct_t * ps = (my_struct_t*)(handler_args);
    CFAL1602Interface *this = ps->this;

    if (ps->isEnabled == false) {
        return;
    }

    // generate printable string
    const char* print_str = ps->msg;
    
    snprintf(print_string, 17, "%s", print_str);
    replace_null_with_spaces(print_string);

    lcd_print(this->spi, ps->line, print_string);

    //ESP_LOGI(TAG, "%s:%s: timer_started_handler, string from char %d", base, get_id_string(base, id), ps->count);
}

// Handler which executes when the timer expiry event gets executed by the loop. This handler keeps track of
// how many times the timer expired. When a set number of expiry is reached, the handler stops the timer
// and sends a timer stopped event.
static void timer_expiry_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    // local vars
    char print_string[17] = {0};
    my_struct_t * ps = (my_struct_t*)(handler_args);
    CFAL1602Interface *this = ps->this;

    if (ps->isEnabled == false) {
        return;
    }

    // generate printable string
    const char* print_str;
    if (id == TIMER_EVENT_EXPIRY) {
        print_str = ps->msg + ps->count;
    } else {
        print_str = ps->msg;
    }
    
    snprintf(print_string, 17, "%s", print_str);
    replace_null_with_spaces(print_string);

    lcd_print(this->spi, ps->line, print_string);

    if (id == TIMER_EVENT_EXPIRY) {
        ps->count += (ps->isAutoScroll) ? 1 : 16;
    }

    if (strlen(print_str) <= 16) {
        ps->count = 0; // Loop string read back to beginning
    }
    //ESP_LOGI(TAG, "%s:%s: timer_expiry_handler, string from char %d", base, get_id_string(base, id), ps->count);
}

/* Event source task related definitions */
ESP_EVENT_DEFINE_BASE(TASK_EVENTS);

/**
 * -------------------------------------
 * Public API
 * -------------------------------------
 */

// Insert SPI handle to module
void WS2_init(CFAL1602Interface *this, gpio_num_t _pin_miso,
    gpio_num_t _pin_mosi, gpio_num_t _pin_sck, gpio_num_t _pin_cs)
{
    esp_err_t ret;

    if (this->initialized) {
        return;
    }
    this->pin_miso = _pin_miso;
    this->pin_mosi = _pin_mosi;
    this->pin_sck = _pin_sck;
    this->pin_cs = _pin_cs;

    // Configure parameters of an SPI driver,
    // communication pins and install the driver
    spi_bus_config_t buscfg={
        .miso_io_num = this->pin_miso,
        .mosi_io_num = this->pin_mosi,
        .sclk_io_num = this->pin_sck,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 1000
    };
    spi_device_interface_config_t devcfg={
#ifdef CONFIG_LCD_OVERCLOCK
        .clock_speed_hz = 26*1000*1000,           //Clock out at 26 MHz
#else
        .clock_speed_hz = 100*1000,           //Clock out at 10 MHz
#endif
        .mode = 0,                                //SPI mode 0
        .spics_io_num = this->pin_cs,               //CS pin
        .queue_size=7,                          //We want to be able to queue 7 transactions at a time
    };

    //Initialize the SPI bus
    ret=spi_bus_initialize(LCD_HOST, &buscfg, DMA_CHAN);
    ESP_ERROR_CHECK(ret);

    //Attach the LCD to the SPI bus
    ret=spi_bus_add_device(LCD_HOST, &devcfg, &(this->spi));
    ESP_ERROR_CHECK(ret);
    //Initialize the LCD
    lcd_init(this->spi);

    // Create and start the event sources
    esp_timer_create_args_t timer_args = {
        .callback = &timer_callback,
    };

    line0.this = this;
    line1.this = this;

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &TIMER)); // creates timer with timer expiry event. Does not start timer.

    esp_event_handler_instance_register(TIMER_EVENTS, TIMER_EVENT_STARTED, timer_started_handler, (void*)(&line1), &s_instanceS_1);

    esp_event_handler_instance_register(TIMER_EVENTS, TIMER_EVENT_EXPIRY, timer_expiry_handler, (void*)(&line0), &s_instance1_1);
    esp_event_handler_instance_register(TIMER_EVENTS, TIMER_EVENT_EXPIRY, timer_expiry_handler, (void*)(&line1), &s_instance2_1);

    ESP_LOGI(TAG, "starting event sources");
    ESP_ERROR_CHECK(esp_timer_start_periodic(TIMER, this->timer_period)); // used to be TIMER_PERIOD
}

void WS2_msg_print(CFAL1602Interface *this, const char* msg, int line, bool isAutoScroll) {
    if (line == 0) {
        line0.msg = msg;
        line0.count = 0;
        line0.isAutoScroll = isAutoScroll;
        line0.isEnabled = true;

        esp_event_handler_instance_register(TIMER_EVENTS, TIMER_EVENT_EXPIRY, timer_expiry_handler, (void*)(&line0), &s_instance1_2);
        esp_event_handler_instance_register(TIMER_EVENTS, TIMER_EVENT_STARTED, timer_started_handler, (void*)(&line0), &s_instanceS_2);

        esp_event_handler_instance_unregister(TIMER_EVENTS, TIMER_EVENT_EXPIRY, s_instance1_1);
        s_instance1_1 = s_instance1_2;
    } else {
        line1.msg = msg;
        line1.count = 0;
        line1.isAutoScroll = isAutoScroll;
        line1.isEnabled = true;

        esp_event_handler_instance_register(TIMER_EVENTS, TIMER_EVENT_EXPIRY, timer_expiry_handler, (void*)(&line1), &s_instance2_2);
        esp_event_handler_instance_register(TIMER_EVENTS, TIMER_EVENT_STARTED, timer_started_handler, (void*)(&line1), &s_instanceS_2);

        esp_event_handler_instance_unregister(TIMER_EVENTS, TIMER_EVENT_EXPIRY, s_instance2_1);
        s_instance2_1 = s_instance2_2;
    }

    esp_event_handler_instance_unregister(TIMER_EVENTS, TIMER_EVENT_STARTED, s_instanceS_1);
    s_instanceS_1 = s_instanceS_2;

    //ESP_LOGI(TAG, "%s:%s: posting to default loop", TIMER_EVENTS, get_id_string(TIMER_EVENTS, TIMER_EVENT_STARTED));
    ESP_ERROR_CHECK(esp_event_post(TIMER_EVENTS, TIMER_EVENT_STARTED, NULL, 0, portMAX_DELAY));
}

void WS2_msg_clear(CFAL1602Interface *this, int line) {
    WS2_msg_print(this, clear_string, line, false);
}