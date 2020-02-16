#ifndef __STORAGE_H__
#define __STORAGE_H__

//#include "EEPROM.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "pairing.h"

int homekit_storage_reset();

int homekit_storage_init();

void homekit_storage_save_accessory_id(const char *accessory_id);
int homekit_storage_load_accessory_id(char *data);

void homekit_storage_save_accessory_key(const ed25519_key *key);
int homekit_storage_load_accessory_key(ed25519_key *key);

bool homekit_storage_can_add_pairing();
int homekit_storage_add_pairing(const char *device_id, const ed25519_key *device_key, byte permissions);
int homekit_storage_update_pairing(const char *device_id, byte permissions);
int homekit_storage_remove_pairing(const char *device_id);
int homekit_storage_find_pairing(const char *device_id, pairing_t *pairing);

typedef struct {
    int idx;
} pairing_iterator_t;


void homekit_storage_pairing_iterator_init(pairing_iterator_t *it);
int homekit_storage_next_pairing(pairing_iterator_t *it, pairing_t *pairing);
void homekit_storage_pairing_iterator_done(pairing_iterator_t *iterator);

#ifdef __cplusplus
}
#endif
#endif // __STORAGE_H__
