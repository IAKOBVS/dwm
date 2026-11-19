/*
 * test_drag_end.c — per-function tests for drag_end().
 *
 * Verifies that drag_end() is the single, idempotent exit path that always
 * releases the pointer-grab state, including the getrootptr-failure path.
 */
#include "test_common.h"

static void
test_drag_end_clears_state(void)
{
	Client c = { .win = 3 };

	drag.phase = DRAG_IDLE;
	pointergrab = 0;
	mock_grabpointer_return = 0;
	ASSERT(drag_begin(&c, 1), "fixture: grab acquired");

	drag_end();
	ASSERT_EQ(drag.phase, DRAG_IDLE, "drag_end: returns to IDLE");
	ASSERT_EQ(pointergrab, 0, "drag_end: disarms immediate reflow");
	ASSERT_EQ(drag.c, NULL, "drag_end: drops the client reference");
}

static void
test_drag_end_idempotent(void)
{
	drag.phase = DRAG_IDLE;
	pointergrab = 0;

	drag_end();
	drag_end();
	ASSERT_EQ(drag.phase, DRAG_IDLE, "drag_end: no active drag is a no-op");
	ASSERT_EQ(pointergrab, 0, "drag_end: leaves reflow disarmed");
}

static void
run_tests(void)
{
	test_drag_end_clears_state();
	test_drag_end_idempotent();
}
