/*
 * test_drw_fontset_getwidth.c — per-function drw.c tests for
 * drw_fontset_getwidth() and drw_fontset_getwidth_clamp().
 *
 * Uses the real drw.c with the stateful mock Xft fixture: each byte advances
 * mock_xft_advance pixels (default 7).
 */
#include "test_drw_common.h"

static Drw *
make_drw(void)
{
	Display *dpy = XOpenDisplay(NULL);
	Drw *drw = drw_create(dpy, 0, 100, 800, 30);
	const char *fonts[] = { "mono" };

	mock_x11_reset();
	drw_fontset_create(drw, fonts, 1);
	drw_fontset_invalidate_cache();
	return drw;
}

static void
free_drw(Drw *drw)
{
	Display *dpy = drw->dpy;

	drw_free(drw);
	XCloseDisplay(dpy);
}

static void
test_drw_fontset_getwidth_null(void)
{
	unsigned int w = drw_fontset_getwidth(NULL, "hello");
	ASSERT_EQ(w, 0, "drw_fontset_getwidth(NULL): returns 0");
}

static void
test_drw_fontset_getwidth_null_fonts(void)
{
	Drw drw;
	memset(&drw, 0, sizeof(drw));
	drw.fonts = NULL;
	ASSERT_EQ(drw_fontset_getwidth(&drw, "hello"), 0,
		"drw_fontset_getwidth(NULL fonts): returns 0");
}

static void
test_drw_fontset_getwidth_null_text(void)
{
	Drw *drw = make_drw();
	ASSERT_EQ(drw_fontset_getwidth(drw, NULL), 0,
		"drw_fontset_getwidth(NULL text): returns 0");
	ASSERT_EQ(drw_fontset_getwidth(drw, ""), 0,
		"drw_fontset_getwidth(empty): returns 0");
	free_drw(drw);
}

static void
test_drw_fontset_getwidth_ascii(void)
{
	Drw *drw = make_drw();

	mock_xft_advance = 7;
	ASSERT_EQ(drw_fontset_getwidth(drw, "hello"), 35,
		"drw_fontset_getwidth: five ASCII bytes at 7px");

	free_drw(drw);
}

static void
test_drw_fontset_getwidth_utf8(void)
{
	Drw *drw = make_drw();

	drw_fontset_invalidate_cache();
	mock_xft_advance = 7;
	ASSERT_EQ(drw_fontset_getwidth(drw, "\xc3\xa9"), 14,
		"drw_fontset_getwidth: non-ASCII bytes use glyph cache");

	free_drw(drw);
}

static void
test_drw_fontset_getwidth_clamp(void)
{
	Drw *drw = make_drw();

	drw_fontset_invalidate_cache();
	mock_xft_advance = 7;
	ASSERT_EQ(drw_fontset_getwidth_clamp(drw, "hello", 20), 20,
		"getwidth_clamp: caps at the budget");
	ASSERT_EQ(drw_fontset_getwidth_clamp(drw, "hello", 100), 35,
		"getwidth_clamp: returns full width below the budget");
	ASSERT_EQ(drw_fontset_getwidth_clamp(drw, "hello", 0), 0,
		"getwidth_clamp: zero budget returns 0");

	free_drw(drw);
}

static void
run_tests(void)
{
	test_drw_fontset_getwidth_null();
	test_drw_fontset_getwidth_null_fonts();
	test_drw_fontset_getwidth_null_text();
	test_drw_fontset_getwidth_ascii();
	test_drw_fontset_getwidth_utf8();
	test_drw_fontset_getwidth_clamp();
}
