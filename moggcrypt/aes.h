/*
 * Code originally from https://github.com/kokke/tiny-AES128-C
 * Modified for our purposes.
 */
#ifndef _AES_H_
#define _AES_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef union {
	uint64_t qwords[2];
	uint32_t dwords[4];
	uint16_t words[8];
	uint8_t bytes[16];
} aes_ctr_128;

void AES128_ECB_encrypt(const uint8_t* input, const uint8_t* key, uint8_t *output);
void AES128_ECB_decrypt(const uint8_t* input, const uint8_t* key, uint8_t *output);

#ifdef __cplusplus
}
#endif

#endif