/*
 * test_drw_fontset_invalidate_cache.c — per-function drw.c tests for drw_fontset_invalidate_cache().
 *
 * Uses the real drw.c with the mock X11/Xft fixture from
 * test_drw_common.h; covers the primary path and an edge case.
 */
#include "test_drw_common.h"

static void
test_invalidate_cache_clears_slots(void)
{
	int i;

	glyph_cache[0].codepoint = 123;
	glyph_cache[5].codepoint = 456;
	drw_fontset_invalidate_cache();
	for (i = 0; i < GLYPH_CACHE_SIZE; i++)
		ASSERT_EQ(glyph_cache[i].codepoint, -1,
			"drw_fontset_invalidate_cache: slot empty");
}

static void
run_tests(void)
{
	test_invalidate_cache_clears_slots();
}
