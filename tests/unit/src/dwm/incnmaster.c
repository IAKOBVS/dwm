/*
 * test_incnmaster.c — per-function tests for dwm.c's incnmaster().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through incnmaster(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- incnmaster --- */
static void
test_incnmaster_increases(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	m->nmaster = 1;
	save_selmon();
	selmon = m;
	mons = m;
	Arg arg = { .i = 1 };

	incnmaster(&arg);
	ASSERT_EQ(selmon->nmaster, 2, "incnmaster: increases nmaster");

	restore_selmon();
	free(m); /* fixture leaked (ASan-reported) */
}

static void
run_tests(void)
{
	test_incnmaster_increases();
}
