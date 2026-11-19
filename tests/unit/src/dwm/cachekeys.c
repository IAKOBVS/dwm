/*
 * test_cachekeys.c — per-function tests for dwm.c's cachekeys().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through cachekeys(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- cachekeys loop (lines 354-358) --- */
static void
test_cachekeys(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	key_keysym_used = 0;
	key_mod_used = 0;
	cachekeys();
	ASSERT(key_keysym_used != 0, "cachekeys: sets key_keysym_used");
	ASSERT(key_mod_used != 0, "cachekeys: sets key_mod_used");
}

static void
run_tests(void)
{
	test_cachekeys();
}
