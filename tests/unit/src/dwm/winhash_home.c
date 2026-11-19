/*
 * test_winhash_home.c — per-function tests for winhash_home.
 *
 * Covers the primary path and at least one edge case, using the
 * shared mock fixture from test_common.h.
 */
#include "test_common.h"

static void
test_winhash_home_in_range(void)
{
	Window vals[] = { 0, 1, 42, 0xdeadbeef };
	size_t i;

	for (i = 0; i < LENGTH(vals); i++)
		ASSERT(winhash_home(vals[i]) < WINHASH_SIZE,
			"winhash_home: slot stays inside table");
	ASSERT_EQ(winhash_home(0), 0, "winhash_home: window 0 hashes to 0");
}

static void
run_tests(void)
{
	test_winhash_home_in_range();
}
