/*
 * test_setgaps.c — per-function tests for dwm.c's setgaps().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through setgaps(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- setgaps --- */
static void
test_setgaps_default(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	m->gap.isgap = 1;
	m->gap.realgap = 17;
	m->gap.gappx = 17;
	save_selmon();
	selmon = m;
	mons = m;
	Arg arg = { .i = GAP_TOGGLE };

	setgaps(&arg);
	ASSERT_EQ(selmon->gap.gappx, 0, "setgaps: toggle gap off");

	restore_selmon();
	free(m); /* fixture leaked (ASan-reported) */
}

/* --- setgaps GAP_RESET --- */
static void
test_setgaps_reset(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	m->gap.isgap = 1;
	m->gap.realgap = 10;
	m->gap.gappx = 10;
	save_selmon();
	selmon = m;
	mons = m;
	Arg arg = { .i = GAP_RESET };

	setgaps(&arg);
	ASSERT_EQ(selmon->gap.gappx, default_gap.gappx,
		"setgaps: GAP_RESET restores default gappx");
	ASSERT_EQ(selmon->gap.realgap, default_gap.realgap,
		"setgaps: GAP_RESET restores default realgap");
	ASSERT_EQ(selmon->gap.isgap, default_gap.isgap,
		"setgaps: GAP_RESET restores default isgap");

	restore_selmon();
	free(m); /* fixture leaked (ASan-reported) */
}

static void
test_setgaps_adjust(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	m->gap.isgap = 1;
	m->gap.realgap = 10;
	m->gap.gappx = 10;
	save_selmon();
	selmon = m;
	mons = m;
	Arg arg = { .i = 5 };

	setgaps(&arg);
	ASSERT_EQ(selmon->gap.realgap, 15, "setgaps: adjust adds to realgap");
	ASSERT_EQ(selmon->gap.gappx, 15, "setgaps: adjust adds to gappx");

	restore_selmon();
	free(m); /* fixture leaked (ASan-reported) */
}

/* --- negative adjustment clamps to zero --- */
static void
test_setgaps_clamps_negative(void)
{
	reset_index();
	Monitor *m = make_monitor(0);
	m->gap.isgap = 1;
	m->gap.realgap = 3;
	m->gap.gappx = 3;
	save_selmon();
	selmon = m;
	mons = m;
	Arg arg = { .i = -100 };

	setgaps(&arg);

	ASSERT_EQ(selmon->gap.realgap, 0, "setgaps: realgap clamps to zero");
	ASSERT_EQ(selmon->gap.gappx, 0, "setgaps: gappx clamps to zero");

	restore_selmon();
	free(m);
}

static void
run_tests(void)
{
	test_setgaps_default();
	test_setgaps_reset();
	test_setgaps_adjust();
	test_setgaps_clamps_negative();
}
