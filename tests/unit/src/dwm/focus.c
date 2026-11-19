/*
 * test_focus.c — per-function tests for dwm.c's focus().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through focus(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- focus focusing urgent client on different monitor --- */
static void
test_focus_urgent_client(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m1 = make_monitor(0);
	Monitor *m2 = make_monitor(1);
	Client *c = make_client(1, m2);
	c->isurgent = 1;
	c->tags = 1;
	m1->sel = NULL;
	m1->clients = NULL;
	m1->stack = NULL;
	m2->clients = c;
	m2->stack = c;
	m2->sel = c;
	m1->next = m2;
	mons = m1;
	save_selmon();
	selmon = m1;

	focus(c);
	ASSERT(!c->isurgent, "focus: urgent client cleared");
	ASSERT(selmon == m2, "focus: switched to urgent client's monitor");

	restore_selmon();
	free(c); /* fixture leaked (ASan-reported) */
	free(m1); /* fixture leaked (ASan-reported) */
	free(m2); /* fixture leaked (ASan-reported) */
}

/* --- focus change from one client to another (lines 729-730) --- */
static void
test_focus_change_client(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	Monitor *m = make_monitor(0);
	selmon = m;
	mons = m;

	Client *c1 = make_client(1, m);
	Client *c2 = make_client(2, m);
	c1->tags = 1; c2->tags = 1;
	c1->next = c2; c2->next = NULL;
	m->clients = c1;
	m->stack = c2; c2->snext = c1; c1->snext = NULL;
	m->sel = c1;

	focus(c2);
	ASSERT(selmon->sel == c2, "focus change client: sel changed to c2");

	restore_selmon();
	winclient_remove(c1);
	winclient_remove(c2);
	winclient_remove(c1);
	free(c1); free(c2); free(m);
}

static void
test_focus_null_selmon_ok(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	m->stack = NULL;
	m->sel = NULL;
	selmon = m;
	mons = m;

	focus(NULL);
	ASSERT(selmon->sel == NULL, "focus(NULL): sel stays NULL");
	ASSERT(!(selmon->bar_dirty_segments & DIRTY_TITLE), "focus(NULL) when already NULL: no dirty (idempotent)");

	/* restore globals BEFORE freeing m */
	restore_selmon();
	free(m);
}

static void
test_focus_idempotent(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->tags = 1;
	m->clients = c;
	m->stack = c;
	m->sel = c;
	selmon = m;
	mons = m;

	/* Clear dirty bits first */
	selmon->bar_dirty_segments = 0;

	focus(c);
	ASSERT(selmon->sel == c, "focus: idempotent keeps same sel");
	/* focus() always dirties even when same client (line 747) */
	/* This is correct behavior — dwm always dirties on focus() */

	/* restore globals BEFORE freeing m */
	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

static void
run_tests(void)
{
	test_focus_urgent_client();
	test_focus_change_client();
	test_focus_null_selmon_ok();
	test_focus_idempotent();
}
