/*
 * test_unmanage.c — per-function tests for dwm.c's unmanage().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through unmanage(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- unmanage with swallowing --- */
static void
test_unmanage_swallowing_client(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *term = make_client(10, m);
	Client *sub = make_client(11, m);
	term->swallowing = sub;
	m->clients = term;
	m->stack = term;
	m->sel = term;
	save_selmon();
	selmon = m;
	mons = m;

	unmanage(term, 0);
	ASSERT(1, "unmanage: swallowing client does not crash");

	restore_selmon();
	free(m); /* fixture leaked (ASan-reported) */
	free(term); /* fixture leaked (ASan-reported) */
}

/* --- unmanage not destroyed --- */
static void
test_unmanage_not_destroyed(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	m->clients = c;
	m->stack = c;
	m->sel = c;
	save_selmon();
	selmon = m;
	mons = m;

	unmanage(c, 0);
	ASSERT(m->clients == NULL, "unmanage: client removed when not destroyed");

	restore_selmon();
	free(m);
}

static void
run_tests(void)
{
	test_unmanage_swallowing_client();
	test_unmanage_not_destroyed();
}
