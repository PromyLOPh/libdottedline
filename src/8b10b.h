/*
Copyright (c) 2015–2018 Lars-Dominik Braun <lars@6xq.net>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

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

