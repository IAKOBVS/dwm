/*
 * test_termforwin.c — per-function tests for dwm.c's termforwin().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through termforwin(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- termforwin --- */
static void
test_termforwin_no_pid(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client w = { .win = 1, .mon = m, .pid = 0 };
	Client *term = termforwin(&w);
	ASSERT(term == NULL, "termforwin: no pid returns NULL");
	free(m);
}

static void
test_termforwin_not_terminal(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client w = { .win = 1, .mon = m, .pid = 100, .isterminal = 1 };
	Client *term = termforwin(&w);
	ASSERT(term == NULL, "termforwin: isterminal returns NULL");
	free(m);
}

/* --- termforwin returns NULL when no matching terminal (line 2269) --- */
static void
test_termforwin_no_matching_terminal(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	m->clients = NULL;
	mons = m;
	Client w = { .win = 1, .mon = m, .pid = 500, .isterminal = 0 };

	Client *term = termforwin(&w);
	ASSERT(term == NULL, "termforwin: no clients returns NULL (line 2269)");

	free(m);
}

/* --- termforwin returns NULL when terminal pid doesn't match --- */
static void
test_termforwin_pid_mismatch(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client terminal = { .win = 10, .mon = m, .pid = 999, .isterminal = 1,
		.swallowing = NULL, .next = NULL };
	m->clients = &terminal;
	mons = m;

	/* w has a pid but is not a terminal; terminal has a different pid.
	 * isdescprocess(999, 500) walks /proc/500/stat -> won't match 999 */
	Client w = { .win = 20, .mon = m, .pid = 500, .isterminal = 0 };

	Client *term = termforwin(&w);
	ASSERT(term == NULL, "termforwin: pid mismatch returns NULL (line 2269)");
	free(m); /* fixture leaked (ASan-reported) */
}

static void
run_tests(void)
{
	test_termforwin_no_pid();
	test_termforwin_not_terminal();
	test_termforwin_no_matching_terminal();
	test_termforwin_pid_mismatch();
}
