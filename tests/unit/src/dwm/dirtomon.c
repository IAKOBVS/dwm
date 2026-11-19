/*
 * test_dirtomon.c — per-function tests for dwm.c's dirtomon().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through dirtomon(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- dirtomon --- */
static void
test_dirtomon_positive(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m1 = make_monitor(0);
	Monitor *m2 = make_monitor(1);
	m1->next = m2;
	mons = m1;
	save_selmon();
	selmon = m1;

	Monitor *r = dirtomon(1);
	ASSERT(r == m2, "dirtomon: +1 returns next monitor");

	restore_selmon();
	free(m1); /* fixture leaked (ASan-reported) */
	free(m2); /* fixture leaked (ASan-reported) */
}

/* --- dirtomon multi-monitor --- */
static void
test_dirtomon_negative_wraps_to_last(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m1 = make_monitor(0);
	Monitor *m2 = make_monitor(1);
	m1->next = m2;
	mons = m1;
	save_selmon();
	selmon = m1;

	Monitor *r = dirtomon(-1);
	ASSERT(r == m2, "dirtomon: -1 when selmon==head returns last monitor");

	restore_selmon();
	free(m1); /* fixture leaked (ASan-reported) */
	free(m2); /* fixture leaked (ASan-reported) */
}

/* --- dirtomon single monitor (line 596-597) --- */
static void
test_dirtomon_single_monitor(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	Monitor *m = make_monitor(0);
	selmon = m;
	mons = m;
	/* selmon->next == NULL, dir > 0 => m = mons */
	Monitor *result = dirtomon(1);
	ASSERT(result == mons, "dirtomon single monitor: returns mons");

	/* dir < 0, selmon == mons => loop finds last (mons itself) */
	result = dirtomon(-1);
	ASSERT(result == mons, "dirtomon single monitor: negative returns mons (only monitor)");

	restore_selmon();
	free(m);
}

static void
run_tests(void)
{
	test_dirtomon_positive();
	test_dirtomon_negative_wraps_to_last();
	test_dirtomon_single_monitor();
}
