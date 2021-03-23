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

/**
 * \brief Initialize SD interface, must call first
 */
void SD_init();


#endif /* SD_INTERFACE_H_ */