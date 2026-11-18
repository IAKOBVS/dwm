/*
 * test_drw_resize.c — per-function drw.c tests for drw_resize().
 *
 * Migrated from test_drw_safety.c; includes the real drw.c against
 * the mock X11/Xft stubs (test_drw_common.h) so no X server is needed.
 */
#include "test_drw_common.h"

static void
test_drw_resize_null(void)
{
	drw_resize(NULL, 100, 50);
	ASSERT(1, "drw_resize(NULL): no crash");
}

static void
test_drw_resize_zero(void)
{
	Drw drw;
	memset(&drw, 0, sizeof(drw));
	drw_resize(&drw, 0, 0);
	ASSERT(drw.w == 1, "drw_resize(0x0): w clamped to 1");
	ASSERT(drw.h == 1, "drw_resize(0x0): h clamped to 1");
}

static void
run_tests(void)
{
	test_drw_resize_null();
	test_drw_resize_zero();
}
