/*
 * \Example07_DHTsensors.ino
 *
 * This example,
 * reports temperature and humidity data measured from the DHT sensor to homekit.
 * 
 *
 *  Created on: 2021-07-29
 *      Author: Ruzgar Erik
 *
 */
#include <Arduino.h>
#include <arduino_homekit_server.h>
#include "wifi_info.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define LOG_D(fmt, ...)   printf_P(PSTR(fmt "\n") , ##__VA_ARGS__);
#define DHTPIN 5     // Digital pin connected to the DHT sensor 
#define DHTTYPE DHT22 //DHT type defined.
DHT dht(DHTPIN, DHTTYPE);

void setup() {
	Serial.begin(115200);
  dht.begin();
	wifi_connect(); // in wifi_info.h
	my_homekit_setup();
}

void loop() {
	my_homekit_loop();
	delay(10);
}

//==============================
// Homekit setup and loop
//==============================

// access your homekit characteristics defined in my_accessory.c
extern "C" homekit_server_config_t config;
extern "C" homekit_characteristic_t cha_current_temperature;
extern "C" homekit_characteristic_t cha_humidity;

static uint32_t next_heap_millis = 0;
static uint32_t next_report_millis = 0;

void my_homekit_setup() {
	arduino_homekit_setup(&config);
}

void my_homekit_loop() {
	arduino_homekit_loop();
	const uint32_t t = millis();
	if (t > next_report_millis) {
		// report sensor values every 10 seconds
		next_report_millis = t + 10 * 1000;
		my_homekit_report();
	}
	if (t > next_heap_millis) {
		// show heap info every 5 seconds
		next_heap_millis = t + 5 * 1000;
		LOG_D("Free heap: %d, HomeKit clients: %d",
				ESP.getFreeHeap(), arduino_homekit_connected_clients_count());

	}
}

void my_homekit_report() {
	float temperature_value = dht.readTemperature();  // FIXME, read your real sensor here.
	cha_current_temperature.value.float_value = temperature_value;
	LOG_D("Current temperature: %.1f", temperature_value);
	homekit_characteristic_notify(&cha_current_temperature, cha_current_temperature.value);
  float humidity_value = dht.readHumidity();  // FIXME, read your real sensor here.
  cha_humidity.value.float_value = humidity_value;
  LOG_D("Current Humidity: %.1f", humidity_value);
  homekit_characteristic_notify(&cha_humidity, cha_humidity.value);
  
}

int random_value(int min, int max) {
	return min + random(max - min);
}
