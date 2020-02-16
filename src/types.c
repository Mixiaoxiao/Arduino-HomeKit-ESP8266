#include "homekit/types.h"

//=========================
// Compat for cplusplus
//=========================

homekit_value_t HOMEKIT_DEFAULT_CPP() {
	homekit_value_t homekit_value;
	//homekit_value.is_null = false;//该值为默认，不用设置
	return homekit_value;
}

homekit_value_t HOMEKIT_NULL_CPP() {
	homekit_value_t homekit_value;
	homekit_value.is_null = true;
	return homekit_value;
}

homekit_value_t HOMEKIT_BOOL_CPP(bool value) {
	homekit_value_t homekit_value = HOMEKIT_DEFAULT_CPP();
	homekit_value.format = homekit_format_bool;
	homekit_value.bool_value = value;
	return homekit_value;
}

homekit_value_t HOMEKIT_INT_CPP(uint8_t value) {
	homekit_value_t homekit_value = HOMEKIT_DEFAULT_CPP();
	homekit_value.format = homekit_format_int;
	homekit_value.int_value = value;
	return homekit_value;
}

homekit_value_t HOMEKIT_UINT8_CPP(uint8_t value) {
	homekit_value_t homekit_value = HOMEKIT_DEFAULT_CPP();
	homekit_value.format = homekit_format_uint8;
	homekit_value.uint8_value = value;
	return homekit_value;
}

homekit_value_t HOMEKIT_UINT16_CPP(uint16_t value) {
	homekit_value_t homekit_value = HOMEKIT_DEFAULT_CPP();
	homekit_value.format = homekit_format_uint16;
	homekit_value.uint16_value = value;
	return homekit_value;
}

homekit_value_t HOMEKIT_UINT32_CPP(uint32_t value) {
	homekit_value_t homekit_value = HOMEKIT_DEFAULT_CPP();
	homekit_value.format = homekit_format_uint32;
	homekit_value.uint32_value = value;
	return homekit_value;
}

homekit_value_t HOMEKIT_UINT64_CPP(uint64_t value) {
	homekit_value_t homekit_value = HOMEKIT_DEFAULT_CPP();
	homekit_value.format = homekit_format_uint64;
	homekit_value.uint64_value = value;
	return homekit_value;
}

homekit_value_t HOMEKIT_FLOAT_CPP(float value) {
	homekit_value_t homekit_value = HOMEKIT_DEFAULT_CPP();
	homekit_value.format = homekit_format_float;
	homekit_value.float_value = value;
	return homekit_value;
}

homekit_value_t HOMEKIT_STRING_CPP(char *value) {
	homekit_value_t homekit_value = HOMEKIT_DEFAULT_CPP();
	homekit_value.format = homekit_format_string;
	homekit_value.string_value = value;
	return homekit_value;
}

homekit_value_t HOMEKIT_TLV_CPP(tlv_values_t *value) {
	homekit_value_t homekit_value = HOMEKIT_DEFAULT_CPP();
	homekit_value.format = homekit_format_tlv;
	homekit_value.tlv_values = value;
	return homekit_value;
}

homekit_value_t HOMEKIT_DATA_CPP(uint8_t *value, size_t size) {
	homekit_value_t homekit_value = HOMEKIT_DEFAULT_CPP();
	homekit_value.format = homekit_format_data;
	homekit_value.data_value = value;
	homekit_value.data_size = size;
	return homekit_value;
}

