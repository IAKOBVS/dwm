/*
 * test_killatfullscreen_stop.c — per-function tests for killatfullscreen_stop.
 *
 * Covers the primary path and at least one edge case, using the
 * shared mock fixture from test_common.h.
 */
#include "test_common.h"

static int rec_calls;
static const char *rec_signal;

static void
recorder(const char *name, const char *signal)
{
	(void)name;
	rec_calls++;
	rec_signal = signal;
}

static void
test_stop_sends_stop(void)
{
	void (*saved)(const char *, const char *) = killall_impl;

	killall_impl = recorder;
	rec_calls = 0;
	rec_signal = NULL;
	killatfullscreen_stop();
	ASSERT_EQ(rec_calls, (int)LENGTH(killatfullscreen),
		"killatfullscreen_stop: one call per target");
	ASSERT(strcmp(rec_signal, "-STOP") == 0,
		"killatfullscreen_stop: sends SIGSTOP");

	killall_impl = saved;
}

static void
run_tests(void)
{
	test_stop_sends_stop();
}
