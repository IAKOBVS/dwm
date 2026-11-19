/*
 * test_winclient_put.c — per-function tests for winclient_put.
 *
 * Covers the primary path and at least one edge case, using the
 * shared mock fixture from test_common.h.
 */
#include "test_common.h"

static void
test_winclient_put_inserts_and_dedups(void)
{
	Client c;

	memset(&c, 0, sizeof c);
	c.win = 700;
	reset_index();

	winclient_put(&c);
	ASSERT_EQ(winhash_count, 1, "winclient_put: first insert counted");
	ASSERT(wintoclient(700) == &c, "winclient_put: lookup finds client");

	winclient_put(&c);
	ASSERT_EQ(winhash_count, 1, "winclient_put: duplicate insert ignored");
	reset_index();
}

static void
run_tests(void)
{
	test_winclient_put_inserts_and_dedups();
}
