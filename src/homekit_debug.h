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

#define HOMEKIT_NO_LOG 0
#define HOMEKIT_LOG_ERROR 1
#define HOMEKIT_LOG_INFO 2
#define HOMEKIT_LOG_DEBUG 3

#ifndef HOMEKIT_LOG_LEVEL
#define HOMEKIT_LOG_LEVEL HOMEKIT_LOG_INFO
#endif

#define HOMEKIT_PRINTF XPGM_PRINTF

#if HOMEKIT_LOG_LEVEL >= HOMEKIT_LOG_DEBUG

#define DEBUG(message, ...)  HOMEKIT_PRINTF(">>> %s: " message "\n", __func__, ##__VA_ARGS__)
static uint32_t start_time = 0;
#define DEBUG_TIME_BEGIN()  start_time=millis();
#define DEBUG_TIME_END(func_name)  HOMEKIT_PRINTF("### [%7d] %s took %6dms\n", millis(), func_name, (millis() - start_time));
#define DEBUG_HEAP() DEBUG("Free heap: %d", system_get_free_heap_size());

#else

#define DEBUG(message, ...)
#define DEBUG_TIME_BEGIN()
#define DEBUG_TIME_END(func_name)
#define DEBUG_HEAP()

#endif

#if HOMEKIT_LOG_LEVEL >= HOMEKIT_LOG_ERROR

#define ERROR(message, ...) HOMEKIT_PRINTF("!!! [%7d] HomeKit: " message "\n", millis(), ##__VA_ARGS__)

#else

#define ERROR(message, ...)

#endif

#if HOMEKIT_LOG_LEVEL >= HOMEKIT_LOG_INFO

#define INFO(message, ...) HOMEKIT_PRINTF(">>> [%7d] HomeKit: " message "\n", millis(), ##__VA_ARGS__)
#define INFO_HEAP() INFO("Free heap: %d", system_get_free_heap_size());

#else

#define INFO(message, ...)
#define INFO_HEAP()

#endif

char *binary_to_string(const byte *data, size_t size);
void print_binary(const char *prompt, const byte *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif // __HOMEKIT_DEBUG_H__
