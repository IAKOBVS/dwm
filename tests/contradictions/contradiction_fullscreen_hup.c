/*
 * Reproduction for the fullscreen process-control contradiction: a HUP
 * scheduled by killatfullscreen_start() must be cancelled when a new
 * fullscreen session begins during the grace period (fs_epoch bump).
 */
#include "test_common.h"

static int calls;
static int hups;

static void
recorder(const char *name, const char *signal)
{
	(void)name;
	calls++;
	if (!strcmp(signal, "-HUP"))
		hups++;
}

static void
test_stale_hup_is_cancelled(void)
{
	void (*saved)(const char *, const char *) = killall_impl;

	killall_impl = recorder;
	calls = hups = 0;

	/* Simulate: start captured epoch 0, then a new session bumped it. */
	start_epoch = 0;
	fs_epoch = 1;
	killatfullscreen_start_body();

	ASSERT_EQ(calls, (int)LENGTH(killatfullscreen),
		"repro: CONT is still sent for the current session");
	ASSERT_EQ(hups, 0, "repro: stale HUP is cancelled by a new session");

	start_epoch = 0;
	fs_epoch = 0;
	killall_impl = saved;
}

static void
run_tests(void)
{
	test_stale_hup_is_cancelled();
}
