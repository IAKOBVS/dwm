/*
 * test_keyset_home.c — per-function tests for keyset_home.
 *
 * Covers the primary path and at least one edge case, using the
 * shared mock fixture from test_common.h.
 */
#include "test_common.h"

static void
test_keyset_home_folds_in_range(void)
{
	unsigned long long vals[] = { 0, 1, 0xdeadbeefULL, ~0ULL };
	size_t i;

	for (i = 0; i < LENGTH(vals); i++)
		ASSERT(keyset_home(vals[i]) < KEYSET_SIZE,
			"keyset_home: slot inside table");
	ASSERT_EQ(keyset_home(0), 0, "keyset_home: zero key lands at slot 0");
}

static void
run_tests(void)
{
	test_keyset_home_folds_in_range();
}
