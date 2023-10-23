/*
 * air_conditioner.ino
 *
 * This example shows how to:
 * 1. define a heater cooler accessory and its characteristics (in my_accessory.c).
 *
 * This example will use the IRremoteESP8266 library to emulate an air conditioner remote control. Example using fujitsu.
 * Other examples can be found in https://github.com/crankyoldgit/IRremoteESP8266
 *
 *  Created on: 2023-08-15
 *      Author: gonzalovlchz (Gonzalo Vilchez)
 *
 * Note:
 *
 * You are recommended to read the Apple's HAP doc before using this library.
 * https://developer.apple.com/support/homekit-accessory-protocol/
 *
 * This HomeKit library is mostly written in C,
 * you can define your accessory/service/characteristic in a .c file,
 * since the library provides convenient Macro (C only, CPP can not compile) to do this.
 * But it is possible to do this in .cpp or .ino (just not so conveniently), do it yourself if you like.
 * Check out homekit/characteristics.h and use the Macro provided to define your accessory.
 *
 * Generally, the Arduino libraries (e.g. sensors, ws2812) are written in cpp,
 * you can include and use them in a .ino or a .cpp file (but can NOT in .c).
 * A .ino is a .cpp indeed.
 *
 * You can define some variables in a .c file, e.g. int my_value = 1;,
 * and you can access this variable in a .ino or a .cpp by writing extern "C" int my_value;.
 *
 * So, if you want use this HomeKit library and other Arduino Libraries together,
 * 1. define your HomeKit accessory/service/characteristic in a .c file
 * 2. in your .ino, include some Arduino Libraries and you can use them normally
 *                  write extern "C" homekit_characteristic_t xxxx; to access the characteristic defined in your .c file
 *                  write your logic code (eg. read sensors) and
 *                  report your data by writing your_characteristic.value.xxxx_value = some_data; homekit_characteristic_notify(..., ...)
 * done.
 */

#include <Arduino.h>
#include <arduino_homekit_server.h>
#include "wifi_info.h"

#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Fujitsu.h>

#include "DHT.h"
//include the Arduino library for your real sensor here, e.g. <DHT.h>

#define LOG_D(fmt, ...) printf_P(PSTR(fmt "\n"), ##__VA_ARGS__);

// Digital pin connected to the DHT sensor
#define DHTPIN 14

// Uncomment whatever DHT sensor type you're using
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22  // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// Variables to hold sensor readings
float temp;
float hum;

const uint16_t kIrLed = 4;      // ESP8266 GPIO pin to use. Recommended: 4 (D2).
IRFujitsuAC ac(kIrLed, ARDB1);  // Set the GPIO to be used to sending the message


void setup() {
  Serial.begin(115200);
  wifi_connect();  // in wifi_info.h

  my_homekit_setup();
  ac.begin();
  ac.setFanSpeed(kFujitsuAcFanAuto);
  ac.setCmd(kFujitsuAcCmdTurnOn);
  dht.begin();
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
extern "C" homekit_characteristic_t cha_current_relative_humidity;
extern "C" homekit_characteristic_t cha_active;
extern "C" homekit_characteristic_t cha_current_heater_cooler_state;
extern "C" homekit_characteristic_t cha_target_heater_cooler_state;
extern "C" homekit_characteristic_t cha_cooling_threshold_temperature;
extern "C" homekit_characteristic_t cha_heating_threshold_temperature;
extern "C" homekit_characteristic_t cha_swing_mode;

static uint32_t next_heap_millis = 0;
static uint32_t next_report_millis = 0;

float ac_temp = 22;
uint8_t ac_mode = kFujitsuAcModeCool;
bool ac_active = true;
uint8_t ac_swing_mode = kFujitsuAcSwingOff;


//Called when the target_heater_cooler_state value is changed by iOS Home APP
void cha_target_heater_cooler_state_setter(const homekit_value_t value) {
  uint8_t state = value.uint8_value;

  cha_target_heater_cooler_state.value.uint8_value = state;  //sync the value
  cha_active.value.bool_value = true;

  fujitsu_setter_homekit();
}
//Called when the swing_mode value is changed by iOS Home APP
void cha_swing_mode_setter(const homekit_value_t value) {
  uint8_t state = value.uint8_value;

  cha_swing_mode.value.uint8_value = state;  //sync the value

  fujitsu_setter_homekit();
}

//Called when the active value is changed by iOS Home APP
void cha_active_setter(const homekit_value_t value) {
  bool active = value.bool_value;

  cha_active.value.bool_value = active;  //sync the value

  fujitsu_setter_homekit();
}

//Called when the cooling_threshold_temperature value is changed by iOS Home APP
void cha_cooling_threshold_temperature_setter(const homekit_value_t value) {
  float temp = value.float_value;

  cha_cooling_threshold_temperature.value.float_value = temp;  //sync the value

  fujitsu_setter_homekit();

  LOG_D("Temperature selected: %f", temp);
}

//Called when the cooling_heating_temperature value is changed by iOS Home APP
void cha_heating_threshold_temperature_setter(const homekit_value_t value) {
  float temp = value.float_value;

  cha_heating_threshold_temperature.value.float_value = temp;  //sync the value

  fujitsu_setter_homekit();

  LOG_D("Temperature selected: %f", temp);
}


// This sets the changed parameters by homekit to the global parameters of the accesory.
void fujitsu_setter_homekit() {
  uint8_t state = cha_target_heater_cooler_state.value.uint8_value;
  uint8_t swing_mode = cha_swing_mode.value.uint8_value;

  switch (state) {
    case 1:
      ac_mode = kFujitsuAcModeHeat;
      ac_temp = cha_heating_threshold_temperature.value.float_value;
      break;
    case 2:
      ac_mode = kFujitsuAcModeCool;
      ac_temp = cha_cooling_threshold_temperature.value.float_value;
      break;
  }

  switch (swing_mode) {
    case 0:
      ac_swing_mode = kFujitsuAcSwingOff;
      break;
    case 1:
      ac_swing_mode = kFujitsuAcSwingBoth;
      break;
  }

  ac_active = cha_active.value.bool_value;

  fujitsu_setter();
}

// This sends the global parameters of the accesory to the IR led
void fujitsu_setter() {
  status_update_homekit(); // We update the current status to homekit

  ac.setMode(ac_mode);
  ac.setTemp(int(ac_temp));
  ac.setSwing(ac_swing_mode);


  if (ac_active) {
    ac.setCmd(kFujitsuAcCmdTurnOn);
    ac.on();
  } else {
    ac.setCmd(kFujitsuAcCmdTurnOff);
    ac.off();
  }

// Now send the IR signal.
#if SEND_FUJITSU_AC
  ac.send();
#endif  // SEND_DAIKIN

  // Display what we are going to send.
  Serial.println(ac.toString());
}

//This updates current status of the accesory to homekit
void status_update_homekit() {
  switch (ac_mode) {
    case kFujitsuAcModeHeat:
      cha_current_heater_cooler_state.value.uint8_value = 2;
      homekit_characteristic_notify(&cha_current_heater_cooler_state, cha_current_heater_cooler_state.value);

      LOG_D("Current heater cooler state: HEAT");
      break;
    case kFujitsuAcModeCool:
      cha_current_heater_cooler_state.value.uint8_value = 3;
      homekit_characteristic_notify(&cha_current_heater_cooler_state, cha_current_heater_cooler_state.value);
      LOG_D("Current heater cooler state: COOL");

      break;
  }

  cha_swing_mode.value.uint8_value = ac_swing_mode;
  homekit_characteristic_notify(&cha_swing_mode, cha_swing_mode.value);

  cha_active.value.bool_value = ac_active;
  homekit_characteristic_notify(&cha_active, cha_active.value);

  if (ac_active) LOG_D("Current active status: active")
  else LOG_D("Current active status: inactive");
}



void my_homekit_setup() {
  cha_target_heater_cooler_state.setter = cha_target_heater_cooler_state_setter;
  cha_cooling_threshold_temperature.setter = cha_cooling_threshold_temperature_setter;
  cha_heating_threshold_temperature.setter = cha_heating_threshold_temperature_setter;
  cha_swing_mode.setter = cha_swing_mode_setter;
  cha_active.setter = cha_active_setter;
  arduino_homekit_setup(&config);
}

void my_homekit_loop() {
  arduino_homekit_loop();
  const uint32_t t = millis();
  if (t > next_report_millis) {
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
  hum = dht.readHumidity();
  // Read temperature as Celsius (the default)
  temp = dht.readTemperature();
  LOG_D("Current temperature: %.1f", temp);
  LOG_D("Current humidity: %.1f", hum);

  cha_current_temperature.value.float_value = temp;
  cha_current_relative_humidity.value.float_value = hum;

  homekit_characteristic_notify(&cha_current_temperature, cha_current_temperature.value);
  homekit_characteristic_notify(&cha_current_relative_humidity, cha_current_relative_humidity.value);



  status_update_homekit();
}