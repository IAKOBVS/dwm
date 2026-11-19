/*
 * test_isdescprocess.c — per-function tests for dwm.c's isdescprocess().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through isdescprocess(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- isdescprocess --- */
static void
test_isdescprocess_same(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	int r = isdescprocess(100, 100);
	ASSERT(r, "isdescprocess: same pid returns true");
}

static void
test_isdescprocess_different(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	int r = isdescprocess(1, 99999);
	/* Loop exits when c becomes 0 (getparentprocess fails) */
	ASSERT_EQ(r, 0, "isdescprocess: different pid chain returns 0");
}

static void
run_tests(void)
{
	test_isdescprocess_same();
	test_isdescprocess_different();
}
