/*
 * test_keypack.c — per-function tests for keypack.
 *
 * Covers the primary path and at least one edge case, using the
 * shared mock fixture from test_common.h.
 */
#include "test_common.h"

static void
test_keypack_roundtrip(void)
{
	unsigned long long k = keypack(0x70, 0x5);

	ASSERT_EQ((unsigned int)(k >> 32), 0x70, "keypack: keysym in high word");
	ASSERT_EQ((unsigned int)(k & 0xffffffffULL), 0x5,
		"keypack: modifier in low word");
	ASSERT(keypack(0x70, 0x5) != keypack(0x70, 0x4),
		"keypack: distinct modifiers produce distinct keys");
}

static void
run_tests(void)
{
	test_keypack_roundtrip();
}
