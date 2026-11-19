/*
 * test_xerrorstart.c — per-function tests for dwm.c's xerrorstart().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through xerrorstart(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- xerrorstart calls DIE/abort (line 2350) --- */
static void
test_xerrorstart_calls_die(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	mock_die_abort = 1;
	xerrorstart(NULL, NULL);
	ASSERT_EQ(mock_die_abort, 2,
		"xerrorstart: calls DIE (mock intercepts abort)");
	mock_x11_reset();
}

/* --- xerrorstart via mock_die (lines 2348-2351) --- */
static void
test_xerrorstart_mock(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	mock_die_abort = 1;

	xerrorstart(NULL, NULL);
	ASSERT_EQ(mock_die_abort, 2, "xerrorstart mock: die() was intercepted, mock_die_abort set to 2");

	mock_x11_reset();
}

static void
run_tests(void)
{
	test_xerrorstart_calls_die();
	test_xerrorstart_mock();
}
