/*
 * test_bar_mark_dirty.c — per-function tests for bar_mark_dirty().
 *
 * Verifies the single writer sets segment bits and requests a coalesced draw,
 * and safely ignores a NULL monitor.
 */
#include "test_common.h"

static void
test_bar_mark_dirty_sets_bits_and_pending(void)
{
	reset_index();
	Monitor *m = make_monitor(0);

	m->bar_dirty_segments = 0;
	bar_draw_pending = 0;
	bar_mark_dirty(m, DIRTY_TAGS);
	bar_mark_dirty(m, DIRTY_TITLE);
	ASSERT_EQ(m->bar_dirty_segments, DIRTY_TAGS | DIRTY_TITLE,
		"bar_mark_dirty: accumulates segment bits");
	ASSERT_EQ(bar_draw_pending, 1, "bar_mark_dirty: requests a redraw");

	bar_mark_dirty(NULL, DIRTY_ALL);
	ASSERT_EQ(bar_draw_pending, 1, "bar_mark_dirty: NULL monitor is ignored");

	free(m);
}

static void
run_tests(void)
{
	test_bar_mark_dirty_sets_bits_and_pending();
}
