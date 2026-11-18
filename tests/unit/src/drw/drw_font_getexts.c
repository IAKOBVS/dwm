/*
 * test_drw_font_getexts.c — per-function drw.c tests for drw_font_getexts().
 *
 * Uses the real drw.c with the stateful mock Xft fixture: XftTextExtentsUtf8
 * reports mock_xft_advance (default 7) pixels per byte and the font's
 * ascent+descent (11+4) as the line height.
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
test_drw_font_getexts_metrics(void)
{
	Drw *drw = make_drw();
	unsigned int w = 0, h = 0;

	mock_xft_advance = 7;
	drw_font_getexts(drw->fonts, "abc", 3, &w, &h);
	ASSERT_EQ(w, 21, "drw_font_getexts: width = advance * bytes");
	ASSERT_EQ(h, 15, "drw_font_getexts: height = ascent + descent");

	w = h = 99;
	drw_font_getexts(drw->fonts, "abc", 0, &w, &h);
	ASSERT_EQ(w, 0, "drw_font_getexts: zero length has zero width");
	ASSERT_EQ(h, 15, "drw_font_getexts: height is font-wide");

	drw_font_getexts(drw->fonts, "abc", 3, NULL, NULL);
	ASSERT(1, "drw_font_getexts: NULL outputs are ignored");

	free_drw(drw);
}

static void
test_drw_font_getexts_guards(void)
{
	unsigned int w = 99, h = 99;

	drw_font_getexts(NULL, "abc", 3, &w, &h);
	ASSERT_EQ(w, 99, "drw_font_getexts: NULL font leaves w untouched");
	ASSERT_EQ(h, 99, "drw_font_getexts: NULL font leaves h untouched");

	drw_font_getexts(NULL, NULL, 0, NULL, NULL);
	ASSERT(1, "drw_font_getexts: all-NULL arguments are safe");
}

static void
run_tests(void)
{
	test_drw_font_getexts_metrics();
	test_drw_font_getexts_guards();
}
