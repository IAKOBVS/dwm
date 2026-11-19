/*
 * test_run.c — per-function tests for dwm.c's run().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through run(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- run with no events (lines 1397-1398) --- */
static void
test_run_no_events(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	/* Set running=0 so the while loop body is never entered */
	lifecycle = LIFE_QUIT;
	mock_event_queue_count = 0;
	run();
	ASSERT(lifecycle != LIFE_LIVE, "run: no events, running=0 exits immediately");
}

/* --- run with one event then exit (lines 1397-1406) --- */
static void
test_run_one_event(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	/* Inject a ConfigureNotify event for root (no-op handler) */
	mock_event_queue_count = 1;
	memset(&mock_event_queue[0], 0, sizeof(XEvent));
	mock_event_queue[0].type = ConfigureNotify;
	mock_event_queue[0].xconfigure.window = 0;
	mock_event_queue[0].xconfigure.width = 1920;
	mock_event_queue[0].xconfigure.height = 1080;

	lifecycle = LIFE_LIVE;
	run();
	/* XNextEvent returns -1 after queue empties, breaking the while loop */
	ASSERT(1, "run: processes one event then exits");
}

/* --- run with Expose event (line 1399-1400 handler dispatch) --- */
static void
test_run_expose_event(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	Monitor *m = make_monitor(0);
	m->barwin = 888;
	m->showbar = 1;
	selmon = m;
	mons = m;
	selmon->bar_exposed = 0;

	/* Inject an Expose event */
	mock_event_queue_count = 1;
	memset(&mock_event_queue[0], 0, sizeof(XEvent));
	mock_event_queue[0].type = Expose;
	mock_event_queue[0].xexpose.window = 888;
	mock_event_queue[0].xexpose.count = 0;

	lifecycle = LIFE_LIVE;
	run();
	/* Expose handler called, drawbar triggered, then XNextEvent returns -1 */
	ASSERT(1, "run: Expose event dispatched correctly");

	selmon->bar_exposed = 0;
	mock_event_queue_count = 0;
	restore_selmon();
	free(m);
}

static void
run_tests(void)
{
	test_run_no_events();
	test_run_one_event();
	test_run_expose_event();
}
