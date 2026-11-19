/*
 * test_togglebar.c — per-function tests for dwm.c's togglebar().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through togglebar(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- togglebar --- */
static void
test_togglebar_toggles(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	Monitor *m = make_monitor(0);
	m->showbar = 1;
	selmon = m;
	mons = m;
	Arg arg = {0};

	togglebar(&arg);
	ASSERT_EQ(selmon->showbar, 0, "togglebar: toggles showbar off");

	restore_selmon();
	free(m); /* fixture leaked (ASan-reported) */
}

static void
run_tests(void)
{
	test_togglebar_toggles();
}
