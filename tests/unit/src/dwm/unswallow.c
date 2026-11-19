/*
 * test_unswallow.c — per-function tests for dwm.c's unswallow().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through unswallow(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

static void
test_unswallow_basic(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *term = make_client(10, m);
	Client *c = make_client(11, m);
	term->swallowing = c;
	c->win = 10;
	term->win = 11;
	m->clients = term;
	m->sel = term;

	save_selmon();
	selmon = m;
	mons = m;

	unswallow(term);
	ASSERT(term->swallowing == NULL, "unswallow: swallowing freed");
	ASSERT(term->win == 10, "unswallow: term restores original win");

	restore_selmon();
	free(m); /* fixture leaked (ASan-reported) */
	free(term); /* fixture leaked (ASan-reported) */
}

static void
run_tests(void)
{
	test_unswallow_basic();
}
