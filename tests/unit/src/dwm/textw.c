/*
 * test_textw.c — per-function tests for textw.
 *
 * Covers the primary path and at least one edge case, using the
 * shared mock fixture from test_common.h.
 */
#include "test_common.h"

static void
test_textw_adds_lrpad(void)
{
	unsigned int w = TEXTW("test");

	ASSERT(w > lrpad, "TEXTW: non-empty text contributes glyph width");
	ASSERT_EQ(w, drw_fontset_getwidth(drw, "test") + lrpad,
		"TEXTW: glyph width plus lrpad");
	ASSERT_EQ(TEXTW(""), lrpad, "TEXTW: empty text is just lrpad");
}

static void
run_tests(void)
{
	test_textw_adds_lrpad();
}
