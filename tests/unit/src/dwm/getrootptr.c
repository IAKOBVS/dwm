/*
 * test_getrootptr.c — per-function tests for getrootptr.
 *
 * Covers the primary path and at least one edge case, using the
 * shared mock fixture from test_common.h.
 */
#include "test_common.h"

static void
test_getrootptr_success(void)
{
	int x = -1, y = -1;
	int saved_ret = mock_querypointer_return;
	int saved_x = mock_querypointer_root_x;
	int saved_y = mock_querypointer_root_y;

	mock_querypointer_return = 1;
	mock_querypointer_root_x = 123;
	mock_querypointer_root_y = 456;
	ASSERT_EQ(getrootptr(&x, &y), 1, "getrootptr: forwards True result");
	ASSERT_EQ(x, 123, "getrootptr: writes root x");
	ASSERT_EQ(y, 456, "getrootptr: writes root y");

	mock_querypointer_return = 0;
	ASSERT_EQ(getrootptr(&x, &y), 0, "getrootptr: forwards False result");

	mock_querypointer_return = saved_ret;
	mock_querypointer_root_x = saved_x;
	mock_querypointer_root_y = saved_y;
}

static void
run_tests(void)
{
	test_getrootptr_success();
}
