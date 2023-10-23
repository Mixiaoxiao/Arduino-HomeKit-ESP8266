#include "homekit/types.h"
/*
 * my_accessory.c
 * Define the accessory in C language using the Macro in characteristics.h
 *
 *  Created on: 2023-08-15
 *      Author: gonzalovlchz (Gonzalo Vilchez)
 */

#include <homekit/homekit.h>
#include <homekit/characteristics.h>

// Called to identify this accessory. See HAP section 6.7.6 Identify Routine
// Generally this is called when paired successfully or click the "Identify Accessory" button in Home APP.
void my_accessory_identify(homekit_value_t _value) {
	printf("accessory identify\n");
}

// For HEATER COOLER,
// the required characteristics are: CURRENT_TEMPERATURE, ACTIVE, CURRENT_HEATER_COOLER_STATE, TARGET_HEATER_COOLER_STATE
// the optional characteristics are: NAME, ROTATION_SPEED, TEMPERATURE_DISPLAY_UNITS, SWING_MODE, COOLING_THRESHOLD_TEMPERATURE, HEATING_THRESHOLD_TEMPERATURE, LOCK_PHYSICAL_CONTROLS
// See HAP section 8.41 and characteristics.h

// (required) format: float; HAP section 9.35; min 0, max 100, step 0.1, unit celsius
homekit_characteristic_t cha_current_temperature = HOMEKIT_CHARACTERISTIC_(CURRENT_TEMPERATURE, 0);
homekit_characteristic_t cha_current_relative_humidity = HOMEKIT_CHARACTERISTIC_(CURRENT_RELATIVE_HUMIDITY, 50);

// (optional) format: string; HAP section 9.62; max length 64
homekit_characteristic_t cha_name = HOMEKIT_CHARACTERISTIC_(NAME, "Air conditioner");

homekit_characteristic_t cha_active = HOMEKIT_CHARACTERISTIC_(ACTIVE, true);
homekit_characteristic_t cha_current_heater_cooler_state = HOMEKIT_CHARACTERISTIC_(CURRENT_HEATER_COOLER_STATE, 1); 
homekit_characteristic_t cha_target_heater_cooler_state =HOMEKIT_CHARACTERISTIC_(TARGET_HEATER_COOLER_STATE, 2); 

homekit_characteristic_t cha_cooling_threshold_temperature =HOMEKIT_CHARACTERISTIC_(COOLING_THRESHOLD_TEMPERATURE, 22); 
homekit_characteristic_t cha_heating_threshold_temperature =HOMEKIT_CHARACTERISTIC_(HEATING_THRESHOLD_TEMPERATURE, 25); 

homekit_characteristic_t cha_rotation_speed =HOMEKIT_CHARACTERISTIC_(ROTATION_SPEED, 100); 

homekit_characteristic_t cha_swing_mode =HOMEKIT_CHARACTERISTIC_(SWING_MODE, 0);



// (optional) format: bool; HAP section 9.96
// homekit_characteristic_t cha_status_active = HOMEKIT_CHARACTERISTIC_(STATUS_ACTIVE, true);

// (optional) format: uint8; HAP section 9.97; 0 "No Fault", 1 "General Fault"
// homekit_characteristic_t cha_status_fault = HOMEKIT_CHARACTERISTIC_(STATUS_FAULT, 0);

// (optional) format: uint8; HAP section 9.100; 0 "Accessory is not tampered", 1 "Accessory is tampered with"
// homekit_characteristic_t cha_status_tampered = HOMEKIT_CHARACTERISTIC_(STATUS_TAMPERED, 0);

// (optional) format: uint8; HAP section 9.99; 0 "Battery level is normal", 1 "Battery level is low"
// homekit_characteristic_t cha_status_low_battery = HOMEKIT_CHARACTERISTIC_(STATUS_LOW_BATTERY, 0);

// example for humidity
// homekit_characteristic_t cha_humidity  = HOMEKIT_CHARACTERISTIC_(CURRENT_RELATIVE_HUMIDITY, 0);

homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_air_conditioner, .services=(homekit_service_t*[]) {
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Air conditioner"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "Arduino HomeKit"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "0123456"),
            HOMEKIT_CHARACTERISTIC(MODEL, "ESP8266/ESP32"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, my_accessory_identify),
            NULL
        }),
        HOMEKIT_SERVICE(HEATER_COOLER, .primary=true, .characteristics=(homekit_characteristic_t*[]) {
            &cha_current_temperature,
			&cha_name,//optional
      &cha_active,
      &cha_current_heater_cooler_state,
      &cha_target_heater_cooler_state,
      &cha_cooling_threshold_temperature,//optional
      &cha_heating_threshold_temperature,//optional
      &cha_current_relative_humidity,//optional
      &cha_swing_mode,//optional

            NULL
        }),
		// Add this HOMEKIT_SERVICE if you has a HUMIDITY_SENSOR together
		/*
        HOMEKIT_SERVICE(HUMIDITY_SENSOR, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Humidity Sensor"),
            &cha_humidity,
            NULL
        }),*/
        NULL
    }),
    NULL
};

homekit_server_config_t config = {
		.accessories = accessories,
		.password = "111-11-111"
};


