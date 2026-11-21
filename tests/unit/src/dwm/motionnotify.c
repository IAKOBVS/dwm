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
	mock_x11_reset();
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
	/* A single monitor has no cross-monitor tracking to do. */
	ASSERT(selmon == m, "motionnotify single monitor: selection unchanged");
	ASSERT_EQ(mock_xset_input_focus.count, 0, "motionnotify single monitor: no focus reassertion");
	ASSERT_EQ(mock_xdelete_property.count, 0, "motionnotify single monitor: no _NET_ACTIVE_WINDOW change");
	ASSERT_EQ(mock_xsync_calls, 0, "motionnotify single monitor: no XSync");

	restore_selmon();
	free(m);
}

/* --- motionnotify multi-monitor --- */
static void
test_motionnotify_multi_monitor(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	Monitor *m1 = make_monitor(0);
	Monitor *m2 = make_monitor(1);
	m1->next = m2;
	m2->wx = 1920;            /* m2 sits to the right of m1 */
	m1->sel = NULL;
	m2->sel = NULL;
	mons = m1;
	save_selmon();
	selmon = m1;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xmotion.window = root;
	/* First event establishes the static 'mon' on m1. */
	ev.xmotion.x_root = 100;
	ev.xmotion.y_root = 500;
	motionnotify(&ev);
	ASSERT(selmon == m1, "motionnotify multi monitor: no switch while on m1");
	ASSERT_EQ(mock_xset_input_focus.count, 0, "motionnotify multi monitor: no focus change on m1");

	/* Second event lands in m2, forcing a focus switch. */
	ev.xmotion.x_root = 2000;
	motionnotify(&ev);
	ASSERT(selmon == m2, "motionnotify multi monitor: switches to m2");
	ASSERT_EQ(mock_xset_input_focus.count, 1, "motionnotify multi monitor: focus reasserted once");
	ASSERT_EQ(mock_xset_input_focus.win, (unsigned long)root, "motionnotify multi monitor: root focused when no client");
	ASSERT_EQ(mock_xdelete_property.count, 1, "motionnotify multi monitor: active-window cleared");
	ASSERT_EQ(mock_xdelete_property.win, (unsigned long)root, "motionnotify multi monitor: property on root");
	ASSERT_EQ(mock_xdelete_property.mask, (unsigned long)netatom[NetActiveWindow], "motionnotify multi monitor: NetActiveWindow atom");

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
	mock_x11_reset();
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
	/* Each monitor switch reasserts focus on the root when nothing is selected. */
	ASSERT(mock_xset_input_focus.count >= 1, "motionnotify cross-monitor: focus reasserted");
	ASSERT_EQ(mock_xset_input_focus.win, (unsigned long)root, "motionnotify cross-monitor: root focused");
	ASSERT(mock_xdelete_property.count >= 1, "motionnotify cross-monitor: active-window cleared");
	ASSERT_EQ(mock_xdelete_property.mask, (unsigned long)netatom[NetActiveWindow], "motionnotify cross-monitor: NetActiveWindow cleared");

	restore_selmon();
	free(m1);
	free(m2);
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
	/* No crash, early return taken with no focus/X side effects. */
	ASSERT(selmon == m1, "motionnotify non-root: selection unchanged");
	ASSERT_EQ(mock_xset_input_focus.count, 0, "motionnotify non-root: no focus reassertion");
	ASSERT_EQ(mock_xdelete_property.count, 0, "motionnotify non-root: no active-window change");
	ASSERT_EQ(mock_xsync_calls, 0, "motionnotify non-root: no XSync");

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
