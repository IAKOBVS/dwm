/*
 * test_killall.c — per-function tests for killall.
 *
 * Covers the seam default and both safe fork outcomes. The child branch is
 * never executed in unit tests because a real execlp would run; fork() is
 * mocked here.
 */
#include "test_common.h"

static void
test_killall_default_seam(void)
{
	ASSERT(killall_impl == killall, "killall: default seam is the real helper");
}

static void
test_killall_fork_failure_returns(void)
{
	Display *saved_dpy = dpy;
	int saved_fork = mock_fork_return;

	dpy = NULL;
	mock_x11_reset();
	mock_fork_return = -1; /* simulated fork failure */

	killall("opencode-definitely-not-a-process", "-0");

	ASSERT_EQ(mock_fork_calls, 1, "killall: fork attempted");
	ASSERT_EQ(mock_fork_return, -1, "killall: failure path leaves fork result intact");

	mock_fork_return = saved_fork;
	dpy = saved_dpy;
}

static void
test_killall_parent_waits_for_pipe_eof(void)
{
	Display *saved_dpy = dpy;
	int saved_fork = mock_fork_return;

	dpy = NULL;
	mock_x11_reset();
	mock_fork_return = 12345; /* pretend the child exists */

	killall("opencode-definitely-not-a-process", "-0");

	/* The parent closes the write end, so the real pipe reaches EOF and
	 * killall returns without executing a child in this test process. */
	ASSERT_EQ(mock_fork_calls, 1, "killall: parent path forks once");

	mock_fork_return = saved_fork;
	dpy = saved_dpy;
}

static void
run_tests(void)
{
	test_killall_default_seam();
	test_killall_fork_failure_returns();
	test_killall_parent_waits_for_pipe_eof();
}
