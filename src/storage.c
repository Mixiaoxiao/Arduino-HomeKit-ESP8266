#include <string.h>
#include <ctype.h>
#include "constants.h"
#include "pairing.h"
#include "port.h"

#include "storage.h"

#include "crypto.h"
#include "homekit_debug.h"

//#include "c_types.h"
//#include "ets_sys.h"
//#include "os_type.h"
//#include "osapi.h"
//#include "spi_flash.h"
//#include "spi_flash_geometry.h"
//#include "FS.h"
//#include "spiffs_api.h"

// ESP82666  spi_flash_geometry.h
// FLASH_SECTOR_SIZE 0x1000 = 4096B
// EEPROM use _sector = _EEPROM_start
// the "_EEPROM_start" value is provided in tools/sdk/ld/eagle.flash.**.ld

// We use the EEPROM address for this storage.c in Arduino environment
// EEPROM = one SPI_FLASH_SEC_SIZE = 0x1000 = 4096B
// SPI_FLASH_SEC_SIZE = 4096B
// sizeof(pairing_data_t) = 80B
// MAX_PAIRINGS = 16
// PAIRINGS_OFFSET = 128(address)
// so max use address of (128 + 80 * 16) = 1408 B

// This storage use [0, 1408) of EEPROM, leave [1408, 4096) for user to use safely.
// Leave the FS(file system) for user to use freely.

/*

See https://arduino-esp8266.readthedocs.io/en/2.6.3/filesystem.html
https://arduino-esp8266.readthedocs.io/en/2.6.3/libraries.html#eeprom

EEPROM library uses one sector of flash located just after the SPIFFS.

The following diagram illustrates flash layout used in Arduino environment:

|--------------|-------|---------------|--|--|--|--|--|
^              ^       ^               ^     ^
Sketch    OTA update   File system   EEPROM  WiFi config (SDK)

*/

#pragma GCC diagnostic ignored "-Wunused-value"

// These two values are provided in tools/sdk/ld/eagle.flash.**.ld
extern uint32_t _EEPROM_start; //See EEPROM.cpp
extern uint32_t _SPIFFS_start; //See spiffs_api.h

#define HOMEKIT_EEPROM_PHYS_ADDR ((uint32_t) (&_EEPROM_start) - 0x40200000)
#define HOMEKIT_SPIFFS_PHYS_ADDR ((uint32_t) (&_SPIFFS_start) - 0x40200000)

//#ifndef SPIFLASH_BASE_ADDR
#define STORAGE_BASE_ADDR HOMEKIT_EEPROM_PHYS_ADDR//0x200000
//#endif

#define MAGIC_OFFSET           0
#define ACCESSORY_ID_OFFSET    4
#define ACCESSORY_KEY_OFFSET   32
#define PAIRINGS_OFFSET        128

#define MAGIC_ADDR           (STORAGE_BASE_ADDR + MAGIC_OFFSET)
#define ACCESSORY_ID_ADDR    (STORAGE_BASE_ADDR + ACCESSORY_ID_OFFSET)
#define ACCESSORY_KEY_ADDR   (STORAGE_BASE_ADDR + ACCESSORY_KEY_OFFSET)
#define PAIRINGS_ADDR        (STORAGE_BASE_ADDR + PAIRINGS_OFFSET)

#define MAX_PAIRINGS 16

#define ACCESSORY_KEY_SIZE  64

#define STORAGE_DEBUG(message, ...) //printf("*** [Storage] %s: " message "\n", __func__, ##__VA_ARGS__)

const char magic1[] = "HAP";

// TODO: figure out alignment issues
typedef struct {
    char magic[sizeof(magic1)];
    byte permissions;
    char device_id[DEVICE_ID_SIZE];
    byte device_public_key[32];

    byte _reserved[7]; // align record to be 80 bytes
} pairing_data_t;


int homekit_storage_init() {

	STORAGE_DEBUG("EEPROM max: %d B", SPI_FLASH_SEC_SIZE);//4096B
	STORAGE_DEBUG("Pairing_data size: %d ", (sizeof(pairing_data_t)));//80B
	STORAGE_DEBUG("MAX pairing count: %d ", MAX_PAIRINGS);//16
	STORAGE_DEBUG("_EEPROM_start: 0x%x (%u)",
			HOMEKIT_EEPROM_PHYS_ADDR, HOMEKIT_EEPROM_PHYS_ADDR);
	STORAGE_DEBUG("_SPIFFS_start: 0x%x (%u)",
			HOMEKIT_SPIFFS_PHYS_ADDR, HOMEKIT_SPIFFS_PHYS_ADDR);

    char magic[sizeof(magic1)];
    memset(magic, 0, sizeof(magic));

    if (!spiflash_read(MAGIC_ADDR, (byte *)magic, sizeof(magic))) {
        ERROR("Failed to read HomeKit storage magic");
    }

    if (strncmp(magic, magic1, sizeof(magic1))) {
        INFO("Formatting HomeKit storage at 0x%x", STORAGE_BASE_ADDR);
        if (!spiflash_erase_sector(STORAGE_BASE_ADDR)) {
            ERROR("Failed to erase HomeKit storage");
            return -1;
        }

        strncpy(magic, magic1, sizeof(magic));
        if (!spiflash_write(MAGIC_ADDR, (byte *)magic, sizeof(magic))) {
            ERROR("Failed to write HomeKit storage magic");
            return -1;
        }

        return 1;
    }

    return 0;
}


int homekit_storage_reset() {
    byte blank[sizeof(magic1)];
    memset(blank, 0, sizeof(blank));

    if (!spiflash_write(MAGIC_ADDR, blank, sizeof(blank))) {
        ERROR("Failed to reset HomeKit storage");
        return -1;
    }

    return homekit_storage_init();
}


void homekit_storage_save_accessory_id(const char *accessory_id) {
    if (!spiflash_write(ACCESSORY_ID_ADDR, (byte *)accessory_id, ACCESSORY_ID_SIZE)) {
        ERROR("Failed to write accessory ID to HomeKit storage");
    }
}


static char ishex(unsigned char c) {
    c = toupper(c);
    return isdigit(c) || (c >= 'A' && c <= 'F');
}

int homekit_storage_load_accessory_id(char *data) {
    if (!spiflash_read(ACCESSORY_ID_ADDR, (byte *)data, ACCESSORY_ID_SIZE)) {
        ERROR("Failed to read accessory ID from HomeKit storage");
        return -1;
    }
    if (!data[0])
        return -2;
    data[ACCESSORY_ID_SIZE] = 0;

    for (int i=0; i<ACCESSORY_ID_SIZE; i++) {
        if (i % 3 == 2) {
           if (data[i] != ':')
               return -3;
        } else if (!ishex(data[i]))
            return -4;
    }

    return 0;
}

void homekit_storage_save_accessory_key(const ed25519_key *key) {
    byte key_data[ACCESSORY_KEY_SIZE];
    size_t key_data_size = sizeof(key_data);
    int r = crypto_ed25519_export_key(key, key_data, &key_data_size);
    if (r) {
        ERROR("Failed to export accessory key (code %d)", r);
        return;
    }

    if (!spiflash_write(ACCESSORY_KEY_ADDR, key_data, key_data_size)) {
        ERROR("Failed to write accessory key to HomeKit storage");
        return;
    }
}

int homekit_storage_load_accessory_key(ed25519_key *key) {
    byte key_data[ACCESSORY_KEY_SIZE];
    if (!spiflash_read(ACCESSORY_KEY_ADDR, key_data, sizeof(key_data))) {
        ERROR("Failed to read accessory key from HomeKit storage");
        return -1;
    }

    crypto_ed25519_init(key);
    int r = crypto_ed25519_import_key(key, key_data, sizeof(key_data));
    if (r) {
        ERROR("Failed to import accessory key (code %d)", r);
        return -2;
    }

    return 0;
}

bool homekit_storage_can_add_pairing() {
    pairing_data_t data;
    for (int i=0; i<MAX_PAIRINGS; i++) {
        spiflash_read(PAIRINGS_ADDR + sizeof(data)*i, (byte *)&data, sizeof(data));
        if (strncmp(data.magic, magic1, sizeof(magic1)))
            return true;
    }
    return false;
}

static int compact_data() {
    byte *data = malloc(SPI_FLASH_SECTOR_SIZE);
    if (!spiflash_read(STORAGE_BASE_ADDR, data, SPI_FLASH_SECTOR_SIZE)) {
        free(data);
        ERROR("Failed to compact HomeKit storage: sector data read error");
        return -1;
    }

    int next_pairing_idx = 0;
    for (int i=0; i<MAX_PAIRINGS; i++) {
        pairing_data_t *pairing_data = (pairing_data_t *)&data[PAIRINGS_OFFSET + sizeof(pairing_data_t)*i];
        if (!strncmp(pairing_data->magic, magic1, sizeof(magic1))) {
            if (i != next_pairing_idx) {
                memcpy(&data[PAIRINGS_ADDR + sizeof(pairing_data_t)*next_pairing_idx],
                       pairing_data, sizeof(*pairing_data));
            }
            next_pairing_idx++;
        }
    }

    if (next_pairing_idx == MAX_PAIRINGS) {
        // We are full, no compaction possible, do not waste flash erase cycle
        free(data);
        return 0;
    }

    if (homekit_storage_reset()) {
        ERROR("Failed to compact HomeKit storage: error resetting flash");
        free(data);
        return -1;
    }
    if (!spiflash_write(STORAGE_BASE_ADDR, data, PAIRINGS_OFFSET + sizeof(pairing_data_t)*next_pairing_idx)) {
        ERROR("Failed to compact HomeKit storage: error writing compacted data");
        free(data);
        return -1;
    }

    free(data);

    return 0;
}

static int find_empty_block() {
    byte data[sizeof(pairing_data_t)];

    for (int i=0; i<MAX_PAIRINGS; i++) {
        spiflash_read(PAIRINGS_ADDR + sizeof(data)*i, data, sizeof(data));

        bool block_empty = true;
        for (int j=0; j<sizeof(data); j++)
            if (data[j] != 0xff) {
                block_empty = false;
                break;
            }

        if (block_empty)
            return i;
    }

    return -1;
}

int homekit_storage_add_pairing(const char *device_id, const ed25519_key *device_key, byte permissions) {
    int next_block_idx = find_empty_block();
    if (next_block_idx == -1) {
        compact_data();
        next_block_idx = find_empty_block();
    }

    if (next_block_idx == -1) {
        ERROR("Failed to write pairing info to HomeKit storage: max number of pairings");
        return -2;
    }

    pairing_data_t data;

    memset(&data, 0, sizeof(data));
    strncpy(data.magic, magic1, sizeof(data.magic));
    data.permissions = permissions;
    strncpy(data.device_id, device_id, sizeof(data.device_id));
    size_t device_public_key_size = sizeof(data.device_public_key);
    int r = crypto_ed25519_export_public_key(
        device_key, data.device_public_key, &device_public_key_size
    );
    if (r) {
        ERROR("Failed to export device public key (code %d)", r);
        return -1;
    }

    if (!spiflash_write(PAIRINGS_ADDR + sizeof(data)*next_block_idx, (byte *)&data, sizeof(data))) {
        ERROR("Failed to write pairing info to HomeKit storage");
        return -1;
    }

    return 0;
}


int homekit_storage_update_pairing(const char *device_id, byte permissions) {
    pairing_data_t data;
    for (int i=0; i<MAX_PAIRINGS; i++) {
        spiflash_read(PAIRINGS_ADDR + sizeof(data)*i, (byte *)&data, sizeof(data));
        if (strncmp(data.magic, magic1, sizeof(data.magic)))
            continue;

        if (!strncmp(data.device_id, device_id, sizeof(data.device_id))) {
            int next_block_idx = find_empty_block();
            if (next_block_idx == -1) {
                compact_data();
                next_block_idx = find_empty_block();
            }

            if (next_block_idx == -1) {
                ERROR("Failed to write pairing info to HomeKit storage: max number of pairings");
                return -2;
            }

            data.permissions = permissions;

            if (!spiflash_write(PAIRINGS_ADDR + sizeof(data)*next_block_idx, (byte *)&data, sizeof(data))) {
                ERROR("Failed to write pairing info to HomeKit storage");
                return -1;
            }

            memset(&data, 0, sizeof(data));
            if (!spiflash_write(PAIRINGS_ADDR + sizeof(data)*i, (byte *)&data, sizeof(data))) {
                ERROR("Failed to update pairing: error erasing old record from HomeKit storage");
                return -2;
            }

            return 0;
        }
    }
    return -1;
}


int homekit_storage_remove_pairing(const char *device_id) {
    pairing_data_t data;
    for (int i=0; i<MAX_PAIRINGS; i++) {
        spiflash_read(PAIRINGS_ADDR + sizeof(data)*i, (byte *)&data, sizeof(data));
        if (strncmp(data.magic, magic1, sizeof(data.magic)))
            continue;

        if (!strncmp(data.device_id, device_id, sizeof(data.device_id))) {
            memset(&data, 0, sizeof(data));
            if (!spiflash_write(PAIRINGS_ADDR + sizeof(data)*i, (byte *)&data, sizeof(data))) {
                ERROR("Failed to remove pairing from HomeKit storage");
                return -2;
            }

            return 0;
        }
    }
    return 0;
}


int homekit_storage_find_pairing(const char *device_id, pairing_t *pairing) {
    pairing_data_t data;
    for (int i=0; i<MAX_PAIRINGS; i++) {
        spiflash_read(PAIRINGS_ADDR + sizeof(data)*i, (byte *)&data, sizeof(data));
        if (strncmp(data.magic, magic1, sizeof(data.magic)))
            continue;

        if (!strncmp(data.device_id, device_id, sizeof(data.device_id))) {
            crypto_ed25519_init(&pairing->device_key);
            int r = crypto_ed25519_import_public_key(&pairing->device_key, data.device_public_key, sizeof(data.device_public_key));
            if (r) {
                ERROR("Failed to import device public key (code %d)", r);
                return -2;
            }

            pairing->id = i;
            strncpy(pairing->device_id, data.device_id, DEVICE_ID_SIZE);
            pairing->device_id[DEVICE_ID_SIZE] = 0;
            pairing->permissions = data.permissions;

            return 0;
        }
    }

    return -1;
}


void homekit_storage_pairing_iterator_init(pairing_iterator_t *it) {
    it->idx = 0;
}


void homekit_storage_pairing_iterator_done(pairing_iterator_t *iterator) {
}


int homekit_storage_next_pairing(pairing_iterator_t *it, pairing_t *pairing) {
    pairing_data_t data;
    while(it->idx < MAX_PAIRINGS) {
        int id = it->idx++;

        spiflash_read(PAIRINGS_ADDR + sizeof(data)*id, (byte *)&data, sizeof(data));
        if (!strncmp(data.magic, magic1, sizeof(data.magic))) {
            crypto_ed25519_init(&pairing->device_key);
            int r = crypto_ed25519_import_public_key(&pairing->device_key, data.device_public_key, sizeof(data.device_public_key));
            if (r) {
                ERROR("Failed to import device public key (code %d)", r);
                continue;
            }

            pairing->id = id;
            strncpy(pairing->device_id, data.device_id, DEVICE_ID_SIZE);
            pairing->device_id[DEVICE_ID_SIZE] = 0;
            pairing->permissions = data.permissions;

            return 0;
        }
    }

    return -1;
}


#undef HOMEKIT_EEPROM_PHYS_ADDR
#undef HOMEKIT_SPIFFS_PHYS_ADDR
