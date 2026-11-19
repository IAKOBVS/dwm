/*
 * test_winclient_remove.c — per-function tests for winclient_remove.
 *
 * Covers the primary path and at least one edge case, using the
 * shared mock fixture from test_common.h.
 */
#include "test_common.h"

static void
test_winclient_remove_clears_slot(void)
{
	Client c;

	memset(&c, 0, sizeof c);
	c.win = 701;
	reset_index();
	winclient_put(&c);

	winclient_remove(&c);
	ASSERT_EQ(winhash_count, 0, "winclient_remove: decrements count");
	ASSERT(wintoclient(701) == NULL, "winclient_remove: lookup misses");

	/* Removing an absent entry is a safe no-op */
	winclient_remove(&c);
	ASSERT_EQ(winhash_count, 0, "winclient_remove: absent entry ignored");
}

static void
run_tests(void)
{
	test_winclient_remove_clears_slot();
}
