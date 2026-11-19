/*
 * test_keyset_put.c — per-function tests for keyset_put.
 *
 * Covers the primary path and at least one edge case, using the
 * shared mock fixture from test_common.h.
 */
#include "test_common.h"

static void
test_keyset_put_inserts_and_dedups(void)
{
	memset(keyset, 0, sizeof keyset);
	keyset_count = 0;
	keyset_saturated = 0;

	keyset_put(keypack('a', 0));
	keyset_put(keypack('a', 0));
	ASSERT_EQ(keyset_count, 1, "keyset_put: duplicate key stored once");

	keyset_put(keypack('b', 0));
	ASSERT_EQ(keyset_count, 2, "keyset_put: distinct key inserted");
}

static void
test_keyset_put_saturates(void)
{
	memset(keyset, 0, sizeof keyset);
	keyset_count = KEYSET_SIZE - 1;
	keyset_saturated = 0;

	keyset_put(keypack('z', 0));
	ASSERT(keyset_saturated, "keyset_put: near-full table sets saturation flag");
	ASSERT_EQ(keyset_count, KEYSET_SIZE - 1,
		"keyset_put: saturated insert is suppressed");
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

/* --- randomized differential: exact membership vs a boolean model --- */
static void
test_keyset_put_random_vs_model(void)
{
	enum { KS_DOMAIN = 64, MOD_DOMAIN = 8, OPS = 200 };
	static const unsigned int mods[MOD_DOMAIN] = {
		0, ShiftMask, ControlMask, Mod1Mask,
		Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask
	};
	unsigned int present[KS_DOMAIN][MOD_DOMAIN];
	unsigned int seed = 0x5EEDu;
	unsigned int expected = 0;
	int i, k, mod;

	memset(keyset, 0, sizeof keyset);
	keyset_count = 0;
	keyset_saturated = 0;
	memset(present, 0, sizeof present);

	for (i = 0; i < OPS; i++) {
		k = (int)(fuzz_rand(&seed) % KS_DOMAIN);
		mod = (int)(fuzz_rand(&seed) % MOD_DOMAIN);
		/* CLEANMASK strips LockMask; use only real modifier bits. */
		keyset_put(keypack(0x1000 + k, mods[mod]));
		if (!present[k][mod]) {
			present[k][mod] = 1;
			expected++;
		}
	}

	ASSERT_EQ(keyset_count, expected,
		"fuzz keyset: count matches unique insertions");
	ASSERT(!keyset_saturated, "fuzz keyset: domain stays below saturation");

	for (k = 0; k < KS_DOMAIN; k++) {
		for (mod = 0; mod < MOD_DOMAIN; mod++) {
			int bound = iskeybound(0x1000 + k, mods[mod]);
			if (present[k][mod])
				ASSERT(bound, "fuzz keyset: inserted binding is found");
			else
				ASSERT(!bound, "fuzz keyset: unbound pair is rejected");
		}
	}
}

static void
run_tests(void)
{
	test_keyset_put_inserts_and_dedups();
	test_keyset_put_saturates();
	test_keyset_put_random_vs_model();
}
