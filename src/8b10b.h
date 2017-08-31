#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
	/* current running disparity */
	enum {RD_NEG = 0, RD_POS = 1} __attribute__((packed)) rd;
	/* current encoding */
	enum {FIVEBSIXB = 0, THREEBFOURB = 1} __attribute__((packed)) coding;
	/* encoded data pointer */
	uint8_t *data, *dataPos;
	/* current position */
	uint8_t bitPos;
} eightbtenbCtx;

void eightbtenbEncode (eightbtenbCtx * const ctx, const uint8_t * const data,
		const size_t size);
bool eightbtenbDecode (eightbtenbCtx * const ctx, const uint8_t * const data,
		const size_t size);
void eightbtenbInit (eightbtenbCtx * const ctx);
void eightbtenbSetDest (eightbtenbCtx * const ctx, uint8_t * const dest);

