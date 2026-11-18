/*
 * test_utf8validate.c — per-function drw.c tests for utf8validate().
 *
 * Uses the real drw.c with the mock X11/Xft fixture from
 * test_drw_common.h; covers the primary path and an edge case.
 */
#include "test_drw_common.h"

static void
test_utf8validate_accepts_and_rejects(void)
{
	long u = 'A';
	ASSERT_EQ(utf8validate(&u, 1), 1, "utf8validate: ASCII valid");
	ASSERT_EQ(u, 'A', "utf8validate: valid codepoint unchanged");

	u = 0xD800; /* surrogate */
	ASSERT_EQ(utf8validate(&u, 3), 3,
		"utf8validate: surrogate promoted to replacement length");
	ASSERT_EQ(u, 0xFFFD, "utf8validate: surrogate replaced");

	u = 0x110000; /* out of range */
	utf8validate(&u, 4);
	ASSERT_EQ(u, 0xFFFD, "utf8validate: out-of-range replaced");
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

/* --- property: valid codepoints survive, surrogates/out-of-range replaced --- */
static void
test_utf8validate_random_codepoints(void)
{
	unsigned int seed = 0x51CA5Eu;
	int iter;

	for (iter = 0; iter < 5000; iter++) {
		long cp = (long)(fuzz_rand(&seed) % 0x120000);
		size_t len = cp < 0x80 ? 1 : cp < 0x800 ? 2 : cp < 0x10000 ? 3 : 4;
		long u = cp;
		size_t got = utf8validate(&u, len);
		int valid = cp <= 0x10FFFF && !(cp >= 0xD800 && cp <= 0xDFFF);

		ASSERT(got >= 1 && got <= 4,
			"fuzz utf8validate: returns a valid sequence length");
		if (valid) {
			ASSERT_EQ(u, cp, "fuzz utf8validate: valid codepoint preserved");
		} else {
			ASSERT_EQ(u, 0xFFFD, "fuzz utf8validate: invalid codepoint replaced");
		}
	}
}

static void
run_tests(void)
{
	test_utf8validate_accepts_and_rejects();
	test_utf8validate_random_codepoints();
}
