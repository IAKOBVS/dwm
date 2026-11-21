/*
 * test_configurerequest.c — per-function tests for dwm.c's configurerequest().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through configurerequest(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- configurerequest --- */
static void
test_configurerequest_nonclient(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xconfigurerequest.window = 99999;
	ev.xconfigurerequest.value_mask = CWX|CWY|CWWidth|CWHeight|CWBorderWidth;
	ev.xconfigurerequest.x = 11;
	ev.xconfigurerequest.y = 22;
	ev.xconfigurerequest.width = 333;
	ev.xconfigurerequest.height = 444;
	ev.xconfigurerequest.border_width = 5;
	ev.xconfigurerequest.above = 777;
	ev.xconfigurerequest.detail = Above;

	configurerequest(&ev);
	/* Unmanaged windows are configured exactly as requested and synced. */
	ASSERT_EQ(mock_xconfigure_window.count, 1, "configurerequest nonclient: one XConfigureWindow");
	ASSERT_EQ(mock_xconfigure_window.win, (unsigned long)99999, "configurerequest nonclient: requested window");
	ASSERT_EQ(mock_xconfigure_window.mask,
	          (unsigned long)(CWX|CWY|CWWidth|CWHeight|CWBorderWidth),
	          "configurerequest nonclient: value mask forwarded");
	ASSERT_EQ(mock_xconfigure_last.x, 11, "configurerequest nonclient: x payload");
	ASSERT_EQ(mock_xconfigure_last.y, 22, "configurerequest nonclient: y payload");
	ASSERT_EQ(mock_xconfigure_last.width, 333, "configurerequest nonclient: width payload");
	ASSERT_EQ(mock_xconfigure_last.height, 444, "configurerequest nonclient: height payload");
	ASSERT_EQ(mock_xconfigure_last.border_width, 5, "configurerequest nonclient: border payload");
	ASSERT_EQ(mock_xconfigure_last.sibling, (unsigned long)777, "configurerequest nonclient: above payload");
	ASSERT_EQ(mock_xconfigure_last.stack_mode, Above, "configurerequest nonclient: detail payload");
	ASSERT_EQ(mock_xsync_calls, 1, "configurerequest nonclient: one XSync");
}

/* --- configurerequest with floating geometry --- */
static void
test_configurerequest_floating_fullmask(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->bw = 0;
	c->isfloating = 1;
	c->x = 100; c->y = 100; c->w = 200; c->h = 200;
	m->clients = c;
	m->sel = c;
	save_selmon();
	selmon = m;
	mons = m;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xconfigurerequest.window = 1;
	ev.xconfigurerequest.value_mask = CWX|CWY|CWWidth|CWHeight;
	ev.xconfigurerequest.x = 50;
	ev.xconfigurerequest.y = 60;
	ev.xconfigurerequest.width = 300;
	ev.xconfigurerequest.height = 400;

	configurerequest(&ev);
	/* A floating client accepts the full geometry, offset by the monitor. */
	ASSERT_EQ(c->x, 50, "configurerequest floating fullmask: x updated");
	ASSERT_EQ(c->y, 60, "configurerequest floating fullmask: y updated");
	ASSERT_EQ(c->w, 300, "configurerequest floating fullmask: width updated");
	ASSERT_EQ(c->h, 400, "configurerequest floating fullmask: height updated");
	ASSERT_EQ(c->oldx, 100, "configurerequest floating fullmask: oldx saved");
	ASSERT_EQ(c->oldy, 100, "configurerequest floating fullmask: oldy saved");
	ASSERT_EQ(mock_xmoveresize_window.count, 1, "configurerequest floating fullmask: XMoveResizeWindow called");
	ASSERT_EQ(mock_xmoveresize_window.win, (unsigned long)c->win, "configurerequest floating fullmask: resized client window");
	ASSERT_EQ(mock_xmoveresize_window.x, 50, "configurerequest floating fullmask: moved x");
	ASSERT_EQ(mock_xmoveresize_window.y, 60, "configurerequest floating fullmask: moved y");
	ASSERT_EQ(mock_xmoveresize_window.w, 300u, "configurerequest floating fullmask: resized width");
	ASSERT_EQ(mock_xmoveresize_window.h, 400u, "configurerequest floating fullmask: resized height");
	ASSERT_EQ(mock_xsend_event.count, 0, "configurerequest floating fullmask: no synthetic configure");
	ASSERT_EQ(mock_xsync_calls, 1, "configurerequest floating fullmask: one XSync");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

static void
test_configurerequest_client_borderwidth(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->bw = 0;
	c->isfloating = 1;
	m->clients = c;
	m->sel = c;
	save_selmon();
	selmon = m;
	mons = m;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xconfigurerequest.window = 1;
	ev.xconfigurerequest.value_mask = CWBorderWidth;
	ev.xconfigurerequest.border_width = 5;

	configurerequest(&ev);
	ASSERT_EQ(c->bw, 5, "configurerequest: border width updated");
	/* Border-width requests take the first branch: geometry is untouched. */
	ASSERT_EQ(c->w, 200, "configurerequest borderwidth: width unchanged");
	ASSERT_EQ(mock_xmoveresize_window.count, 0, "configurerequest borderwidth: no geometry move");
	ASSERT_EQ(mock_xsend_event.count, 0, "configurerequest borderwidth: no synthetic configure");
	ASSERT_EQ(mock_xsync_calls, 1, "configurerequest borderwidth: one XSync");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

static void
test_configurerequest_floating_geometry_partial(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->bw = 0;
	c->isfloating = 1;
	c->x = 0; c->y = 0; c->w = 100; c->h = 100;
	m->clients = c;
	save_selmon();
	selmon = m;
	mons = m;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xconfigurerequest.window = 1;
	ev.xconfigurerequest.value_mask = CWX | CWY | CWWidth | CWHeight;
	ev.xconfigurerequest.x = 50;
	ev.xconfigurerequest.y = 60;
	ev.xconfigurerequest.width = 300;
	ev.xconfigurerequest.height = 400;

	configurerequest(&ev);
	ASSERT_EQ(c->x, 50, "configurerequest: x updated");
	ASSERT_EQ(c->y, 60, "configurerequest: y updated");
	ASSERT_EQ(c->w, 300, "configurerequest: width updated");
	ASSERT_EQ(c->h, 400, "configurerequest: height updated");
	ASSERT_EQ(c->oldw, 100, "configurerequest: old width saved");
	ASSERT_EQ(c->oldh, 100, "configurerequest: old height saved");
	ASSERT_EQ(mock_xmoveresize_window.count, 1, "configurerequest partial: XMoveResizeWindow called");
	ASSERT_EQ(mock_xmoveresize_window.win, (unsigned long)c->win, "configurerequest partial: resized client window");
	ASSERT_EQ(mock_xsync_calls, 1, "configurerequest partial: one XSync");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- configurerequest floating position-only configure (line 517) --- */
static void
test_configurerequest_floating_pos_only(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->bw = 0;
	c->isfloating = 1;
	c->x = 0; c->y = 0; c->w = 200; c->h = 200;
	m->clients = c;
	m->sel = c;
	save_selmon();
	selmon = m;
	mons = m;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xconfigurerequest.window = 1;
	ev.xconfigurerequest.value_mask = CWX | CWY;
	ev.xconfigurerequest.x = 50;
	ev.xconfigurerequest.y = 60;

	configurerequest(&ev);
	ASSERT_EQ(c->x, 50, "configurerequest floating pos-only: x updated");
	ASSERT_EQ(c->y, 60, "configurerequest floating pos-only: y updated");
	ASSERT_EQ(c->w, 200, "configurerequest floating pos-only: width unchanged");
	/* A position-only request also synthesizes a ConfigureNotify to the client. */
	ASSERT_EQ(mock_xsend_event.count, 1, "configurerequest pos-only: synthetic configure sent");
	ASSERT_EQ(mock_xsend_event.win, (unsigned long)c->win, "configurerequest pos-only: configure targets client");
	ASSERT_EQ(mock_xsend_event.mask, (unsigned long)StructureNotifyMask, "configurerequest pos-only: StructureNotifyMask");
	ASSERT_EQ(mock_xsend_event_last.type, ConfigureNotify, "configurerequest pos-only: ConfigureNotify type");
	ASSERT_EQ(mock_xsend_event_last.xconfigure.x, 50, "configurerequest pos-only: notify x");
	ASSERT_EQ(mock_xsend_event_last.xconfigure.y, 60, "configurerequest pos-only: notify y");
	ASSERT_EQ(mock_xsend_event_last.xconfigure.width, 200, "configurerequest pos-only: notify width");
	ASSERT_EQ(mock_xsend_event_last.xconfigure.height, 200, "configurerequest pos-only: notify height");
	ASSERT_EQ(mock_xmoveresize_window.count, 1, "configurerequest pos-only: XMoveResizeWindow called");
	ASSERT_EQ(mock_xsync_calls, 1, "configurerequest pos-only: one XSync");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- configurerequest non-floating arrange configure (line 521) --- */
static void
test_configurerequest_nonfloating_arrange(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->bw = 0;
	c->isfloating = 0;
	m->clients = c;
	m->sel = c;
	save_selmon();
	selmon = m;
	mons = m;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xconfigurerequest.window = 1;
	ev.xconfigurerequest.value_mask = CWX | CWY;
	ev.xconfigurerequest.x = 10;
	ev.xconfigurerequest.y = 20;

	configurerequest(&ev);
	/* Tiled clients are refused: dwm replies with its own (unchanged) geometry. */
	ASSERT_EQ(c->x, 100, "configurerequest nonfloating: x refused");
	ASSERT_EQ(c->y, 100, "configurerequest nonfloating: y refused");
	ASSERT_EQ(mock_xsend_event.count, 1, "configurerequest nonfloating: synthetic configure sent");
	ASSERT_EQ(mock_xsend_event.win, (unsigned long)c->win, "configurerequest nonfloating: configure targets client");
	ASSERT_EQ(mock_xsend_event_last.type, ConfigureNotify, "configurerequest nonfloating: ConfigureNotify type");
	ASSERT_EQ(mock_xsend_event_last.xconfigure.x, 100, "configurerequest nonfloating: dwm's x reported");
	ASSERT_EQ(mock_xsend_event_last.xconfigure.y, 100, "configurerequest nonfloating: dwm's y reported");
	ASSERT_EQ(mock_xmoveresize_window.count, 0, "configurerequest nonfloating: no XMoveResizeWindow");
	ASSERT_EQ(mock_xsync_calls, 1, "configurerequest nonfloating: one XSync");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

static void
run_tests(void)
{
	test_configurerequest_nonclient();
	test_configurerequest_floating_fullmask();
	test_configurerequest_client_borderwidth();
	test_configurerequest_floating_geometry_partial();
	test_configurerequest_floating_pos_only();
	test_configurerequest_nonfloating_arrange();
}
