/*
 * test_arrange.c — per-function tests for dwm.c's arrange().
 *
 * Focused on arrange()'s coalescing contract: an ordinary call runs one layout
 * pass, a call during event dispatch is deferred into arrange_pending, and the
 * pointer-grab path (and a direct arrangenow()) bypasses the deferral.  Full
 * geometry equivalence is covered by integration_optimize_layout.
 */
#include "test_common.h"

static void
reset_arrange_globals(void)
{
	dispatching = 0;
	pointergrab = 0;
	arrange_pending = 0;
	arrange_calls = 0;
}

/* --- ordinary call arranges immediately --- */
static void
test_arrange_nulls_calls_showhide(void)
{
	reset_index();
	reset_arrange_globals();
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	m->clients = m->stack = c;
	m->sel = NULL;
	save_selmon();
	selmon = m;
	mons = m;

	arrange(m);

	ASSERT_EQ(arrange_calls, 1,
		"arrange: outside dispatch performs exactly one layout pass");
	ASSERT_EQ(arrange_pending, 0,
		"arrange: outside dispatch leaves no deferred request");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
	reset_arrange_globals();
}

/* --- calls during dispatch are coalesced into one deferred pass --- */
static void
test_arrange_all_monitors(void)
{
	reset_index();
	reset_arrange_globals();
	Monitor *m1 = make_monitor(0);
	Monitor *m2 = make_monitor(1);
	m1->next = m2;
	m1->sel = NULL;
	m2->sel = NULL;
	mons = m1;
	save_selmon();
	selmon = m1;

	dispatching = 1;
	arrange(m1);
	arrange(m2);

	ASSERT_EQ(arrange_calls, 0,
		"arrange: dispatch-time calls perform no inline layout work");
	ASSERT_EQ(arrange_pending, 1,
		"arrange: dispatch-time calls set the coalescing flag");

	/* A direct arrangenow() is the bypass used by low-frequency paths. */
	arrangenow(m1);
	ASSERT_EQ(arrange_calls, 1,
		"arrange: arrangenow bypasses the deferral and runs immediately");
	ASSERT_EQ(arrange_pending, 1,
		"arrange: arrangenow does not clear the pending flag");

	dispatching = 0;
	restore_selmon();
	free(m1); free(m2);
	reset_arrange_globals();
}

/* --- pointer grabs force an immediate reflow even during dispatch --- */
static void
test_arrange_pointergrab_immediate(void)
{
	reset_index();
	reset_arrange_globals();
	Monitor *m = make_monitor(0);
	Client *c = make_client(3, m);
	m->clients = m->stack = c;
	m->sel = NULL;
	save_selmon();
	selmon = m;
	mons = m;

	dispatching = 1;
	pointergrab = 1;
	arrange(m);

	ASSERT_EQ(arrange_calls, 1,
		"arrange: pointer grab bypasses coalescing");
	ASSERT_EQ(arrange_pending, 0,
		"arrange: pointer grab does not set the pending flag");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
	reset_arrange_globals();
}

static void
run_tests(void)
{
	test_arrange_nulls_calls_showhide();
	test_arrange_all_monitors();
	test_arrange_pointergrab_immediate();
}
