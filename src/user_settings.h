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
#include "osapi.h"
#include "homekit_debug.h"

static inline int hwrand_generate_block(uint8_t *buf, size_t len) {
	os_get_random(buf, len);
    return 0;
}

#define WC_NO_HARDEN
#define NO_WOLFSSL_DIR
#define SINGLE_THREADED
#define WOLFSSL_LWIP
#define NO_INLINE

#define NO_WOLFSSL_MEMORY
#define MP_LOW_MEM

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
#define NO_SHA
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

#if defined(ARDUINO_HOMEKIT_LOWROM)

#define CURVE25519_SMALL
#define ED25519_SMALL  //关联ED25519，关闭这个之后编译dram会超（stack mem太大）
//`.bss' is not within region `dram0_0_seg'

#else
// crypto.c  crypto_ed25519_verify
// No sufficient ram to run ge_double_scalarmult_vartime in ge_operations.c
// Run the ge_double_scalarmult_vartime in ge_low_mem
#define ESP_GE_DOUBLE_SCALARMULT_VARTIME_LOWMEM

#endif

#endif
