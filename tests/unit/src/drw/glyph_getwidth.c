/*
 * test_glyph_getwidth.c — per-function drw.c tests for glyph_getwidth().
 *
 * Uses the real drw.c with the stateful mock Xft fixture. The glyph-width
 * cache is global, so every case invalidates it first.
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
test_glyph_getwidth_cache_hit(void)
{
	Drw drw;
	long cp = 0x1F600;

	memset(&drw, 0, sizeof drw);
	drw_fontset_invalidate_cache();
	glyph_cache[cp & (GLYPH_CACHE_SIZE - 1)].codepoint = cp;
	glyph_cache[cp & (GLYPH_CACHE_SIZE - 1)].width = 42;
	glyph_cache[cp & (GLYPH_CACHE_SIZE - 1)].generation = font_generation;

	ASSERT_EQ(glyph_getwidth(&drw, cp, "x", 1), 42,
		"glyph_getwidth: cache hit returns stored width");
}

static void
test_glyph_getwidth_no_font(void)
{
	Drw drw;

	memset(&drw, 0, sizeof drw);
	drw_fontset_invalidate_cache();
	ASSERT_EQ(glyph_getwidth(&drw, 0x1F600, "x", 1), 0,
		"glyph_getwidth: miss without fonts returns 0");
}

static void
test_glyph_getwidth_measures_and_caches(void)
{
	Drw *drw = make_drw();
	int before;

	drw_fontset_invalidate_cache();
	mock_xft_advance = 7;
	before = mock_xft_text_extents_calls;

	ASSERT_EQ(glyph_getwidth(drw, 0xE9, "\xc3\xa9", 2), 14,
		"glyph_getwidth: measures a non-ASCII glyph");
	ASSERT_EQ(mock_xft_text_extents_calls, before + 1,
		"glyph_getwidth: cache miss consults Xft once");

	ASSERT_EQ(glyph_getwidth(drw, 0xE9, "\xc3\xa9", 2), 14,
		"glyph_getwidth: cache hit returns the same width");
	ASSERT_EQ(mock_xft_text_extents_calls, before + 1,
		"glyph_getwidth: cache hit does not call Xft again");

	free_drw(drw);
}

static void
test_glyph_getwidth_missing_glyph(void)
{
	Drw *drw = make_drw();

	drw_fontset_invalidate_cache();
	mock_xft_missing_codepoint = 0xE9;
	ASSERT_EQ(glyph_getwidth(drw, 0xE9, "\xc3\xa9", 2), 0,
		"glyph_getwidth: missing glyph contributes zero width");
	mock_xft_missing_codepoint = -1;

	free_drw(drw);
}

static void
run_tests(void)
{
	test_glyph_getwidth_cache_hit();
	test_glyph_getwidth_no_font();
	test_glyph_getwidth_measures_and_caches();
	test_glyph_getwidth_missing_glyph();
}
