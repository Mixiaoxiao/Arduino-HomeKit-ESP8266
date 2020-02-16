#ifndef __PAIRING_H__
#define __PAIRING_H__

#include "constants.h"
#include "crypto.h"


typedef enum {
    pairing_permissions_admin = (1 << 0),
} pairing_permissions_t;

typedef struct {
    int id;
    char device_id[DEVICE_ID_SIZE + 1];
    ed25519_key device_key;
    pairing_permissions_t permissions;
} pairing_t;

#endif // __PAIRING_H__
