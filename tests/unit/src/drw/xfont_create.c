/*
 * test_xfont_create.c — per-function drw.c tests for xfont_create().
 *
 * Uses the real drw.c with the stateful mock Xft fixture from
 * test_drw_common.h. The mock returns a deterministic XftFont (ascent 11,
 * descent 4) unless mock_xft_font_fail is set.
 */
#include "test_drw_common.h"

static void
test_xfont_create_success(void)
{
	Display *dpy = XOpenDisplay(NULL);
	Drw *drw = drw_create(dpy, 0, 100, 800, 30);
	Fnt *f;

	mock_xft_font_fail = 0;
	f = xfont_create(drw, "mono", NULL);
	ASSERT(f != NULL, "xfont_create: loadable font returns a node");
	ASSERT(f->xfont != NULL, "xfont_create: node owns an XftFont");
	ASSERT(f->pattern != NULL, "xfont_create: name loads keep a pattern");
	ASSERT_EQ(f->h, 15, "xfont_create: height is ascent+descent");
	ASSERT_EQ(f->dpy, dpy, "xfont_create: display stored");

	xfont_free(f);
	drw_free(drw);
	XCloseDisplay(dpy);
}

static void
test_xfont_create_missing_font_returns_null(void)
{
	Display *dpy = XOpenDisplay(NULL);
	Drw *drw = drw_create(dpy, 0, 100, 800, 30);

	mock_xft_font_fail = 1;
	ASSERT(xfont_create(drw, "no-such-font", NULL) == NULL,
		"xfont_create: unloadable font returns NULL");
	mock_xft_font_fail = 0;

	drw_free(drw);
	XCloseDisplay(dpy);
}

static void
run_tests(void)
{
	test_xfont_create_success();
	test_xfont_create_missing_font_returns_null();
}
