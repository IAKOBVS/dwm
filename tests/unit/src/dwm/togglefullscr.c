/*
 * test_togglefullscr.c — per-function tests for dwm.c's togglefullscr().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through togglefullscr(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- togglefullscr --- */
static void
test_togglefullscr_basic(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->isfullscreen = 0;
	m->sel = c;
	save_selmon();
	selmon = m;
	mons = m;
	Arg arg = {0};

	togglefullscr(&arg);
	ASSERT(c->isfullscreen, "togglefullscr: sets fullscreen");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

static void
run_tests(void)
{
	test_togglefullscr_basic();
}
