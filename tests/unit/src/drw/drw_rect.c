/*
 * test_drw_rect.c — per-function drw.c tests for drw_rect().
 *
 * Uses the real drw.c with the stateful mock X11 fixture, which records
 * XSetForeground/XFillRectangle/XDrawRectangle arguments.
 */
#include "test_drw_common.h"

static void
test_drw_rect_null_drw(void)
{
	mock_x11_reset();
	drw_rect(NULL, 0, 0, 10, 10, 1, 0);
	ASSERT_EQ(mock_xfill_rectangle.count, 0,
		"drw_rect(NULL): no fill");
}

static void
test_drw_rect_zero_dim(void)
{
	Display *dpy = XOpenDisplay(NULL);
	Drw *drw = drw_create(dpy, 0, 100, 800, 30);
	Clr scheme[3];

	memset(scheme, 0, sizeof scheme);
	scheme[ColFg].pixel = 0xAA;
	scheme[ColBg].pixel = 0xBB;
	drw->scheme = scheme;
	mock_x11_reset();

	drw_rect(drw, 0, 0, 0, 0, 0, 0);

	ASSERT_EQ(mock_xfill_rectangle.count, 0,
		"drw_rect(0x0): no fill");
	ASSERT_EQ(mock_xdraw_rectangle.count, 0,
		"drw_rect(0x0): no outline");

	drw->scheme = NULL;
	drw_free(drw);
	XCloseDisplay(dpy);
}

static void
test_drw_rect_filled(void)
{
	Display *dpy = XOpenDisplay(NULL);
	Drw *drw = drw_create(dpy, 0, 100, 800, 30);
	Clr scheme[3];

	memset(scheme, 0, sizeof scheme);
	scheme[ColFg].pixel = 0xAA;
	scheme[ColBg].pixel = 0xBB;
	drw->scheme = scheme;
	mock_x11_reset();

	drw_rect(drw, 10, 20, 30, 40, 1, 0);

	ASSERT_EQ(mock_xset_foreground.count, 1,
		"drw_rect filled: selects a foreground");
	ASSERT_EQ(mock_xset_foreground.mask, scheme[ColFg].pixel,
		"drw_rect filled: uses the fg colour");
	ASSERT_EQ(mock_xfill_rectangle.count, 1,
		"drw_rect filled: fills once");
	ASSERT_EQ(mock_xfill_rectangle.x, 10, "drw_rect filled: x");
	ASSERT_EQ(mock_xfill_rectangle.y, 20, "drw_rect filled: y");
	ASSERT_EQ(mock_xfill_rectangle.w, 30, "drw_rect filled: w");
	ASSERT_EQ(mock_xfill_rectangle.h, 40, "drw_rect filled: h");
	ASSERT_EQ(mock_xdraw_rectangle.count, 0,
		"drw_rect filled: does not outline");

	drw->scheme = NULL;
	drw_free(drw);
	XCloseDisplay(dpy);
}

static void
test_drw_rect_outline_inverted(void)
{
	Display *dpy = XOpenDisplay(NULL);
	Drw *drw = drw_create(dpy, 0, 100, 800, 30);
	Clr scheme[3];

	memset(scheme, 0, sizeof scheme);
	scheme[ColFg].pixel = 0xAA;
	scheme[ColBg].pixel = 0xBB;
	drw->scheme = scheme;
	mock_x11_reset();

	drw_rect(drw, 1, 2, 10, 20, 0, 1);

	ASSERT_EQ(mock_xset_foreground.mask, scheme[ColBg].pixel,
		"drw_rect inverted: uses the bg colour");
	ASSERT_EQ(mock_xfill_rectangle.count, 0,
		"drw_rect outline: does not fill");
	ASSERT_EQ(mock_xdraw_rectangle.count, 1,
		"drw_rect outline: draws once");
	ASSERT_EQ(mock_xdraw_rectangle.x, 1, "drw_rect outline: x");
	ASSERT_EQ(mock_xdraw_rectangle.y, 2, "drw_rect outline: y");
	ASSERT_EQ(mock_xdraw_rectangle.w, 9, "drw_rect outline: w-1");
	ASSERT_EQ(mock_xdraw_rectangle.h, 19, "drw_rect outline: h-1");

	drw->scheme = NULL;
	drw_free(drw);
	XCloseDisplay(dpy);
}

static void
run_tests(void)
{
	test_drw_rect_null_drw();
	test_drw_rect_zero_dim();
	test_drw_rect_filled();
	test_drw_rect_outline_inverted();
}
