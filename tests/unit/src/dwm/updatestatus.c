/*
 * test_updatestatus.c — per-function tests for dwm.c's updatestatus().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through updatestatus(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- updatestatus --- */
static void
test_updatestatus(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	selmon = make_monitor(0);
	mons = selmon;

	updatestatus();
	ASSERT(1, "updatestatus: does not crash");

{
		Monitor *tmpm = selmon; /* free the fixture, then restore globals */
		restore_selmon();
		free(tmpm);
	}
}

/* --- updatestatus fullscreen freeze --- */
static void
test_updatestatus_fullscreen_freeze(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	char saved[sizeof stext] = "dwm-6.4";
	save_selmon();
	selmon = make_monitor(0);
	mons = selmon;

	Client c = { .win = 1, .isfullscreen = 1 };
	selmon->sel = &c;

	memcpy(stext, saved, sizeof stext);
	updatestatus();
	ASSERT_EQ(strcmp(stext, saved), 0,
		"updatestatus: fullscreen freeze prevents stext modification");
{
		Monitor *tmpm = selmon; /* free the fixture, then restore globals */
		restore_selmon();
		free(tmpm);
	}
}

static void
run_tests(void)
{
	test_updatestatus();
	test_updatestatus_fullscreen_freeze();
}
