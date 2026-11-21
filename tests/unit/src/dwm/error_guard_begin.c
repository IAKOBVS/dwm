/*
 * test_error_guard_begin.c — per-function tests for error_guard_begin().
 *
 * Verifies that nested guards save the handler only once and increment depth.
 */
#include "test_common.h"

static void
test_error_guard_begin_nesting(void)
{
	mock_x11_reset();
	error_guard_depth = 0;

	error_guard_begin();
	ASSERT_EQ(error_guard_depth, 1, "error_guard_begin: enters first guard");
	ASSERT_EQ(mock_error_handler_current, xerrordummy,
		"error_guard_begin: installs the quiet handler");
	error_guard_begin();
	ASSERT_EQ(error_guard_depth, 2, "error_guard_begin: nests guards");
	ASSERT_EQ(mock_error_handler_current, xerrordummy,
		"error_guard_begin: nested guard keeps the quiet handler");

	error_guard_end();
	error_guard_end();
	ASSERT_EQ(error_guard_depth, 0, "error_guard_begin: fixture unwound");
	ASSERT_EQ(mock_error_handler_current, NULL,
		"error_guard_begin: outer exit restores the saved handler");
}

static void
run_tests(void)
{
	test_error_guard_begin_nesting();
}
