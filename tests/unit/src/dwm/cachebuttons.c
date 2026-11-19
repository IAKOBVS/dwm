/*
 * test_cachebuttons.c — per-function tests for dwm.c's cachebuttons().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through cachebuttons(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- cachebuttons loop (lines 338-343) --- */
static void
test_cachebuttons(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	button_button_used = 0;
	button_mask_used = 0;
	cachebuttons();
	ASSERT(button_button_used != 0, "cachebuttons: sets button_button_used");
	ASSERT(button_mask_used != 0, "cachebuttons: sets button_mask_used");
}

static void
run_tests(void)
{
	test_cachebuttons();
}
