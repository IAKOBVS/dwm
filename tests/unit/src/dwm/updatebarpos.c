/*
 * test_updatebarpos.c — per-function tests for dwm.c's updatebarpos().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through updatebarpos(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- updatebarpos --- */
static void
test_updatebarpos_top(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor m = { .wx=0, .wy=0, .ww=1920, .wh=1080, .topbar=1, .showbar=1 };
	updatebarpos(&m);
	ASSERT_EQ(m.by, 0, "updatebarpos: top bar at y=0");
	ASSERT(m.wy > 0, "updatebarpos: window area starts below bar");
}

static void
run_tests(void)
{
	test_updatebarpos_top();
}
