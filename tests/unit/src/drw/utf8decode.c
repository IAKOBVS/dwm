/*
 * test_utf8decode.c — per-function drw.c tests for utf8decode().
 *
 * Uses the real drw.c with the mock X11/Xft fixture from
 * test_drw_common.h; covers the primary path and an edge case.
 */
#include "test_drw_common.h"

static void
test_utf8decode_sequences(void)
{
	long u = 0;

	ASSERT_EQ(utf8decode("A", &u, 4), 1, "utf8decode: ASCII consumes 1");
	ASSERT_EQ(u, 'A', "utf8decode: ASCII codepoint");

	ASSERT_EQ(utf8decode("\xC2\xA2", &u, 4), 2,
		"utf8decode: two-byte sequence consumes 2");
	ASSERT_EQ(u, 0xA2, "utf8decode: two-byte codepoint");

	ASSERT_EQ(utf8decode("\xFF", &u, 1), 1,
		"utf8decode: invalid start consumes 1");
	ASSERT_EQ(u, 0xFFFD, "utf8decode: invalid start replaced");

	ASSERT_EQ(utf8decode("\xC2", &u, 1), 0,
		"utf8decode: incomplete sequence returns 0");
}

static unsigned int
fuzz_rand(unsigned int *state)
{
	unsigned int x = *state;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	return (*state = x);
}

/* Encode one codepoint (caller guarantees it is valid). */
static int
encode_utf8(long cp, unsigned char out[4])
{
	if (cp < 0x80) {
		out[0] = (unsigned char)cp;
		return 1;
	}
	if (cp < 0x800) {
		out[0] = (unsigned char)(0xC0 | (cp >> 6));
		out[1] = (unsigned char)(0x80 | (cp & 0x3F));
		return 2;
	}
	if (cp < 0x10000) {
		out[0] = (unsigned char)(0xE0 | (cp >> 12));
		out[1] = (unsigned char)(0x80 | ((cp >> 6) & 0x3F));
		out[2] = (unsigned char)(0x80 | (cp & 0x3F));
		return 3;
	}
	out[0] = (unsigned char)(0xF0 | (cp >> 18));
	out[1] = (unsigned char)(0x80 | ((cp >> 12) & 0x3F));
	out[2] = (unsigned char)(0x80 | ((cp >> 6) & 0x3F));
	out[3] = (unsigned char)(0x80 | (cp & 0x3F));
	return 4;
}

/* --- property: valid encodings round-trip exactly --- */
static void
test_utf8decode_roundtrip_boundaries(void)
{
	static const long cps[] = {
		0x00, 0x7F, 0x80, 0x7FF, 0x800, 0xD7FF, 0xE000,
		0xFFFF, 0x10000, 0x10FFFF
	};
	unsigned char buf[4];
	size_t i;

	for (i = 0; i < sizeof cps / sizeof cps[0]; i++) {
		long u = -1;
		int n = encode_utf8(cps[i], buf);

		ASSERT_EQ(utf8decode((const char *)buf, &u, (size_t)n), n,
			"fuzz utf8decode: encoded length is consumed");
		ASSERT_EQ(u, cps[i], "fuzz utf8decode: codepoint round-trips");
	}
}

/* --- property: arbitrary byte windows never over-read or over-consume --- */
static void
test_utf8decode_random_bytes(void)
{
	unsigned int seed = 0xDEC0DEu;
	int iter;

	for (iter = 0; iter < 2000; iter++) {
		unsigned char buf[8];
		size_t len = fuzz_rand(&seed) % (sizeof buf + 1);
		size_t i;
		long u = 0;
		int consumed;

		for (i = 0; i < len; i++)
			buf[i] = (unsigned char)(fuzz_rand(&seed) & 0xFF);

		consumed = utf8decode((const char *)buf, &u, len);
		if (len == 0) {
			ASSERT_EQ(consumed, 0, "fuzz utf8decode: empty window returns 0");
			continue;
		}
		ASSERT(consumed == 0 || (consumed >= 1 && consumed <= 4),
			"fuzz utf8decode: consumption is 0 or 1..4");
		ASSERT(consumed == 0 || (size_t)consumed <= len,
			"fuzz utf8decode: never consumes past the window");
		ASSERT(u >= 0 && u <= 0x10FFFF,
			"fuzz utf8decode: codepoint stays in Unicode range");
	}
}

static void
run_tests(void)
{
	test_utf8decode_sequences();
	test_utf8decode_roundtrip_boundaries();
	test_utf8decode_random_bytes();
}
