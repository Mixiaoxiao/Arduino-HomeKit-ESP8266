/*
 * builtin_led_accessory.c
 * Define the accessory in pure C language using the Macro in characteristics.h
 *
 *  Created on: 2020-04-13
 *      Author: Mixiaoxiao (Wang Bin)
 */

#include <Arduino.h>
#include <homekit/types.h>
#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include <stdio.h>
#include <port.h>
#include <esp_wifi_types.h>
#include <esp_wifi.h>

static char ACCESSORY_NAME[32] = "ESP32_LED";
#define ACCESSORY_SN  ("SN_0123456")  //SERIAL_NUMBER
#define ACCESSORY_MANUFACTURER ("Arduino HomeKit")
#define ACCESSORY_MODEL  ("ESP32_DEVKIT")

#define PIN_LED  2

void led_set_power(bool on) {
	digitalWrite(PIN_LED, on ? HIGH : LOW);
}

void led_setter(homekit_value_t value);

homekit_characteristic_t name = HOMEKIT_CHARACTERISTIC_(NAME, ACCESSORY_NAME);
homekit_characteristic_t serial_number = HOMEKIT_CHARACTERISTIC_(SERIAL_NUMBER, ACCESSORY_SN);
homekit_characteristic_t led_on = HOMEKIT_CHARACTERISTIC_(ON, false, .setter=led_setter);

void led_setter(homekit_value_t value) {
	if (value.format != homekit_format_bool) {
		printf("Invalid on-value format: %d\n", value.format);
		return;
	}
	const bool power = value.bool_value;
	led_on.value.bool_value = power; // Sync the value, and no getter needed.
	led_set_power(power);
	printf("Set Builtin-LED to %s\n", power ? "ON" : "OFF");
}

void led_toggle() {
	bool power = !led_on.value.bool_value;
	led_on.value.bool_value = power;
	led_set_power(power);
	homekit_characteristic_notify(&led_on, led_on.value);
}

void led_blink_task(void *_args) {
	for (int i = 0; i < 3; i++) {
		led_set_power(true);
		vTaskDelay(100 / portTICK_PERIOD_MS);
		led_set_power(false);
		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
	led_set_power(false);
	vTaskDelete(NULL);
}

void led_blink() {
	xTaskCreate(led_blink_task, "led_blink_task", 2048, NULL, 1, NULL);
}

void accessory_identify(homekit_value_t _value) {
	printf("accessory identify\n");
	led_blink();
}

homekit_accessory_t *accessories[] = {
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
						    HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0.1"),
						    HOMEKIT_CHARACTERISTIC(IDENTIFY, accessory_identify),
						    NULL
						  }),
						  HOMEKIT_SERVICE(LIGHTBULB, .primary=true,
						  .characteristics=(homekit_characteristic_t*[]){
						    HOMEKIT_CHARACTERISTIC(NAME, "Led"),
						    &led_on,
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
	led_set_power(false);
	//Rename ACCESSORY_NAME base on MAC address.
	uint8_t mac[6];
	esp_wifi_get_mac(WIFI_IF_STA, mac);
	sprintf(ACCESSORY_NAME, "ESP32_LED_%02X%02X%02X", mac[3], mac[4], mac[5]);
}
