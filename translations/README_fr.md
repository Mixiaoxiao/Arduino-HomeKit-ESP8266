# Arduino HomeKit ESP8266 


[中文说明](README_cn.md) | [Português Brasileiro](README-ptbr.md) | [Français](README-fr.md)


## Bibliothèque de serveur d'accessoires HomeKit d'Apple pour Arduino ESP8266

Cette bibliothèque Arduino est une implémentation native de l'accessoire HomeKit d'Apple pour le... [ESP8266 Arduino core](https://github.com/esp8266/Arduino), et fonctionne sans aucun pont supplémentaire.

This project is mainly based on [esp-homekit](https://github.com/maximkulkin/esp-homekit) for [ESP-OPEN-RTOS](https://github.com/SuperHouse/esp-open-rtos).

J'ai porté l'implémentation basée sur RTOS de [esp-homekit](https://github.com/maximkulkin/esp-homekit) vers l'environnement Arduino pur, dans le but de construire facilement et rapidement des projets en utilisant Arduino IDE (ou Eclipse avec sloeber, PlatformIO).

Profitez de la construction "one-key", du téléchargement "one-key", et du travail pour lier diverses autres bibliothèques Arduino avec Apple HomeKit !

Voici une [discussion](https://github.com/HomeACcessoryKid/Arduino-HomeKit/issues/1) sur le RTOS est nécessaire pour exécuter Apple HomeKit, et ce projet est une preuve de concept que Apple HomeKit peut être mis en œuvre et fonctionner correctement sans le RTOS.

Cette bibliothèque est construite avec ESP8266 Arduino Core 2.6.3. Les versions inférieures peuvent compiler avec des erreurs.

Pour ESP32, voir [Arduino-HomeKit-ESP32](https://github.com/Mixiaoxiao/Arduino-HomeKit-ESP32). Le HomeKit fonctionnant sur ESP32 a une **GREATTE PERFORMANCE** qui est 10x plus rapide que ESP8266.


## Prévisualisation

![Preview](https://raw.github.com/Mixiaoxiao/Arduino-HomeKit-ESP8266/master/extras/preview.jpg) 


## Code de configuration du sketch d'exemple:

``111-11-111``


## Utilisation

1. Définissez votre accessoire dans un fichier .c pour profiter de la déclaration pratique de style "Macro". Vous pouvez également définir votre accessoire dans un fichier .ino en utilisant du code C++.
	```C
		homekit_accessory_t *accessories[] = ...
		homekit_server_config_t config = {
			.accessories = accessories,
			.password = "111-11-111",
			//.on_event = on_homekit_event, //en option
			//.setupId = "ABCD" //en option
		};
	```
2. Dans votre croquis
	```C
		#include <arduino_homekit_server.h>;
		
		//accéder à la configuration définie dans le code C
		extern "C" homekit_server_config_t config; 
		
		void setup() {
			WiFi.begin(ssid, password);
			arduino_homekit_setup(&config);
		}
		
		void loop() {
			arduino_homekit_loop();
		}
	```
Fait.

## Performance

Remarque : Vous devez régler le CPU de l'ESP8266 pour qu'il fonctionne à 160 MHz (au moins pendant le processus de couplage), afin d'éviter la déconnexion du socket tcp de l'appareil iOS causée par un dépassement de délai.
&
* Preinit : ~9.1s (Vous pouvez voir l'accessoire sur votre application iOS HOME après Preinit)
* Pair Setup Step 1/3 : ~0s (Le calcul cryptographique lourd est fait dans Preinit)
* Installation du couple, étape 2/3 : ~12.1s 
* Configuration de l'appairage, étape 3/3 : ~0,8 s (La configuration de l'appairage n'est traitée que lors du premier appairage avec l'appareil iOS).
* Vérification du jumelage, étape 1/2 : ~0,3 s
* Vérification de l'appairage, étape 2/2 : ~0,8 s (l'étape de vérification est nécessaire chaque fois que l'iOS se connecte ou se reconnecte à l'ESP8266 pour établir une session sécurisée).

L'ensemble du processus d'appairage prend environ 14 secondes après la saisie du code d'installation sur votre iPhone. Notez que Preinit nécessite ~9s avant de pouvoir commencer le couplage.


## Heap (mémoire)

Le tas est essentiel pour l'ESP8266 avec un support TCP/IP complet. L'ESP8266 se plante facilement lorsque la mémoire est inférieure à ~5000.

J'ai essayé de faire fonctionner WolfSSL crypto en toute sécurité sur ESP8266 avec de meilleures performances et une mémoire plus faible ou un compromis. Voir les détails dans la section suivante.

Voici les valeurs du tas libre de l'exécution du sketch d'exemple :

* Boot : ~26000
* Preinit over : ~22000
* Appariement : ~17000 (ou même plus bas avec le crypto-computing)
* Appairage et connexion avec un appareil iOS : ~21700
* Appairage et aucun appareil iOS connecté : ~23400

Après l'optimisation de la mémoire dans la v1.1.0 :

* Boot : ~46000
* Préinitialisation : ~41000
* Appariement : ~37000 (ou même plus bas avec le crypto-computing)
* Appairage et connexion avec un appareil iOS : ~41700
* Appairage et aucun appareil iOS connecté : ~43000


## WolfSSL

* Basé sur wolfssl-3.13.0-stable.
* Code source propre : les fichiers inutilisés sont supprimés.
* `CURVE25519_SMALL` et `ED25519_SMALL` : ESP8266 ne peut pas fonctionner directement sans `SMALL` défini car la mémoire n'est pas suffisante. Mais la version NO `SMALL` est plus rapide. Je marque le gros `ge_precomp base[32][8]` avec PROGMEM pour le stocker en Flash (environ 70KB). Aussi le `ge_double_scalarmult_vartime` ne peut pas fonctionner à cause du manque de heap. Je définis `ESP_GE_DOUBLE_SCALARMULT_VARTIME_LOWMEM` dans `user_settings.h` pour utiliser la version LOWMEM de `ge_double_scalarmult_vartime` dans `ge_low_mem.c`. C'est un compromis entre les performances et la mémoire. Si vous voulez plus d'espace Flash, vous devriez définir `CURVE25519_SMALL` et `ED25519_SMALL` et undefine `ESP_GE_DOUBLE_SCALARMULT_VARTIME_LOWMEM` dans `user_settings.h` (cela conduira les étapes de vérification de la paire à prendre 1.2s + 0.9s).
* `integer.c`(opérations sur les grands nombres entiers) : `MP_16BIT` et `ESP_FORCE_S_MP_EXPTMOD` sont définis pour de meilleures performances dans ESP8266. `ESP_INTEGER_WINSIZE` (la valeur est 3) est défini pour éviter un crash causé par l'épuisement de la mémoire et les valeurs de {3, 4, 5} ont des performances similaires.

## Stockage

* Les données d'appairage sont stockées dans l'adresse `EEPROM` du noyau Arduino ESP8266.
* Ce projet n'utilise pas la bibliothèque `EEPROM` avec cache de données pour réduire l'utilisation de la mémoire (appel direct de flash_read et write). 
* Le `EEPROM` est de 4096B dans l'ESP8266, ce projet utilise max [0, 1408B).
* Voir les commentaires dans `storge.c` et [ESP8266-EEPROM-doc](https://arduino-esp8266.readthedocs.io/en/2.6.3/libraries.html#eeprom).
* Une `EEPROM` de [1408, 4096] est sans danger pour vous. 
* Ce projet n'utilise pas `FS(file system)`, vous pouvez donc utiliser `FS` librement.


## Chien de garde

* Il y a des chiens de garde logiciels et matériels dans le noyau de l'Arduino ESP8266. Les calculs cryptographiques lourds conduiront à la réinitialisation des chiens de garde.
* Il y a des api de désactivation/activation de chien de garde logiciel dans le noyau de l'ESP8266 Arduino.
* J'ai trouvé le [esp_hw_wdt](https://github.com/ComSuite/esp_hw_wdt) pour désactiver/activer le chien de garde matériel.
* Les deux chiens de garde sont désactivés pendant `Preinit` et `Pair Setup Step 2/3`.

## Paramètres recommandés dans l'IDE

* Module : Module ESP8266 générique (pour activer tous les paramètres)
* FlashSize : au moins 470KB pour le sketch (voir la section `WolfSSL` si vous voulez un sketch plus petit) 
* Variante LwIP : v2 Lower Memory (pour une utilisation plus faible de la mémoire)
* Niveau de débogage : None (pour une utilisation moindre de la mémoire)
* Espressif FW : nonos-sdk 2.2.1+119(191122) (que j'ai utilisé pour construire ce projet)
* Support SSL : Chiffres SSL de base (utilisation réduite de la ROM)
* VTables : Flash (n'a pas d'importance peut-être)
* Effacement de la mémoire Flash : sélectionnez "All Flash Contents" lors du premier téléchargement.
* Fréquence du CPU : 160MHz (obligatoire)

## Port Arduino

* `ESP8266WiFi` (WiFiServer et WiFiClient) est utilisé pour la connexion tcp.
* `ESP8266mDNS` est utilisé pour la publicité (Bonjour) 

## Dépannage

* Vérifiez votre sortie série avec [example_serial_output.txt](https://raw.github.com/Mixiaoxiao/Arduino-HomeKit-ESP8266/master/extras/example_serial_output_v1.1.0.txt)


## Journal des modifications

#### v1.4.0

* Ajout de `yield()` pendant le crypto calcul, pour éviter la déconnexion du WiFi. L'idée vient de [BbIKTOP-issues80](https://github.com/Yurik72/ESPHap/issues/80#issuecomment-803685175)
* Un nouvel exemple.

#### v1.3.0

* Petites améliorations.

#### v1.2.0

* Nouveaux exemples.

#### v1.1.0

* Optimisation de la mémoire : déplacement des constantes String/byte autant que possible vers Flash. La section `RODATA` de `bin` est seulement 4672. Un espace libre supplémentaire de ~20K est disponible par rapport à la v1.0.1.
* Mise en ligne de [ESP8266WiFi_nossl_noleak](https://github.com/Mixiaoxiao/Arduino-HomeKit-ESP8266/tree/master/extras/ESP8266WiFi_nossl_noleak/), une version `nossl` et `noleak` de la bibliothèque officielle `ESP8266WiFi` de Arduino Core 2.6.3. Suppression de tous les codes de `SSL` pour économiser de la mémoire (~3K supplémentaires) puisque le HomeKit ne requiert pas SSL. Correction de la fuite de mémoire dans `WiFiClinet.stop()` en ajoutant `tcp_abandon(_pcb, 0)` dans `stop()`, basé sur l'idée de [esp8266/Arduino/pull/2767](https://github.com/esp8266/Arduino/pull/2767).

#### v1.0.1 
* Réduire la taille de la fenêtre de 3 à 2 (même performance) pour diminuer le tas nécessaire. Le jumelage peut être fait avec un faible heap libre de ~14000.
* Spécifier le MDNS fonctionne sur l'adresse IPAddress de STA pour s'assurer que le HomeKit peut fonctionner avec certaines bibliothèques WiFi-Config basées sur SoftAP.
* Renommez les `HTTP_METHOD`(s) dans `http_parser.h` pour éviter les erreurs de multi-définition lors de l'utilisation conjointe de `ESP8266WebServer`.


## Thanks
* [esp-homekit](https://github.com/maximkulkin/esp-homekit)
* [esp-homekit-demo](https://github.com/maximkulkin/esp-homekit-demo)
* [esp_hw_wdt](https://github.com/ComSuite/esp_hw_wdt)
* [WolfSSL/WolfCrypt](https://www.wolfssl.com/products/wolfcrypt-2/)
* [cJSON](https://github.com/DaveGamble/cJSON)
* [cQueue](https://github.com/SMFSW/cQueue)

