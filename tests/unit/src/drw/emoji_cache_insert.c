/*
 * test_emoji_cache_insert.c — per-function drw.c tests for emoji_cache_insert().
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
test_emoji_cache_insert_and_overwrite(void)
{
	Drw drw;
	int idx;

	init_cache(&drw);
	emoji_cache_insert(&drw, 0x1F600, 5, 10, 1, 2);
	idx = emoji_cache_lookup(&drw, 0x1F600, 1, 2);
	ASSERT(idx >= 0, "emoji_cache_insert: entry retrievable");

	/* Same codepoint, new scheme: overwrite the slot, old pixmap freed */
	emoji_cache_insert(&drw, 0x1F600, 6, 20, 3, 4);
	ASSERT_EQ(emoji_cache_lookup(&drw, 0x1F600, 3, 4), idx,
		"emoji_cache_insert: overwrite reuses slot");
	ASSERT_EQ(emoji_cache_lookup(&drw, 0x1F600, 1, 2), -1,
		"emoji_cache_insert: old scheme no longer matches");
	ASSERT_EQ(drw.emoji_cache[idx].w, 20,
		"emoji_cache_insert: width updated");
}

static void
run_tests(void)
{
	test_emoji_cache_insert_and_overwrite();
}
