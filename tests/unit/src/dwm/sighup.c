/*
 * test_sighup.c — per-function tests for dwm.c's sighup().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through sighup(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- sighup / sigterm --- */
static void
test_sighup(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	sighup(0);
	ASSERT(lifecycle != LIFE_LIVE, "sighup: sets running to 0");
}

static void
run_tests(void)
{
	test_sighup();
}
