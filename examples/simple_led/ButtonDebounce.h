#pragma once
#ifndef BUTTON_DEBOUNCE_H_
#define BUTTON_DEBOUNCE_H_

#include <Arduino.h>
#include <functional>
#include <FunctionalInterrupt.h> //attachInterrupt可以传function

//Ref debounce_time = 35 ， dbTime=25
//https://github.com/evert-arias/EasyButton/blob/master/src/EasyButton.h
//https://github.com/JChristensen/JC_Button/blob/master/src/JC_Button.h


/**
 * ButtonDebounce
 * WangBin 2019.12.26
 *
 * Example:
 * ButtonDebounce flashButton(D3, INPUT_PULLUP, LOW);
 * flashButton.setCallback(keyCallback);
 * flashButton.setInterrupt(interrupt_callback); // mark "IRAM_ATTR" on ESP32
 * call flashButton.update() in interrupt_callback
 * use flashButton.checkIsDown() to check the debounced state
 */


class ButtonDebounce {

private:

	std::function<void(const bool)> callback;
	uint32_t lastchange_ms = 0;
	uint32_t debounce_ms = 35;
	bool laststate_is_down = false; // true is down;
	int pin_down_digital = LOW;
	int pin;

	bool readIsDown() {
		return pin_down_digital == digitalRead(pin);
	}

public:
	//IRAM_ATTR
	ButtonDebounce(uint8_t pin, uint8_t pin_mode, uint8_t pin_down_digital,
			uint32_t debounce_ms = 35) : //冒号后赋值和this赋值等价
			pin(pin), pin_down_digital(pin_down_digital), debounce_ms(debounce_ms) {
		// callback(callback),
		//this->pin = pin;
		//this->pin_down_digital = pin_down_digital;
		//this->debounce_ms = debounce_ms;
		pinMode(pin, pin_mode);
//		if (attach_interrupt) {
//			using namespace std::placeholders;
////			std::function<void(const ButtonDebounce&, void)> ttt = &ButtonDebounce::buttonInterruptFunc;
//			std::function<void()> tvctt = std::bind(&ButtonDebounce::buttonInterruptFunc, this);
////			std::function<void()> tvctt =  &ButtonDebounce::buttonInterruptFunc; // @suppress("Invalid arguments")
////			void (*ff)(void);
////			ff = tvctt;
////			ff = [](void) {
////				ButtonDebounce::update();
////			};
//			attachInterrupt(digitalPinToInterrupt(pin),
//					tvctt, CHANGE);
//		}

	}

	void update(bool down) {
		const uint32_t t = millis();
		if (t - lastchange_ms < debounce_ms) {
			lastchange_ms = t;
			//debounce
			//Serial.println(F("[ButtonDebounce] debounce"));
		} else {
			if (laststate_is_down == down) {
				//same state
				//Serial.println(F("[ButtonDebounce] same state"));
			} else { //state changed, up->down or down->up
				lastchange_ms = t;
				laststate_is_down = down;
				if (callback) {
					callback(down);
				}
			}
		}
	}

	void update() {
		bool down = readIsDown();
		update(down);
	}

	//返回当前debouce后的按钮状态
	bool checkIsDown(){
		return laststate_is_down;
	}

	void setCallback(std::function<void(const bool down)> callback) {
		this->callback = callback;
	}
	//interrupt_function 需标记为 IRAM_ATTR
	void setInterrupt(std::function<void(void)> interrupt_function) {
		if (interrupt_function) {
			attachInterrupt(digitalPinToInterrupt(pin), interrupt_function, CHANGE);
		}
	}

};

#endif
