/*
 * test_updatewmhints.c — per-function tests for dwm.c's updatewmhints().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through updatewmhints(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- updatewmhints --- */
static void
test_updatewmhints(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Client c = { .win = 1 };
	updatewmhints(&c);
	ASSERT(1, "updatewmhints: does not crash");
}

/* --- updatewmhints urgency for selmon->sel --- */
static void
test_updatewmhints_urgency_sel(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	selmon = make_monitor(0);
	mons = selmon;

	Client c = { .win = 1, .isurgent = 0, .neverfocus = 0 };
	selmon->sel = &c;

	mock_x11_reset();
	mock_wmhints_flags = XUrgencyHint | InputHint;
	mock_wmhints_input = True;

	updatewmhints(&c);
	ASSERT_EQ(c.isurgent, 0,
		"updatewmhints urgency sel: isurgent unchanged (urgency cleared for sel)");
	mock_x11_reset();
{
		Monitor *tmpm = selmon; /* free the fixture, then restore globals */
		restore_selmon();
		free(tmpm);
	}
}

/* --- updatewmhints neverfocus = 0 (else branch, no InputHint) --- */
static void
test_updatewmhints_neverfocus_else(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Client c = { .win = 1, .neverfocus = 1 };

	mock_x11_reset();
	mock_wmhints_flags = 0;  /* no InputHint set */

	updatewmhints(&c);
	ASSERT_EQ(c.neverfocus, 0,
		"updatewmhints: neverfocus=0 when InputHint not set");
	mock_x11_reset();
}

static void
run_tests(void)
{
	test_updatewmhints();
	test_updatewmhints_urgency_sel();
	test_updatewmhints_neverfocus_else();
}
