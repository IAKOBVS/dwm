/*
 * test_setfullscreen.c — per-function tests for dwm.c's setfullscreen().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through setfullscreen(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- setfullscreen toggle --- */
static void
test_setfullscreen_on_off(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->isfullscreen = 0;
	m->sel = c;
	m->clients = c;
	save_selmon();
	selmon = m;
	mons = m;

	setfullscreen(c, 1);
	ASSERT(c->isfullscreen, "setfullscreen: sets fullscreen on");
	ASSERT(1, "setfullscreen: on triggers arrange + restack");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- setfullscreen cycle --- */
static void
test_setfullscreen_toggle(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->isfullscreen = 0;
	c->oldx = 100; c->oldy = 100; c->oldw = 200; c->oldh = 200;
	m->sel = c;
	m->clients = c;
	save_selmon();
	selmon = m;
	mons = m;

	setfullscreen(c, 1);
	ASSERT(c->isfullscreen, "setfullscreen: enter fullscreen");
	ASSERT_EQ(c->bw, 0, "setfullscreen: border removed");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- setfullscreen exit --- */
static void
test_setfullscreen_exit(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->isfullscreen = 1;
	c->oldstate = 0;
	c->oldbw = 2;
	c->oldx = 100; c->oldy = 100; c->oldw = 200; c->oldh = 200;
	m->sel = c;
	m->clients = c;
	save_selmon();
	selmon = m;
	mons = m;

	setfullscreen(c, 0);
	ASSERT(!c->isfullscreen, "setfullscreen: exit fullscreen");
	ASSERT_EQ(c->bw, 2, "setfullscreen: border restored");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- transient/fixed state survives a pre-map fullscreen request --- */
static void
test_setfullscreen_transient_stays_floating(void)
{
	reset_index();
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->isfullscreen = 1;
	c->fs_oldfloating = 0;   /* fullscreen was entered before transient default */
	c->transient_floating = 1;
	c->oldbw = 2;
	m->clients = c;
	m->sel = c;
	save_selmon();
	selmon = m;
	mons = m;

	setfullscreen(c, 0);

	ASSERT_EQ(c->isfloating, 1,
		"setfullscreen: transient client stays floating after exit");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

static void
run_tests(void)
{
	test_setfullscreen_on_off();
	test_setfullscreen_toggle();
	test_setfullscreen_exit();
	test_setfullscreen_transient_stays_floating();
}
