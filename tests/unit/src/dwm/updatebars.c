/*
 * test_updatebars.c — per-function tests for dwm.c's updatebars().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through updatebars(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- updatebars --- */
static void
test_updatebars(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	Monitor *m = make_monitor(0);
	m->barwin = 0;
	selmon = m;
	mons = m;

	updatebars();
	ASSERT(m->barwin != 0, "updatebars: creates barwin");

	restore_selmon();
	free(m); /* fixture leaked (ASan-reported) */
}

static void
run_tests(void)
{
	test_updatebars();
}
