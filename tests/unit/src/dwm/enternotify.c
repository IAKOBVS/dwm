/*
 * test_enternotify.c — per-function tests for dwm.c's enternotify().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through enternotify(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- enternotify --- */
static void
test_enternotify_normal_same_sel(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	Monitor *m = make_monitor(0);
	m->sel = NULL;
	save_selmon();
	selmon = m;
	mons = m;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xcrossing.mode = NotifyNormal;
	ev.xcrossing.detail = NotifyNonlinear;
	ev.xcrossing.window = 99999; /* not a client, not root => wintomon returns selmon */

	enternotify(&ev);
	/* Crossing an unknown window on the selected monitor is a no-op. */
	ASSERT(selmon == m, "enternotify normal: selected monitor unchanged");
	ASSERT_EQ(selmon->sel, NULL, "enternotify normal: selection unchanged");
	ASSERT_EQ(mock_xset_input_focus.count, 0, "enternotify normal: no focus reassertion");
	ASSERT_EQ(mock_xset_window_border.count, 0, "enternotify normal: no border change");

	restore_selmon();
	free(m);
}

static void
test_enternotify_inferior_returns_early(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	save_selmon();
	selmon = make_monitor(0);
	mons = selmon;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xcrossing.mode = NotifyNormal;
	ev.xcrossing.detail = NotifyInferior;
	ev.xcrossing.window = root;

	enternotify(&ev);
	/* Inferior detail on root is still tracked, but resolves to selmon. */
	ASSERT_EQ(mock_xset_input_focus.count, 0, "enternotify inferior: no focus reassertion");
	ASSERT_EQ(mock_xset_window_border.count, 0, "enternotify inferior: no border change");
	ASSERT_EQ(selmon->sel, NULL, "enternotify inferior: selection unchanged");

{
		Monitor *tmpm = selmon; /* free the fixture, then restore globals */
		restore_selmon();
		free(tmpm);
	}
}

/* --- enternotify cross-monitor (lines 701, 705-706, 709) --- */
static void
test_enternotify_different_monitor(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	Monitor *m1 = make_monitor(0);
	Monitor *m2 = make_monitor(1);
	m2->wx = 1920;
	m1->next = m2;
	m2->next = NULL;

	save_selmon();
	selmon = m1;
	mons = m1;

	Client *c = make_client(200, m2);
	c->tags = 1;
	m2->clients = c;
	m2->sel = c;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xcrossing.type = EnterNotify;
	ev.xcrossing.window = 200;
	ev.xcrossing.mode = NotifyNormal;
	ev.xcrossing.detail = NotifyNonlinear;

	enternotify(&ev);

	ASSERT(selmon == m2, "enternotify different monitor: selmon changed to m2");
	/* The entered client becomes the selection and focus is reasserted. */
	ASSERT_EQ(m2->sel, c, "enternotify different monitor: entered client selected");
	ASSERT_EQ(mock_xset_input_focus.count, 1, "enternotify different monitor: focus reasserted");
	ASSERT_EQ(mock_xset_input_focus.win, (unsigned long)c->win, "enternotify different monitor: focus on entered client");
	ASSERT_EQ(mock_xset_window_border.count, 1, "enternotify different monitor: selection border set");
	ASSERT_EQ(mock_xset_window_border.win, (unsigned long)c->win, "enternotify different monitor: border on entered client");
	ASSERT_EQ(mock_xchange_property.win, (unsigned long)root, "enternotify different monitor: _NET_ACTIVE_WINDOW on root");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m2); free(m1);
}

/* --- enternotify early return guard (line 701) --- */
static void
test_enternotify_guard_notifyinferior(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	save_selmon();
	Monitor *m = make_monitor(0);
	mons = m;
	selmon = m;

	Client *c = make_client(201, m);
	c->tags = 1;
	m->clients = c;
	m->sel = c;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xcrossing.type = EnterNotify;
	ev.xcrossing.window = 201;
	ev.xcrossing.mode = NotifyNormal;
	ev.xcrossing.detail = NotifyInferior;

	enternotify(&ev);

	/* Line 701: early return because detail == NotifyInferior and window != root */
	ASSERT(m->sel == c, "enternotify guard: NotifyInferior returns early, sel unchanged");
	ASSERT_EQ(mock_xset_input_focus.count, 0, "enternotify guard: no focus reassertion");
	ASSERT_EQ(mock_xset_window_border.count, 0, "enternotify guard: no border change");

	winclient_remove(c);
	free(c); free(m);
	restore_selmon();
}

static void
test_enternotify_grab_mode(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	Monitor *m = make_monitor(0);
	save_selmon();
	selmon = m;
	mons = m;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xcrossing.mode = NotifyGrab;
	ev.xcrossing.detail = NotifyNonlinear;
	ev.xcrossing.window = root;

	enternotify(&ev);
	/* Grab-mode crossing on root resolves to selmon but has no client to focus. */
	ASSERT_EQ(mock_xset_input_focus.count, 0, "enternotify grab: no focus reassertion");
	ASSERT_EQ(mock_xset_window_border.count, 0, "enternotify grab: no border change");
	ASSERT(selmon == m, "enternotify grab: selected monitor unchanged");

	/* restore globals BEFORE freeing m */
	restore_selmon();
	free(m);
}

static void
test_enternotify_own_window(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	m->sel = c;
	m->clients = c;
	save_selmon();
	selmon = m;
	mons = m;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xcrossing.mode = NotifyNormal;
	ev.xcrossing.detail = NotifyNonlinear;
	ev.xcrossing.window = c->win;

	enternotify(&ev);
	/* Entering the already-selected client is a no-op. */
	ASSERT_EQ(m->sel, c, "enternotify own window: selection unchanged");
	ASSERT_EQ(mock_xset_input_focus.count, 0, "enternotify own window: no focus reassertion");
	ASSERT_EQ(mock_xset_window_border.count, 0, "enternotify own window: no border change");

	/* restore globals BEFORE freeing m */
	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

static void
test_enternotify_ungrab_mode(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	Monitor *m = make_monitor(0);
	save_selmon();
	selmon = m;
	mons = m;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xcrossing.mode = NotifyUngrab;
	ev.xcrossing.detail = NotifyNonlinear;
	ev.xcrossing.window = root;

	enternotify(&ev);
	ASSERT_EQ(mock_xset_input_focus.count, 0, "enternotify ungrab: no focus reassertion");
	ASSERT_EQ(mock_xset_window_border.count, 0, "enternotify ungrab: no border change");
	ASSERT(selmon == m, "enternotify ungrab: selected monitor unchanged");

	/* restore globals BEFORE freeing m */
	restore_selmon();
	free(m);
}

static void
run_tests(void)
{
	test_enternotify_normal_same_sel();
	test_enternotify_inferior_returns_early();
	test_enternotify_different_monitor();
	test_enternotify_guard_notifyinferior();
	test_enternotify_grab_mode();
	test_enternotify_own_window();
	test_enternotify_ungrab_mode();
}
