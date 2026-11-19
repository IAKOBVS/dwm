/*
 * test_checkotherwm.c — per-function tests for dwm.c's checkotherwm().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through checkotherwm(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- checkotherwm --- */
static void
test_checkotherwm(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	checkotherwm();
	ASSERT(1, "checkotherwm: does not crash (calls XSetErrorHandler)");
}

static void
run_tests(void)
{
	test_checkotherwm();
}
