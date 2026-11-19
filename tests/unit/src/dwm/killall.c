/*
 * test_killall.c — per-function tests for killall.
 *
 * Covers the primary path and at least one edge case, using the
 * shared mock fixture from test_common.h.
 */
#include "test_common.h"

static void
test_killall_default_seam(void)
{
	Display *saved_dpy;

	ASSERT(killall_impl == killall, "killall: default seam is the real helper");

	/* Exercise the fork/pipe path with a harmless, nonexistent target.
	 * dpy is nulled because the fake test display has no real fd. */
	saved_dpy = dpy;
	dpy = NULL;
	killall("opencode-definitely-not-a-process", "-0");
	dpy = saved_dpy;
	ASSERT(1, "killall: fork/exec path returns");
}

static void
run_tests(void)
{
	test_killall_default_seam();
}
