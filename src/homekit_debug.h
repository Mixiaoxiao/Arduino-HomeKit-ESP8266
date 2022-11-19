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

#define HOMEKIT_LOG_LEVEL HOMEKIT_NO_LOG

#define HOMEKIT_PRINTF XPGM_PRINTF


#if HOMEKIT_LOG_LEVEL >= HOMEKIT_LOG_INFO

char *binary_to_string(const byte *data, size_t size);
void print_binary(const char *prompt, const byte *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif // __HOMEKIT_DEBUG_H__
