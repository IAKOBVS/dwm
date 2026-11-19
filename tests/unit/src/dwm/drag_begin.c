/*
 * test_drag_begin.c — per-function tests for drag_begin().
 *
 * Verifies that the drag FSM only owns the pointer grab on XGrabPointer
 * success and leaves the IDLE state untouched on failure.
 */
#include "test_common.h"

static void
test_drag_begin_success(void)
{
	Client c = { .win = 1 };

	drag.phase = DRAG_IDLE;
	pointergrab = 0;
	mock_grabpointer_return = 0;

	ASSERT(drag_begin(&c, 1), "drag_begin: succeeds when XGrabPointer succeeds");
	ASSERT_EQ(drag.phase, DRAG_GRABBED, "drag_begin: enters GRABBED phase");
	ASSERT_EQ(pointergrab, 1, "drag_begin: arms immediate reflow");
	ASSERT_EQ(drag.c, &c, "drag_begin: records the client");

	drag_end(); /* cleanup; asserted in test_drag_end.c */
}

static void
test_drag_begin_failure(void)
{
	Client c = { .win = 2 };

	drag.phase = DRAG_IDLE;
	pointergrab = 0;
	mock_grabpointer_return = 1;

	ASSERT(!drag_begin(&c, 1), "drag_begin: returns 0 when the grab fails");
	ASSERT_EQ(drag.phase, DRAG_IDLE, "drag_begin: failure leaves state IDLE");
	ASSERT_EQ(pointergrab, 0, "drag_begin: failure does not arm reflow");

	mock_grabpointer_return = 0;
}

static void
run_tests(void)
{
	test_drag_begin_success();
	test_drag_begin_failure();
}
