#ifndef ARDUINO_HOMEKIT_SERVER_H_
#define ARDUINO_HOMEKIT_SERVER_H_


#include <WiFiServer.h>
#include <WiFiClient.h>
#include <string.h> //size_t

#ifdef __cplusplus
extern "C" {
#endif

#include "constants.h"
#include "base64.h"
#include "crypto.h"
#include "pairing.h"
#include "storage.h"
#include "query_params.h"
#include "json.h"
#include "homekit_debug.h"
#include "port.h"
#include "cQueue.h"
#include "homekit/homekit.h"
#include "http_parser.h"

typedef enum {
	HOMEKIT_CLIENT_STEP_NONE = 0,
	HOMEKIT_CLIENT_STEP_PAIR_SETUP_1OF3,//1, need receive data
	HOMEKIT_CLIENT_STEP_PAIR_SETUP_2OF3,//2, need receive data
	HOMEKIT_CLIENT_STEP_PAIR_SETUP_3OF3,//3, need receive data
	HOMEKIT_CLIENT_STEP_PAIR_VERIFY_1OF2,//4, need receive data
	HOMEKIT_CLIENT_STEP_PAIR_VERIFY_2OF2,//5, secure session established
	//HOMEKIT_CLIENT_STEP_IDLE,//
	//HOMEKIT_CLIENT_STEP_PAIRINGS, // not used currently
	HOMEKIT_CLIENT_STEP_END // disconnected
} homekit_client_step_t;

typedef enum {
	HOMEKIT_ENDPOINT_UNKNOWN = 0,
	HOMEKIT_ENDPOINT_PAIR_SETUP,
	HOMEKIT_ENDPOINT_PAIR_VERIFY,
	HOMEKIT_ENDPOINT_IDENTIFY,
	HOMEKIT_ENDPOINT_GET_ACCESSORIES,
	HOMEKIT_ENDPOINT_GET_CHARACTERISTICS,
	HOMEKIT_ENDPOINT_UPDATE_CHARACTERISTICS,
	HOMEKIT_ENDPOINT_PAIRINGS,
	HOMEKIT_ENDPOINT_RESOURCE,
} homekit_endpoint_t;

struct _client_context_t;
typedef struct _client_context_t client_context_t;

typedef struct {
	Srp *srp;
	byte *public_key;
	size_t public_key_size;

	client_context_t *client;
} pairing_context_t;

typedef struct {
	byte *secret;
	size_t secret_size;
	byte *session_key;
	size_t session_key_size;
	byte *device_public_key;
	size_t device_public_key_size;
	byte *accessory_public_key;
	size_t accessory_public_key_size;
} pair_verify_context_t;

typedef struct {
	WiFiServer *wifi_server;
	char accessory_id[ACCESSORY_ID_SIZE + 1];
	ed25519_key accessory_key;

	homekit_server_config_t *config;

	bool paired;
	pairing_context_t *pairing_context;

	//int listen_fd;
	//fd_set fds;
	//int max_fd;
	int nfds;// arduino homekit uses this to record client count

	client_context_t *clients;
} homekit_server_t;

typedef struct {
	homekit_characteristic_t *characteristic;
	homekit_value_t value;
} characteristic_event_t;

struct _client_context_t {
	homekit_server_t *server;
	//int socket;
	WiFiClient *socket; //new and delete

	homekit_endpoint_t endpoint;
	query_param_t *endpoint_params;

	byte data[1024 + 18];
	size_t data_size;
	size_t data_available;

	char *body;
	size_t body_length;
	http_parser parser;

	int pairing_id;
	byte permissions;

	bool disconnect;

	homekit_characteristic_t *current_characteristic;
	homekit_value_t *current_value;

	bool encrypted;
	byte read_key[32];
	byte write_key[32];
	int count_reads;
	int count_writes;

	//QueueHandle_t event_queue;
	Queue_t *event_queue;
	pair_verify_context_t *verify_context;

	homekit_client_step_t step; // WangBin added
	bool error_write; // WangBin added

	struct _client_context_t *next;
};



typedef enum {
	TLVType_Method = 0,        // (integer) Method to use for pairing. See PairMethod
	TLVType_Identifier = 1,    // (UTF-8) Identifier for authentication
	TLVType_Salt = 2,          // (bytes) 16+ bytes of random salt
	TLVType_PublicKey = 3,     // (bytes) Curve25519, SRP public key or signed Ed25519 key
	TLVType_Proof = 4,         // (bytes) Ed25519 or SRP proof
	TLVType_EncryptedData = 5, // (bytes) Encrypted data with auth tag at end
	TLVType_State = 6,         // (integer) State of the pairing process. 1=M1, 2=M2, etc.
	TLVType_Error = 7,         // (integer) Error code. Must only be present if error code is
							   // not 0. See TLVError
	TLVType_RetryDelay = 8,    // (integer) Seconds to delay until retrying a setup code
	TLVType_Certificate = 9,   // (bytes) X.509 Certificate
	TLVType_Signature = 10,    // (bytes) Ed25519
	TLVType_Permissions = 11,  // (integer) Bit value describing permissions of the controller
							   // being added.
							   // None (0x00): Regular user
							   // Bit 1 (0x01): Admin that is able to add and remove
							   // pairings against the accessory
	TLVType_FragmentData = 13, // (bytes) Non-last fragment of data. If length is 0,
							   // it's an ACK.
	TLVType_FragmentLast = 14, // (bytes) Last fragment of data
	TLVType_Separator = 0xff,
} TLVType;

typedef enum {
	TLVMethod_PairSetup = 1,
	TLVMethod_PairVerify = 2,
	TLVMethod_AddPairing = 3,
	TLVMethod_RemovePairing = 4,
	TLVMethod_ListPairings = 5,
} TLVMethod;

typedef enum {
	TLVError_Unknown = 1,         // Generic error to handle unexpected errors
	TLVError_Authentication = 2,  // Setup code or signature verification failed
	TLVError_Backoff = 3,         // Client must look at the retry delay TLV item and
								  // wait that many seconds before retrying
	TLVError_MaxPeers = 4,        // Server cannot accept any more pairings
	TLVError_MaxTries = 5,        // Server reached its maximum number of
								  // authentication attempts
	TLVError_Unavailable = 6,     // Server pairing method is unavailable
	TLVError_Busy = 7,            // Server is busy and cannot accept a pairing
								   // request at this time
} TLVError;

typedef enum {
	// This specifies a success for the request
	HAPStatus_Success = 0,
	// Request denied due to insufficient privileges
	HAPStatus_InsufficientPrivileges = -70401,
	// Unable to communicate with requested services,
	// e.g. the power to the accessory was turned off
	HAPStatus_NoAccessoryConnection = -70402,
	// Resource is busy, try again
	HAPStatus_ResourceBusy = -70403,
	// Connot write to read only characteristic
	HAPStatus_ReadOnly = -70404,
	// Cannot read from a write only characteristic
	HAPStatus_WriteOnly = -70405,
	// Notification is not supported for characteristic
	HAPStatus_NotificationsUnsupported = -70406,
	// Out of resources to process request
	HAPStatus_OutOfResources = -70407,
	// Operation timed out
	HAPStatus_Timeout = -70408,
	// Resource does not exist
	HAPStatus_NoResource = -70409,
	// Accessory received an invalid value in a write request
	HAPStatus_InvalidValue = -70410,
	// Insufficient Authorization
	HAPStatus_InsufficientAuthorization = -70411,
} HAPStatus;

typedef enum {
	characteristic_format_type = (1 << 1),
	characteristic_format_meta = (1 << 2),
	characteristic_format_perms = (1 << 3),
	characteristic_format_events = (1 << 4),
} characteristic_format_t;

typedef struct _client_event {
	const homekit_characteristic_t *characteristic;
	homekit_value_t value;

	struct _client_event *next;
} client_event_t;

#define ISDIGIT(x) isdigit((unsigned char)(x))
#define ISBASE36(x) (isdigit((unsigned char)(x)) || (x >= 'A' && x <= 'Z'))


void arduino_homekit_setup(homekit_server_config_t *config);
void arduino_homekit_loop();

homekit_server_t * arduino_homekit_get_running_server();
int arduino_homekit_connected_clients_count();

#ifdef __cplusplus
}
#endif
#endif /* ARDUINO_HOMEKIT_SERVER_H_ */
