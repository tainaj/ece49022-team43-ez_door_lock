#ifndef SD_INTERFACE_H_
#define SD_INTERFACE_H_

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "sdkconfig.h"

#include "freertos/task.h"
#include "freertos/FreeRTOS.h"

#ifdef CONFIG_IDF_TARGET_ESP32
#include "driver/sdmmc_host.h"
#endif

/**
 * @mainpage SD card reader code
 * This code is an abstraction interface for use by the profile recognition subsystem.
 * 
 * This is an ESP-IDF component developed for the esp32 to interface with the
 * SD card reader via SPI
 */

/**
 * \brief Provides command-level api to interact with the SD card reader
 */

#define MOUNT_POINT "/sdcard"

// This example can use SDMMC and SPI peripherals to communicate with SD card.
// By default, SDMMC peripheral is used.
// To enable SPI mode, uncomment the following line:

#define USE_SPI_MODE

// ESP32-S2 doesn't have an SD Host peripheral, always use SPI:
#ifdef CONFIG_IDF_TARGET_ESP32S2
#ifndef USE_SPI_MODE
#define USE_SPI_MODE
#endif // USE_SPI_MODE
// on ESP32-S2, DMA channel must be the same as host id
#define SPI_DMA_CHAN    host.slot
#endif //CONFIG_IDF_TARGET_ESP32S2

// DMA channel to be used by the SPI peripheral
#ifndef SPI_DMA_CHAN
#define SPI_DMA_CHAN    1
#endif //SPI_DMA_CHAN

// When testing SD and SPI modes, keep in mind that once the card has been
// initialized in SPI mode, it can not be reinitialized in SD mode without
// toggling power to the card.

#ifdef USE_SPI_MODE
// Pin mapping when using SPI mode.
// With this mapping, SD card can be used both in SPI and 1-line SD mode.
// Note that a pull-up on CS line is required in SD mode.
#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5
#endif //USE_SPI_MODE

/**
 * \brief Initialize SD interface, must call first
 */
void SD_init();

/**
 * \brief Read profile contents from submodule buffer to SD card
 * \param profile_id Profile number, 1 to MAX_PROFILES
 * \param pin_p Pointer to PIN buffer
 * \param pin_n Size of PIN buffer
 * \param priv_p Pointer to privilege buffer
 * \param priv_n Size of privilege buffer
 * \param fp_p Pointer to fingerprint buffer
 * \param fp_n Size of fingerprint buffer
 * \retval See vfy_pass for description of all possible return values
 */
esp_err_t SD_readProfile(int profile_id, uint8_t *pin_p, int pin_n,
    uint8_t *priv_p, int priv_n, uint8_t *fp_p, int fp_n);

/**
 * \brief Write profile contents from submodule buffer to SD card
 * \param profile_id Profile number, 1 to MAX_PROFILES
 * \param pin_p Pointer to PIN buffer
 * \param pin_n Size of PIN buffer
 * \param priv_p Pointer to privilege buffer
 * \param priv_n Size of privilege buffer
 * \param fp_p Pointer to fingerprint buffer
 * \param fp_n Size of fingerprint buffer
 * \retval See vfy_pass for description of all possible return values
 */
esp_err_t SD_writeProfile(int profile_id, uint8_t *pin_p, int pin_n,
    uint8_t *priv_p, int priv_n, uint8_t *fp_p, int fp_n);

/**
 * \brief Delete profile contents in SD card
 * \param profile_id Profile number, 1 to MAX_PROFILES
 * \retval See vfy_pass for description of all possible return values
 */
esp_err_t SD_deleteProfile(int profile_id);

#endif /* SD_INTERFACE_H_ */