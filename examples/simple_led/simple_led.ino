/*
 * simple_led.ino
 *
 * This accessory contains a builtin-led on NodeMCU and a "virtual" occupancy sensor.
 * Setup code: 111-11-111
 * The Flash-Button(D3, GPIO0) on NodeMCU:
 * 		single-click: turn on/off the builtin-led (D4, GPIO2)
 * 		double-click: toggle the occupancy sensor state
 * 		long-click: reset the homekit server (remove the saved pairing)
 *
 *  Created on: 2020-02-08
 *      Author: Mixiaoxiao (Wang Bin)
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>

#include <arduino_homekit_server.h>
#include "ButtonDebounce.h"
#include "ButtonHandler.h"

//D0 16 //led
//D3  0 //flash button
//D4  2 //led

#define PIN_LED 16//D0

const char *ssid = "your-ssid";
const char *password = "your-password";

#define SIMPLE_INFO(fmt, ...)   printf_P(PSTR(fmt "\n") , ##__VA_ARGS__);

void blink_led(int interval, int count) {
	for (int i = 0; i < count; i++) {
		builtinledSetStatus(true);
		delay(interval);
		builtinledSetStatus(false);
		delay(interval);
	}
}

void setup() {
	Serial.begin(115200);
	Serial.setRxBufferSize(32);
	Serial.setDebugOutput(false);

	pinMode(PIN_LED, OUTPUT);
	WiFi.mode(WIFI_STA);
	WiFi.persistent(false);
	WiFi.disconnect(false);
	WiFi.setAutoReconnect(true);
	WiFi.begin(ssid, password);

	SIMPLE_INFO("");
	SIMPLE_INFO("SketchSize: %d", ESP.getSketchSize());
	SIMPLE_INFO("FreeSketchSpace: %d", ESP.getFreeSketchSpace());
	SIMPLE_INFO("FlashChipSize: %d", ESP.getFlashChipSize());
	SIMPLE_INFO("FlashChipRealSize: %d", ESP.getFlashChipRealSize());
	SIMPLE_INFO("FlashChipSpeed: %d", ESP.getFlashChipSpeed());
	SIMPLE_INFO("SdkVersion: %s", ESP.getSdkVersion());
	SIMPLE_INFO("FullVersion: %s", ESP.getFullVersion().c_str());
	SIMPLE_INFO("CpuFreq: %dMHz", ESP.getCpuFreqMHz());
	SIMPLE_INFO("FreeHeap: %d", ESP.getFreeHeap());
	SIMPLE_INFO("ResetInfo: %s", ESP.getResetInfo().c_str());
	SIMPLE_INFO("ResetReason: %s", ESP.getResetReason().c_str());
	INFO_HEAP();
	homekit_setup();
	INFO_HEAP();
	blink_led(200, 3);
}

void loop() {
	homekit_loop();
}

void builtinledSetStatus(bool on) {
	digitalWrite(PIN_LED, on ? LOW : HIGH);
}

//==============================
// Homekit setup and loop
//==============================

extern "C" homekit_server_config_t config;
extern "C" homekit_characteristic_t name;
extern "C" void occupancy_toggle();
extern "C" void led_toggle();
extern "C" void accessory_init();

ButtonDebounce btn(0, INPUT_PULLUP, LOW);
ButtonHandler btnHandler;

void IRAM_ATTR btnInterrupt() {
	btn.update();
}

void homekit_setup() {
	accessory_init();
	uint8_t mac[WL_MAC_ADDR_LENGTH];
	WiFi.macAddress(mac);
	int name_len = snprintf(NULL, 0, "%s_%02X%02X%02X",
			name.value.string_value, mac[3], mac[4], mac[5]);
	char *name_value = (char*) malloc(name_len + 1);
	snprintf(name_value, name_len + 1, "%s_%02X%02X%02X",
			name.value.string_value, mac[3], mac[4], mac[5]);
	name.value = HOMEKIT_STRING_CPP(name_value);

	arduino_homekit_setup(&config);

	btn.setCallback(std::bind(&ButtonHandler::handleChange, &btnHandler,
			std::placeholders::_1));
	btn.setInterrupt(btnInterrupt);
	btnHandler.setIsDownFunction(std::bind(&ButtonDebounce::checkIsDown, &btn));
	btnHandler.setCallback([](button_event e) {
		if (e == BUTTON_EVENT_SINGLECLICK) {
			SIMPLE_INFO("Button Event: SINGLECLICK");
			led_toggle();
		} else if (e == BUTTON_EVENT_DOUBLECLICK) {
			SIMPLE_INFO("Button Event: DOUBLECLICK");
			occupancy_toggle();
		} else if (e == BUTTON_EVENT_LONGCLICK) {
			SIMPLE_INFO("Button Event: LONGCLICK");
			SIMPLE_INFO("Rebooting...");
			homekit_storage_reset();
			ESP.restart(); // or system_restart();
		}
	});
}

void homekit_loop() {
	btnHandler.loop();
	arduino_homekit_loop();
	static uint32_t next_heap_millis = 0;
	uint32_t time = millis();
	if (time > next_heap_millis) {
		SIMPLE_INFO("heap: %d, sockets: %d",
				ESP.getFreeHeap(), arduino_homekit_connected_clients_count());
		next_heap_millis = time + 5000;
	}
}
