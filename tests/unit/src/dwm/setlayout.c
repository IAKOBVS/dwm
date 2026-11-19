/*
 * test_setlayout.c — per-function tests for dwm.c's setlayout().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through setlayout(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- setlayout --- */
static void
test_setlayout_zero(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	m->lt[0] = &layouts[0];
	m->lt[1] = &layouts[0];
	save_selmon();
	selmon = m;
	mons = m;
	Arg arg = { .v = NULL }; /* zero pointer arg toggles the layout */

	setlayout(&arg);
	ASSERT(1, "setlayout: does not crash");

	restore_selmon();
	free(m); /* fixture leaked (ASan-reported) */
}

/* --- setlayout with non-zero arg --- */
static void
test_setlayout_with_arg(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	m->lt[0] = &layouts[0];
	m->lt[1] = &layouts[0];
	m->sellt = 0;
	save_selmon();
	selmon = m;
	mons = m;
	Arg arg = { .v = (void*)&layouts[1] };

	setlayout(&arg);
	ASSERT(selmon->lt[selmon->sellt] == &layouts[1], "setlayout: layout set by arg");

	restore_selmon();
	free(m); /* fixture leaked (ASan-reported) */
}

/* --- setlayout with selmon->sel non-NULL (line 1569) --- */
static void
test_setlayout_with_sel(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->tags = 1;
	m->lt[0] = &layouts[0];
	m->lt[1] = &layouts[0];
	m->sellt = 0;
	m->sel = c;
	m->clients = c;
	save_selmon();
	selmon = m;
	mons = m;
	Arg arg = { .v = (void*)&layouts[1] };

	setlayout(&arg);
	/* Line 1568: selmon->sel is non-NULL, so line 1569: arrange(selmon) called */
	ASSERT(selmon->lt[selmon->sellt] == &layouts[1], "setlayout with sel: layout set");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

static void
test_setlayout_arrange_monitor_null_gap(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	m->lt[0] = &layouts[0];
	m->lt[1] = &layouts[0];
	m->sel = NULL;
	selmon = m;
	mons = m;
	Arg arg = { .v = (void*)&layouts[1] };

	setlayout(&arg);
	ASSERT(selmon->lt[selmon->sellt] == &layouts[1], "setlayout: switches to monocle");
	ASSERT(selmon->bar_dirty_segments & DIRTY_TAGS, "setlayout: dirties tags");

	/* restore globals BEFORE freeing m — later tests (setup/updategeom)
	 * must not read through the dangling pointer */
	restore_selmon();
	free(m);
}

static void
run_tests(void)
{
	test_setlayout_zero();
	test_setlayout_with_arg();
	test_setlayout_with_sel();
	test_setlayout_arrange_monitor_null_gap();
}
