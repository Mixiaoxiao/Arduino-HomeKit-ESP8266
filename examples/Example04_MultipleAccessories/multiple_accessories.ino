/*
 * multiple_accessories.ino
 *
 *  Created on: 2020-05-16
 *      Author: Mixiaoxiao (Wang Bin)
 *
 *
 * This example is a bridge (aka a gateway) which contains multiple accessories.
 *
 * This example includes 5 sensors:
 * 1. Temperature Sensor (HAP section 8.41)
 * 2. Humidity Sensor (HAP section 8.20)
 * 3. Light Sensor (HAP section 8.24)
 * 4. Contact Sensor (HAP section 8.9)
 * 5. Motion Sensor (HAP section 8.28)
 * 6. Occupancy Sensor (HAP section 8.29)
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
extern "C" homekit_characteristic_t cha_temperature;
extern "C" homekit_characteristic_t cha_humidity;
extern "C" homekit_characteristic_t cha_light;
extern "C" homekit_characteristic_t cha_contact;
extern "C" homekit_characteristic_t cha_motion;
extern "C" homekit_characteristic_t cha_occupancy;

#define HOMEKIT_CONTACT_SENSOR_DETECTED       0
#define HOMEKIT_CONTACT_SENSOR_NOT_DETECTED   1

#define HOMEKIT_OCCUPANCY_DETECTED                  0
#define HOMEKIT_OCCUPANCY_NOT_DETECTED              1

// Called when the value is read by iOS Home APP
homekit_value_t cha_programmable_switch_event_getter() {
	// Should always return "null" for reading, see HAP section 9.75
	return HOMEKIT_NULL_CPP();
}

void my_homekit_setup() {
	arduino_homekit_setup(&config);
}

static uint32_t next_heap_millis = 0;
static uint32_t next_report_millis = 0;

void my_homekit_loop() {
	arduino_homekit_loop();
	const uint32_t t = millis();
	if (t > next_report_millis) {
		// report sensor values every 10 seconds
		next_report_millis = t + 10 * 1000;
		my_homekit_report();
	}
	if (t > next_heap_millis) {
		// Show heap info every 5 seconds
		next_heap_millis = t + 5 * 1000;
		LOG_D("Free heap: %d, HomeKit clients: %d",
				ESP.getFreeHeap(), arduino_homekit_connected_clients_count());

	}
}

void my_homekit_report() {
	// FIXME, read your real sensors here.
	float t = random_value(10, 30);
	float h = random_value(30, 70);
	float l = random_value(1, 10000);
	uint8_t c = random_value(0, 10) < 5 ?
			HOMEKIT_CONTACT_SENSOR_DETECTED : HOMEKIT_CONTACT_SENSOR_NOT_DETECTED;
	bool m = random_value(0, 10) < 5;
	uint8_t o = random_value(0, 10) < 5 ?
			HOMEKIT_OCCUPANCY_DETECTED : HOMEKIT_OCCUPANCY_NOT_DETECTED;

	cha_temperature.value.float_value = t;
	homekit_characteristic_notify(&cha_temperature, cha_temperature.value);

	cha_humidity.value.float_value = h;
	homekit_characteristic_notify(&cha_humidity, cha_humidity.value);

	cha_light.value.float_value = l;
	homekit_characteristic_notify(&cha_light, cha_light.value);

	cha_contact.value.uint8_value = c;
	homekit_characteristic_notify(&cha_contact, cha_contact.value);

	cha_motion.value.bool_value = m;
	homekit_characteristic_notify(&cha_motion, cha_motion.value);

	cha_occupancy.value.uint8_value = o;
	homekit_characteristic_notify(&cha_occupancy, cha_occupancy.value);

	LOG_D("t %.1f, h %.1f, l %.1f, c %u, m %u, o %u", t, h, l, c, (uint8_t)m, o);
}

int random_value(int min, int max) {
	return min + random(max - min);
}
