#ifndef __HOMEKIT_PORT_H_
#define __HOMEKIT_PORT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "homekit_debug.h"

uint32_t homekit_random();
void homekit_random_fill(uint8_t *data, size_t size);

void homekit_system_restart();
void homekit_overclock_start();
void homekit_overclock_end();

#ifdef ESP_OPEN_RTOS
#include <spiflash.h>
#define ESP_OK 0
#endif

//#ifdef ESP_IDF
//#include <esp_system.h>
//#include <esp_spi_flash.h>

#if defined(ARDUINO_ARCH_ESP8266)
#include "Arduino.h"
#include <spi_flash.h>
#include <ets_sys.h>
#include <osapi.h>
#include <user_interface.h>
#define ESP_OK SPI_FLASH_RESULT_OK //0

#define SPI_FLASH_SECTOR_SIZE SPI_FLASH_SEC_SIZE
#define spiflash_read(addr, buffer, size) (spi_flash_read((addr), (buffer), (size)) == ESP_OK)
#define spiflash_write(addr, data, size) (spi_flash_write((addr), (data), (size)) == ESP_OK)
#define spiflash_erase_sector(addr) (spi_flash_erase_sector((addr) / SPI_FLASH_SECTOR_SIZE) == ESP_OK)

#endif

//#ifdef ESP_IDF
#ifdef ARDUINO_ARCH_ESP32
#define SERVER_TASK_STACK 10240//12288// USE_FAST_MATH requires a lot of memory!!!
#else
#define SERVER_TASK_STACK 2048
#endif

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_spi_flash.h>
#include <nvs.h>
#define SPI_FLASH_SECTOR_SIZE SPI_FLASH_SEC_SIZE

// Use nvs of ESP32, See EEPROM.cpp for example

// 1536 is enough, but require 4096 in compact_data() (for compatibility)
#define HOMEKIT_STORAGE_SIZE     SPI_FLASH_SECTOR_SIZE
#define HOMEKIT_STORAGE_NVS_NAME "homekit"

bool spiflash_init();
bool spiflash_erase_sector(size_t sector);
bool spiflash_write(size_t dest_addr, const void *src, size_t size);
bool spiflash_read(size_t src_addr, void *dest, size_t size);

#endif

//void homekit_mdns_init();
//void homekit_mdns_configure_init(const char *instance_name, int port);
//void homekit_mdns_add_txt(const char *key, const char *format, ...);
//void homekit_mdns_configure_finalize();

#ifdef __cplusplus
}
#endif

#endif /* __HOMEKIT_PORT_H_ */
