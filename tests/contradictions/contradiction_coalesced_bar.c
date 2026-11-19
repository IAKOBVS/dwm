/*
 * Reproduction for the coalescing contradiction: arrangenow(NULL) skips
 * restack(), which used to be the only arrange-path writer of
 * bar_draw_pending. A coalesced full layout pass therefore left dirty bar
 * segments unflushed for that event batch.
 */
#include "test_common.h"

static void
test_coalesced_arrange_requests_bar_draw(void)
{
	reset_index();
	Monitor *m = make_monitor(0);

	m->showbar = 1;
	m->bar_dirty_segments = DIRTY_TAGS;
	m->bar_exposed = 1;
	mons = m;
	save_selmon();
	selmon = m;
	mock_drw_reset();

	dispatching = 1;
	arrange_pending = 0;
	bar_draw_pending = 0;
	arrange(m); /* defers; no restack, no bar_draw_pending */
	ASSERT_EQ(arrange_pending, 1, "repro: arrange deferred into the tail");
	ASSERT_EQ(bar_draw_pending, 0, "repro: no direct bar request yet");

	dispatching = 0;
	flusheventtail();

	ASSERT(mock_drw_map_calls >= 1,
		"repro: coalesced flush still redraws the dirty bar");

	restore_selmon();
	free(m);
}

static void
run_tests(void)
{
	test_coalesced_arrange_requests_bar_draw();
}
