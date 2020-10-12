/*
 * outlet_accessory.c
 * Define the accessory in pure C language using the Macro in characteristics.h
 *
 *  Created on: 2020-02-08
 *      Author: Mixiaoxiao (Wang Bin) 
 */

#include <Arduino.h>
#include <homekit/types.h>
#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include <stdio.h>
#include <port.h>

#define ACCESSORY_NAME  ("ESP8266_OUTLET")
#define ACCESSORY_SN  ("0001")  //SERIAL_NUMBER
#define ACCESSORY_MANUFACTURER ("Arduino Homekit")
#define ACCESSORY_MODEL  ("ESP8266")

#define PIN_RELAY  2 //D4 //define as your pin for outlet relay

bool relay_power = false; //true or flase

homekit_value_t relay_on_get() {
	return HOMEKIT_BOOL(relay_power);
}

void relay_on_set(homekit_value_t value) {
	if (value.format != homekit_format_bool) {
		printf("Invalid on-value format: %d\n", value.format);
		return;
	}
	relay_power = value.bool_value;
	relay_update();
}

homekit_characteristic_t name = HOMEKIT_CHARACTERISTIC_(NAME, ACCESSORY_NAME);
homekit_characteristic_t serial_number = HOMEKIT_CHARACTERISTIC_(SERIAL_NUMBER, ACCESSORY_SN);
homekit_characteristic_t relay_on = HOMEKIT_CHARACTERISTIC_(ON, false,
		.getter=relay_on_get,
		.setter=relay_on_set
);

void relay_update() {
	if (relay_power) {
		digitalWrite(PIN_RELAY, HIGH);
		printf("ON\n");
	} else {
		printf("OFF\n");
		digitalWrite(PIN_RELAY, LOW);
	}
}

void relay_toggle() {
	relay_on.value.bool_value = !relay_on.value.bool_value;
	relay_on.setter(relay_on.value);
	homekit_characteristic_notify(&relay_on, relay_on.value);
}

void accessory_identify(homekit_value_t _value) {
	printf("accessory identify\n");
	for (int j = 0; j < 3; j++) {
		relay_power = true;
		relay_update();
		delay(100);
		relay_power = false;
		relay_update();
		delay(100);
	}
}

homekit_accessory_t *accessories[] =
		{
				HOMEKIT_ACCESSORY(
						.id = 1,
						.category = homekit_accessory_category_outlet,
						.services=(homekit_service_t*[]){
						HOMEKIT_SERVICE(ACCESSORY_INFORMATION,
						.characteristics=(homekit_characteristic_t*[]){
						&name,
						HOMEKIT_CHARACTERISTIC(MANUFACTURER, ACCESSORY_MANUFACTURER),
						&serial_number,
						HOMEKIT_CHARACTERISTIC(MODEL, ACCESSORY_MODEL),
						HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.0.1"),
						HOMEKIT_CHARACTERISTIC(IDENTIFY, accessory_identify),
						NULL
						}),
						HOMEKIT_SERVICE(OUTLET, .primary=true,
						.characteristics=(homekit_characteristic_t*[]){
						HOMEKIT_CHARACTERISTIC(NAME, "Outlet"),
						&relay_on,
						HOMEKIT_CHARACTERISTIC(OUTLET_IN_USE, true),
						NULL
						}),
						NULL
						}),
				NULL
		};

homekit_server_config_t config = {
		.accessories = accessories,
		.password = "111-11-111",
		.setupId = "ABCD"
};

void accessory_init() {
	pinMode(PIN_RELAY, OUTPUT);
	relay_update();
}
