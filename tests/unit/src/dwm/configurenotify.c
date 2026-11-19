/*
 * test_configurenotify.c — per-function tests for dwm.c's configurenotify().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through configurenotify(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- configurenotify --- */
static void
test_configurenotify_root(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	selmon = make_monitor(0);
	mons = selmon;
	selmon->barwin = 0;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xconfigure.window = root;
	ev.xconfigure.width = 1920;
	ev.xconfigure.height = 1080;

	configurenotify(&ev);
	ASSERT(1, "configurenotify: root event does not crash");

{
		Monitor *tmpm = selmon; /* free the fixture, then restore globals */
		restore_selmon();
		free(tmpm);
	}
}

/* --- configurenotify resize (lines 465-478) --- */
static void
test_configurenotify_resize(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	m->barwin = 100;
	mons = m;
	save_selmon();
	selmon = m;

	/* Set sw/sh to values different from event size so dirty=true */
	int old_sw = sw;
	int old_sh = sh;
	sw = 100;
	sh = 100;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xconfigure.window = root;
	ev.xconfigure.width = 1920;
	ev.xconfigure.height = 1080;

	configurenotify(&ev);
	ASSERT(sw == 1920, "configurenotify resize: sw updated");
	ASSERT(sh == 1080, "configurenotify resize: sh updated");

	restore_selmon();
	free(m);
	(void)old_sw; (void)old_sh;
}

/* --- configurenotify resize with fullscreen client (lines 472-474) --- */
static void
test_configurenotify_resize_with_fullscreen(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	m->barwin = 100;
	Client *c = make_client(1, m);
	c->isfullscreen = 1;
	c->x = 0; c->y = 0; c->w = 1920; c->h = 1080;
	m->clients = c;
	mons = m;
	save_selmon();
	selmon = m;

	sw = 100;
	sh = 100;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xconfigure.window = root;
	ev.xconfigure.width = 1920;
	ev.xconfigure.height = 1080;

	configurenotify(&ev);
	ASSERT(sw == 1920, "configurenotify resize fullscreen: sw updated");
	ASSERT(sh == 1080, "configurenotify resize fullscreen: sh updated");
	ASSERT(c->isfullscreen, "configurenotify resize fullscreen: client still fullscreen");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

static void
test_configurenotify_non_root(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	selmon = make_monitor(0);
	mons = selmon;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xconfigure.window = 12345; /* non-root */
	ev.xconfigure.width = 1920;
	ev.xconfigure.height = 1080;

	configurenotify(&ev);
	ASSERT(1, "configurenotify: non-root window returns early no crash");

{
		Monitor *tmpm = selmon; /* free the fixture, then restore globals */
		restore_selmon();
		free(tmpm);
	}
}

static void
test_configurenotify_fullscreen_resize(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	Monitor *m = make_monitor(0);
	m->barwin = 0;
	selmon = m;
	mons = m;

	Client *c = make_client(1, m);
	c->isfullscreen = 1;
	c->x = 0; c->y = 0; c->w = 100; c->h = 100;
	m->clients = c;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xconfigure.window = root;
	/* Use a different size than sw=1920 to trigger dirty path */
	ev.xconfigure.width = 2560;
	ev.xconfigure.height = 1440;

	configurenotify(&ev);
	ASSERT(c->isfullscreen, "configurenotify: fullscreen client still fullscreen");
	ASSERT(c->w > 100, "configurenotify: fullscreen client resized to monitor");

	winclient_remove(c);
	free(c);
	restore_selmon();
	free(m); /* fixture leaked (ASan-reported) */
}

static void
run_tests(void)
{
	test_configurenotify_root();
	test_configurenotify_resize();
	test_configurenotify_resize_with_fullscreen();
	test_configurenotify_non_root();
	test_configurenotify_fullscreen_resize();
}
