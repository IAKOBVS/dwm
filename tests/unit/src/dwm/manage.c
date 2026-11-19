/*
 * test_manage.c — per-function tests for dwm.c's manage().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through manage(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- manage --- */
static void
test_manage_new_window(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	Monitor *m = make_monitor(0);
	selmon = m;
	mons = m;
	m->clients = NULL;
	m->stack = NULL;
	m->sel = NULL;
	m->tagset[0] = 1;

	XWindowAttributes wa;
	memset(&wa, 0, sizeof wa);
	wa.x = 0; wa.y = 0;
	wa.width = 200; wa.height = 200;
	wa.border_width = 0;
	wa.colormap = DefaultColormap(dpy, screen);
	wa.map_state = IsViewable;

	manage(99, &wa);
	ASSERT(m->clients != NULL, "manage: client added to list");

	drain_clients(); /* release dwm-created clients while list is live */
	restore_selmon();
	free(m); /* fixture leaked (ASan-reported) */
}

/* --- configurerequest centering floating windows (lines 494, 512-517) --- */
static void
test_manage_centers_floating(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	Monitor *m = make_monitor(0);
	m->showbar = 0;
	selmon = m;
	mons = m;

	/* Create a floating client on this monitor that extends beyond bounds */
	Client *c = make_client(100, m);
	c->isfloating = 1;
	c->x = 50;
	c->y = 50;
	c->w = 2000;  /* wider than monitor (1920) */
	c->h = 1200;  /* taller than monitor (1080) */
	c->oldx = c->x;
	c->oldy = c->y;
	c->oldw = c->w;
	c->oldh = c->h;
	m->clients = c;
	m->sel = c;
	m->tagset[0] = 1;

	/* Build a configurerequest event requesting the oversized geometry */
	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xconfigurerequest.window = 100;
	ev.xconfigurerequest.x = 50;
	ev.xconfigurerequest.y = 50;
	ev.xconfigurerequest.width = 2000;
	ev.xconfigurerequest.height = 1200;
	ev.xconfigurerequest.border_width = 0;
	ev.xconfigurerequest.value_mask = CWX | CWY | CWWidth | CWHeight;

	configurerequest(&ev);

	/* Lines 512-513: x+w > mx+mw => centered in x direction.
	 * c->x should be mx + (mw/2 - WIDTH(c)/2) = 0 + (960 - 1000) = -40
	 * Since WIDTH(c) = c->w + 2*c->bw = 2000, c->x = 0 + (960 - 1000) = -40 */
	ASSERT(c->x == -40, "configurerequest: floating oversized x centered");

	/* Lines 514-515: y+h > my+mh => centered in y direction.
	 * HEIGHT(c) = h + 2*bw = 1200 + 0 = 1200
	 * c->y = my + (mh/2 - HEIGHT(c)/2) = 0 + (540 - 600) = -60 */
	ASSERT(c->y == -60, "configurerequest: floating oversized y centered");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- manage transient for hint (lines 1021-1023, 1048) --- */
static void
test_manage_transient(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	save_selmon();
	Monitor *m = make_monitor(0);
	m->clients = NULL;
	m->stack = NULL;
	m->sel = NULL;
	m->tagset[0] = 1;
	selmon = m;
	mons = m;

	/* Create a "parent" client that owns window 50 */
	Client *parent = make_client(50, m);
	parent->tags = 4;
	strncpy(parent->name, "parent", sizeof parent->name);
	m->clients = parent;

	/* Configure mock: XGetTransientForHint succeeds, returns window 50 */
	mock_gettransient_return = 1;
	mock_gettransient_win = 50;

	XWindowAttributes wa;
	memset(&wa, 0, sizeof wa);
	wa.x = 0; wa.y = 0;
	wa.width = 100; wa.height = 100;
	wa.border_width = 0;
	wa.colormap = DefaultColormap(dpy, screen);
	wa.map_state = IsViewable;

	manage(60, &wa);

	/* Find the new client (window 60) */
	Client *c = wintoclient(60);
	ASSERT(c != NULL, "manage transient: new client created");

	/* Lines 1021-1023: inherited parent's mon and tags */
	if (c) {
		ASSERT(c->mon == m, "manage transient: inherits parent mon");
		ASSERT_EQ(c->tags, (unsigned)4, "manage transient: inherits parent tags");
	}

	/* Line 1048: isfloating set because trans != None */
	if (c) {
		ASSERT(c->isfloating, "manage transient: isfloating set from trans");
	}

	drain_clients(); /* release dwm-created clients while list is live */
	restore_selmon();
	mock_x11_reset();
	free(m);
}

/* --- manage swallow path (line 1063) --- */
static void
test_manage_swallows(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	save_selmon();
	Monitor *m = make_monitor(0);
	m->clients = NULL;
	m->stack = NULL;
	m->sel = NULL;
	m->tagset[0] = 1;
	selmon = m;
	mons = m;

	/* Create a terminal client already in the client list.
	 * Even though winpid() returns 0 in mock (so termforwin returns NULL
	 * and line 1063 is not reached), this exercises the manage path
	 * with a terminal client present. */
	Client *term = make_client(40, m);
	term->isterminal = 1;
	term->pid = 1000;
	term->noswallow = 0;
	term->swallowing = NULL;
	term->tags = 1;
	strncpy(term->name, "terminal", sizeof term->name);
	m->clients = term;

	XWindowAttributes wa;
	memset(&wa, 0, sizeof wa);
	wa.x = 0; wa.y = 0;
	wa.width = 200; wa.height = 200;
	wa.border_width = 0;
	wa.colormap = DefaultColormap(dpy, screen);
	wa.map_state = IsViewable;

	manage(70, &wa);

	/* No transient hint, so termforwin is called.
	 * winpid() returns 0 in mock, so termforwin returns NULL.
	 * Line 1062: if (term) => false, swallow not called.
	 * But the manage function completes without crash. */
	Client *c = wintoclient(70);
	ASSERT(c != NULL, "manage swallow: new client created alongside terminal");
	ASSERT(c != term, "manage swallow: new client is not the terminal");

	drain_clients(); /* release dwm-created clients while list is live */
	restore_selmon();
	mock_x11_reset();
	free(m);
}

/* --- manage swallow (line 1063) --- */
static void
test_manage_swallow_line1063(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	save_selmon();
	Monitor *m = make_monitor(0);
	selmon = m;
	mons = m;
	m->clients = NULL;
	m->stack = NULL;
	m->sel = NULL;
	m->tagset[0] = 1;
	m->gap.isgap = 0;

	/* Mock winpid to return PID 5000 for any window */
	mock_winpid_set = 1;
	mock_winpid_value = 5000;

	/* Create a terminal client with matching PID */
	Client *term = make_client(40, m);
	term->isterminal = 1;
	term->pid = 5000;
	term->noswallow = 0;
	term->swallowing = NULL;
	term->tags = 1;
	m->clients = term;

	/* manage() calls winpid() which returns 5000 via mock,
	 * termforwin() finds term (same PID, isdescprocess(5000,5000) → true),
	 * swallow(term, c) is called at line 1063 */
	XWindowAttributes wa;
	memset(&wa, 0, sizeof wa);
	wa.x = 100; wa.y = 100;
	wa.width = 200; wa.height = 200;
	wa.border_width = 0;
	wa.colormap = DefaultColormap(dpy, screen);
	wa.map_state = IsViewable;

	manage(601, &wa);

	/* After swallow: term->swallowing should be set */
	ASSERT(term->swallowing != NULL, "manage swallow: terminal has swallowing set");
	ASSERT(term->swallowing->win == 40, "manage swallow: swallowed client has terminal's old win");

	mock_x11_reset();
	drain_clients(); /* consumes term via unmanage(swallow path) + frees it */
	restore_selmon();
	free(m);
}

static void
run_tests(void)
{
	test_manage_new_window();
	test_manage_centers_floating();
	test_manage_transient();
	test_manage_swallows();
	test_manage_swallow_line1063();
}
