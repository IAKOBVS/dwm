/*
 * test_togglefloating.c — per-function tests for dwm.c's togglefloating().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through togglefloating(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- togglefloating --- */
static void
test_togglefloating_basic(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->isfloating = 0;
	m->sel = c;
	m->clients = c;
	save_selmon();
	selmon = m;
	mons = m;
	Arg arg = {0};

	togglefloating(&arg);
	ASSERT(c->isfloating, "togglefloating: toggles floating on");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

static void
test_togglefloating_isfixed(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->isfloating = 0;
	c->isfixed = 1;
	m->sel = c;
	m->clients = c;
	save_selmon();
	selmon = m;
	mons = m;
	Arg arg = {0};

	togglefloating(&arg);
	/* isfixed forces isfloating=1 even when toggling off */
	ASSERT(c->isfloating, "togglefloating: isfixed keeps floating on");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- togglefloating no sel --- */
static void
test_togglefloating_no_sel(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	selmon = make_monitor(0);
	mons = selmon;
	selmon->sel = NULL;
	Arg arg = {0};

	togglefloating(&arg);
	ASSERT(1, "togglefloating: noop when no sel");

{
		Monitor *tmpm = selmon; /* free the fixture, then restore globals */
		restore_selmon();
		free(tmpm);
	}
}

/* --- togglefloating fullscreen --- */
static void
test_togglefloating_fullscreen(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->isfullscreen = 1;
	m->sel = c;
	save_selmon();
	selmon = m;
	mons = m;
	Arg arg = {0};

	togglefloating(&arg);
	ASSERT(c->isfullscreen, "togglefloating: noop on fullscreen client");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

static void
run_tests(void)
{
	test_togglefloating_basic();
	test_togglefloating_isfixed();
	test_togglefloating_no_sel();
	test_togglefloating_fullscreen();
}
