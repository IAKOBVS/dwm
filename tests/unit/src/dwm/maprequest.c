/*
 * test_maprequest.c — per-function tests for dwm.c's maprequest().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through maprequest(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- maprequest --- */
static void
test_maprequest(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	Monitor *m = make_monitor(0);
	selmon = m;
	mons = m;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xmaprequest.window = 999;

	maprequest(&ev);
	ASSERT(1, "maprequest: new window does not crash");

	drain_clients(); /* release dwm-created clients while list is live */
	restore_selmon();
	free(m); /* fixture leaked (ASan-reported) */
}

/* --- maprequest already-managed window --- */
static void
test_maprequest_already_managed(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(42, m);
	m->clients = c;
	mons = m;
	save_selmon();
	selmon = m;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xmaprequest.window = 42;

	maprequest(&ev);
	ASSERT(1, "maprequest: already-managed window no crash");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- maprequest override_redirect early return (line 1089) --- */
static void
test_maprequest_override_redirect(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	save_selmon();
	Monitor *m = make_monitor(0);
	selmon = m;
	mons = m;

	/* Make XGetWindowAttributes return override_redirect=True */
	mock_override_redirect = 1;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xmaprequest.window = 777;

	maprequest(&ev);
	/* Should return early without calling manage, so no client created */
	Client *c = wintoclient(777);
	ASSERT(c == NULL, "maprequest override_redirect: window not managed");

	mock_x11_reset();
	restore_selmon();
	free(m); /* fixture leaked (ASan-reported) */
}

static void
run_tests(void)
{
	test_maprequest();
	test_maprequest_already_managed();
	test_maprequest_override_redirect();
}
