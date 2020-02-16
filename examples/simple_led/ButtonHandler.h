#pragma once
#ifndef BUTTONHANDLER_H_
#define BUTTONHANDLER_H_

#include <Arduino.h>
#include <functional>
#include <FunctionalInterrupt.h>

enum button_event {
	BUTTON_EVENT_SINGLECLICK = 0,
	BUTTON_EVENT_DOUBLECLICK,
	BUTTON_EVENT_LONGCLICK
};
/**
 * ButtonHandler
 * WangBin 2019.12.26
 * Example:
 * ButtonHandler handler;
 * handler.setCallback(
 [](const button_event event) {
 if (event == BUTTON_EVENT_SINGLECLICK) {
 ...
 } else if (event == BUTTON_EVENT_DOUBLECLICK) {
 ...
 } else if (event == BUTTON_EVENT_LONGCLICK) {
 ...
 }
 }
 );
 handler.setIsDownFunction(
 [](void) {
 //return bool: the button is down or not
 return flashButton.checkIsDown();
 });

 //when the button state changed, call ~.handle(down or not)
 handler.handleChange(down);

 //call ~.loop() in Arduino's loop
 */
class ButtonHandler {

private:
	std::function<void(const button_event)> callback;
	std::function<bool(void)> is_down_function;

	bool longclicked = false; //用来保证一次按下只能触发一次长按
	bool down_handled = false; //标志着是否已经处理了该次down按下事件（比如已经触发了长按)
	//down->up，在规定时间内再次down就是双击，否则超时就是单击
	bool wait_doubleclick = false; //标志着是否等待着双击事件
	uint32_t down_time = 0; //ms
	uint32_t up_time = 0;

	uint32_t longclick_threshold = 5000;
	uint32_t doubleclick_threshold = 200; //按下释放后在此时间间隔内又按下认为是双击

	bool longclick_enable = true;
	bool doubleclick_enable = true;

public:
	ButtonHandler(uint32_t longclick_duration = 5000, uint32_t doubleclick_duration = 200) :
			longclick_threshold(longclick_duration),
					doubleclick_threshold(doubleclick_duration) {
	}

	void setCallback(std::function<void(const button_event)> callback) {
		this->callback = callback;
	}

	void setIsDownFunction(std::function<bool(void)> is_down_function) {
		this->is_down_function = is_down_function;
	}

	void setLongClickEnable(bool enable) {
		longclick_enable = enable;
	}
	void setDoubleClickEnable(bool enable) {
		doubleclick_enable = enable;
	}
	//当pin点评改变时调用
	void handleChange(bool down) {
		//单击：按下释放且释放一段时间内没有第二次按下
		//双击：按下释放且释放一段时间内执行第二次按下时触发
		//长按：按下一段时间内未释放
		if (down) { //down
			if (wait_doubleclick && doubleclick_enable) {
				//规定时间内第二次down了，认为是双击
				//亲测，一般情况下我的双击up->第二次down的间隔是80~100左右
				//Serial.println(String("doubleclick->duration=") + (millis() - up_time));
				down_handled = true;
				//key2DoDoubleClick();
				if (callback) {
					callback(BUTTON_EVENT_DOUBLECLICK);
				}
			} else {
				//第一次按下
				down_handled = false;
			}
			down_time = millis();
			longclicked = false;
			wait_doubleclick = false;
		} else { //up
			if (!down_handled) {
				if (doubleclick_enable) {
					//在loop中延时等待第二次按下
					up_time = millis();
					wait_doubleclick = true;
				} else {
					down_handled = true;
					if (callback) {
						callback(BUTTON_EVENT_SINGLECLICK);
					}
				}
			}
		}
	}

	void loop() {
		bool down = is_down_function();
		if (down) {
			if (longclick_enable) {
				if (!longclicked && !down_handled) {
					if (millis() - down_time > longclick_threshold) {
						//key2DoLongClick();
						longclicked = true;
						down_handled = true;
						if (callback) {
							callback(BUTTON_EVENT_LONGCLICK);
						}
					}
				}
			}
		} else { //up
			longclicked = false;
			if (wait_doubleclick && doubleclick_enable) {
				if (millis() - up_time > doubleclick_threshold) {
					wait_doubleclick = false;
					down_handled = true;
					//key2DoClick();
					if (callback) {
						callback(BUTTON_EVENT_SINGLECLICK);
					}
				}
			}

		}
	}
};

#endif /* BUTTONHANDLER_H_ */

