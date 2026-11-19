/*
 * test_toggleview.c — per-function tests for dwm.c's toggleview().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through toggleview(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- toggleview --- */
static void
test_toggleview_basic(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	Monitor *m = make_monitor(0);
	m->tagset[0] = 1;
	m->tagset[1] = 1;
	selmon = m;
	mons = m;
	Arg arg = { .ui = 2 };

	toggleview(&arg);
	ASSERT_EQ(selmon->tagset[selmon->seltags], (unsigned)3, "toggleview: adds tag to view");

	restore_selmon();
	free(m); /* fixture leaked (ASan-reported) */
}

static void
run_tests(void)
{
	test_toggleview_basic();
}
