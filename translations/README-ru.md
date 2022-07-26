# Arduino HomeKit ESP8266 (Rus) 

**Оригинал:** [Английский](https://github.com/Mixiaoxiao/Arduino-HomeKit-ESP8266)

**Перевел:** [ph4nt0m7](https://github.com/ph4nt0m7)

## Библиотека Apple HomeKit аксессуаров для ESP8266 Arduino

Эта Arduino библиотека представляет собой нативную реализацию Apple HomeKit аксессуара для [ESP8266 Arduino core](https://github.com/esp8266/Arduino), и работает без каких-либо дополнительных мостов.

Этот проект основан на [esp-homekit](https://github.com/maximkulkin/esp-homekit) для [ESP-OPEN-RTOS](https://github.com/SuperHouse/esp-open-rtos).

Я перенес реализацию [esp-homekit](https://github.com/maximkulkin/esp-homekit) на основе RTOS в чистую среду Arduino, нацеленную на простую и быструю сборку проекта с использованием Arduino IDE (или Eclipse с sloeber, PlatformIO).

Наслаждайтесь сборкой и загрузкой "в один клик", работайте над связыванием различных других библиотек Arduino с Apple HomeKit!

Вот [обсуждение](https://github.com/HomeACcessoryKid/Arduino-HomeKit/issues/1) о том, что для запуска Apple HomeKit требуется RTOS, и этот проект является доказательством концепции того, что Apple HomeKit может быть реализован и нормально работать без RTOS.

Эта библиотека построена с помощью ESP8266 Arduino Core 2.6.3. Более ранние версии могут компилироваться с ошибками.

Для ESP32 смотреть [Arduino-HomeKit-ESP32](https://github.com/Mixiaoxiao/Arduino-HomeKit-ESP32). HomeKit, работающий на ESP32, имеет **ОТЛИЧНУЮ ПРОИЗВОДИТЕЛЬНОСТЬ**, которая в 10 раз быстрее, чем ESP8266.

## Предварительный просмотр

![Preview](https://raw.github.com/Mixiaoxiao/Arduino-HomeKit-ESP8266/master/extras/preview.jpg) 


## Код настройки Apple HomeKit в скетчах-примерах

``111-11-111``


## Применение

1. Определите свой аксессуар в файле .c, чтобы воспользоваться объявлением как "Макро". Вы также можете определить свой аксессуар в файле .ino, используя код C++.
	```C
		homekit_accessory_t *accessories[] = ...
		homekit_server_config_t config = {
			.accessories = accessories,
			.password = "111-11-111",
			//.on_event = on_homekit_event, //optional
			//.setupId = "ABCD" //optional
		};
	```
2. В вашем скетче
	```C
		#include <arduino_homekit_server.h>;
		
		//access the config defined in C code
		extern "C" homekit_server_config_t config; 
		
		void setup() {
			WiFi.begin(ssid, password);
			arduino_homekit_setup(&config);
		}
		
		void loop() {
			arduino_homekit_loop();
		}
	```
Все.

## Производительность

Примечание. Вы должны настроить процессор ESP8266 на работу на частоте 160 МГц (по крайней мере, во время процесса сопряжения), чтобы избежать отключения tcp-сокета от устройства iOS, вызванного тайм-аутом.

* Preinit: ~9.1s (После данного шага вы можете увидеть ваш аксессуар в приложении "Дом")
* Pair Setup Step 1/3: ~0s (Тяжелые крипто-вычисления выполняются в Preinit)
* Pair Setup Step 2/3: ~12.1s 
* Pair Setup Step 3/3: ~0.8s (Настройка пары обрабатывается только при первом сопряжении с устройством iOS)
* Pair Verify Step 1/2: ~0.3s
* Pair Verify Step 2/2: ~0.8s (Шаг проверки требуется каждый раз, когда iOS подключается или повторно подключается к ESP8266 для установления безопасного сеанса)

Весь процесс сопряжения занимает ~14 секунд после того, как вы введете код настройки на своем iPhone. Обратите внимание, что Preinit требует ~9 секунд, прежде чем вы сможете начать сопряжение.

## Heap память

Heap-память имеет решающее значение для ESP8266 с полной поддержкой TCP/IP. ESP8266 легко падает, когда памяти меньше ~5000.

Я попытался заставить криптографию WolfSSL безопасно работать на ESP8266 с лучшей производительностью и меньшим объемом памяти или компромиссом. Подробнее см. в следующем разделе.

Вот значения свободной heap-памяти для запуска примерного скетча:

* Загрузка: ~26000
* Preinit завершен: ~22000
* Сопряжение: ~17000 (или еще ниже при крипто-вычислениях)
* Сопряжено и подключено к одному устройству iOS: ~21700
* Сопряжено, но устройство iOS не подключено: ~23400

После оптимизации памяти в v1.1.0:

* Загрузка: ~46000
* Preinit завершен: ~41000
* Сопряжение: ~37000 (или еще ниже при крипто-вычислениях)
* Сопряжено и подключено к одному устройству iOS: ~41700
* Сопряжено, но устройство iOS не подключено: ~43000


## WolfSSL

* На основе wolfssl-3.13.0-stable.
* Чистый исходный код: неиспользуемые файлы удаляются..
* `CURVE25519_SMALL` и `ED25519_SMALL`: ESP8266 не может работать напрямую без `SMALL` определений, так как памяти не хватает. Но версия без `SMALL` быстрее. Я пометил большой `ge_precomp base[32][8]` с помощью PROGMEM, чтобы сохранить ее во флэш-памяти (около 70 КБ). Также `ge_double_scalarmult_vartime` не может работать из-за нехватки heap-памяти. Я определил `ESP_GE_DOUBLE_SCALARMULT_VARTIME_LOWMEM` в `user_settings.h`, чтобы использовать версию LOWMEM `ge_double_scalarmult_vartime` в `ge_low_mem.c`. Это компромисс между производительностью и памятью. Если вам нужно больше флэш-памяти, вы должны определить `CURVE25519_SMALL` и `ED25519_SMALL` и отменить определение `ESP_GE_DOUBLE_SCALARMULT_VARTIME_LOWMEM` в `user_settings.h` (это приведет к тому, что этапы проверки пары `Pair Verify` займут 1,2 с + 0,9 с).
* `integer.c` (операции с большими целыми числами): `MP_16BIT` и `ESP_FORCE_S_MP_EXPTMOD` определены для лучшей производительности в ESP8266. `ESP_INTEGER_WINSIZE` (по умолчанию = 3) определено, чтобы избежать сбоя, вызванного нехваткой памяти, а значения {3, 4, 5} имеют аналогичную производительность.

## Хранилище

* Данные сопряжения хранятся в `EEPROM` в ядре ESP8266 Arduino.
* Этот проект не использует библиотеку `EEPROM` с кэшем данных для уменьшения использования памяти (напрямую вызывает flash_read и write).
* `EEPROM` составляет 4096 байт в ESP8266, в этом проекте используется максимум [0, 1408) байт.
* Смотрите комментарии в `storge.c` и [ESP8266-EEPROM-doc](https://arduino-esp8266.readthedocs.io/en/2.6.3/libraries.html#eeprom).
* `EEPROM` в [1408, 4096) безопасно для использования. 
* Этот проект НЕ использует `FS (файловая система)`, поэтому вы можете использовать `FS` свободно.

## WatchDog

* В ядре ESP8266 Arduino есть программные и аппаратные сторожевые таймеры. Тяжелые криптографические вычисления приведут к сбросу сторожевого таймера.
* В ядре ESP8266 Arduino есть отключение/включение API программного сторожевого таймера..
* Я нашел [esp_hw_wdt](https://github.com/ComSuite/esp_hw_wdt) для отключения/включения аппаратного сторожевого таймера.
* Два сторожевых таймера отключены во время `Preinit` и `Pair Setup Step 2/3`.

## Рекомендуемые настройки в IDE

* Плата: Generic ESP8266 Module (чтобы получить все настройки)
* Flash Size: как минимум 470KB для скетчей (см. раздел `WolfSSL`, если вам нужен эскиз меньшего размера) 
* LwIP Variant: v2 Lower Memory (для меньшего использования памяти)
* Debug Level: Ничего (для меньшего использования памяти)
* Espressif FW: nonos-sdk 2.2.1+119(191122) (использовал его для создания этого проекта)
* SSL Support: Basic SSL ciphers (меньшее использование ПЗУ)
* VTables: Flash (неважно, может быть)
* Erase Flash: выберите `All Flash Contents` при первой загрузке
* CPU Frequency: 160MHz (необходимо)

## Порт Arduino 

* `ESP8266WiFi` (WiFiServer и WiFiClient) используется для TCP-подключения.
* `ESP8266mDNS` нужно для Bonjour.

## Поиск проблемы

* Проверьте свой последовательный вывод с помощью [example_serial_output.txt](https://raw.github.com/Mixiaoxiao/Arduino-HomeKit-ESP8266/master/extras/example_serial_output_v1.1.0.txt)

## Журнал изменений

#### v1.4.0

* Добавлен `yield()` во время криптографических вычислений, чтобы предотвратить отключение Wi-Fi. Идея взята у [BbIKTOP-issues80](https://github.com/Yurik72/ESPHap/issues/80#issuecomment-803685175)
* Добавлен один новый пример.

#### v1.3.0

* Небольшие улучшения.

#### v1.2.0

* Новые примеры.

#### v1.1.0

* Оптимизация памяти: максимально возможное перемещение строковых/байтовых констант во Flash. Раздел `RODATA` файла `bin` имеет размер всего 4672. Доступно около 20 КБ свободной кучи по сравнению с версией 1.0.1.
* Загруженны [ESP8266WiFi_nossl_noleak](https://github.com/Mixiaoxiao/Arduino-HomeKit-ESP8266/tree/master/extras/ESP8266WiFi_nossl_noleak/), версии `nossl` и `noleak` официальной библиотеки `ESP8266WiFi` для Arduino Core 2.6.3. Удалены все коды `SSL` для экономии памяти (дополнительно ~ 3 КБ), поскольку HomeKit не требует SSL. Исправьте утечку памяти в `WiFiClinet.stop()`, добавив `tcp_abandon(_pcb, 0)` в `stop()`, основываясь на идее [esp8266/Arduino/pull/2767](https://github.com/esp8266/Arduino/pull/2767).

#### v1.0.1 
* Уменьшен `winsize` с `3` до `2` (такая же производительность), чтобы уменьшить требуемую heap-память. Сопряжение может быть выполнено с низкой свободной heap-памятью ~14000.
* Теперь MDNS работает на IP-адресе STA, чтобы гарантировать, что HomeKit может работать с некоторыми библиотеками WiFi-Config на основе SoftAP.
* Переименованы `HTTP_METHOD`(ы) в `http_parser.h`, чтобы избежать ошибок множественного определения при совместном использовании `ESP8266WebServer`.

## Спасибо этим проектам
* [esp-homekit](https://github.com/maximkulkin/esp-homekit)
* [esp-homekit-demo](https://github.com/maximkulkin/esp-homekit-demo)
* [esp_hw_wdt](https://github.com/ComSuite/esp_hw_wdt)
* [WolfSSL/WolfCrypt](https://www.wolfssl.com/products/wolfcrypt-2/)
* [cJSON](https://github.com/DaveGamble/cJSON)
* [cQueue](https://github.com/SMFSW/cQueue)

