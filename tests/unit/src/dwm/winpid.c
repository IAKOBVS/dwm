/*
 * test_winpid.c — per-function tests for dwm.c's winpid().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through winpid(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- winpid --- */
static void
test_winpid(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	pid_t pid = winpid(999);
	ASSERT_EQ(pid, (pid_t)0, "winpid: returns 0 for non-existent window");
}

/* --- winpid returns 0 for (pid_t)-1 result (line 2189) --- */
static void
test_winpid_minus_one_returns_zero(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	mock_winpid_set = 1;
	mock_winpid_value = (uint32_t)-1;
	pid_t pid = winpid(42);
	ASSERT_EQ(pid, (pid_t)0, "winpid: result of -1 is normalized to 0");
	mock_x11_reset();
}

/* --- winpid returns valid pid (lines 2176-2182) --- */
static void
test_winpid_returns_valid_pid(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	mock_winpid_set = 1;
	mock_winpid_value = 12345;
	pid_t pid = winpid(42);
	ASSERT_EQ(pid, (pid_t)12345, "winpid: returns valid PID from xcb");
	mock_x11_reset();
}

static void
run_tests(void)
{
	test_winpid();
	test_winpid_minus_one_returns_zero();
	test_winpid_returns_valid_pid();
}
