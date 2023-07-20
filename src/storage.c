#include <string.h>
#include <ctype.h>
#include "constants.h"
#include "pairing.h"
#include "port.h"

#include "storage.h"

#include "crypto.h"
#include "homekit_debug.h"

/*
[...]
See https://arduino-esp8266.readthedocs.io/en/2.6.3/filesystem.html
https://arduino-esp8266.readthedocs.io/en/2.6.3/libraries.html#eeprom

EEPROM library uses one sector of flash located just after the SPIFFS.

The following diagram illustrates flash layout used in Arduino environment:

|--------------|-------|---------------|--|--|--|--|--|
^              ^       ^               ^     ^
Sketch    OTA update   File system   EEPROM  WiFi config (SDK)
[...]

We use the EEPROM region os the flash. however, we can only write once to erased sections.
Alignment needs to be a factor of 4 and blocks written cannot be smaller than 4 bytes else
the ESP has a fit.

ref: https://docs.ai-thinker.com/_media/esp8266/docs/espressif_iot_flash_rw_operation_en.pdf
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

#define MAX_PAIRINGS           16
#define ACCESSORY_KEY_SIZE     64

#ifdef HOMEKIT_DEBUG
#define STORAGE_DEBUG(message, ...) printf("*** [Storage] %s: " message "\n", __func__, ##__VA_ARGS__)
#else
#define STORAGE_DEBUG(message, ...)
#endif

const char hap_magic[] = "HAP";

typedef struct {
    char magic[sizeof(hap_magic)];
    byte permissions;
    char device_id[DEVICE_ID_SIZE];
    byte device_public_key[32];

    byte _reserved[6]; // align record to be 80 bytes
    byte active;
} pairing_data_t;

bool homekit_storage_magic_valid() {
    char magic_test[sizeof(hap_magic)];
    bzero(magic_test, sizeof(magic_test));

    if (!spiflash_read(MAGIC_ADDR, (uint32_t *) magic_test, sizeof(magic_test))) {
        ERROR("Failed to read HomeKit storage magic");
        return false;
    }
    return (memcmp(magic_test, hap_magic, sizeof(hap_magic)) == 0);
}

bool homekit_storage_set_magic() {
    char magic[sizeof(hap_magic)];
    memcpy(magic, hap_magic, sizeof(magic));

    if (!spiflash_write(MAGIC_ADDR, (uint32_t *) magic, sizeof(magic))) {
        ERROR("Failed to write HomeKit storage magic");
        return false;
    }
    return true;
}

#ifdef HOMEKIT_DEBUG
static char buf[128];
static void dump_pairing_data(int index, pairing_data_t *data) {
    if (memcmp(data->magic, hap_magic, sizeof(hap_magic)) || !data->active) return;

    char *t = buf;
    t += sprintf(t, "device_id (%d): %s", index, data->permissions & pairing_permissions_admin ? "(admin) " : "");
    for (int i = 0; i < sizeof(data->device_id); i++) {
        t += sprintf(t, "%c", data->device_id[i] > 31 && data->device_id[i] < 128 ? data->device_id[i] : '.');
    }

    INFO("%s", buf);
}
static void _dump_storage(byte *__buf)
{
    char *t = buf;
    t += sprintf(t, "dump:");
    for (int i = 0; i < (PAIRINGS_OFFSET + (MAX_PAIRINGS * sizeof(pairing_data_t))); i++) {
        if (i % 32 == 0) {
            INFO("%s", buf);
            memset(buf, 0, sizeof(buf));
            t = buf;
        }
        if (i % 16 == 0 && i % 32 != 0) t += sprintf(t, "- ");
        t += sprintf(t, "%02x ", __buf[i]);
    }
    INFO("%s", buf);
}
void dump_storage()
{
    byte *__buf = malloc(PAIRINGS_OFFSET + (MAX_PAIRINGS * sizeof(pairing_data_t)));

    if (!spiflash_read(STORAGE_BASE_ADDR, (uint32_t *) __buf, PAIRINGS_OFFSET + (MAX_PAIRINGS * sizeof(pairing_data_t)))) {
        ERROR("Failed to read the storage sector");
        free(__buf);
        return;
    }

    _dump_storage(__buf);

    free(__buf);
}
#endif

int homekit_storage_init(int purge) {
    STORAGE_DEBUG("EEPROM max: %d B", SPI_FLASH_SEC_SIZE);//4096B
    STORAGE_DEBUG("Pairing_data size: %d ", (sizeof(pairing_data_t)));//80B
    STORAGE_DEBUG("MAX pairing count: %d ", MAX_PAIRINGS);//16
    STORAGE_DEBUG("_EEPROM_start: 0x%x (%u)", HOMEKIT_EEPROM_PHYS_ADDR, HOMEKIT_EEPROM_PHYS_ADDR);
    STORAGE_DEBUG("_SPIFFS_start: 0x%x (%u)", HOMEKIT_SPIFFS_PHYS_ADDR, HOMEKIT_SPIFFS_PHYS_ADDR);

    int res = 0; // Was valid
    if (!homekit_storage_magic_valid() || purge) {
        INFO("Formatting HomeKit storage at 0x%x", STORAGE_BASE_ADDR);
        if (!spiflash_erase_sector(STORAGE_BASE_ADDR) || !homekit_storage_set_magic()) {
            ERROR("Failed to erase HomeKit storage");
            return -1; // Fail case
        }
        res = 1; // Wasn't valid, is now
    }

#ifdef HOMEKIT_DEBUG
    pairing_data_t data;
    int paired = 0;
    for (int i = 0; i < MAX_PAIRINGS; i++) {
        spiflash_read(PAIRINGS_ADDR + (sizeof(data) * i), (uint32_t *) &data, sizeof(data));
        dump_pairing_data(i, &data);
        if (!memcmp(data.magic, hap_magic, sizeof(hap_magic)) && data.active) {
            paired++;
        }
    }
    STORAGE_DEBUG("%d paired devices, res: %d", paired, res);
#endif

    return res;
}

int homekit_storage_reset() {
    INFO("Formatting HomeKit storage sector");

    return homekit_storage_init(true);
}

int homekit_storage_reset_pairing_data() {
    INFO("Removing all pairings");

    pairing_data_t data;
    for (int i = 0; i < MAX_PAIRINGS; i++) {
        spiflash_read(PAIRINGS_ADDR + (sizeof(data) * i), (uint32_t *) &data, sizeof(data));
        if (!memcmp(data.magic, hap_magic, sizeof(data.magic)) && data.active) {
            memset(&data, 0, 4);
            if (!spiflash_write(PAIRINGS_ADDR + (sizeof(data) * i) + 76, (uint32_t *) &data, 4)) {
                ERROR("Failed to remove pairing from HomeKit storage");
                return -2;
            }

            return 0;
        }
    }

    return 0;
}

void homekit_storage_save_accessory_id(const char *accessory_id) {
    int size = ACCESSORY_ID_SIZE;
    if (size & 3)
        size += 4 - (size & 3);
    if (!spiflash_write(ACCESSORY_ID_ADDR, (uint32_t *) accessory_id, size)) {
        ERROR("Failed to write accessory ID to HomeKit storage");
    }
}

static char ishex(unsigned char c) {
    c = toupper(c);
    return isdigit(c) || (c >= 'A' && c <= 'F');
}

int homekit_storage_load_accessory_id(char *data) {
    int size = ACCESSORY_ID_SIZE;
    if (size & 3)
        size += 4 - (size & 3);
    char *d[size];
    if (!spiflash_read(ACCESSORY_ID_ADDR, (uint32_t *) d, size)) {
        ERROR("Failed to read accessory ID from HomeKit storage");
        return -1;
    }
    if (!d[0])
        return -2;
    memcpy(data, d, ACCESSORY_ID_SIZE);
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

    if (!spiflash_write(ACCESSORY_KEY_ADDR, (uint32_t *) key_data, key_data_size)) {
        ERROR("Failed to write accessory key to HomeKit storage");
        return;
    }
}

int homekit_storage_load_accessory_key(ed25519_key *key) {
    byte key_data[ACCESSORY_KEY_SIZE];
    if (!spiflash_read(ACCESSORY_KEY_ADDR, (uint32_t *) key_data, sizeof(key_data))) {
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

static int _find_empty_block() {
    pairing_data_t data;
    for (int i = 0; i < MAX_PAIRINGS; i++) {
        spiflash_read(PAIRINGS_ADDR + (sizeof(data) * i), (uint32_t *) &data, sizeof(data));
        if (memcmp(data.magic, hap_magic, sizeof(hap_magic)))
            return i;
    }
    return -1;
}

bool homekit_storage_can_add_pairing() {
    pairing_data_t data;
    for (int i = 0; i < MAX_PAIRINGS; i++) {
        spiflash_read(PAIRINGS_ADDR + (sizeof(data) * i), (uint32_t *) &data, sizeof(data));
        if (memcmp(data.magic, hap_magic, sizeof(hap_magic)) || !data.active)
            return true;
    }
    return false;
}

static int compact_data() {
    byte *storage = malloc(PAIRINGS_OFFSET + (MAX_PAIRINGS * sizeof(pairing_data_t)));
    if (!spiflash_read(STORAGE_BASE_ADDR, (uint32_t *) storage, PAIRINGS_OFFSET + (MAX_PAIRINGS * sizeof(pairing_data_t)))) {
        ERROR("Failed to read storage sector");
        free(storage);
        return -3;
    }

    int next_pairing_idx = 0;
    for (int i = 0; i < MAX_PAIRINGS; i++) {
        pairing_data_t *src = (pairing_data_t *) &storage[PAIRINGS_OFFSET + (i * sizeof(pairing_data_t))];
        if (!memcmp(src->magic, hap_magic, sizeof(hap_magic)) && src->active) {
            if (i != next_pairing_idx) {
                pairing_data_t *dest = (pairing_data_t *) &storage[PAIRINGS_OFFSET + (next_pairing_idx * sizeof(pairing_data_t))];
                memcpy(dest, src, sizeof(pairing_data_t));
            }
            next_pairing_idx++;
        }
    }

    if (next_pairing_idx == MAX_PAIRINGS) {
        // We are full, no compaction possible, do not waste flash erase cycle
        free(storage);
        return 0;
    }

    if (!spiflash_erase_sector(STORAGE_BASE_ADDR)) {
        ERROR("Failed to erase storage sector");
        free(storage);
        return -2;
    }
    if (!spiflash_write(STORAGE_BASE_ADDR, (uint32_t *) storage, PAIRINGS_OFFSET)) {
        ERROR("Failed to compact HomeKit storage: error writing compacted storage header");
        free(storage);
        return -1;
    }
    for (int i = 0; i < next_pairing_idx; i++) {
        int offset = PAIRINGS_OFFSET + (i * sizeof(pairing_data_t));
        pairing_data_t *data = (pairing_data_t *) &storage[offset];
        if (!memcmp(data->magic, hap_magic, sizeof(hap_magic)) && data->active) {
            if (!spiflash_write(STORAGE_BASE_ADDR + offset, (uint32_t *) &storage[offset], sizeof(pairing_data_t) - 4)) {
                ERROR("Failed to compact HomeKit storage: error writing compacted storage");
                free(storage);
                return -1;
            }
        } else break;
    }

    free(storage);

    return 0;
}

static int find_empty_block() {
    int index = _find_empty_block();
    if (index == -1) {
        compact_data();
        index = _find_empty_block();
    }
    INFO("empty block %d", index);
    return index;
}

int homekit_storage_add_pairing(const char *device_id, const ed25519_key *device_key, byte permissions) {
    int next_block_idx = find_empty_block();
    if (next_block_idx == -1) {
        ERROR("Failed to write pairing info to HomeKit storage: max number of pairings");
        return -2;
    }

    pairing_data_t data;
    memset(&data, 0xff, sizeof(data));
    memcpy(data.magic, hap_magic, sizeof(data.magic));
    memcpy(data.device_id, device_id, sizeof(data.device_id));
    size_t device_public_key_size = sizeof(data.device_public_key);
    int r = crypto_ed25519_export_public_key(
        device_key, data.device_public_key, &device_public_key_size
    );
    if (r) {
        ERROR("Failed to export device public key (code %d)", r);
        return -1;
    }
    data.permissions = permissions;

    if (!spiflash_write(PAIRINGS_ADDR + (sizeof(data) * next_block_idx), (uint32_t *) &data, sizeof(data) - 4)) { // last 4 bytes is the status, ff = active, 00 = deleted
        ERROR("Failed to write pairing info to HomeKit storage");
        return -1;
    }

    return 0;
}

int homekit_storage_update_pairing(const char *device_id, byte permissions) {
    pairing_data_t data;
    for (int i = 0; i < MAX_PAIRINGS; i++) {
        spiflash_read(PAIRINGS_ADDR + (sizeof(data) * i), (uint32_t *) &data, sizeof(data));
        if (memcmp(data.magic, hap_magic, sizeof(data.magic)))
            continue;

        if (!memcmp(data.device_id, device_id, sizeof(data.device_id)) && data.active) {
            if (data.permissions == permissions) {
                INFO("permissions have not changed?");
                return 0;
            }

            int next_block_idx = find_empty_block();
            if (next_block_idx == -1) {
                ERROR("Failed to write pairing info to HomeKit storage: max number of pairings");
                return -2;
            }

            data.permissions = permissions;

            if (!spiflash_write(PAIRINGS_ADDR + (sizeof(data) * next_block_idx), (uint32_t *) &data, sizeof(data))) {
                ERROR("Failed to write pairing info to HomeKit storage");
                return -1;
            }

            memset(&data, 0, sizeof(data));
            if (!spiflash_write(PAIRINGS_ADDR + (sizeof(data) * i) + 76, (uint32_t *) &data, 4)) {
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
    for (int i = 0; i < MAX_PAIRINGS; i++) {
        spiflash_read(PAIRINGS_ADDR + (sizeof(data) * i), (uint32_t *) &data, sizeof(data));
        if (memcmp(data.magic, hap_magic, sizeof(data.magic)))
            continue;

        if (!memcmp(data.device_id, device_id, sizeof(data.device_id)) && data.active) {
            memset(&data, 0, sizeof(data));
            if (!spiflash_write(PAIRINGS_ADDR + (sizeof(data) * i) + 76, (uint32_t *) &data, 4)) {
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
    for (int i = 0; i < MAX_PAIRINGS; i++) {
        spiflash_read(PAIRINGS_ADDR + (sizeof(data) * i), (uint32_t *) &data, sizeof(data));
        if (memcmp(data.magic, hap_magic, sizeof(data.magic)))
            continue;

        if (!memcmp(data.device_id, device_id, sizeof(data.device_id)) && data.active) {
            crypto_ed25519_init(&pairing->device_key);
            int r = crypto_ed25519_import_public_key(&pairing->device_key, data.device_public_key, sizeof(data.device_public_key));
            if (r) {
                ERROR("Failed to import device public key (code %d)", r);
                return -2;
            }

            pairing->id = i;
            memcpy(pairing->device_id, data.device_id, DEVICE_ID_SIZE);
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

        spiflash_read(PAIRINGS_ADDR + (sizeof(data) * id), (uint32_t *) &data, sizeof(data));
        if (!memcmp(data.magic, hap_magic, sizeof(data.magic)) && data.active) {
            crypto_ed25519_init(&pairing->device_key);
            int r = crypto_ed25519_import_public_key(&pairing->device_key, data.device_public_key, sizeof(data.device_public_key));
            if (r) {
                ERROR("Failed to import device public key (code %d)", r);
                continue;
            }

            pairing->id = id;
            memcpy(pairing->device_id, data.device_id, DEVICE_ID_SIZE);
            pairing->device_id[DEVICE_ID_SIZE] = 0;
            pairing->permissions = data.permissions;

            return 0;
        }
    }

    return -1;
}

#undef HOMEKIT_EEPROM_PHYS_ADDR
#undef HOMEKIT_SPIFFS_PHYS_ADDR
