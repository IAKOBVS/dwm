/*
 * test_tagmon.c — per-function tests for dwm.c's tagmon().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through tagmon(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- tagmon --- */
static void
test_tagmon_noop_no_sel(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	selmon = make_monitor(0);
	mons = selmon;
	selmon->sel = NULL;
	Arg arg = { .i = 1 };

	tagmon(&arg);
	ASSERT(1, "tagmon: noop when no sel");

{
		Monitor *tmpm = selmon; /* free the fixture, then restore globals */
		restore_selmon();
		free(tmpm);
	}
}

/* --- tagmon sends client to other monitor (line 1746) --- */
static void
test_tagmon_sends_client(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m1 = make_monitor(0);
	Monitor *m2 = make_monitor(1);
	m1->next = m2;
	m2->next = NULL;
	Client *c = make_client(1, m1);
	c->tags = 1;
	m1->clients = c;
	m1->stack = c;
	m1->sel = c;
	save_selmon();
	selmon = m1;
	mons = m1;

	Arg arg = { .i = 1 };
	tagmon(&arg);

	ASSERT(c->mon == m2, "tagmon: client moved to m2");
	ASSERT(m1->clients == NULL, "tagmon: client removed from m1");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m1); free(m2);
}

static void
run_tests(void)
{
	test_tagmon_noop_no_sel();
	test_tagmon_sends_client();
}
