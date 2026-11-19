/*
 * test_quit.c — per-function tests for quit.
 *
 * Covers the primary path and at least one edge case, using the
 * shared mock fixture from test_common.h.
 */
#include "test_common.h"

static void
test_quit_stops_and_optional_restart(void)
{
	Arg no_restart = { .i = 0 };
	Arg restart_flag = { .i = 1 };

	lifecycle = LIFE_LIVE;
	quit(&no_restart);
	ASSERT_EQ(lifecycle, LIFE_QUIT, "quit: clears running");
	ASSERT(lifecycle != LIFE_RESTART, "quit: restart stays clear when arg.i == 0");

	lifecycle = LIFE_LIVE;
	quit(&restart_flag);
	ASSERT(lifecycle != LIFE_LIVE, "quit: stops the loop again");
	ASSERT_EQ(lifecycle, LIFE_RESTART, "quit: arg.i requests restart");

	lifecycle = LIFE_LIVE;
}

static void
run_tests(void)
{
	test_quit_stops_and_optional_restart();
}
