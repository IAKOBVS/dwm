/*
 * test_drawbars.c — per-function tests for dwm.c's drawbars().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through drawbars(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- drawbars --- */
static void
test_drawbars(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	Monitor *m = make_monitor(0);
	m->barwin = 888;
	m->sel = NULL;
	m->clients = NULL;
	selmon = m;
	mons = m;

	drawbars();
	ASSERT(1, "drawbars: does not crash");

	restore_selmon();
	free(m); /* fixture leaked (ASan-reported) */
}

static void
run_tests(void)
{
	test_drawbars();
}
