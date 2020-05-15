/*
 * ESPButton.h
 * Ticker-scan based Button Handler with debounce.
 * Default scan interval is 16ms (60FPS).
 * All added Buttons are scanned by a global Ticker (by os timer).
 *
 * Usage:
 * in setup():
 * pinMode(pin, INPUT_PULLUP / INPUT /... );
 * ESPButton.add(id, pin, pin_down_digital);
 * ESPButton.setCallback(...); // handle your ButtonEvent by id
 * ESPButton.begin(); // call begin to start scan.
 * in loop():
 * ESPButton.loop(); // will notify the event in loop (not in interrupt timer)
 *
 *  Created on:  2020-03-08
 *  Last update: 2020-04-14
 *      Author:  Wang Bin
 */

#ifndef ESPBUTTON_H_
#define ESPBUTTON_H_

#include <Arduino.h>
#include <functional>
#include <Ticker.h>

#define ESPBUTTON_DEBUG(message, ...)  //printf_P(PSTR("[%7d] ESPButton: " message "\n"), millis(), ##__VA_ARGS__)

typedef struct _ESPButtonEntry {
	uint8_t id = -1;
	uint8_t pin = -1;
	uint8_t pin_down_digital = LOW; //按下状态时的digital值

	bool stable_down = false; //稳定状态的下的按键状态 (true: down, false: up)
	uint32_t stable_threshold = 40; //如果按键状态维持一段时间未变化就认为是stable
	bool is_stable = false;	//当前是否是稳定状态，只有稳定状态下才会进一步处理
	bool raw_down = false; //未消抖的原始按键状态
	uint32_t raw_changed_time = 0; //未消抖的原始按键状态的改变的时间

	//void (*ext_digitalRead)(uint8_t pin);
	//如果该pin不是ESP芯片的引脚，而是从其他的扩展芯片读取的，就需要传入ext_digitalRead
	std::function<uint8_t(uint8_t pin)> ext_digitalRead = nullptr;
	//======
	bool longclicked = false; //用来保证一次按下只能触发一次长按
	bool down_handled = false; //标志着是否已经处理了该次down按下事件（比如已经触发了长按)
	//down->up，在规定时间内再次down就是双击，否则超时就是单击
	bool wait_doubleclick = false; //标志着是否等待着双击事件
	uint32_t down_time = 0; //ms
	uint32_t up_time = 0;

	uint32_t longclick_threshold = 5000;
	uint32_t doubleclick_threshold = 150; //按下释放后在此时间间隔内又按下认为是双击

	bool longclick_enable = true;
	bool doubleclick_enable = true;
	//======
	struct _ESPButtonEntry *next;
} ESPButtonEntry;

enum ESPButtonEvent {
	ESPBUTTONEVENT_NONE = 0,
	ESPBUTTONEVENT_SINGLECLICK,
	ESPBUTTONEVENT_DOUBLECLICK,
	ESPBUTTONEVENT_LONGCLICK
};
class ESPButtonClass;

static void _esp32_ticker_cb(ESPButtonClass *esp_button);

class ESPButtonClass {

public:

	typedef std::function<void(uint8_t id, ESPButtonEvent event)> espbutton_callback;

	Ticker ticker;
	ESPButtonEntry *entries = nullptr;
	espbutton_callback callback;
	ESPButtonEvent notify_event = ESPBUTTONEVENT_NONE;
	uint8_t notify_id = 0;

	ESPButtonClass() {
	}
	~ESPButtonClass() {
	}

	void begin() {
		ticker.detach();
#if defined(ESP8266)
		ticker.attach_ms(16, std::bind(&ESPButtonClass::tick, this));
#elif defined(ESP32)
		ticker.attach_ms(16, _esp32_ticker_cb, this);
#endif
	}

	ESPButtonEntry* add(uint8_t _id, uint8_t _pin, uint8_t _pin_down_digital,
			bool _doubleclick_enable = false, bool _longclick_enable = true) {
		ESPButtonEntry *entry = new ESPButtonEntry();
		entry->id = _id;
		entry->pin = _pin;
		entry->pin_down_digital = _pin_down_digital;
		entry->doubleclick_enable = _doubleclick_enable;
		entry->longclick_enable = _longclick_enable;

		//初始化entry的状态？？暂时不需要，我们就是认为按键默认就是未按下的
		//entry->laststate_is_down = digitalReadEntryIsDown(entry);
		//加入链表
		entry->next = entries;
		entries = entry;
		return entry;
	}

	void setCallback(espbutton_callback _callback) {
		callback = _callback;
	}

	PGM_P getButtonEventDescription(ESPButtonEvent e) {
		switch (e) {
		case ESPBUTTONEVENT_SINGLECLICK:
			return PSTR("SingleClick");
		case ESPBUTTONEVENT_DOUBLECLICK:
			return PSTR("DoubleClick");
		case ESPBUTTONEVENT_LONGCLICK:
			return PSTR("LongClick");
		default:
			return PSTR("<unknown event>");
		}
	}

	void tick() {
		ESPButtonEntry *entry = entries;
		while (entry) {
			tickEntry(entry);
			entry = entry->next;
		}
	}

	void loop() {
		if (callback && (notify_event != ESPBUTTONEVENT_NONE)) {
			callback(notify_id, notify_event);
			notify_id = 0;
			notify_event = ESPBUTTONEVENT_NONE;
		}
	}

private:

	bool digitalReadEntryIsDown(ESPButtonEntry *entry) {
		if (entry->ext_digitalRead) {
			return entry->ext_digitalRead(entry->pin) == entry->pin_down_digital;
		}
		return digitalRead(entry->pin) == entry->pin_down_digital;
	}

	void tickEntry(ESPButtonEntry *entry) {
		const uint32_t t = millis();
		const bool down = digitalReadEntryIsDown(entry);
		if (down != entry->raw_down) {
			entry->raw_down = down;
			entry->is_stable = false;
			entry->raw_changed_time = t;
			ESPBUTTON_DEBUG("change (%s)", down ? PSTR("down") : PSTR("up"));
		} else { // down == raw_down
			// 在stable_threshold时间内一直没有变化，认为是stable
			if (!entry->is_stable) {
				if (t - entry->raw_changed_time > entry->stable_threshold) {
					ESPBUTTON_DEBUG("t: %d, raw: %d", t, entry->raw_changed_time);ESPBUTTON_DEBUG("stable (%s)", down ? PSTR("down") : PSTR("up"));
					entry->is_stable = true;
				}
			}
		}
		if (!entry->is_stable) {
			//ESPBUTTON_DEBUG("not stable");
			return;
		}
		//以上代码能检测出超过一定时间的稳定了的状态，等稳定了之后再做处理

		if (entry->stable_down == down) {
			handleEntryUnchanged(entry);
			return;
		} else {
			entry->stable_down = down;
			handleEntryChanged(entry);
		}

	}

	void handleEntryChanged(ESPButtonEntry *entry) {
		const bool down = entry->stable_down;
		//仅有单击事件就在down的时候直接回调？ 暂时不这么做，类比实体开关，按下不松手的时候，就是一直开着的状态
		//逻辑如下：
		//单击：按下->释放->且释放一段时间内没有第二次按下
		//双击：按下->释放->且释放一段时间内执行第二次按下时触发
		//长按：按下一段时间内未释放
		if (down) { //down
			if (entry->wait_doubleclick && entry->doubleclick_enable) {
				//规定时间内第二次down了，认为是双击
				//亲测，一般情况下我的双击up->第二次down的间隔是80~100左右
				ESPBUTTON_DEBUG("doubleclick, wait %d", (millis() - entry->up_time));
				entry->down_handled = true;
				notifyEvent(entry, ESPBUTTONEVENT_DOUBLECLICK);
			} else {
				//第一次按下
				entry->down_handled = false;
			}
			entry->down_time = millis();
			entry->longclicked = false;
			entry->wait_doubleclick = false;
		} else { //up
			if (!entry->down_handled) {
				if (entry->doubleclick_enable) {
					//在loop中延时等待第二次按下
					entry->up_time = millis();
					entry->wait_doubleclick = true;
				} else {
					entry->down_handled = true;
					notifyEvent(entry, ESPBUTTONEVENT_SINGLECLICK);
				}
			}
		}

	}

	void handleEntryUnchanged(ESPButtonEntry *entry) {
		bool down = entry->stable_down;
		if (down) { //down
			if (entry->longclick_enable) {
				if (!entry->longclicked && !entry->down_handled) {
					if (millis() - entry->down_time > entry->longclick_threshold) {
						entry->longclicked = true;
						entry->down_handled = true;
						notifyEvent(entry, ESPBUTTONEVENT_LONGCLICK);
					}
				}
			}
		} else { //up
			entry->longclicked = false;
			if (entry->wait_doubleclick && entry->doubleclick_enable) {
				if (millis() - entry->up_time > entry->doubleclick_threshold) {
					entry->wait_doubleclick = false;
					entry->down_handled = true;
					//key2DoClick();
					notifyEvent(entry, ESPBUTTONEVENT_SINGLECLICK);
				}
			}

		}
	}

	void notifyEvent(ESPButtonEntry *entry, ESPButtonEvent event) {
		ESPBUTTON_DEBUG("Button(%d): %s", entry->id, getButtonEventDescription(event));
		// Save the Event and notify it in loop
		if (notify_event != ESPBUTTONEVENT_NONE) {
			ESPBUTTON_DEBUG("Warnning! Previous Button Event is not handled in loop!");
		}
		notify_event = event;
		notify_id = entry->id;
//		if (callback) {
//			callback(entry->id, event);
//		}
	}

};

ESPButtonClass ESPButton;

static void _esp32_ticker_cb(ESPButtonClass *esp_button) {
	esp_button->tick();
}

#endif /* ESPBUTTON_H_ */
