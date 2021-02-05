/*
 * makesmart_lock.ino
 * 
 *  Created on: 2021-02-05
 *      Author: cooper @ makesmart.net
 *      Thank you for this great library!
 *      
 * This example shows how to:
 * 1. define a lock accessory and its characteristics in my_accessory.c
 * 2. get the target-state sent from iOS Home APP.
 * 3. report the current-state value to HomeKit.
 * 
 * you can use both:
 *    void open_lock(){}
 * and
 *    void close_lock(){}
 *    
 * at the end of this file to let the lock-mechanism do whatever you want. 
 * 
 * 
 * Pairing Code: 123-45-678
 * 
 * 
 * You should:
 * 1. read and use the Example01_TemperatureSensor with detailed comments
 *    to know the basic concept and usage of this library before other examples。
 * 2. erase the full flash or call homekit_storage_reset() in setup()
 *    to remove the previous HomeKit pairing storage and
 *    enable the pairing with the new accessory of this new HomeKit example.
 * 
 */

#include <Arduino.h>
#include <arduino_homekit_server.h>
#include "wifi_info.h"

#define LOG_D(fmt, ...)   printf_P(PSTR(fmt "\n") , ##__VA_ARGS__);

void setup() {
  Serial.begin(115200);
  wifi_connect();
  homekit_storage_reset();
  my_homekit_setup();
}

void loop() {
  my_homekit_loop();
  delay(10);
}

//==============================
// HomeKit setup and loop
//==============================

// access lock-mechanism HomeKit characteristics defined in my_accessory.c
extern "C" homekit_server_config_t config;

extern "C" homekit_characteristic_t cha_lock_current_state;
extern "C" homekit_characteristic_t cha_lock_target_state;

static uint32_t next_heap_millis = 0;


// called when the lock-mechanism target-set is changed by iOS Home APP
void set_lock(const homekit_value_t value) {
  
  uint8_t state = value.int_value;
  cha_lock_current_state.value.int_value = state;
  
  if(state == 0){
    // lock-mechanism was unsecured by iOS Home APP
    open_lock();
  }
  if(state == 1){
    // lock-mechanism was secured by iOS Home APP
    close_lock();
  }
  
  //report the lock-mechanism current-sate to HomeKit
  homekit_characteristic_notify(&cha_lock_current_state, cha_lock_current_state.value);
  
}

void my_homekit_setup() {
  
  cha_lock_target_state.setter = set_lock;
  arduino_homekit_setup(&config);

  
}


void my_homekit_loop() {
  arduino_homekit_loop();
  const uint32_t t = millis();
  if (t > next_heap_millis) {
    // show heap info every 30 seconds
    next_heap_millis = t + 30 * 1000;
    LOG_D("Free heap: %d, HomeKit clients: %d",
        ESP.getFreeHeap(), arduino_homekit_connected_clients_count());

  }
}



/* use this functions to let your lock mechanism do whatever yoi want */
void open_lock(){
  Serial.println("unsecure"); 
  // add your code here eg switch a relay or whatever 
}

void close_lock(){
  Serial.println("secure");  
  // add your code here eg switch a relay or whatever
}
