/*
 * test_getparentprocess.c — per-function tests for dwm.c's getparentprocess().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through getparentprocess(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- getparentprocess --- */
static void
test_getparentprocess(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	(void)getparentprocess(1);
	ASSERT(1, "getparentprocess: does not crash");
}

static void
run_tests(void)
{
	test_getparentprocess();
}
