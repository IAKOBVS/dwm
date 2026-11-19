/*
 * test_updategeom.c — per-function tests for dwm.c's updategeom().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through updategeom(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- updategeom --- */
static void
test_updategeom_single_monitor(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	mons = NULL;
	selmon = NULL;

	int dirty = updategeom();
	ASSERT(mons != NULL, "updategeom: creates monitor");
	ASSERT(dirty != 0, "updategeom: returns dirty");

	{
		Monitor *tmpm = mons; /* monitors replaced by updategeom */
		restore_selmon();
		while (tmpm) { Monitor *n = tmpm->next; free(tmpm); tmpm = n; }
	}
}

static void
run_tests(void)
{
	test_updategeom_single_monitor();
}
