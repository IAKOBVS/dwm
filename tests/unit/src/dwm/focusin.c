/*
 * test_focusin.c — per-function tests for dwm.c's focusin().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through focusin(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- focusin --- */
static void
test_focusin_different_window(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->neverfocus = 0;
	m->sel = c;
	save_selmon();
	selmon = m;
	mons = m;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xfocus.window = 999; /* different from selmon->sel->win */

	focusin(&ev);
	ASSERT(1, "focusin: different window does not crash");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

static void
run_tests(void)
{
	test_focusin_different_window();
}
