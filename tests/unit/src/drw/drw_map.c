/*
 * test_drw_map.c — per-function drw.c tests for drw_map().
 *
 * Uses the real drw.c with the stateful mock X11 fixture, which records the
 * XCopyArea arguments and XSync calls.
 */
#include "test_drw_common.h"

static void
test_drw_map_null_drw(void)
{
	mock_x11_reset();
	drw_map(NULL, 0, 0, 0, 100, 100);
	ASSERT_EQ(mock_xcopy_area.count, 0, "drw_map(NULL): no copy");
	ASSERT_EQ(mock_xsync_calls, 0, "drw_map(NULL): no sync");
}

static void
test_drw_map_copies_region(void)
{
	Display *dpy = XOpenDisplay(NULL);
	Drw *drw = drw_create(dpy, 0, 100, 800, 30);

	mock_x11_reset();
	drw_map(drw, 200, 5, 6, 100, 30);

	ASSERT_EQ(mock_xcopy_area.count, 1, "drw_map: copies once");
	ASSERT_EQ(mock_xcopy_area.win, 200, "drw_map: targets the window");
	ASSERT_EQ(mock_xcopy_area.x, 5, "drw_map: source x");
	ASSERT_EQ(mock_xcopy_area.y, 6, "drw_map: source y");
	ASSERT_EQ(mock_xcopy_area.w, 100, "drw_map: width");
	ASSERT_EQ(mock_xcopy_area.h, 30, "drw_map: height");
	ASSERT_EQ(mock_xsync_calls, 1, "drw_map: flushes with XSync");

	drw_free(drw);
	XCloseDisplay(dpy);
}

static void
run_tests(void)
{
	test_drw_map_null_drw();
	test_drw_map_copies_region();
}
