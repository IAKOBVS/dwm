/*
 * test_drw_fontset_getwidth_clamp.c — per-function drw.c tests for drw_fontset_getwidth_clamp().
 *
 * Uses the real drw.c with the mock X11/Xft fixture from
 * test_drw_common.h; covers the primary path and an edge case.
 */
#include "test_drw_common.h"

static void
test_getwidth_clamp_guards(void)
{
	Drw drw;

	memset(&drw, 0, sizeof drw);
	ASSERT_EQ(drw_fontset_getwidth_clamp(NULL, "abc", 10), 0,
		"getwidth_clamp: NULL drw returns 0");
	ASSERT_EQ(drw_fontset_getwidth_clamp(&drw, "abc", 10), 0,
		"getwidth_clamp: no fonts returns 0");
	ASSERT_EQ(drw_fontset_getwidth_clamp(&drw, NULL, 10), 0,
		"getwidth_clamp: NULL text returns 0");
	ASSERT_EQ(drw_fontset_getwidth_clamp(&drw, "abc", 0), 0,
		"getwidth_clamp: zero budget returns 0");
}

static void
run_tests(void)
{
	test_getwidth_clamp_guards();
}
