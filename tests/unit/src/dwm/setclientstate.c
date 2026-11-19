/*
 * test_setclientstate.c — per-function tests for dwm.c's setclientstate().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through setclientstate(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- setclientstate --- */
static void
test_setclientstate_normal(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Client c = { .win = 1 };
	setclientstate(&c, NormalState);
	ASSERT(1, "setclientstate(NormalState): no crash");
}

static void
test_setclientstate_withdrawn(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Client c = { .win = 1 };
	setclientstate(&c, WithdrawnState);
	ASSERT(1, "setclientstate(WithdrawnState): no crash");
}

/* --- setclientstate (line 1569 comment: arrange is in setlayout) --- */
static void
test_setclientstate_arrange(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	m->sel = c;
	m->clients = c;
	save_selmon();
	selmon = m;
	mons = m;

	setclientstate(c, NormalState);
	ASSERT(1, "setclientstate arrange: NormalState no crash");

	setclientstate(c, WithdrawnState);
	ASSERT(1, "setclientstate arrange: WithdrawnState no crash");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

static void
run_tests(void)
{
	test_setclientstate_normal();
	test_setclientstate_withdrawn();
	test_setclientstate_arrange();
}
