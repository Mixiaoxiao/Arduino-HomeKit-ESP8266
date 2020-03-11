#ifndef __HOMEKIT_DEBUG_H__
#define __HOMEKIT_DEBUG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include "Arduino.h"
#include <string.h>
#include <esp_xpgm.h>

typedef unsigned char byte;

//#define HOMEKIT_DEBUG

#define HOMEKIT_PRINTF XPGM_PRINTF

#ifdef HOMEKIT_DEBUG

#define DEBUG(message, ...)  HOMEKIT_PRINTF(">>> %s: " message "\n", __func__, ##__VA_ARGS__)
static uint32_t start_time = 0;
#define DEBUG_TIME_BEGIN()  start_time=millis();
#define DEBUG_TIME_END(func_name)  HOMEKIT_PRINTF("### [%7d] %s took %6dms\n", millis(), func_name, (millis() - start_time));

#else

#define DEBUG(message, ...)
#define DEBUG_TIME_BEGIN()
#define DEBUG_TIME_END(func_name)

#endif

#define INFO(message, ...) HOMEKIT_PRINTF(">>> [%7d] HomeKit: " message "\n", millis(), ##__VA_ARGS__)
#define ERROR(message, ...) HOMEKIT_PRINTF("!!! [%7d] HomeKit: " message "\n", millis(), ##__VA_ARGS__)
#define INFO_HEAP() INFO("Free heap: %d", system_get_free_heap_size());
#define DEBUG_HEAP() DEBUG("Free heap: %d", system_get_free_heap_size());

char *binary_to_string(const byte *data, size_t size);
void print_binary(const char *prompt, const byte *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif // __HOMEKIT_DEBUG_H__
