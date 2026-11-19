/*
 * test_killatfullscreen_start.c — per-function tests for killatfullscreen_start.
 *
 * Covers the primary path and at least one edge case, using the
 * shared mock fixture from test_common.h.
 */
#include "test_common.h"

static int start_calls;
static char start_order[8];

static void
start_recorder(const char *name, const char *signal)
{
	(void)name;
	if (start_calls < (int)sizeof start_order - 1)
		start_order[start_calls] = signal[1];
	start_calls++;
}

static void
test_start_runs_body_synchronously_in_tests(void)
{
	void (*saved)(const char *, const char *) = killall_impl;
	int per = (int)LENGTH(killatfullscreen);

	killall_impl = start_recorder;
	start_calls = 0;
	killatfullscreen_start();
	ASSERT_EQ(start_calls, per * 2,
		"killatfullscreen_start: DWM_TEST runs body synchronously");
	if (per == 1)
		ASSERT_EQ(start_order[0], 'C',
			"killatfullscreen_start: CONT precedes HUP");

	killall_impl = saved;
}

static void
run_tests(void)
{
	test_start_runs_body_synchronously_in_tests();
}
