#ifndef __HOMEKIT_DEBUG_H__
#define __HOMEKIT_DEBUG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include "Arduino.h"

typedef unsigned char byte;

//#define HOMEKIT_DEBUG

#ifdef HOMEKIT_DEBUG

#define DEUBG_NUM(num) //printf("%d =============== %s\n", num, __func__);
#define DEBUG(message, ...) printf(">>> %s: " message "\n", __func__, ##__VA_ARGS__)

#else


#define DEBUG(message, ...)

#endif

#define INFO(message, ...) printf(">>> [%7d] HomeKit: " message "\n", millis(), ##__VA_ARGS__)
#define ERROR(message, ...) printf("!!! [%7d] HomeKit: " message "\n", millis(), ##__VA_ARGS__)

static uint32_t start_time = 0;
#define DEBUG_TIME_BEGIN()  start_time=millis();
//%-28s
#define DEBUG_TIME_END(func_name)  printf("### [%7d] %s took %6dms\n", millis(), func_name, (millis() - start_time));


//#ifdef ESP_IDF

//#define DEBUG_HEAP() DEBUG("Free heap: %d", esp_get_free_heap_size());
#define DEBUG_HEAP() DEBUG("Free heap: %d", system_get_free_heap_size());
#define INFO_HEAP() INFO("Free heap: %d", system_get_free_heap_size());

#define DEUBG_NUM(num) //{printf("%d =============== %s\n", num, __func__); INFO_HEAP()};

//#else

//#define DEBUG_HEAP() DEBUG("Free heap: %d", xPortGetFreeHeapSize());

//#endif

char *binary_to_string(const byte *data, size_t size);
void print_binary(const char *prompt, const byte *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif // __HOMEKIT_DEBUG_H__
