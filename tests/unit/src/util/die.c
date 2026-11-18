/*
 * test_die.c — per-function util helper tests for die().
 *
 * util.c is included directly (test_util_common.h); these tests do
 * not use the X11 mocks.
 */
#include "test_util_common.h"

static void
test_die_exits_failure(void)
{
	pid_t pid;
	int st;

	pid = fork();
	if (pid == 0) {
		/* Silence the diagnostic; the child must exit, not return. */
		freopen("/dev/null", "w", stderr);
		die("test_die.c", 1, "run_tests", "boom");
		_exit(0); /* unreachable */
	}
	ASSERT(pid > 0, "die: fork succeeded");
	ASSERT(waitpid(pid, &st, 0) == pid, "die: child reaped");
	ASSERT(WIFEXITED(st), "die: child terminated normally");
	ASSERT_EQ(WEXITSTATUS(st), EXIT_FAILURE, "die: exits EXIT_FAILURE");
}

static void
run_tests(void)
{
	test_die_exits_failure();
}
