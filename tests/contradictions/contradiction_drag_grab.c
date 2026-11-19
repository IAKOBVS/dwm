/*
 * Reproduction for the drag FSM contradiction: a pointer grab acquired by
 * XGrabPointer used to leak when getrootptr() failed (movemouse returned
 * before XUngrabPointer). The fixed code funnels through drag_end().
 */
#include "test_common.h"

static void
test_getrootptr_failure_releases_grab(void)
{
	reset_index();
	Monitor *m = make_monitor(0);
	Client *c = make_client(7, m);

	m->sel = c;
	m->clients = c;
	m->stack = c;
	save_selmon();
	selmon = m;
	mons = m;

	drag.phase = DRAG_IDLE;
	pointergrab = 0;
	mock_x11_reset();
	mock_querypointer_return = 0; /* force getrootptr() failure */

	movemouse(NULL);

	ASSERT_EQ(mock_xgrabpointer_calls, 1, "repro: XGrabPointer was called");
	ASSERT_EQ(mock_xungrabpointer_calls, 1,
		"repro: failed getrootptr still releases the grab");
	ASSERT_EQ(drag.phase, DRAG_IDLE, "repro: drag returns to IDLE");
	ASSERT_EQ(pointergrab, 0, "repro: reflow override cleared");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

static void
run_tests(void)
{
	test_getrootptr_failure_releases_grab();
}
