/*
 * test_flusheventtail.c — per-function tests for flusheventtail.
 *
 * Covers the primary path and at least one edge case, using the
 * shared mock fixture from test_common.h.
 */
#include "test_common.h"

static void
test_flusheventtail_drains_pending(void)
{
	Monitor *m;

	save_selmon();
	reset_index();
	m = make_monitor(0);
	selmon = mons = m;
	m->bar_dirty_segments = 0;
	arrange_pending = 1;
	bar_draw_pending = 1;
	arrange_calls = 0;

	flusheventtail();
	ASSERT_EQ(arrange_pending, 0, "flusheventtail: clears arrange_pending");
	ASSERT_EQ(bar_draw_pending, 0, "flusheventtail: clears bar_draw_pending");
	ASSERT_EQ(arrange_calls, 1, "flusheventtail: runs one deferred arrange");

	/* Nothing pending: no extra layout pass */
	flusheventtail();
	ASSERT_EQ(arrange_calls, 1, "flusheventtail: idle tail is a no-op");

	restore_selmon();
	free(m);
}

static void
run_tests(void)
{
	test_flusheventtail_drains_pending();
}
