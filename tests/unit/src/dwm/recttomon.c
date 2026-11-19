/*
 * test_recttomon.c — per-function tests for dwm.c's recttomon().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through recttomon(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- recttomon --- */
static void
test_recttomon_returns_selmon(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	selmon = make_monitor(0);
	mons = selmon;

	Monitor *r = recttomon(0, 0, 1, 1);
	ASSERT(r == selmon, "recttomon: returns selmon");

{
		Monitor *tmpm = selmon; /* free the fixture, then restore globals */
		restore_selmon();
		free(tmpm);
	}
}

static void
run_tests(void)
{
	test_recttomon_returns_selmon();
}
