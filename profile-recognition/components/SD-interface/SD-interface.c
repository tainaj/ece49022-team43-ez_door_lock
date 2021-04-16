#include "SD-interface.h"

static const char *TAG = "SD-interface";

esp_err_t SD_init(void)
{

    esp_err_t ret;
    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    sdmmc_card_t* card;
    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(TAG, "Initializing SD card");

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
    // Please check its source code and implement error recovery when developing
    // production applications.
#ifndef USE_SPI_MODE
    ESP_LOGI(TAG, "Using SDMMC peripheral");
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    // To use 1-line SD mode, uncomment the following line:
    // slot_config.width = 1;

    // GPIOs 15, 2, 4, 12, 13 should have external 10k pull-ups.
    // Internal pull-ups are not sufficient. However, enabling internal pull-ups
    // does make a difference some boards, so we do that here.
    gpio_set_pull_mode(15, GPIO_PULLUP_ONLY);   // CMD, needed in 4- and 1- line modes
    gpio_set_pull_mode(2, GPIO_PULLUP_ONLY);    // D0, needed in 4- and 1-line modes
    gpio_set_pull_mode(4, GPIO_PULLUP_ONLY);    // D1, needed in 4-line mode only
    gpio_set_pull_mode(12, GPIO_PULLUP_ONLY);   // D2, needed in 4-line mode only
    gpio_set_pull_mode(13, GPIO_PULLUP_ONLY);   // D3, needed in 4- and 1-line modes

    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);
#else
    ESP_LOGI(TAG, "Using SPI peripheral");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CHAN);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return ESP_FAIL;
    }

    // NEW: Change GPIO2 to pull-up mode
    gpio_set_pull_mode(GPIO_NUM_2, GPIO_PULLUP_ONLY);

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    // Note: Uses HSPI port
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);
#endif //USE_SPI_MODE

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

    // Use POSIX and C standard library functions to work with files.
    // First create a file.
    
    // All done, unmount partition and disable SDMMC or SPI peripheral
    /*esp_vfs_fat_sdcard_unmount(mount_point, card);
    ESP_LOGI(TAG, "Card unmounted");
#ifdef USE_SPI_MODE
    //deinitialize the bus after all devices are removed
    //spi_bus_free(host.slot);
#endif*/
    return ESP_OK;
}

esp_err_t SD_readProfile(int profile_id, uint8_t *pin_p, int pin_n,
    uint8_t *priv_p, int priv_n, uint8_t *fp_p, int fp_n) {

    const char* directory = "/sdcard/profiles/";
    const char* fileName = "profile";
    const char* fileType = ".bin";

    char name_buffer[512];
    struct stat st;

    sprintf(name_buffer, "%s%s%d%s", directory, fileName, profile_id, fileType);
    ESP_LOGI("SD_readProfile", "Looking for file %s", name_buffer);

    // Check if file exists
    if (stat(name_buffer, &st) != 0) {
        return ESP_ERR_NOT_FOUND;
    }
    ESP_LOGI("SD_readProfile", "File found");

    // Open file
    FILE *f = fopen(name_buffer, "r");
    if (f == NULL) {
        ESP_LOGE("SD_readProfile", "Failed to open file");
        return ESP_FAIL;
    }

    // Read 4-byte PIN from file
    if (fread(pin_p, sizeof(uint8_t), pin_n, f) != pin_n) {
        ESP_LOGE("SD_readProfile", "PIN not fully read");
        return ESP_FAIL;
    }

    // Read 1-byte privilege from file
    if (fread(priv_p, sizeof(uint8_t), priv_n, f) != priv_n) {
        ESP_LOGE("SD_readProfile", "Privilege not fully read");
        return ESP_FAIL;
    }

    // Read 384x4 (1536) byte fingerprint template from file
    if (fread(fp_p, sizeof(uint8_t), fp_n, f) != fp_n) {
        ESP_LOGE("SD_readProfile", "Fingerprint not fully read");
        return ESP_FAIL;
    }

    fclose(f);

    return ESP_OK;
}

esp_err_t SD_writeProfile(int profile_id, uint8_t *pin_p, int pin_n,
    uint8_t *priv_p, int priv_n, uint8_t *fp_p, int fp_n) {

    const char* directory = "/sdcard/profiles/";
    const char* fileName = "profile";
    const char* fileType = ".bin";

    char name_buffer[512];

    sprintf(name_buffer, "%s%s%d%s", directory, fileName, profile_id, fileType);
    ESP_LOGI("SD_writeProfile", "Writing to file %s", name_buffer);

    // Open file
    FILE* f = fopen(name_buffer, "w");
    if (f == NULL) {
        ESP_LOGE("SD_writeProfile", "Failed to open file for writing");
        return ESP_FAIL;
    }

    // Write 4-byte PIN to file
    if (fwrite(pin_p, sizeof(uint8_t), pin_n, f) != pin_n) {
        ESP_LOGE("SD_writeProfile", "PIN not fully written");
        return ESP_FAIL;
    }

    // Write 1-byte privilege to file
    if (fwrite(priv_p, sizeof(uint8_t), priv_n, f) != priv_n) {
        ESP_LOGE("SD_writeProfile", "Privilege not fully written");
        return ESP_FAIL;
    }

    // Write 384x4 (1536) byte fingerprint template to file
    if (fwrite(fp_p, sizeof(uint8_t), fp_n, f) != fp_n) {
        ESP_LOGE("SD_writeProfile", "Fingerprint not fully written");
        return ESP_FAIL;
    }

    fclose(f);

    return ESP_OK;
}

esp_err_t SD_deleteProfile(int profile_id) {
    // Check if destination file exists before renaming
    struct stat st;
    const char* directory = "/sdcard/profiles/";
    const char* fileName = "profile";
    const char* fileType = ".bin";

    char name_buffer[512];

    sprintf(name_buffer, "%s%s%d%s", directory, fileName, profile_id, fileType);
    ESP_LOGI("SD_deleteProfile", "Looking for file %s", name_buffer);

    // Check if file exists
    if (stat(name_buffer, &st) == 0) {
        // Delete it if it exists
        unlink(name_buffer);
    } else {
        ESP_LOGE("SD_deleteProfile", "profile %d does not exist", profile_id);
    }

    return ESP_OK;
}