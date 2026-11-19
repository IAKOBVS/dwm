/*
 * test_propertynotify.c — per-function tests for dwm.c's propertynotify().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through propertynotify(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- propertynotify --- */
static void
test_propertynotify_root_wmname(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	selmon = make_monitor(0);
	mons = selmon;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xproperty.window = root;
	ev.xproperty.atom = XA_WM_NAME;
	ev.xproperty.state = 0; /* PropertyNewValue */

	propertynotify(&ev);
	ASSERT(1, "propertynotify: root WM_NAME does not crash");

{
		Monitor *tmpm = selmon; /* free the fixture, then restore globals */
		restore_selmon();
		free(tmpm);
	}
}

static void
test_propertynotify_propertydelete_ignored(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	selmon = make_monitor(0);
	mons = selmon;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xproperty.window = 999;
	ev.xproperty.atom = XA_WM_NAME;
	ev.xproperty.state = PropertyDelete;

	propertynotify(&ev);
	ASSERT(1, "propertynotify: PropertyDelete no crash");

{
		Monitor *tmpm = selmon; /* free the fixture, then restore globals */
		restore_selmon();
		free(tmpm);
	}
}

/* --- propertynotify WM_TRANSIENT_FOR --- */
static void
test_propertynotify_transient_for(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	selmon = make_monitor(0);
	mons = selmon;

	Client target = { .win = 200, .tags = 1, .next = NULL, .mon = selmon };
	Client c = { .win = 100, .tags = 1, .mon = selmon, .next = &target,
		.isfloating = 0 };
	selmon->clients = &c;
	selmon->stack = &c;
	c.snext = &target;
	target.snext = NULL;
	winclient_put(&c);
	winclient_put(&target);

	mock_x11_reset();
	mock_gettransient_return = 1;
	mock_gettransient_win = 200;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xproperty.window = 100;
	ev.xproperty.atom = XA_WM_TRANSIENT_FOR;

	propertynotify(&ev);
	ASSERT(c.isfloating, "propertynotify transient: client set floating");
	mock_x11_reset();
{
		Monitor *tmpm = selmon; /* free the fixture, then restore globals */
		restore_selmon();
		free(tmpm);
	}
}

/* --- propertynotify netatom[NetWMWindowType] --- */
static void
test_propertynotify_windowtype(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	selmon = make_monitor(0);
	mons = selmon;

	Client c = { .win = 100, .tags = 1, .mon = selmon, .next = NULL };
	selmon->clients = &c;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xproperty.window = 100;
	ev.xproperty.atom = netatom[NetWMWindowType];

	propertynotify(&ev);
	ASSERT(1, "propertynotify windowtype: does not crash");
{
		Monitor *tmpm = selmon; /* free the fixture, then restore globals */
		restore_selmon();
		free(tmpm);
	}
}

/* --- propertynotify root WM_NAME fullscreen skip --- */
static void
test_propertynotify_root_wmname_fullscreen_skip(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	selmon = make_monitor(0);
	mons = selmon;

	Client c = { .win = 1, .isfullscreen = 1 };
	selmon->sel = &c;

	selmon->bar_dirty_segments = 0;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xproperty.window = root;
	ev.xproperty.atom = XA_WM_NAME;

	propertynotify(&ev);

	ASSERT_EQ(selmon->bar_dirty_segments, 0,
		"propertynotify: root WM_NAME skipped when optimizefullscreen + fullscreen");

	free(selmon);
	restore_selmon();
}

/* --- propertynotify XA_WM_NORMAL_HINTS (lines 1240-1242) --- */
static void
test_propertynotify_normal_hints(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	selmon = make_monitor(0);
	mons = selmon;

	Client *c = make_client(100, selmon);
	c->hintsvalid = 1;
	c->tags = 1;
	c->next = NULL;
	selmon->clients = c;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xproperty.window = 100;
	ev.xproperty.atom = XA_WM_NORMAL_HINTS;

	propertynotify(&ev);
	ASSERT_EQ(c->hintsvalid, 0, "propertynotify normal hints: hintsvalid cleared");
	ASSERT(1, "propertynotify normal hints: no crash");

	Monitor *local_mon = selmon;
	restore_selmon();
	winclient_remove(c);
	free(c); free(local_mon);
}

/* --- propertynotify XA_WM_HINTS (lines 1243-1248) --- */
static void
test_propertynotify_wm_hints(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	selmon = make_monitor(0);
	mons = selmon;

	Client *c = make_client(100, selmon);
	c->tags = 1;
	c->isurgent = 0;
	c->neverfocus = 0;
	c->next = NULL;
	selmon->clients = c;
	selmon->sel = c;

	mock_x11_reset();
	mock_wmhints_flags = InputHint;
	mock_wmhints_input = True;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xproperty.window = 100;
	ev.xproperty.atom = XA_WM_HINTS;

	selmon->bar_dirty_segments = 0;
	propertynotify(&ev);
	/* updatewmhints: InputHint set, wmh->input=True => neverfocus=0 */
	ASSERT_EQ(c->neverfocus, 0, "propertynotify wm hints: neverfocus=0 with InputHint");
	/* urgency indicators in tag rects changed */
	ASSERT(selmon->bar_dirty_segments & DIRTY_TAGS, "propertynotify wm hints: DIRTY_TAGS set");
	ASSERT(1, "propertynotify wm hints: no crash");

	mock_x11_reset();
	Monitor *local_mon = selmon;
	restore_selmon();
	winclient_remove(c);
	free(c); free(local_mon);
}

/* --- propertynotify XA_WM_NAME on client window (lines 1250-1256) --- */
static void
test_propertynotify_wm_name(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	selmon = make_monitor(0);
	mons = selmon;

	Client *c = make_client(100, selmon);
	c->tags = 1;
	c->next = NULL;
	selmon->clients = c;
	selmon->sel = c;

	mock_x11_reset();
	mock_gettextprop_return = 1;
	mock_gettextprop_value = "new title";

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xproperty.window = 100;
	ev.xproperty.atom = XA_WM_NAME;

	selmon->bar_dirty_segments = 0;
	propertynotify(&ev);
	/* updatetitle called; c == selmon->sel so DIRTY_TITLE set */
	ASSERT(selmon->bar_dirty_segments & DIRTY_TITLE, "propertynotify wm name: DIRTY_TITLE set for sel");
	ASSERT(1, "propertynotify wm name: no crash");

	mock_x11_reset();
	Monitor *local_mon = selmon;
	restore_selmon();
	winclient_remove(c);
	free(c); free(local_mon);
}

/* --- propertynotify netatom[NetWMName] on client window (lines 1250-1256) --- */
static void
test_propertynotify_net_wm_name(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	selmon = make_monitor(0);
	mons = selmon;

	Client *c = make_client(100, selmon);
	c->tags = 1;
	c->next = NULL;
	selmon->clients = c;
	selmon->sel = c;

	mock_x11_reset();
	mock_gettextprop_return = 1;
	mock_gettextprop_value = "net wm name";

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xproperty.window = 100;
	ev.xproperty.atom = netatom[NetWMName];

	selmon->bar_dirty_segments = 0;
	propertynotify(&ev);
	/* updatetitle called; c == selmon->sel so DIRTY_TITLE set */
	ASSERT(selmon->bar_dirty_segments & DIRTY_TITLE, "propertynotify net wm name: DIRTY_TITLE set for sel");
	ASSERT(1, "propertynotify net wm name: no crash");

	mock_x11_reset();
	Monitor *local_mon = selmon;
	restore_selmon();
	winclient_remove(c);
	free(c); free(local_mon);
}

/* --- propertynotify XA_WM_NAME on non-selected client (lines 1250-1256) --- */
static void
test_propertynotify_wm_name_non_sel(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	selmon = make_monitor(0);
	mons = selmon;

	Client *c = make_client(100, selmon);
	c->tags = 1;
	c->next = NULL;
	selmon->clients = c;

	/* sel != c, so DIRTY_TITLE should NOT be set */
	selmon->sel = NULL;

	mock_x11_reset();
	mock_gettextprop_return = 1;
	mock_gettextprop_value = "other title";

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xproperty.window = 100;
	ev.xproperty.atom = XA_WM_NAME;

	selmon->bar_dirty_segments = 0;
	propertynotify(&ev);
	/* c != selmon->sel, so DIRTY_TITLE should not be set */
	ASSERT(!(selmon->bar_dirty_segments & DIRTY_TITLE), "propertynotify wm name non-sel: DIRTY_TITLE not set");
	ASSERT(1, "propertynotify wm name non-sel: no crash");

	mock_x11_reset();
	Monitor *local_mon = selmon;
	restore_selmon();
	winclient_remove(c);
	free(c); free(local_mon);
}

/* --- propertynotify NetWMWindowType (line 1259) --- */
static void
test_propertynotify_net_wm_window_type(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	selmon = make_monitor(0);
	mons = selmon;

	Client c = { .win = 100, .tags = 1, .mon = selmon, .next = NULL };
	selmon->clients = &c;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xproperty.window = 100;
	ev.xproperty.atom = netatom[NetWMWindowType];

	propertynotify(&ev);
	ASSERT(1, "propertynotify NetWMWindowType: reaches updatewindowtype");
{
		Monitor *tmpm = selmon; /* free the fixture, then restore globals */
		restore_selmon();
		free(tmpm);
	}
}

static void
test_propertynotify_unsupported_atom(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	m->clients = NULL;
	selmon = m;
	mons = m;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xproperty.window = root;
	ev.xproperty.atom = None; /* unsupported atom */
	ev.xproperty.state = 0;

	propertynotify(&ev);
	ASSERT(1, "propertynotify: unsupported atom no crash");

	/* restore globals BEFORE freeing m */
	restore_selmon();
	free(m);
}

static void
test_propertynotify_transient_non_sel(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->tags = 1;
	m->clients = c;
	m->sel = NULL; /* not selected */
	selmon = m;
	mons = m;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xproperty.window = c->win;
	ev.xproperty.atom = XA_WM_TRANSIENT_FOR;
	ev.xproperty.state = 0;

	propertynotify(&ev);
	ASSERT(1, "propertynotify: WM_TRANSIENT_FOR on non-sel no crash");

	/* restore globals BEFORE freeing m */
	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

static void
run_tests(void)
{
	test_propertynotify_root_wmname();
	test_propertynotify_propertydelete_ignored();
	test_propertynotify_transient_for();
	test_propertynotify_windowtype();
	test_propertynotify_root_wmname_fullscreen_skip();
	test_propertynotify_normal_hints();
	test_propertynotify_wm_hints();
	test_propertynotify_wm_name();
	test_propertynotify_net_wm_name();
	test_propertynotify_wm_name_non_sel();
	test_propertynotify_net_wm_window_type();
	test_propertynotify_unsupported_atom();
	test_propertynotify_transient_non_sel();
}
