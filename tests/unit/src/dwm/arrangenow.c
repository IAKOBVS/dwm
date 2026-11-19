/*
 * test_arrangenow.c — per-function tests for arrangenow.
 *
 * Covers the primary path and at least one edge case, using the
 * shared mock fixture from test_common.h.
 */
#include "test_common.h"

static void
test_arrangenow_counts_pass(void)
{
	Monitor *m;
	Client *c;

	save_selmon();
	reset_index();
	m = make_monitor(0);
	c = make_client(1, m);
	m->clients = m->stack = c;
	selmon = mons = m;
	arrange_calls = 0;

	arrangenow(m);
	ASSERT_EQ(arrange_calls, 1, "arrangenow: one layout pass");
	ASSERT_EQ(c->x, m->wx + m->gap.gappx,
		"arrangenow: client reflowed by tile with outer gap");

	/* NULL monitor walks the whole monitor list instead */
	arrangenow(NULL);
	ASSERT_EQ(arrange_calls, 2, "arrangenow(NULL): list walk pass");

	reset_index();
	restore_selmon();
	free(c);
	free(m);
}

static void
run_tests(void)
{
	test_arrangenow_counts_pass();
}
