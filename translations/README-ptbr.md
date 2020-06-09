# Arduino HomeKit ESP8266 (PT-BR) 

**Original:** [Inglês](https://github.com/Mixiaoxiao/Arduino-HomeKit-ESP8266)

**Tradutor:** [Rodolfo Vieira](https://github.com/rodolfovieira95)
## Biblioteca para ESP8266/ESP32 Arduino de um server de acessório para o Apple HomeKit 

Essa biblioteca do Arduino é uma implementação para um acessório nativo do Apple HomeKit[ESP8266 Arduino core](https://github.com/esp8266/Arduino), e funciona sem uma "bridge" (ponte) adicional.

Este projeto é principalmente baseado no [esp-homekit](https://github.com/maximkulkin/esp-homekit) para [ESP-OPEN-RTOS](https://github.com/SuperHouse/esp-open-rtos).

Eu portei a implementação do RTOS-based do [esp-homekit](https://github.com/maximkulkin/esp-homekit) para o ambiente puro do Arduino, focado em uma build de projeto fácil e rápida usando a IDE do Arduino (ou Eclipse com sloeber, PlatformIO).

Aproveite a build "one-key", "one-key" upload, e trabalhe ligando diversas outras bibliotecas do Arduino com Apple HomeKit!

Aqui está a [discussão](https://github.com/HomeACcessoryKid/Arduino-HomeKit/issues/1) sobre a necessidade do RTOS para implementar o Apple HomeKit, e o projeto é a prova de conceito de que o Apple HomeKit pode ser implementado e funciona sem o RTOS.

A biblioteca foi feita com o ESP8266 Arduino Core 2.6.3. Versões anteriores podem compilar com erros.


## ESP32 agora é suportado (2020-04-15)

Confira a pasta do "ESP32 HomeKit"

Isso é a versão "only-can-work" (apenas-pode-funcionar) para o ESP32 sem o merge na biblioteca original. Ainda há algumas melhorias a serem realizadas.

O `WolfSSL` usado para o ESP32 é baseado na versão `4.3.0-stable` sem **Suporte a Aceleração de Hardware**.

O HomeKit rodando no ESP32 tem uma **BOA PERFORMANCE** em que a configuração do pareamento pode ser feito em até ˜1,2s e a verificação do pareamento em < 0,1s (10x mais rápido que o ESP8266).

O armazenamento do HomeKit no ESP32 é baseado no `nvs`.

#### Performance COM a Aceleração de Hardware no ESP32

* Pré-inicialização: ~0.53s
* Passo de Configuração do Pareamento 1/3: ~0s (A criptografia pesada é computada no passo de pré-inicialização)
* Passo de Configuração do Pareamento 2/3: ~0.53s 
* Passo de Configuração do Pareamento 3/3: ~0.20s 
* Passo de Verificação do Pareamente 1/2: ~0.05s
* Passo de Verificação do Pareamente 2/2: ~0.02s

#### Performance SEM a Aceleração de Hardware no ESP32

* Pré-inicialização: ~2.2s
* Passo de Configuração 1/3: ~0s (A criptografia pesada é computada no passo de pré-inicialização)
* Passo de Configuração do Pareamento 2/3: ~2.5s 
* Passo de Configuração do Pareamento 3/3: ~0.1s 
* Passo de Verificação 1/2 do Pareamente: ~0.06s
* Passo de Verificação 2/2 do Pareamente: ~0.03s

## O conteúdo a seguir é apenas para o ESP8266

Este readme será refatorado na próxima versão.

## Preview

![Preview](https://raw.github.com/Mixiaoxiao/Arduino-HomeKit-ESP8266/master/extras/preview.jpg) 


## Código de configuração do sketch de exemplo

``111-11-111``


## Usabilidade

1. Defina o seu acessório em um arquivo .c e aproveite a declaração da "Macro" convenientementeto estilizada. Você pode definir seu acessório em um arquivo .ino usando código C++.

    ```C
		homekit_accessory_t *accessories[] = ...
		homekit_server_config_t config = {
			.accessories = accessories,
			.password = "111-11-111",
			//.on_event = on_homekit_event, //opcional
			//.setupId = "ABCD" //opcional
		};
	```

2. No seu sketch
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
Pronto.

## Performance

Aviso: Você deve definir a CPU do ESP8266 rodando em 160MHz (ao menos durante o processo de pareamento), para evitar que o tcp-socket disconecte do dispositivo iOS causado pelo timeout.

* Pré-inicialização: ~9,1s (Você pode ver o acessório no seu aplicativo CASA para iOS após a Pré-inicialização)
* Passo de Configuração do Pareamento 1/3: ~0s (A criptografia pesada é computada no passo de pré-inicialização)
* Passo de Configuração do Pareamento 2/3: ~12.1s 
* Passo de Configuração do Pareamento 3/3: ~0.8s  (A configuração de pareamento apenas é precessada quando pareado a primeira vez com o dispositivo iOS)
* Passo de Verificação do Pareamente 1/2: ~0.3s
* Passo de Verificação do Pareamente 2/2: ~0.8s (O passo de verificação é requerido toda vez que o iOS conecta ou reconecta com o ESP8266 para estabelecer uma conexão segura)

Todo processo de pareamento leva ~14s após você inserir o código de configuração no seu iPhone. Note que a Pré-inicialização requer ~9s antes de iniciar o pareamento.


## Heap (memória)

O heap é critico para o suporte do ESP8266 com TCP/IP completo. O ESP8266 trava facilmente quando a memória é inferior a ~5000.

Eu tentei fazer a criptografia do WolfSSL funcionar no ESP8266 com uma performance melhor e com um trade-off de memória menor. Veja em detalhes na próxima seção.

Aqui estão os valores de heap livres rodando o sketch:

* Inicialização (Boot): ~26000
* Durante a pré-inicialização: ~22000
* Pareamento: ~17000 (or even low when crypto computing)
* Pareado e conectado com o dispositivo iOS: ~21700
* Pareado e sem um device iOS conectado: ~23400

Após a otimização da memória na versão v1.1.0:

* Inicialização (Boot): ~46000
* Durante a pré-inicialização: ~41000
* Pareamento: ~37000 (ou ainda menor com criptografia computacional)
* Pareado e conectado com o dispositivo iOS: ~41700
* Pareado e sem um device iOS conectado: ~43000


## WolfSSL

* Baseado no wolfssl-3.13.0-stable.
* Código fonte limpo: os arquivos não utilizados foram removidos.
* `CURVE25519_SMALL` e `ED25519_SMALL`: ESP8266 não consegue rodar diretamente sem o `SMALL` definido uma vez que a memória não seja suficiente. Mas a versão NÃO `SMALL` é mais rápida. Eu marquei o grande `ge_precomp base[32][8]` com PROGMEM para o armazenar na Flash (por volta de 70KB). O `ge_double_scalarmult_vartime` também pode não rodar pela falta de heap. Eu defini `ESP_GE_DOUBLE_SCALARMULT_VARTIME_LOWMEM` no `user_settings.h` para usar a versão LOWMEM do `ge_double_scalarmult_vartime` no `ge_low_mem.c`. Isso é um trade-off de performance e memória. Se você quiser mais espaço em Flash, você deve definir `CURVE25519_SMALL` e `ED25519_SMALL` e não definir `ESP_GE_DOUBLE_SCALARMULT_VARTIME_LOWMEM` no `user_settings.h` (isso fará com que os Passos de Verificação vão levar aproximadamente 1.2s + 0.9s)
* `integer.c`(operaçoes com big integer): `MP_16BIT` e `ESP_FORCE_S_MP_EXPTMOD` são definidos para melhorar a performance no ESP8266. `ESP_INTEGER_WINSIZE` (o valor é 3) é definido para evitar o travamento por conta da exaustão da memória e os valores de {3, 4, 5} são de performance similar.

## Armazenamento

* O pareamente de dados é armazenado no endereço da `EEPROM` no ESP8266 Arduino core. 
* Esse projeto não utiliza a biblioteca `EEPROM` com cache de dados para reduzir o uso de memória (chama diretamente a leitura e escrita de flash)
* O `EEPROM` é 4096B no ESP8266, esse projeto utiliza no máximo [0,1408B)
* Veja os comentários no `storge.c` e [ESP8266-EEPROM-doc](https://arduino-esp8266.readthedocs.io/en/2.6.3/libraries.html#eeprom).
* `EEPROM` de [1408, 4096) é mais seguro para você usar. 
* Esse projeto NÃO utiliza `FS(file system)`, então você pode utilizar `FS` livremente.


## WatchDog

* Existem watchdogs via hardware e software no ESP8266 Arduino core. A criptografia computacional pesada levará o watchdog a resetar.
* Existe uma API que habilita/desabilita o watchdog via software no ESP8266 Arduino core.
* Eu encontrei o [esp_hw_wdt](https://github.com/ComSuite/esp_hw_wdt) para habilitar/desabilitar o watchdog via hardware.
* Os dois watchdogs são desabilitados durante a `Pré-inicialização` e `Passo de Configuração do Pareamento 2/3`.

## Recomendação de configurações da IDE

* Placa: Generic ESP8266 Module (para habilitar todas as configurações)
* Flash Size: ao menos 470KB para o sketch (veja a seção `WolfSSL` se quiser um sketch menor) 
* LwIP Variant: v2 Lower Memory (para menor uso de memória)
* Debug Level: Nenhum (para menor uso de memória)
* Espressif FW: nonos-sdk 2.2.1+119(191122) (o qual usei para rodar esse projeto)
* SSL Support: Basic SSL ciphers (menor uso de ROM)
* VTables: Flash (talvez não faça diferença)
* Erase Flash: selecione `All Flash Contents` quando você fizer o upload pela primeira vez
* CPU Frequency: 160MHz (OBRIGATÓRIO)

## Porta do Arduino

* `ESP8266WiFi` (WiFiServer e WiFiClient) são usadas para conexão tcp.
* `ESP8266mDNS` é usado para advertising (Bonjour) 

## TODO

* Versão do ESP32 Arduino (ESP32 Arduino é baseado no RTOS e não é difícil de portar).


## Mais exemplos e demos

* Veja [esp-homekit-demo](https://github.com/maximkulkin/esp-homekit-demo)

## Troubleshooting

* Cheque a saída da sua porta serial com [example_serial_output.txt](https://raw.github.com/Mixiaoxiao/Arduino-HomeKit-ESP8266/master/extras/example_serial_output_v1.1.0.txt)


## Change Log

#### v1.1.0
* Otimização de memória: Constantes String/byte movidas o máximo possível para Flash. A seção `RODATA` do `bin` é de apenas 4672. Aproximadamente ~20K extra de free-heap está disponível nessa versão em comparação com a v1.0.1. 
* Upload [ESP8266WiFi_nossl_noleak](https://github.com/Mixiaoxiao/Arduino-HomeKit-ESP8266/tree/master/extras/ESP8266WiFi_nossl_noleak/), da versão `nossl` e `noleak` oficial da biblioteca `ESP8266WiFi` do Arduino Core 2.6.3. Remoção de todo código `SSL` para salvar memória(extra ~3K) sendo que o HomeKit não requer SSL. Coreção de memory-leak no `WiFiClinet.stop()` ao adicionar `tcp_abandon(_pcb, 0)` no `stop()`, baseado na ideia do[esp8266/Arduino/pull/2767](https://github.com/esp8266/Arduino/pull/2767).

#### v1.0.1 
* Redução do `winsize` de `3` para `2` (mesma performance) para diminuir o heap requirido. O pareamento consegue ser feito com diminuição de free-heap de ~14000.
* Especificando o MDNS roda no IPAddress do STA para certificar que o HomeKit consegue trabalhar com alguma biblioteca SoftAp-based WiFi-Config.
* Renomeado os`HTTP_METHOD`(s) no `http_parser.h` para evitar erros de multipla definição quando estiver usando o `ESP8266WebServer` em conjunto.

## Agradecimentos
* [esp-homekit](https://github.com/maximkulkin/esp-homekit)
* [esp-homekit-demo](https://github.com/maximkulkin/esp-homekit-demo)
* [esp_hw_wdt](https://github.com/ComSuite/esp_hw_wdt)
* [WolfSSL/WolfCrypt](https://www.wolfssl.com/products/wolfcrypt-2/)
* [cJSON](https://github.com/DaveGamble/cJSON)
* [cQueue](https://github.com/SMFSW/cQueue)

