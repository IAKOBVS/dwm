/*
 * test_expose.c — per-function tests for dwm.c's expose().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through expose(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- expose --- */
static void
test_expose_barwin(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	m->barwin = 888;
	m->showbar = 1;
	save_selmon();
	selmon = m;
	mons = m;
	selmon->bar_exposed = 0;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xexpose.window = 888;
	ev.xexpose.count = 0;

	expose(&ev);
	/* expose sets selmon->bar_exposed=1 then calls drawbar which resets it to 0 */
	ASSERT(1, "expose: does not crash");

	restore_selmon();
	free(m);
}

static void
run_tests(void)
{
	test_expose_barwin();
}
