/*
 * test_setmfact.c — per-function tests for dwm.c's setmfact().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through setmfact(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- setmfact --- */
static void
test_setmfact_default(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	m->mfact = 0.55f;
	save_selmon();
	selmon = m;
	mons = m;
	Arg arg = { .f = 1.6f }; /* > 1.0 means absolute; 1.6 - 1.0 = 0.6 */

	setmfact(&arg);
	ASSERT_EQ((int)(selmon->mfact * 100 + 0.5f), 60, "setmfact: updates mfact");

	restore_selmon();
	free(m); /* fixture leaked (ASan-reported) */
}

/* --- setmfact edge cases --- */
static void
test_setmfact_no_layout_arrange(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Layout noarr = { "", NULL }; /* NULL arrange function */
	Monitor *m = make_monitor(0);
	m->lt[0] = &noarr;
	m->lt[1] = &noarr;
	save_selmon();
	selmon = m;
	mons = m;
	float old = m->mfact;
	Arg arg = { .f = 1.5f };

	setmfact(&arg);
	ASSERT_EQ(selmon->mfact, old, "setmfact: unchanged when no arrange function");

	restore_selmon();
	free(m); /* fixture leaked (ASan-reported) */
}

static void
test_setmfact_out_of_range_high(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	m->mfact = 0.55f;
	m->lt[0] = &layouts[0];
	m->lt[1] = &layouts[0];
	save_selmon();
	selmon = m;
	mons = m;
	Arg arg = { .f = 0.99f }; /* f < 1.0 => arg.f + mfact; 0.99 + 0.55 = 1.54 > 0.95 => rejected */
	float old = m->mfact;
	setmfact(&arg);
	ASSERT_EQ(selmon->mfact, old, "setmfact: f=0.99 -> relative -> 1.54 > 0.95, unchanged");

	restore_selmon();
	free(m); /* fixture leaked (ASan-reported) */
}

static void
run_tests(void)
{
	test_setmfact_default();
	test_setmfact_no_layout_arrange();
	test_setmfact_out_of_range_high();
}
