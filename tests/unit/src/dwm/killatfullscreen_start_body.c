/*
 * test_killatfullscreen_start_body.c — per-function tests for killatfullscreen_start_body.
 *
 * Covers the primary path and at least one edge case, using the
 * shared mock fixture from test_common.h.
 */
#include "test_common.h"

static int body_calls;
static char body_order[8];

static void
body_recorder(const char *name, const char *signal)
{
	(void)name;
	if (body_calls < (int)sizeof body_order - 1)
		body_order[body_calls] = signal[1]; /* 'C' for CONT, 'H' for HUP */
	body_calls++;
}

static void
test_start_body_cont_then_hup(void)
{
	void (*saved)(const char *, const char *) = killall_impl;
	int per = (int)LENGTH(killatfullscreen);

	killall_impl = body_recorder;
	body_calls = 0;
	killatfullscreen_start_body();
	ASSERT_EQ(body_calls, per * 2,
		"killatfullscreen_start_body: CONT and HUP per target");
	if (per == 1) {
		ASSERT_EQ(body_order[0], 'C',
			"killatfullscreen_start_body: CONT before HUP");
		ASSERT_EQ(body_order[1], 'H',
			"killatfullscreen_start_body: HUP after CONT");
	}

	killall_impl = saved;
}

/* --- a new fullscreen session cancels a pending HUP --- */
static void
test_start_body_cancels_stale_hup(void)
{
	void (*saved)(const char *, const char *) = killall_impl;
	int per = (int)LENGTH(killatfullscreen);

	killall_impl = body_recorder;
	body_calls = 0;
	start_epoch = 0;
	fs_epoch = 1; /* a new session began during the grace period */
	killatfullscreen_start_body();
	ASSERT_EQ(body_calls, per,
		"killatfullscreen_start_body: stale session sends CONT but no HUP");

	start_epoch = 0;
	fs_epoch = 0;
	killall_impl = saved;
}

static void
run_tests(void)
{
	test_start_body_cont_then_hup();
	test_start_body_cancels_stale_hup();
}
