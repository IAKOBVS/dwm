/*
 * test_seturgent.c — per-function tests for dwm.c's seturgent().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through seturgent(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- seturgent --- */
static void
test_seturgent_sets_flag(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = ecalloc(1, sizeof(Client));
	c->win = 1; c->mon = m; c->tags = 1;
	m->sel = c;
	save_selmon();
	selmon = m;
	mons = m;
	seturgent(c, 1);
	ASSERT(c->isurgent, "seturgent: sets isurgent flag");
	winclient_remove(c);
	free(c);
	restore_selmon();
	free(m); /* fixture leaked (ASan-reported) */
}

/* --- seturgent null wmhints (line 1679) --- */
static void
test_seturgent_null_wmhints(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(50, m);
	m->clients = c;
	m->sel = c;
	save_selmon();
	selmon = m;
	mons = m;

	c->isurgent = 0;
	mock_wmhints_return_null = 1;
	seturgent(c, 1);
	/* line 1678: XGetWMHints returns NULL, line 1679: early return */
	ASSERT_EQ(c->isurgent, 1, "seturgent null wmhints: isurgent set before return");
	mock_wmhints_return_null = 0;

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

static void
run_tests(void)
{
	test_seturgent_sets_flag();
	test_seturgent_null_wmhints();
}
