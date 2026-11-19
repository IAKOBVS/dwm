/*
 * test_killatfullscreen_resume.c — per-function tests for killatfullscreen_resume.
 *
 * Covers the primary path and at least one edge case, using the
 * shared mock fixture from test_common.h.
 */
#include "test_common.h"

static int resume_calls;
static const char *resume_signal;

static void
resume_recorder(const char *name, const char *signal)
{
	(void)name;
	resume_calls++;
	resume_signal = signal;
}

static void
test_resume_sends_cont_only(void)
{
	void (*saved)(const char *, const char *) = killall_impl;

	killall_impl = resume_recorder;
	resume_calls = 0;
	resume_signal = NULL;
	killatfullscreen_resume();
	ASSERT_EQ(resume_calls, (int)LENGTH(killatfullscreen),
		"killatfullscreen_resume: one call per target");
	ASSERT(strcmp(resume_signal, "-CONT") == 0,
		"killatfullscreen_resume: only sends SIGCONT");

	killall_impl = saved;
}

static void
run_tests(void)
{
	test_resume_sends_cont_only();
}
