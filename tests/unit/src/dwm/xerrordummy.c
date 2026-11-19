/*
 * test_xerrordummy.c — per-function tests for dwm.c's xerrordummy().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through xerrordummy(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

static void
test_xerrordummy(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	int r = xerrordummy(NULL, NULL);
	ASSERT_EQ(r, 0, "xerrordummy: returns 0");
}

static void
run_tests(void)
{
	test_xerrordummy();
}
