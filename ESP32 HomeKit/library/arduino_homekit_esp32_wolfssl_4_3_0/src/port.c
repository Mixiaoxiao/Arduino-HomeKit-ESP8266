#include <stdarg.h>
#include "port.h"

#ifdef ESP_OPEN_RTOS

#include <string.h>
#include <esp/hwrand.h>
#include <espressif/esp_common.h>
#include <esplibs/libmain.h>
#include "mdnsresponder.h"

#ifndef MDNS_TTL
#define MDNS_TTL 4500
#endif

uint32_t homekit_random() {
    return hwrand();
}

void homekit_random_fill(uint8_t *data, size_t size) {
    hwrand_fill(data, size);
}

void homekit_system_restart() {
    sdk_system_restart();
}

void homekit_overclock_start() {
    sdk_system_overclock();
}

void homekit_overclock_end() {
    sdk_system_restoreclock();
}

static char mdns_instance_name[65] = {0};
static char mdns_txt_rec[128] = {0};
static int mdns_port = 80;

void homekit_mdns_init() {
    mdns_init();
}

void homekit_mdns_configure_init(const char *instance_name, int port) {
    strncpy(mdns_instance_name, instance_name, sizeof(mdns_instance_name));
    mdns_txt_rec[0] = 0;
    mdns_port = port;
}

void homekit_mdns_add_txt(const char *key, const char *format, ...) {
    va_list arg_ptr;
    va_start(arg_ptr, format);

    char value[128];
    int value_len = vsnprintf(value, sizeof(value), format, arg_ptr);

    va_end(arg_ptr);

    if (value_len && value_len < sizeof(value)-1) {
        char buffer[128];
        int buffer_len = snprintf(buffer, sizeof(buffer), "%s=%s", key, value);

        if (buffer_len < sizeof(buffer)-1)
            mdns_TXT_append(mdns_txt_rec, sizeof(mdns_txt_rec), buffer, buffer_len);
    }
}

void homekit_mdns_configure_finalize() {
    mdns_clear();
    mdns_add_facility(mdns_instance_name, "_hap", mdns_txt_rec, mdns_TCP, mdns_port, MDNS_TTL);

    printf("mDNS announcement: Name=%s %s Port=%d TTL=%d\n",
           mdns_instance_name, mdns_txt_rec, mdns_port, MDNS_TTL);
}

#endif

//#ifdef ESP_IDF

#ifdef ARDUINO_ARCH_ESP8266

#include <string.h>
#include <stdint.h>

uint32_t homekit_random() {
	return os_random();
//    return esp_random();
}

void homekit_random_fill(uint8_t *data, size_t size) {
//    uint32_t x;
//    for (int i=0; i<size; i+=sizeof(x)) {
//        x = rand();//esp_random();
//        memcpy(data+i, &x, (size-i >= sizeof(x)) ? sizeof(x) : size-i);
//    }
	os_get_random(data, size);
}

void homekit_system_restart() {
	system_restart();
}

void homekit_overclock_start() {
	//ets_update_cpu_frequency(ticks_per_us);
}

void homekit_overclock_end() {
	//ets_update_cpu_frequency(ticks_per_us);
}
/*
void homekit_mdns_init() {
    mdns_init();
}

void homekit_mdns_configure_init(const char *instance_name, int port) {
    mdns_hostname_set(instance_name);
    mdns_instance_name_set(instance_name);
    mdns_service_add(instance_name, "_hap", "_tcp", port, NULL, 0);
}

void homekit_mdns_add_txt(const char *key, const char *format, ...) {
    va_list arg_ptr;
    va_start(arg_ptr, format);

    char value[128];
    int value_len = vsnprintf(value, sizeof(value), format, arg_ptr);

    va_end(arg_ptr);

    if (value_len && value_len < sizeof(value)-1) {
        mdns_service_txt_item_set("_hap", "_tcp", key, value);
    }
}

void homekit_mdns_configure_finalize() {
    printf("mDNS announcement: Name=%s %s Port=%d TTL=%d\n",
           name->value.string_value, txt_rec, PORT, 0);

//}*/

#endif


#ifdef ARDUINO_ARCH_ESP32

#include <string.h>
#include <stdint.h>
#include <esp_system.h>

uint32_t homekit_random() {
    return esp_random();
}

void homekit_random_fill(uint8_t *data, size_t size) {
//    uint32_t x;
//    for (int i=0; i<size; i+=sizeof(x)) {
//        x = rand();//esp_random();
//        memcpy(data+i, &x, (size-i >= sizeof(x)) ? sizeof(x) : size-i);
//    }
	esp_fill_random(data, size);
}

void homekit_system_restart() {
	esp_restart();
}

void homekit_overclock_start() {
	//ets_update_cpu_frequency(ticks_per_us);
}

void homekit_overclock_end() {
	//ets_update_cpu_frequency(ticks_per_us);
}

static uint8_t *_data = NULL;
static nvs_handle _handle;

bool spiflash_init() {
	if (_data) {
		ERROR("Storage: should not call spiflash_init twice!!!");
		return true;
	}
	_data = (uint8_t*) malloc(HOMEKIT_STORAGE_SIZE);
	if (!_data) {
		ERROR("Storage: Not enough memory for %d bytes", HOMEKIT_STORAGE_SIZE);
		return false;
	}
	memset(_data, 0xFF, HOMEKIT_STORAGE_SIZE);
	//https://github.com/espressif/esp-idf/blob/2e14149bff63fe211d6d30ff0cd84c90b5fface9/examples/storage/nvs_rw_blob/main/nvs_blob_example_main.c
//	esp_err_t err = nvs_flash_init();
//	    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//	        // NVS partition was truncated and needs to be erased
//	        // Retry nvs_flash_init
//	        ESP_ERROR_CHECK(nvs_flash_erase());
//	        err = nvs_flash_init();
//	    }
//	    ESP_ERROR_CHECK( err );

	const char *_name = HOMEKIT_STORAGE_NVS_NAME;
	esp_err_t res = nvs_open(_name, NVS_READWRITE, &_handle);
	if (res != ESP_OK) {
		ERROR("Unable to open NVS namespace: %d", res);
		return false;
	}
	size_t key_size = 0;
	res = nvs_get_blob(_handle, _name, NULL, &key_size);
	if (res != ESP_OK && res != ESP_ERR_NVS_NOT_FOUND) {
		ERROR("Unable to read NVS key: %d", res);
		return false;
	}
	if (key_size == 0) {
		// New blob
		nvs_set_blob(_handle, _name, _data, HOMEKIT_STORAGE_SIZE);
		INFO("Storage set new blob, name: %s, size: %u", _name, HOMEKIT_STORAGE_SIZE);
	} else {
		if (key_size != HOMEKIT_STORAGE_SIZE) {
			ERROR("Storage error, NVS size is not equal the defined size!!");
			return false;
		} else {
			nvs_get_blob(_handle, _name, _data, &key_size);
		}
	}
	return true;
}

bool spiflash_erase_sector(size_t sector) {
	memset(_data, 0xFF, HOMEKIT_STORAGE_SIZE);
	return true;
}

bool spiflash_write(size_t dest_addr, const void *src, size_t size) {
	if (dest_addr < 0 || dest_addr + size > HOMEKIT_STORAGE_SIZE) {
		return false;
	}
	memcpy(_data + dest_addr, (const void*) src, size);
	return ESP_OK == nvs_set_blob(_handle, HOMEKIT_STORAGE_NVS_NAME, _data, HOMEKIT_STORAGE_SIZE);
}

bool spiflash_read(size_t src_addr, void *dest, size_t size) {
	if (src_addr < 0 || src_addr + size > HOMEKIT_STORAGE_SIZE) {
		return false;
	}
	memcpy(dest, _data + src_addr, size);
	return true;
}

/*
void homekit_mdns_init() {
    mdns_init();
}

void homekit_mdns_configure_init(const char *instance_name, int port) {
    mdns_hostname_set(instance_name);
    mdns_instance_name_set(instance_name);
    mdns_service_add(instance_name, "_hap", "_tcp", port, NULL, 0);
}

void homekit_mdns_add_txt(const char *key, const char *format, ...) {
    va_list arg_ptr;
    va_start(arg_ptr, format);

    char value[128];
    int value_len = vsnprintf(value, sizeof(value), format, arg_ptr);

    va_end(arg_ptr);

    if (value_len && value_len < sizeof(value)-1) {
        mdns_service_txt_item_set("_hap", "_tcp", key, value);
    }
}

void homekit_mdns_configure_finalize() {
    printf("mDNS announcement: Name=%s %s Port=%d TTL=%d\n",
           name->value.string_value, txt_rec, PORT, 0);

//}*/
#endif
