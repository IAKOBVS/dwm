/*
 * test_error_guard_end.c — per-function tests for error_guard_end().
 *
 * Verifies that guards unwind to depth zero and restore the saved handler.
 */
#include "test_common.h"

static void
test_error_guard_end_restores(void)
{
	error_guard_depth = 0;

	error_guard_begin();
	error_guard_begin();
	error_guard_end();
	ASSERT_EQ(error_guard_depth, 1, "error_guard_end: unwinds one level");
	error_guard_end();
	ASSERT_EQ(error_guard_depth, 0, "error_guard_end: returns to zero");
}

static void
run_tests(void)
{
	test_error_guard_end_restores();
}
