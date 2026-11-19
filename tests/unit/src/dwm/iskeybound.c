/*
 * test_iskeybound.c — per-function tests for iskeybound.
 *
 * Covers the primary path and at least one edge case, using the
 * shared mock fixture from test_common.h.
 */
#include "test_common.h"

static void
test_iskeybound_hit_and_miss(void)
{
	memset(keyset, 0, sizeof keyset);
	keyset_count = 0;
	keyset_saturated = 0;
	keyset_put(keypack(0x70, 0));

	ASSERT(iskeybound(0x70, 0), "iskeybound: exact key+mod found");
	ASSERT(!iskeybound(0x71, 0), "iskeybound: unbound keysym rejected");
	ASSERT(!iskeybound(0x70, ShiftMask), "iskeybound: wrong modifier rejected");
}

/* --- saturated set falls back to the OR-mask guard --- */
static void
test_iskeybound_saturated_fallback(void)
{
	memset(keyset, 0, sizeof keyset);
	keyset_count = KEYSET_SIZE - 1;
	keyset_saturated = 1;
	key_mod_used = Mod1Mask;
	key_keysym_used = 0x70;

	ASSERT(iskeybound(0x70, Mod1Mask),
		"iskeybound: saturated set accepts a mask/keysym match");
	ASSERT(!iskeybound(0x0F, Mod1Mask),
		"iskeybound: saturated set rejects a keysym with no shared bits");
	ASSERT(!iskeybound(0x70, ShiftMask),
		"iskeybound: saturated set rejects a modifier with no shared bits");

	keyset_saturated = 0;
	key_mod_used = 0;
	key_keysym_used = 0;
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

/* --- randomized exact lookups once the table has entries --- */
static void
test_iskeybound_random_exact(void)
{
	static const unsigned int mods[8] = {
		0, ShiftMask, ControlMask, Mod1Mask,
		Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask
	};
	unsigned int seed = 0xBEEFu;
	int i, k, mod;

	memset(keyset, 0, sizeof keyset);
	keyset_count = 0;
	keyset_saturated = 0;

	/* Deterministic sparse insert: keysym 0x200+2*i, mod i%8 */
	for (i = 0; i < 80; i++)
		keyset_put(keypack(0x200 + (KeySym)(2 * i), mods[i % 8]));

	for (i = 0; i < 200; i++) {
		k = (int)(fuzz_rand(&seed) % 160);
		mod = (int)(fuzz_rand(&seed) % 8);
		/* Present iff k is even (inserted as 0x200+2*i) and the modified
		 * pair matches the insertion. CLEANMASK cannot change these bits. */
		{
			int key = 0x200 + k;
			int present = (k % 2 == 0) && (mod == ((k / 2) % 8));
			ASSERT(iskeybound((KeySym)key, mods[mod]) == present,
				"fuzz iskeybound: exact membership matches model");
		}
	}
}

static void
run_tests(void)
{
	test_iskeybound_hit_and_miss();
	test_iskeybound_saturated_fallback();
	test_iskeybound_random_exact();
}
