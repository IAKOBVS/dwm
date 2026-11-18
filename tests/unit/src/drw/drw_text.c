/*
 * test_drw_text.c — per-function drw.c tests for drw_text().
 *
 * Uses the real drw.c with the stateful mock Xft fixture. XftTextExtentsUtf8
 * reports 7 px/byte (mock_xft_advance), fonts have ascent 11 + descent 4, and
 * XftDrawStringUtf8 records the last drawn run.
 */
#include "test_drw_common.h"

static Clr *test_scheme;

static Drw *
make_drw(void)
{
	Display *dpy = XOpenDisplay(NULL);
	Drw *drw = drw_create(dpy, 0, 100, 800, 30);
	const char *fonts[] = { "mono" };

	mock_x11_reset();
	drw_fontset_create(drw, fonts, 1);
	drw_fontset_invalidate_cache();
	test_scheme = calloc(3, sizeof(Clr));
	test_scheme[ColFg].pixel = 0x111;
	test_scheme[ColBg].pixel = 0x222;
	test_scheme[ColBorder].pixel = 0x333;
	drw->scheme = test_scheme;
	return drw;
}

static void
free_drw(Drw *drw)
{
	Display *dpy = drw->dpy;

	drw->scheme = NULL;
	drw_free(drw);
	free(test_scheme);
	test_scheme = NULL;
	XCloseDisplay(dpy);
}

static void
test_drw_text_null_drw(void)
{
	ASSERT_EQ(drw_text(NULL, 0, 0, 100, 20, 0, "hi", 0), 0,
		"drw_text(NULL): returns 0");
}

static void
test_drw_text_null_text(void)
{
	Drw *drw = make_drw();
	ASSERT_EQ(drw_text(drw, 0, 0, 100, 20, 0, NULL, 0), 0,
		"drw_text(NULL text): returns 0");
	free_drw(drw);
}

static void
test_drw_text_empty(void)
{
	Drw *drw = make_drw();

	/* Draw mode returns the rectangle's right edge even with no glyphs. */
	ASSERT_EQ(drw_text(drw, 0, 0, 100, 20, 0, "", 0), 100,
		"drw_text(empty): draw mode returns the right edge");
	ASSERT_EQ(mock_xft_draw_string_calls, 0,
		"drw_text(empty): draws nothing");
	ASSERT_EQ(drw_text(drw, 0, 0, 0, 0, 0, "", 0), 0,
		"drw_text(empty): measure mode returns 0");

	free_drw(drw);
}

static void
test_drw_text_zero_width(void)
{
	Drw *drw = make_drw();
	ASSERT_EQ(drw_text(drw, 0, 0, 0, 20, 0, "hello", 0), 0,
		"drw_text(w=0): returns 0");
	free_drw(drw);
}

static void
test_drw_text_null_scheme(void)
{
	Drw *drw = make_drw();
	drw->scheme = NULL;
	ASSERT_EQ(drw_text(drw, 0, 0, 100, 20, 0, "hi", 0), 0,
		"drw_text(NULL scheme): returns 0");
	free_drw(drw);
}

static void
test_drw_text_null_fonts(void)
{
	Drw *drw = make_drw();
	drw_fontset_free(drw->fonts); /* release the chain before clearing it */
	drw->fonts = NULL;
	ASSERT_EQ(drw_text(drw, 0, 0, 100, 20, 0, "hi", 0), 0,
		"drw_text(NULL fonts): returns 0");
	free_drw(drw);
}

static void
test_drw_text_measure_mode(void)
{
	Drw *drw = make_drw();

	ASSERT_EQ(drw_text(drw, 0, 0, 0, 0, 0, "hi", 0), 14,
		"drw_text: measure mode returns 2 bytes * 7px");
	ASSERT_EQ(mock_xft_draw_string_calls, 0,
		"drw_text: measure mode draws nothing");

	free_drw(drw);
}

static void
test_drw_text_draws_ascii_batch(void)
{
	Drw *drw = make_drw();

	ASSERT_EQ(drw_text(drw, 0, 0, 100, 20, 0, "hi", 0), 100,
		"drw_text: returns the rectangle right edge");
	ASSERT_EQ(mock_xfill_rectangle.count, 1,
		"drw_text: clears the background once");
	ASSERT_EQ(mock_xft_draw_string_calls, 1,
		"drw_text: batches consecutive ASCII into one draw");
	ASSERT_EQ(mock_xft_last_len, 2, "drw_text: draws both ASCII bytes");
	ASSERT(memcmp(mock_xft_last_text, "hi", 2) == 0,
		"drw_text: draws the requested text");

	free_drw(drw);
}

static void
test_drw_text_ellipsis_on_overflow(void)
{
	Drw *drw = make_drw();

	/* "hello" is 35px; a 30px budget leaves room for one glyph + "..." */
	ASSERT_EQ(drw_text(drw, 0, 0, 30, 20, 0, "hello", 0), 30,
		"drw_text: returns the rectangle right edge on overflow");
	ASSERT_EQ(mock_xft_draw_string_calls, 2,
		"drw_text: draws the prefix then the ellipsis");
	ASSERT(memcmp(mock_xft_last_text, "...", 3) == 0,
		"drw_text: final run is the ellipsis");

	free_drw(drw);
}

static void
test_drw_text_missing_glyph_no_fallback(void)
{
	Drw *drw = make_drw();

	/* XftFontMatch returns NULL: the codepoint must be skipped, not retried
	 * forever (regression test for the fallback loop). */
	mock_xft_missing_codepoint = 0xE9;
	mock_xft_font_fail = 1;
	ASSERT_EQ(drw_text(drw, 0, 0, 100, 20, 0, "\xc3\xa9", 0), 100,
		"drw_text: missing glyph without fallback returns the right edge");
	ASSERT_EQ(mock_xft_draw_string_calls, 0,
		"drw_text: missing glyph draws nothing");
	mock_xft_font_fail = 0;
	mock_xft_missing_codepoint = -1;

	free_drw(drw);
}

static void
test_drw_text_missing_glyph_with_fallback(void)
{
	Drw *drw = make_drw();

	mock_xft_missing_codepoint = 0xE9;
	ASSERT_EQ(drw_text(drw, 0, 0, 100, 20, 0, "\xc3\xa9", 0), 100,
		"drw_text: missing glyph with failed fallback returns right edge");
	ASSERT_EQ(mock_xft_draw_string_calls, 0,
		"drw_text: unavailable fallback draws nothing");
	mock_xft_missing_codepoint = -1;

	free_drw(drw);
}

static void
run_tests(void)
{
	test_drw_text_null_drw();
	test_drw_text_null_text();
	test_drw_text_empty();
	test_drw_text_zero_width();
	test_drw_text_null_scheme();
	test_drw_text_null_fonts();
	test_drw_text_measure_mode();
	test_drw_text_draws_ascii_batch();
	test_drw_text_ellipsis_on_overflow();
	test_drw_text_missing_glyph_no_fallback();
	test_drw_text_missing_glyph_with_fallback();
}
