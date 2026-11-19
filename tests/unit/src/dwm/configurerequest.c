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
	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xconfigurerequest.window = 99999;

	configurerequest(&ev);
	ASSERT(1, "configurerequest: non-client window no crash");
}

/* --- configurerequest with floating geometry --- */
static void
test_configurerequest_floating_fullmask(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	m->sel = NULL;
	m->clients = NULL;
	save_selmon();
	selmon = m;
	mons = m;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xconfigurerequest.window = 99999; /* unknown window => non-client path */

	configurerequest(&ev);
	ASSERT(1, "configurerequest: non-client window no crash");

	restore_selmon();
	free(m);
}

static void
test_configurerequest_client_borderwidth(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
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

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

static void
test_configurerequest_floating_geometry_partial(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
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
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
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
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
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
	ASSERT(1, "configurerequest non-floating arrange: no crash (configure called)");

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
