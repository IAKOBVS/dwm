/*
 * test_emoji_cache_lookup.c — per-function drw.c tests for emoji_cache_lookup().
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
test_emoji_cache_lookup_key_matches_colors(void)
{
	Drw drw;
	int idx;

	init_cache(&drw);
	emoji_cache_insert(&drw, 0x1F600, 7, 12, 0x111, 0x222);
	idx = emoji_cache_lookup(&drw, 0x1F600, 0x111, 0x222);
	ASSERT(idx >= 0, "emoji_cache_lookup: matching key hits");
	ASSERT_EQ(drw.emoji_cache[idx].w, 12,
		"emoji_cache_lookup: cached width preserved");
	ASSERT_EQ(emoji_cache_lookup(&drw, 0x1F600, 0x111, 0x333), -1,
		"emoji_cache_lookup: different bg misses");
	ASSERT_EQ(emoji_cache_lookup(&drw, 0x1F601, 0x111, 0x222), -1,
		"emoji_cache_lookup: different codepoint misses");
}

static void
run_tests(void)
{
	test_emoji_cache_lookup_key_matches_colors();
}
