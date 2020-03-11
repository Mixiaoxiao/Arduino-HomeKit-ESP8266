// Base on esp_hw_wdt in github
/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 * Changed:
 * 2017 Alexander Epstine (a@epstine.com)
 */

#include "watchdog.h"
#include "Arduino.h"
#include "stdint.h"
#include "user_interface.h"
#include "homekit_debug.h"

#define REG_WDT_BASE 0x60000900

#define WDT_CTL (REG_WDT_BASE + 0x0)
#define WDT_CTL_ENABLE (BIT(0))
#define WDT_RESET (REG_WDT_BASE + 0x14)
#define WDT_RESET_VALUE 0x73

void esp_hw_wdt_enable() {
	SET_PERI_REG_MASK(WDT_CTL, WDT_CTL_ENABLE);
}

void esp_hw_wdt_disable() {
	CLEAR_PERI_REG_MASK(WDT_CTL, WDT_CTL_ENABLE);
}

void esp_hw_wdt_feed() {
	WRITE_PERI_REG(WDT_RESET, WDT_RESET_VALUE);
}

void watchdog_disable_all() {
	system_soft_wdt_stop();
	esp_hw_wdt_disable();
}

void watchdog_enable_all() {
	esp_hw_wdt_enable();
	system_soft_wdt_restart();
}

#ifdef HOMEKIT_DEBUG

static uint32_t wdt_checkpoint;

void watchdog_check_begin() {
	wdt_checkpoint = millis();
}

void watchdog_check_end(const char *message) {
	uint32_t d = millis() - wdt_checkpoint;
	printf("[WatchDog] Function %s took: %dms\n", message, d);
}

#else

void watchdog_check_begin() {
}
void watchdog_check_end(const char *message) {
}

#endif
