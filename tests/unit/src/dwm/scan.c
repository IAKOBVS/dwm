/*
 * test_scan.c — per-function tests for dwm.c's scan().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through scan(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* Count the clients linked into a monitor's client list. */
static int
client_list_len(Monitor *m)
{
	int n = 0;
	Client *p;
	for (p = m->clients; p; p = p->next)
		n++;
	return n;
}

/* --- scan --- */
static void
test_scan_no_windows(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	save_selmon();
	selmon = make_monitor(0);
	mons = selmon;

	scan();
	/* No children from XQueryTree means no clients are adopted. */
	ASSERT_EQ(wintoclient(501), NULL, "scan no windows: no client indexed");
	ASSERT_EQ(client_list_len(selmon), 0, "scan no windows: client list empty");
	ASSERT_EQ(selmon->stack, NULL, "scan no windows: focus stack empty");
	ASSERT_EQ(winhash_count, 0u, "scan no windows: window index empty");
	ASSERT_EQ(mock_xmap_window.count, 0, "scan no windows: no windows mapped");
	ASSERT_EQ(mock_xconfigure_window.count, 0, "scan no windows: no windows configured");

{
		Monitor *tmpm = selmon; /* free the fixture, then restore globals */
		restore_selmon();
		free(tmpm);
	}
}

/* --- scan with windows (lines 1410-1434) --- */
static void
test_scan_with_windows(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	save_selmon();
	Monitor *m = make_monitor(0);
	selmon = m;
	mons = m;

	/* Set up two windows for XQueryTree to return */
	static Window scan_wins[2];
	scan_wins[0] = 501;
	scan_wins[1] = 502;
	mock_querytree_return = 1;
	mock_querytree_root = 42;
	mock_querytree_children = scan_wins;
	mock_querytree_nchildren = 2;

	/* Override_redirect=0, map_state=IsViewable, no transient */
	mock_override_redirect = 0;

	scan();
	/* Both windows are viewable non-transients, adopted in the first loop. */
	ASSERT(wintoclient(501) != NULL, "scan with windows: window 501 managed");
	ASSERT(wintoclient(502) != NULL, "scan with windows: window 502 managed");
	ASSERT_EQ(client_list_len(m), 2, "scan with windows: two clients linked");
	ASSERT_EQ(winhash_count, 2u, "scan with windows: two windows indexed");
	ASSERT(m->sel != NULL, "scan with windows: a selection was set");
	ASSERT_EQ(mock_xmap_window.count, 2, "scan with windows: both windows mapped");
	ASSERT(mock_xconfigure_window.count >= 2, "scan with windows: windows configured");

	mock_querytree_return = 0;
	mock_querytree_children = NULL;
	mock_querytree_nchildren = 0;
	mock_x11_reset();
	drain_clients(); /* release dwm-created clients while list is live */
	restore_selmon();
	free(m); /* fixture leaked (ASan-reported) */
}

/* --- scan with override_redirect window (line 1419) --- */
static void
test_scan_override_redirect(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	save_selmon();
	Monitor *m = make_monitor(0);
	selmon = m;
	mons = m;

	static Window scan_wins2[1];
	scan_wins2[0] = 601;
	mock_querytree_return = 1;
	mock_querytree_root = 42;
	mock_querytree_children = scan_wins2;
	mock_querytree_nchildren = 1;

	/* Set override_redirect to 1 so window is skipped in both loops. */
	mock_override_redirect = 1;

	scan();
	/* Window 601 should be skipped (override_redirect) */
	Client *c = wintoclient(601);
	ASSERT(c == NULL, "scan: override_redirect window not managed");
	ASSERT_EQ(client_list_len(m), 0, "scan override_redirect: client list empty");
	ASSERT_EQ(winhash_count, 0u, "scan override_redirect: no windows indexed");
	ASSERT_EQ(mock_xmap_window.count, 0, "scan override_redirect: window not mapped");

	mock_querytree_return = 0;
	mock_querytree_children = NULL;
	mock_querytree_nchildren = 0;
	mock_x11_reset();
	restore_selmon();
	free(m); /* fixture leaked (ASan-reported) */
}

/* --- scan with transient window (lines 1424-1429) --- */
static void
test_scan_transient_window(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	save_selmon();
	Monitor *m = make_monitor(0);
	selmon = m;
	mons = m;

	static Window scan_wins3[2];
	scan_wins3[0] = 701; /* normal window */
	scan_wins3[1] = 702; /* transient window */
	mock_querytree_return = 1;
	mock_querytree_root = 42;
	mock_querytree_children = scan_wins3;
	mock_querytree_nchildren = 2;

	/* First loop: with no transient hint both windows are managed there. */
	mock_gettransient_return = 0; /* no transient hints */
	mock_override_redirect = 0;

	scan();
	ASSERT(wintoclient(701) != NULL, "scan transient: window 701 managed in first loop");
	ASSERT(wintoclient(702) != NULL, "scan transient: window 702 managed in first loop");
	ASSERT_EQ(client_list_len(m), 2, "scan transient: two clients linked");
	ASSERT_EQ(winhash_count, 2u, "scan transient: two windows indexed");
	ASSERT_EQ(mock_xmap_window.count, 2, "scan transient: two windows mapped");

	drain_clients();
	mock_x11_reset();

	/* Second loop: with transient hints both windows are still adopted. */
	mock_gettransient_return = 1;
	mock_gettransient_win = 701;
	mock_override_redirect = 0;
	mock_querytree_return = 1;
	mock_querytree_root = 42;
	mock_querytree_children = scan_wins3;
	mock_querytree_nchildren = 2;

	scan();
	ASSERT(wintoclient(701) != NULL, "scan transient: window 701 managed as transient");
	ASSERT(wintoclient(702) != NULL, "scan transient: window 702 managed as transient");
	ASSERT_EQ(client_list_len(m), 2, "scan transient: transients linked into client list");
	ASSERT_EQ(winhash_count, 2u, "scan transient: transients indexed");
	ASSERT_EQ(mock_xmap_window.count, 2, "scan transient: transients mapped");

	mock_gettransient_return = 0;
	mock_querytree_return = 0;
	mock_querytree_children = NULL;
	mock_querytree_nchildren = 0;
	mock_x11_reset();
	drain_clients(); /* release dwm-created clients while list is live */
	restore_selmon();
	free(m); /* fixture leaked (ASan-reported) */
}

static void
test_scan_iconicstate(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	save_selmon();
	Monitor *m = make_monitor(0);
	selmon = m;
	mons = m;

	static Window iconic_wins[1];
	iconic_wins[0] = 801;
	mock_querytree_return = 1;
	mock_querytree_root = 42;
	mock_querytree_children = iconic_wins;
	mock_querytree_nchildren = 1;
	mock_override_redirect = 0;
	mock_map_state = IsUnviewable;
	mock_getwindowproperty_return = 1;
	mock_getwindowproperty_atom = IconicState;
	mock_gettransient_return = 0;

	/* Unmapped WM_STATE=IconicState windows are managed in the first loop. */
	scan();
	ASSERT(wintoclient(801) != NULL, "scan iconicstate: iconic window managed in first loop");
	ASSERT_EQ(client_list_len(m), 1, "scan iconicstate: one client linked");
	ASSERT_EQ(winhash_count, 1u, "scan iconicstate: one window indexed");
	ASSERT_EQ(mock_xmap_window.count, 1, "scan iconicstate: iconic window mapped");

	drain_clients();
	mock_x11_reset();

	/* With a transient hint it is still adopted, now in the second loop. */
	mock_gettransient_return = 1;
	mock_gettransient_win = 42;
	mock_map_state = IsUnviewable;
	mock_getwindowproperty_return = 1;
	mock_getwindowproperty_atom = IconicState;
	mock_querytree_return = 1;
	mock_querytree_root = 42;
	mock_querytree_children = iconic_wins;
	mock_querytree_nchildren = 1;

	scan();
	ASSERT(wintoclient(801) != NULL, "scan iconicstate: iconic transient managed in second loop");
	ASSERT_EQ(client_list_len(m), 1, "scan iconicstate: iconic transient linked");
	ASSERT_EQ(winhash_count, 1u, "scan iconicstate: iconic transient indexed");
	ASSERT_EQ(mock_xmap_window.count, 1, "scan iconicstate: iconic transient mapped");

	mock_getwindowproperty_return = 0;
	mock_getwindowproperty_atom = 0;
	mock_map_state = IsViewable;
	mock_gettransient_return = 0;
	mock_querytree_return = 0;
	mock_querytree_children = NULL;
	mock_querytree_nchildren = 0;
	mock_x11_reset();
	drain_clients(); /* release dwm-created clients while list is live */
	restore_selmon();
	free(m); /* fixture leaked (ASan-reported) */
}

/* --- scan second-loop XGetWindowAttributes fail (line 1426) --- */
static void
test_scan_xgetwindowattr_fail(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	save_selmon();
	Monitor *m = make_monitor(0);
	selmon = m;
	mons = m;

	static Window scan_wins[1];
	scan_wins[0] = 801;
	mock_querytree_return = 1;
	mock_querytree_root = 42;
	mock_querytree_children = scan_wins;
	mock_querytree_nchildren = 1;

	/* Window 801: non-override, non-transient → managed in first loop
	 * Second loop: XGetWindowAttributes fails on call 2 → line 1426 */
	mock_override_redirect = 0;
	mock_gettransient_return = 0;
	mock_getwindowattr_fail_at = 2;  /* fail on 2nd call (second loop) */
	mock_getwindowattr_call_count = 0;

	scan();
	/* Window managed in first loop, skipped in second loop */
	Client *c = wintoclient(801);
	ASSERT(c != NULL, "scan xgetwindowattr fail: window managed in first loop");
	ASSERT_EQ(client_list_len(m), 1, "scan xgetwindowattr fail: single client linked");
	ASSERT_EQ(winhash_count, 1u, "scan xgetwindowattr fail: single window indexed");
	ASSERT_EQ(mock_xmap_window.count, 1, "scan xgetwindowattr fail: managed only once");
	ASSERT_EQ(mock_getwindowattr_call_count, 2, "scan xgetwindowattr fail: both loops probed attributes");

	mock_querytree_return = 0;
	mock_querytree_children = NULL;
	mock_querytree_nchildren = 0;
	mock_getwindowattr_fail_at = 0;
	mock_getwindowattr_call_count = 0;
	mock_x11_reset();
	drain_clients(); /* release dwm-created clients while list is live */
	restore_selmon();
	free(m); /* fixture leaked (ASan-reported) */
}

static void
run_tests(void)
{
	test_scan_no_windows();
	test_scan_with_windows();
	test_scan_override_redirect();
	test_scan_transient_window();
	test_scan_iconicstate();
	test_scan_xgetwindowattr_fail();
}
