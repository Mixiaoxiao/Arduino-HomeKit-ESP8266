/*
 * stateless_programmable_switch.ino
 *
 *  Created on: 2020-05-16
 *      Author: Mixiaoxiao (Wang Bin)
 *
 *
 * HAP section 8.37 Stateless Programmable Switch
 * An accessory contains a stateless programmable switch, also called a button.
 * Used to report Programmable Switch Event to HomeKit.
 *
 * Programmable Switch Event: (HAP section 9.75)
 * 0 "Single Press"
 * 1 "Double Press"
 * 2 "Long Press"
 *
 * Note:
 * Reading this "Programmable Switch Event" should always return "null".
 * The value must only be reported in the events ("ev") property.
 *
 * You should:
 * 1. read and use the Example01_TemperatureSensor with detailed comments
 *    to know the basic concept and usage of this library before other examples。
 * 2. erase the full flash or call homekit_storage_reset() in setup()
 *    to remove the previous HomeKit pairing storage and
 *    enable the pairing with the new accessory of this new HomeKit example.
 */

#include <Arduino.h>
#include <arduino_homekit_server.h>
#include "wifi_info.h"
#include "ESPButton.h"

#define LOG_D(fmt, ...)   printf_P(PSTR(fmt "\n") , ##__VA_ARGS__);

void setup() {
	Serial.begin(115200);
	wifi_connect(); // in wifi_info.h
	//homekit_storage_reset(); // to remove the previous HomeKit pairing storage when you first run this new HomeKit example
	my_homekit_setup();
}

void loop() {
	my_homekit_loop();
	delay(10);
}

//==============================
// HomeKit setup and loop
//==============================

extern "C" homekit_server_config_t config;
extern "C" homekit_characteristic_t cha_programmable_switch_event;

#define PIN_BUTTON 0 // Use the Flash-Button of NodeMCU

#define HOMEKIT_PROGRAMMABLE_SWITCH_EVENT_SINGLE_PRESS   0
#define HOMEKIT_PROGRAMMABLE_SWITCH_EVENT_DOUBLE_PRESS   1
#define HOMEKIT_PROGRAMMABLE_SWITCH_EVENT_LONG_PRESS     2

// Called when the value is read by iOS Home APP
homekit_value_t cha_programmable_switch_event_getter() {
	// Should always return "null" for reading, see HAP section 9.75
	return HOMEKIT_NULL_CPP();
}

void my_homekit_setup() {
	pinMode(PIN_BUTTON, INPUT_PULLUP);
	ESPButton.add(0, PIN_BUTTON, LOW, true, true);
	ESPButton.setCallback([&](uint8_t id, ESPButtonEvent event) {
		// Only one button is added, no need to check the id.
		LOG_D("Button Event: %s", ESPButton.getButtonEventDescription(event));
		uint8_t cha_value = 0;
		if (event == ESPBUTTONEVENT_SINGLECLICK) {
			cha_value = HOMEKIT_PROGRAMMABLE_SWITCH_EVENT_SINGLE_PRESS;
		} else if (event == ESPBUTTONEVENT_DOUBLECLICK) {
			cha_value = HOMEKIT_PROGRAMMABLE_SWITCH_EVENT_DOUBLE_PRESS;
		} else if (event == ESPBUTTONEVENT_LONGCLICK) {
			cha_value = HOMEKIT_PROGRAMMABLE_SWITCH_EVENT_LONG_PRESS;
		}
		cha_programmable_switch_event.value.uint8_value = cha_value;
		homekit_characteristic_notify(&cha_programmable_switch_event,
				cha_programmable_switch_event.value);
	});
	ESPButton.begin();

	cha_programmable_switch_event.getter = cha_programmable_switch_event_getter;
	arduino_homekit_setup(&config);
}

static uint32_t next_heap_millis = 0;

void my_homekit_loop() {
	ESPButton.loop();
	arduino_homekit_loop();
	const uint32_t t = millis();
	if (t > next_heap_millis) {
		// Show heap info every 5 seconds
		next_heap_millis = t + 5 * 1000;
		LOG_D("Free heap: %d, HomeKit clients: %d",
				ESP.getFreeHeap(), arduino_homekit_connected_clients_count());

	}
}
