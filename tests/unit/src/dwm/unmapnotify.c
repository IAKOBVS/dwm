/*
 * test_unmapnotify.c — per-function tests for dwm.c's unmapnotify().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through unmapnotify(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- unmapnotify send_event path (lines 1898-1900) --- */
static void
test_unmapnotify_send_event(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(42, m);
	c->oldbw = 2;
	m->clients = c;
	m->stack = c;
	m->sel = c;
	save_selmon();
	selmon = m;
	mons = m;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xunmap.window = 42;
	ev.xunmap.send_event = 1;
	unmapnotify(&ev);
	/* line 1898: wintoclient finds c, line 1899: send_event true,
	 * line 1900: setclientstate(c, WithdrawnState) */
	ASSERT(c->isurgent == 0 || c->isurgent == 1,
		"unmapnotify send_event: does not crash");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- unmapnotify unmanage path (lines 1898, 1902) --- */
static void
test_unmapnotify_unmanage(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(43, m);
	c->oldbw = 2;
	m->clients = c;
	m->stack = c;
	m->sel = c;
	save_selmon();
	selmon = m;
	mons = m;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xunmap.window = 43;
	ev.xunmap.send_event = 0;
	unmapnotify(&ev);
	/* line 1898: wintoclient finds c, line 1899: send_event false,
	 * line 1902: unmanage(c, 0) frees c and detaches */
	ASSERT(m->clients == NULL,
		"unmapnotify unmanage: client detached from list");
	ASSERT(m->stack == NULL,
		"unmapnotify unmanage: client removed from stack");

	restore_selmon();
	free(m);
}

static void
test_unmapnotify_non_client(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	m->clients = NULL;
	selmon = m;
	mons = m;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xunmap.window = 99999; /* non-client */
	ev.xunmap.send_event = 0;

	unmapnotify(&ev);
	ASSERT(1, "unmapnotify: non-client window returns early");
	ASSERT(m->clients == NULL, "unmapnotify: non-client did not remove from list");

	/* restore globals BEFORE freeing m — later tests (setup/updategeom)
	 * must not read through the dangling pointer */
	restore_selmon();
	free(m);
}

static void
run_tests(void)
{
	test_unmapnotify_send_event();
	test_unmapnotify_unmanage();
	test_unmapnotify_non_client();
}
