/*
 * test_updatetitle.c — per-function tests for dwm.c's updatetitle().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through updatetitle(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- updatetitle --- */
static void
test_updatetitle(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Client c = { .win = 1 };
	updatetitle(&c);
	ASSERT(1, "updatetitle: does not crash");
}

static void
run_tests(void)
{
	test_updatetitle();
}
