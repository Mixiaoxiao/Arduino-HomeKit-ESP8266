/*
 * xpgm.h
 *
 *  Created on: 2020-03-08
 *      Author: Wang Bin
 */

#ifndef ESP_XPGM_H_
#define ESP_XPGM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <pgmspace.h>
#include <stdint.h>
#include <string.h>

//see "osapi.h"
/*
 This function works only after Serial.setDebugOutput(true); ?
 otherwise Serial prints nothing.
 #define esp_printf(fmt, ...) do {	\
	static const char flash_str[] ICACHE_RODATA_ATTR STORE_ATTR = fmt;	\
	os_printf_plus(flash_str, ##__VA_ARGS__);	\
	} while(0)*/

#define XPGM_PRINTF(fmt, ...)   printf_P(PSTR(fmt) , ##__VA_ARGS__);

#define XPGM_VAR(v0, v1) v0##v1

// pgm_ptr --> ram_ptr
#define XPGM_CPY(ram_ptr, pgm_ptr, size)  memcpy_P(ram_ptr, pgm_ptr, size)

#define XPGM_BUFFCPY(buff_type, buff_name, pgm_ptr, size)  size_t XPGM_VAR(buff_name, _size) = size; \
		buff_type buff_name[XPGM_VAR(buff_name, _size)]; \
		XPGM_CPY(buff_name, pgm_ptr, XPGM_VAR(buff_name, _size));

#define XPGM_BUFFCPY_ARRAY(buff_type, buff_name, pgm_array)  XPGM_BUFFCPY(buff_type, buff_name, pgm_array, sizeof(pgm_array))

#define XPGM_BUFFCPY_STRING(buff_type, buff_name, pgm_string) XPGM_BUFFCPY(buff_type, buff_name, pgm_string, strlen_P(pgm_string) + 1)

#ifdef __cplusplus
}
#endif

#endif /* ESP_XPGM_H_ */
