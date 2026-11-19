/*
 * test_sendmon.c — per-function tests for dwm.c's sendmon().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through sendmon(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- sendmon --- */
static void
test_sendmon_same_monitor(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	m->sel = c;
	m->clients = c;
	m->stack = c;
	save_selmon();
	selmon = m;
	mons = m;

	sendmon(c, m);
	ASSERT(c->mon == m, "sendmon: same monitor no change");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

static void
test_sendmon_different_monitor(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m1 = make_monitor(0);
	Monitor *m2 = make_monitor(1);
	m1->next = m2;
	m1->sel = NULL;
	m2->sel = NULL;
	Client *c = make_client(1, m1);
	m1->clients = c;
	m1->stack = c;
	mons = m1;
	save_selmon();
	selmon = m1;

	sendmon(c, m2);
	ASSERT(c->mon == m2, "sendmon: moved to new monitor");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m1); free(m2);
}

/* --- a swallowed placeholder follows its holder across monitors --- */
static void
test_sendmon_moves_swallowed_placeholder(void)
{
	reset_index();
	Monitor *m1 = make_monitor(0);
	Monitor *m2 = make_monitor(1);
	Client *term = make_client(10, m1);
	Client *ph = make_client(11, m1);

	term->swallowing = ph;
	term->state = CST_HOLDER;
	ph->state = CST_PLACEHOLDER;
	ph->holder = term;
	m1->clients = term;
	m1->stack = term;
	mons = m1;
	save_selmon();
	selmon = m1;

	sendmon(term, m2);

	ASSERT_EQ(term->mon, m2, "sendmon: holder moves");
	ASSERT_EQ(ph->mon, m2, "sendmon: swallowed placeholder follows holder");

	restore_selmon();
	winclient_remove(term);
	winclient_remove(ph);
	free(term); free(ph); free(m1); free(m2);
}

static void
run_tests(void)
{
	test_sendmon_same_monitor();
	test_sendmon_different_monitor();
	test_sendmon_moves_swallowed_placeholder();
}
