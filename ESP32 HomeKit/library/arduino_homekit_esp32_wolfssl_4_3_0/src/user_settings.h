#ifndef wolfcrypt_user_settings_h
#define wolfcrypt_user_settings_h

#define WOLFSSL_USER_SETTINGS

//#define ARDUINO_HOMEKIT_LOWROM

//skip ed25519_verify, see crypto_ed25519_verify in crypto.c
//Pair Verify Step 2/2: skip=35ms, not-skip=794ms
//#define ARDUINO_HOMEKIT_SKIP_ED25519_VERIFY

#include "stdint.h"
#include "stddef.h"
#include "stdlib.h"

#include "homekit_debug.h"

#if !defined(ARDUINO_ARCH_ESP8266) && !defined(ARDUINO_ARCH_ESP32)
#error HomeKit library only supports Arduino framework of ESP8266 and ESP32!
#endif

#if defined(ARDUINO_ARCH_ESP8266)
#include "osapi.h"
static inline int hwrand_generate_block(uint8_t *buf, size_t len) {
	os_get_random(buf, len);
    return 0;
}

#elif defined(ARDUINO_ARCH_ESP32)
#include <esp_system.h>
static inline int hwrand_generate_block(uint8_t *buf, size_t len) {
	esp_fill_random(buf, len);
    return 0;
}
#endif


#define WC_NO_HARDEN
#define NO_WOLFSSL_DIR
#define SINGLE_THREADED
#define WOLFSSL_LWIP
#define NO_INLINE

#define NO_WOLFSSL_MEMORY

#define CUSTOM_RAND_GENERATE_BLOCK hwrand_generate_block

//==========
// added
//==========
#define WOLFCRYPT_ONLY

#define NO_ASN
#define NO_AES
#define NO_RC4
#define NO_RSA
#define NO_SHA256
#define NO_DH
#define NO_DSA
#define NO_CODING
//#define NO_SHA // Use SHA512
#define NO_MD5

#define HAVE_CURVE25519
#define HAVE_CHACHA
#define HAVE_POLY1305
#define HAVE_ED25519
#define WOLFSSL_SHA512
#define WOLFCRYPT_HAVE_SRP
#define HAVE_HKDF

#define WC_NO_HASHDRBG

#define WOLFSSL_BASE64_ENCODE

#if defined(ARDUINO_ARCH_ESP8266)

#define MP_LOW_MEM
//see integer.c
//default winsize=5(MP_LOW_MEM), but ram(heap) is not sufficient!
//winsize of {2,3,4,5} are same performance
//lower winsize, lower ram required
#define ESP_INTEGER_WINSIZE 2
//winsize=3 & mp_exptmod_fast : ram(heap) is not sufficient
//force use s_mp_exptmod (lower memory), and smiller performance with mp_exptmod_fast
#define ESP_FORCE_S_MP_EXPTMOD
//winsize = 5 & mp_exptmod_fast 最快，Pair Verify Step 2/2 = 10s左右
//winsize = 6 heap不够


#define MP_16BIT //faster than 32bit in ESP8266

#endif

//#define MP_16BIT //preinit took    7181ms
//#define MP_28BIT (default)  //preinit took    2285ms
//#define MP_31BIT   //preinit took    2283ms
//#define MP_64BIT  //preinit took     350ms //Crypto error, ESP32 NOT support 64bit
//#define ESP_FORCE_S_MP_EXPTMOD //preinit took    2231ms

#if defined(ARDUINO_HOMEKIT_LOWROM)

#define CURVE25519_SMALL
#define ED25519_SMALL  //关联ED25519，关闭这个之后编译dram会超（stack mem太大）
//`.bss' is not within region `dram0_0_seg'

#else

#if defined(ARDUINO_ARCH_ESP8266)
// crypto.c  crypto_ed25519_verify
// No sufficient ram to run ge_double_scalarmult_vartime in ge_operations.c
// Run the ge_double_scalarmult_vartime in ge_low_mem
#define ESP_GE_DOUBLE_SCALARMULT_VARTIME_LOWMEM
#endif

#if defined(ARDUINO_ARCH_ESP32)
//#define HOMEKIT_ESP32_HW_CRYPTO  //See wolfcrypt/src/integer.c
//#define USE_FAST_MATH
//#define WOLFSSL_ESPWROOM32
//#define WOLFSSL_ESPIDF

// copy from WOLFSSL_ESPIDF, but NOT define FREERTOS

#define NO_WRITEV
#define SIZEOF_LONG_LONG 8
#define NO_WOLFSSL_DIR
#define WOLFSSL_NO_CURRDIR

#define TFM_TIMING_RESISTANT
#define ECC_TIMING_RESISTANT
#define WC_RSA_BLINDING

#define HAVE_ECC // to enable esp hw mp-functions (big integer). See esp32-crypt.h
#undef  NO_SHA   // to enable esp hw sha-functions.

//#include "wolfssl/wolfcrypt/types.h"

//#include "wolfssl/wolfcrypt/wc_port.h"
//#include "wolfssl/wolfcrypt/port/Espressif/esp32-crypt.h"


#define WOLFSSL_ESPWROOM32
#define ESP32_USE_RSA_PRIMITIVE

#if defined(WOLFSSL_ESPWROOM32) || defined(WOLFSSL_ESPWROOM32SE)
   #ifndef NO_ESP32WROOM32_CRYPT
        #define WOLFSSL_ESP32WROOM32_CRYPT
        #if defined(ESP32_USE_RSA_PRIMITIVE) && \
            !defined(NO_WOLFSSL_ESP32WROOM32_CRYPT_RSA_PRI)
            #define WOLFSSL_ESP32WROOM32_CRYPT_RSA_PRI
            #define USE_FAST_MATH
            #define WOLFSSL_SMALL_STACK
        #endif
   #endif
#endif

/* Define USE_FAST_MATH and SMALL_STACK                        */
#define ESP32_USE_RSA_PRIMITIVE
/* threshold for performance adjustment for hw primitive use   */
/* X bits of G^X mod P greater than                            */
#define EPS_RSA_EXPT_XBTIS           36
/* X and Y of X * Y mod P greater than                         */
#define ESP_RSA_MULM_BITS            2000

// Fix Error: Failed to dump SPR public key (code -1)
#define FP_MAX_BITS (4096 * 2) // HomeKit need larger integer-bits // See tfm.h and tfm.c (fp_exptmod)
//==========================

#if !defined(ICACHE_RODATA_ATTR)
#define ICACHE_RODATA_ATTR   //Empty, compatibility with ESP8266, used in ge_operations.c

#endif
#endif

#endif

#endif
