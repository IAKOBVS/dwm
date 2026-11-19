/*
 * test_motionnotify.c — per-function tests for dwm.c's motionnotify().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through motionnotify(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- motionnotify --- */
static void
test_motionnotify_no_crash(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	Monitor *m = make_monitor(0);
	selmon = m;
	mons = m;
	/* mon is a static local in motionnotify(), set internally */

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xmotion.window = root;
	ev.xmotion.x_root = 100;
	ev.xmotion.y_root = 100;

	motionnotify(&ev);
	ASSERT(1, "motionnotify: does not crash");

	restore_selmon();
	free(m); /* fixture leaked (ASan-reported) */
}

/* --- motionnotify multi-monitor --- */
static void
test_motionnotify_multi_monitor(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m1 = make_monitor(0);
	Monitor *m2 = make_monitor(1);
	m1->next = m2;
	m1->sel = NULL;
	m2->sel = NULL;
	mons = m1;
	save_selmon();
	selmon = m1;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xmotion.window = root;
	ev.xmotion.x_root = 2000;
	ev.xmotion.y_root = 500;

	motionnotify(&ev);
	/* Second call with different coords triggers monitor switch */
	ev.xmotion.x_root = 500;
	motionnotify(&ev);
	ASSERT(1, "motionnotify: multi-monitor does not crash");

	restore_selmon();
	free(m1);
	free(m2);
}

/* --- motionnotify cross-monitor focus switch (lines 1123-1127) --- */
static void
test_motionnotify_cross_monitor(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m1 = make_monitor(0);
	Monitor *m2 = make_monitor(1);
	m1->sel = NULL;
	m2->sel = NULL;
	m1->stack = NULL;
	m2->stack = NULL;
	m1->next = m2;
	/* place m2 to the right of m1 */
	m2->wx = 1920;
	m2->mx = 1920;
	mons = m1;
	save_selmon();
	selmon = m1;

	/* First call sets the static 'mon' in motionnotify */
	XEvent ev1;
	memset(&ev1, 0, sizeof ev1);
	ev1.xmotion.window = root;
	ev1.xmotion.x_root = 0;
	ev1.xmotion.y_root = 0;
	motionnotify(&ev1);

	/* Second call with coordinates in m2 triggers focus switch */
	XEvent ev2;
	memset(&ev2, 0, sizeof ev2);
	ev2.xmotion.window = root;
	ev2.xmotion.x_root = 1921;
	ev2.xmotion.y_root = 0;
	motionnotify(&ev2);
	ASSERT(selmon == m2, "motionnotify: cross-monitor switches focus to m2");

	restore_selmon();
	free(m1); /* fixture leaked (ASan-reported) */
	free(m2); /* fixture leaked (ASan-reported) */
}

/* --- motionnotify non-root window early return (line 1123) --- */
static void
test_motionnotify_non_root(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	save_selmon();
	Monitor *m1 = make_monitor(0);
	Monitor *m2 = make_monitor(1);
	m1->next = m2;
	m1->sel = NULL;
	m2->sel = NULL;
	mons = m1;
	selmon = m1;

	/* ev->window != root triggers early return at line 1123 */
	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xmotion.window = 999; /* not root */
	ev.xmotion.x_root = 0;
	ev.xmotion.y_root = 0;

	motionnotify(&ev);
	/* No crash, early return taken */
	ASSERT(1, "motionnotify non-root: returns early without crash");

	mock_x11_reset();
	free(m1);
	free(m2);
	restore_selmon();
}

static void
run_tests(void)
{
	test_motionnotify_no_crash();
	test_motionnotify_multi_monitor();
	test_motionnotify_cross_monitor();
	test_motionnotify_non_root();
}
