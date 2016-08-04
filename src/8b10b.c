/* This is a generic 8b10b implementation, based on lookup tables. The tables
 * have been taken from the SATA 3.0 specification, page 339.
 */

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "8b10b.h"
#include "8b10btables.h"

/* XXX: these can be unified
 */
static uint8_t encode5b6b (eightbtenbCtx * const ctx, const uint8_t input) {
	/* upper bits are unused */
	assert ((input & 0xe0) == 0);
	const uint8_t t = fivebsixbEncTbl[input];
	/* need to invert table entry? */
	uint8_t ret = t;
	if (ctx->rd == RD_NEG && ((t >> invertDataBit) & 0x1) == 1) {
		ret = ~t;
	}
	/* invert rd? */
	ctx->rd ^= (t >> invertRdBit) & 0x1;
	return ret & 0x3f;
}

static uint8_t encode3b4b (eightbtenbCtx * const ctx, const uint8_t input) {
	/* upper bits are unused */
	assert ((input & 0xf8) == 0);
	const uint8_t t = threebfourbEncTbl[input];
	/* need to invert table entry? */
	uint8_t ret = t;
	if (ctx->rd == RD_NEG && ((t >> invertDataBit) & 0x1) == 1) {
		ret = ~t;
	}
	/* invert rd? */
	ctx->rd ^= (t >> invertRdBit) & 0x1;
	return ret & 0x0f;
}

static uint8_t decode5b6b (eightbtenbCtx * const ctx, const uint8_t input) {
	assert ((input & 0xc0) == 0);
	return fivebsixbDecTbl[input];
}

static uint8_t decode3b4b (eightbtenbCtx * const ctx, const uint8_t input) {
	assert ((input & 0xf0) == 0);
	return threebfourbDecTbl[input];
}

/*	Encode size bytes of data
 */
void eightbtenbEncode (eightbtenbCtx * const ctx, const uint8_t * const data,
		const size_t size) {
	/* local versions, saves quite a few loads/stores in the loop */
	uint8_t bitPos = ctx->bitPos;
	uint8_t *dest = ctx->dataPos;

	for (size_t i = 0; i < size; i++) {
		const uint8_t in = data[i];
		const uint16_t encoded = (encode3b4b (ctx, (in >> 5) & 0x7) << 6) |
				encode5b6b (ctx, in & 0x1f);
		*dest |= encoded << bitPos;
		/* with 10 bytes output the 8-bit boundary is always crossed exactly
		 * once */
		++dest;
		*dest = encoded >> (8 - bitPos);
		bitPos = (bitPos + 10) & 0x7;
		/* aligned with byte boundary */
		if (bitPos == 0) {
			++dest;
		}
	}

	ctx->bitPos = bitPos;
	ctx->dataPos = dest;
}

/*	Decode size _bits_ of data
 *	TODO: error detection
 */
bool eightbtenbDecode (eightbtenbCtx * const ctx, const uint8_t * const data,
		const size_t size) {
	/* make sure decoded data is always full 8 bit words */
	assert (size % 10 == 0);
	assert (ctx != NULL);
	assert (data != NULL);

	size_t inpos = 0;
	uint8_t filled = 0;
	/* alignment register */
	uint32_t align = 0;
	while (inpos * 8 < size) {
		/* fill buffer */
		while (filled < 10) {
			//printf ("align: %i: %02x << %i\n", inpos, data[inpos], filled);
			align |= data[inpos] << filled;
			inpos++;
			filled += 8;
		}
		//printf ("align: %04x, fill %i\n", align, filled);

		const uint8_t high = decode3b4b (ctx, (align >> 6) & 0xf);
		const uint8_t low = decode5b6b (ctx, align & 0x3f);
		//printf ("%i/%i decoded to %02x\n", align & 0x3f, (align >> 6) & 0xf, high << 5 | low);
		//assert (high != 0xff);
		//assert (low != 0xff);
		if (high == 0xff || low == 0xff) {
			return false;
		}

		*ctx->dataPos = (high << 5) | low;
		ctx->dataPos++;
		align >>= 10;
		filled -= 10;
	}

	return true;
}

void eightbtenbSetDest (eightbtenbCtx * const ctx, uint8_t * const dest) {
	ctx->data = dest;
	ctx->dataPos = dest;
	ctx->bitPos = 0;
}

/*	Initialize encoding/decoding context
 */
void eightbtenbInit (eightbtenbCtx * const ctx) {
	ctx->rd = RD_NEG;
	ctx->coding = FIVEBSIXB;
	ctx->data = NULL;
	ctx->dataPos = NULL;
	ctx->bitPos = 0;
}

#ifdef _TEST
/* tests */
#define _BSD_SOURCE
#include <check.h>
#include <stdlib.h>

eightbtenbCtx encoder, decoder;
uint8_t encoded[64];
uint8_t decoded[64];

static void testSetup () {
	memset (encoded, 0, sizeof (encoded));
	memset (decoded, 0, sizeof (decoded));
	eightbtenbInit (&encoder);
	eightbtenbSetDest (&encoder, encoded);
	eightbtenbInit (&decoder);
	eightbtenbSetDest (&decoder, decoded);
}

START_TEST (test5b6bEncoder) {
	fail_unless (encode5b6b (&encoder, 0) == 0x39);
	fail_unless (encode5b6b (&encoder, 0) == 0x06);

	fail_unless (encode5b6b (&encoder, 3) == 0x23);
	fail_unless (encode5b6b (&encoder, 3) == 0x23);

	/* the special case */
	fail_unless (encode5b6b (&encoder, 7) == 0x07); /* same rd */
	fail_unless (encode5b6b (&encoder, 23) == 0x17); /* inverts rd */
	fail_unless (encode5b6b (&encoder, 7) == 0x38);
} END_TEST

START_TEST (test3b4bEncoder) {
	fail_unless (encode3b4b (&encoder, 0) == 0x0d);
	fail_unless (encode3b4b (&encoder, 0) == 0x02);

	fail_unless (encode3b4b (&encoder, 1) == 0x09);
	fail_unless (encode3b4b (&encoder, 1) == 0x09);

	fail_unless (encode3b4b (&encoder, 3) == 0x03);
	fail_unless (encode3b4b (&encoder, 4) == 0x0b); /* inverts rd */
	fail_unless (encode3b4b (&encoder, 3) == 0x0c);
} END_TEST

START_TEST (testEncoder) {
	{
		const uint8_t data[] = {0x4a}, expect[] = {0xaa, 0x2, 0};
		eightbtenbEncode (&encoder, data, 1);
		fail_unless (memcmp (encoded, expect, sizeof (expect)) == 0);
	}
	/* same again */
	{
		const uint8_t data[] = {0x4a}, expect[] = {0xaa, 0xaa, 0xa};
		eightbtenbEncode (&encoder, data, 1);
		fail_unless (memcmp (encoded, expect, sizeof (expect)) == 0);
	}
} END_TEST

START_TEST (test5b6bDecoder) {
	fail_unless (decode5b6b (&decoder, 0x06) == 0x0);
	fail_unless (decode5b6b (&decoder, 0x39) == 0x0);
} END_TEST

START_TEST (test3b4bDecoder) {
	fail_unless (decode3b4b (&decoder, 0x02) == 0x0);
	fail_unless (decode3b4b (&decoder, 0x0d) == 0x0);
} END_TEST

START_TEST (testDecoder) {
	const uint8_t data[] = {0x35, 0x2}, expect[] = {0xff, 0x00};
	eightbtenbDecode (&decoder, data, 10);
	fail_unless (memcmp (decoded, expect, sizeof (expect)) == 0);
} END_TEST

START_TEST (testDecoder2) {
	/* encoded as D31.7+, D0.0+, D10.5-, D21.2+ */
	const uint8_t data[] = {0xca, 0x19, 0xad, 0x56, 0xa5},
			expect[] = {0xff, 0x00, 0xaa, 0x55, 0x00};
	eightbtenbDecode (&decoder, data, 4*10);
	fail_unless (memcmp (decoded, expect, sizeof (expect)) == 0);
} END_TEST

/*	Feed output from encoder to decoder, random data
 */
START_TEST (testEncodeDecode) {
	srandom (0x9ab13a42);
	const size_t max = sizeof (encoded)*8/10;
	uint8_t * const data = malloc (max);
	for (size_t rounds = 0; rounds < 1000000; rounds++) {
		testSetup ();

		const size_t size = (random () % max) + 1;
#if 0
		printf ("%i bytes\nraw\n", size);
		for (size_t i = 0; i < size; i++) {
			data[i] = random ();
			printf ("0x%02x ", data[i]);
		}
		puts ("");
#endif
		eightbtenbEncode (&encoder, data, size);
#if 0
		puts ("encoded");
		for (size_t i = 0; i < size*10/8; i++) {
			printf ("0x%02x ", encoded[i]);
		}
		puts ("");
#endif
		eightbtenbDecode (&decoder, encoded, size*10);
#if 0
		for (size_t i = 0; i < size; i++) {
			printf ("0x%02x ", decoded[i]);
		}
		puts ("");
#endif
		fail_unless (memcmp (decoded, data, size) == 0);
	}
	free (data);
} END_TEST

Suite *test() {
	Suite *s = suite_create ("8b10b");

	/* add generic tests */
	TCase *tc_core = tcase_create ("generic");
	tcase_add_checked_fixture (tc_core, testSetup, NULL);
	tcase_add_test (tc_core, test5b6bEncoder);
	tcase_add_test (tc_core, test5b6bDecoder);
	tcase_add_test (tc_core, test3b4bEncoder);
	tcase_add_test (tc_core, test3b4bDecoder);
	tcase_add_test (tc_core, testDecoder);
	tcase_add_test (tc_core, testDecoder2);
	tcase_add_test (tc_core, testEncoder);
	tcase_add_test (tc_core, testEncodeDecode);
	suite_add_tcase (s, tc_core);

	return s;
}

/*	test suite runner
 */
int main (int argc, char **argv) {
	int numberFailed;
	SRunner *sr = srunner_create (test ());

	srunner_run_all (sr, CK_ENV);
	numberFailed = srunner_ntests_failed (sr);
	srunner_free (sr);

	return (numberFailed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
#endif

