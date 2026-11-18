/*
 * test_emoji_cache_invalidate.c — per-function drw.c tests for emoji_cache_invalidate().
 *
 * Uses the real drw.c with the mock X11/Xft fixture from
 * test_drw_common.h; covers the primary path and an edge case.
 */
#include "test_drw_common.h"

static void
init_cache(Drw *drw)
{
	int i;
	memset(drw, 0, sizeof *drw);
	for (i = 0; i < EMOJI_CACHE_SIZE; i++)
		drw->emoji_cache[i].codepoint = -1;
}

static void
test_emoji_cache_invalidate_clears_all(void)
{
	Drw drw;
	int i;

	init_cache(&drw);
	emoji_cache_insert(&drw, 0x1F600, 5, 10, 1, 2);
	emoji_cache_insert(&drw, 0x1F601, 6, 11, 1, 2);
	emoji_cache_invalidate(&drw);
	for (i = 0; i < EMOJI_CACHE_SIZE; i++) {
		ASSERT_EQ(drw.emoji_cache[i].codepoint, -1,
			"emoji_cache_invalidate: slot marked empty");
		ASSERT_EQ(drw.emoji_cache[i].pixmap, 0,
			"emoji_cache_invalidate: pixmap cleared");
	}
	ASSERT_EQ(emoji_cache_lookup(&drw, 0x1F600, 1, 2), -1,
		"emoji_cache_invalidate: lookup misses afterwards");
}

static void
run_tests(void)
{
	test_emoji_cache_invalidate_clears_all();
}
