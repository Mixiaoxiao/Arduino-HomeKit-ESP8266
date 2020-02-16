/*
 * simple_led_accessory.c
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

//const char * buildTime = __DATE__ " " __TIME__ " GMT";

#define ACCESSORY_NAME  ("ESP8266_LED")
#define ACCESSORY_SN  ("SN_0123456")  //SERIAL_NUMBER
#define ACCESSORY_MANUFACTURER ("Arduino Homekit")
#define ACCESSORY_MODEL  ("ESP8266")

#define PIN_LED  2//D4

int led_bri = 100; //[0, 100]
bool led_power = false; //true or flase

homekit_value_t led_on_get() {
	return HOMEKIT_BOOL(led_power);
}

void led_on_set(homekit_value_t value) {
	if (value.format != homekit_format_bool) {
		printf("Invalid on-value format: %d\n", value.format);
		return;
	}
	led_power = value.bool_value;
	if (led_power) {
		if (led_bri < 1) {
			led_bri = 100;
		}
	}
	led_update();
}

homekit_value_t light_bri_get() {
	return HOMEKIT_INT(led_bri);
}

homekit_characteristic_t name = HOMEKIT_CHARACTERISTIC_(NAME, ACCESSORY_NAME);
homekit_characteristic_t serial_number = HOMEKIT_CHARACTERISTIC_(SERIAL_NUMBER, ACCESSORY_SN);
homekit_characteristic_t occupancy_detected = HOMEKIT_CHARACTERISTIC_(OCCUPANCY_DETECTED, 0);
homekit_characteristic_t led_on = HOMEKIT_CHARACTERISTIC_(ON, false,
		//.callback=HOMEKIT_CHARACTERISTIC_CALLBACK(switch_on_callback),
		.getter=led_on_get,
		.setter=led_on_set
);

void occupancy_toggle() {
	const uint8_t state = occupancy_detected.value.uint8_value;
	occupancy_detected.value = HOMEKIT_UINT8((!state) ? 1 : 0);
	homekit_characteristic_notify(&occupancy_detected, occupancy_detected.value);
}

void led_update() {
	if (led_power) {
		int pwm = PWMRANGE - (int) (led_bri * 1.0 * PWMRANGE / 100.0 + 0.5f);
		analogWrite(PIN_LED, pwm);
		printf("ON  %3d (pwm: %4d of %d)\n", led_bri, pwm, PWMRANGE);
	} else {
		printf("OFF\n");
		digitalWrite(PIN_LED, HIGH);
	}
}

void led_bri_set(homekit_value_t value) {
	if (value.format != homekit_format_int) {
		return;
	}
	led_bri = value.int_value;
	led_update();
}

void led_toggle() {
	led_on.value.bool_value = !led_on.value.bool_value;
	led_on.setter(led_on.value);
	homekit_characteristic_notify(&led_on, led_on.value);
}

void accessory_identify(homekit_value_t _value) {
	printf("accessory identify\n");
	for (int j = 0; j < 3; j++) {
		led_power = true;
		led_update();
		delay(100);
		led_power = false;
		led_update();
		delay(100);
	}
}

homekit_accessory_t *accessories[] =
		{
				HOMEKIT_ACCESSORY(
						.id = 1,
						.category = homekit_accessory_category_lightbulb,
						.services=(homekit_service_t*[]){
						HOMEKIT_SERVICE(ACCESSORY_INFORMATION,
						.characteristics=(homekit_characteristic_t*[]){
						&name,
						HOMEKIT_CHARACTERISTIC(MANUFACTURER, ACCESSORY_MANUFACTURER),
						&serial_number,
						HOMEKIT_CHARACTERISTIC(MODEL, ACCESSORY_MODEL),
						HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.0.9"),
						HOMEKIT_CHARACTERISTIC(IDENTIFY, accessory_identify),
						NULL
						}),
						HOMEKIT_SERVICE(LIGHTBULB, .primary=true,
						.characteristics=(homekit_characteristic_t*[]){
						HOMEKIT_CHARACTERISTIC(NAME, "Led"),
						&led_on,
						HOMEKIT_CHARACTERISTIC(BRIGHTNESS, 100, .getter=light_bri_get, .setter=led_bri_set),
						NULL
						}),
						HOMEKIT_SERVICE(OCCUPANCY_SENSOR, .primary=false, .characteristics=(homekit_characteristic_t*[]) {
						HOMEKIT_CHARACTERISTIC(NAME, "Occupancy"),
						&occupancy_detected,
						NULL
						}),
						NULL
						}),
				NULL
		};

homekit_server_config_t config = {
		.accessories = accessories,
		.password = "111-11-111",
		//.on_event = on_homekit_event,
		.setupId = "ABCD"
};

void accessory_init() {
	pinMode(PIN_LED, OUTPUT);
	led_update();
}

