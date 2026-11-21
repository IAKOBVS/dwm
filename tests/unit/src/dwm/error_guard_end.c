/*
 * test_error_guard_end.c — per-function tests for error_guard_end().
 *
 * Verifies that guards unwind to depth zero and restore the saved handler.
 */
#include "test_common.h"

static void
test_error_guard_end_restores(void)
{
	mock_x11_reset();
	error_guard_depth = 0;
	XSetErrorHandler(xerror); /* handler that must be restored */

	error_guard_begin();
	error_guard_begin();
	error_guard_end();
	ASSERT_EQ(error_guard_depth, 1, "error_guard_end: unwinds one level");
	ASSERT_EQ(mock_error_handler_current, xerrordummy,
		"error_guard_end: inner exit keeps the quiet handler");
	error_guard_end();
	ASSERT_EQ(error_guard_depth, 0, "error_guard_end: returns to zero");
	ASSERT_EQ(mock_error_handler_current, xerror,
		"error_guard_end: restores the pre-guard handler");
}

static void
run_tests(void)
{
	test_error_guard_end_restores();
}
