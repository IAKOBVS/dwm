/*
 * test_toggletag.c — per-function tests for dwm.c's toggletag().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through toggletag(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- toggletag --- */
static void
test_toggletag_basic(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->tags = 1;
	m->sel = c;
	save_selmon();
	selmon = m;
	mons = m;
	Arg arg = { .ui = 2 };

	toggletag(&arg);
	ASSERT_EQ(c->tags, (unsigned)3, "toggletag: adds tag to client");
	ASSERT(1, "toggletag: no crash");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- toggletag with selmon->sel == NULL (line 1814-1815) --- */
static void
test_toggletag_no_sel(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	Monitor *m = make_monitor(0);
	m->sel = NULL;
	selmon = m;
	mons = m;
	Arg arg = { .ui = 1 };

	toggletag(&arg);
	/* Line 1814: !selmon->sel is true, early return at line 1815 */
	ASSERT(m->sel == NULL, "toggletag no sel: returns early, no crash");

	restore_selmon();
	free(m);
}

static void
run_tests(void)
{
	test_toggletag_basic();
	test_toggletag_no_sel();
}
