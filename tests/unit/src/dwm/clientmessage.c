/*
 * test_clientmessage.c — per-function tests for dwm.c's clientmessage().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through clientmessage(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- clientmessage --- */
static void
test_clientmessage_noop_on_unknown(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	selmon = make_monitor(0);
	mons = selmon;
	selmon->barwin = 0;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xclient.window = 99999;
	ev.xclient.message_type = 0;
	ev.xclient.format = 32;

	clientmessage(&ev);
	ASSERT(1, "clientmessage: unknown window no crash");

	free(selmon);
	restore_selmon();
}

/* --- clientmessage fullscreen/urgent --- */
static void
test_clientmessage_fullscreen_add(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	m->clients = c;
	m->sel = c;
	save_selmon();
	selmon = m;
	mons = m;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xclient.window = 1;
	ev.xclient.message_type = netatom[NetWMState];
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = 1; /* _NET_WM_STATE_ADD */
	ev.xclient.data.l[1] = netatom[NetWMFullscreen];

	clientmessage(&ev);
	ASSERT(c->isfullscreen, "clientmessage: fullscreen add sets isfullscreen");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

static void
test_clientmessage_fullscreen_remove(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->isfullscreen = 1;
	m->clients = c;
	m->sel = c;
	save_selmon();
	selmon = m;
	mons = m;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xclient.window = 1;
	ev.xclient.message_type = netatom[NetWMState];
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = 0; /* _NET_WM_STATE_REMOVE */
	ev.xclient.data.l[1] = netatom[NetWMFullscreen];

	clientmessage(&ev);
	ASSERT(!c->isfullscreen, "clientmessage: fullscreen remove clears isfullscreen");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

static void
test_clientmessage_netactivewindow_urgent(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->isurgent = 0;
	m->sel = NULL; /* not selmon->sel */
	m->clients = c;
	save_selmon();
	selmon = m;
	mons = m;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xclient.window = 1;
	ev.xclient.message_type = netatom[NetActiveWindow];
	ev.xclient.format = 32;

	clientmessage(&ev);
	ASSERT(c->isurgent, "clientmessage: NetActiveWindow sets urgent");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- clientmessage fullscreen toggle via l[2] (line 427) --- */
static void
test_clientmessage_fullscreen_toggle_l2(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->isfullscreen = 0;
	m->clients = c;
	m->sel = c;
	save_selmon();
	selmon = m;
	mons = m;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xclient.window = 1;
	ev.xclient.message_type = netatom[NetWMState];
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = 2; /* _NET_WM_STATE_TOGGLE */
	ev.xclient.data.l[1] = 0;
	ev.xclient.data.l[2] = netatom[NetWMFullscreen];

	clientmessage(&ev);
	ASSERT(c->isfullscreen, "clientmessage: fullscreen toggle via l[2] sets isfullscreen");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

static void
run_tests(void)
{
	test_clientmessage_noop_on_unknown();
	test_clientmessage_fullscreen_add();
	test_clientmessage_fullscreen_remove();
	test_clientmessage_netactivewindow_urgent();
	test_clientmessage_fullscreen_toggle_l2();
}
