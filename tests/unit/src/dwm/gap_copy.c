/*
 * test_gap_copy.c — per-function tests for dwm.c's gap_copy().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through gap_copy(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- gap_copy / setgaps edge cases --- */
static void
test_gap_copy(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Gap src = { .isgap = 1, .realgap = 10, .gappx = 10 };
	Gap dst;
	memset(&dst, 0, sizeof dst);
	gap_copy(&dst, &src);
	ASSERT_EQ(dst.isgap, src.isgap, "gap_copy: isgap");
	ASSERT_EQ(dst.realgap, src.realgap, "gap_copy: realgap");
	ASSERT_EQ(dst.gappx, src.gappx, "gap_copy: gappx");
}

static void
run_tests(void)
{
	test_gap_copy();
}
