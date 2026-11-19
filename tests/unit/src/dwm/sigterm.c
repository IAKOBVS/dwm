/*
 * test_sigterm.c — per-function tests for dwm.c's sigterm().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through sigterm(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

static void
test_sigterm(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	lifecycle = LIFE_LIVE;
	sigterm(0);
	ASSERT(lifecycle != LIFE_LIVE, "sigterm: sets running to 0");
}

static void
run_tests(void)
{
	test_sigterm();
}
