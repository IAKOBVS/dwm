/*
 * test_wintomon.c — per-function tests for dwm.c's wintomon().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through wintomon(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- wintomon --- */
static void
test_wintomon_no_mons(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	mons = NULL;
	Monitor *m = wintomon(1);
	ASSERT(m == selmon, "wintomon: no mons returns selmon");
	restore_selmon();
}

static void
run_tests(void)
{
	test_wintomon_no_mons();
}
