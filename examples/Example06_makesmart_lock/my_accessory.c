/*
 * my_accessory.c
 * Define the accessory in C language using the Macro in characteristics.h
 *
 *  Created on: 2021-02-05
 *      Author: cooper @ makesmart.net
 *      Thank you for this great library!
 *      
 */

#include <homekit/homekit.h>
#include <homekit/characteristics.h>

void my_accessory_identify(homekit_value_t _value) {
  printf("accessory identify\n");
}


// characteristics of the lock-mechanism
// format: uint8_t // 0 is unsecured // 1 is secured
homekit_characteristic_t cha_lock_current_state = HOMEKIT_CHARACTERISTIC_(LOCK_CURRENT_STATE, 1);
homekit_characteristic_t cha_lock_target_state = HOMEKIT_CHARACTERISTIC_(LOCK_TARGET_STATE, 1);

homekit_characteristic_t cha_name = HOMEKIT_CHARACTERISTIC_(NAME, "Lock");

homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_door_lock, .services=(homekit_service_t*[]) {
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Lock"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "makesmart Community"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "1111111"),
            HOMEKIT_CHARACTERISTIC(MODEL, "makesmart Lock"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, my_accessory_identify),
            NULL
        }),
    HOMEKIT_SERVICE(LOCK_MECHANISM, .primary=true, .characteristics=(homekit_characteristic_t*[]){
      &cha_lock_current_state,
      &cha_lock_target_state,
      &cha_name,
      NULL
    }),
        NULL
    }),
    NULL
};

homekit_server_config_t config = {
    .accessories = accessories,
    .password = "123-45-678"
};
