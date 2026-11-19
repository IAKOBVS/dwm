/*
 * test_arrangemon.c — per-function tests for arrangemon.
 *
 * Covers the primary path and at least one edge case, using the
 * shared mock fixture from test_common.h.
 */
#include "test_common.h"

static int fake_arrange_calls;

static void
fake_arrange(Monitor *m)
{
	(void)m;
	fake_arrange_calls++;
}

static void
test_arrangemon_updates_symbol_and_layout(void)
{
	Layout fake = { "FKE", fake_arrange };
	Monitor *m;

	reset_index();
	m = make_monitor(0);
	m->lt[0] = m->lt[1] = &fake;
	m->sellt = 0;
	fake_arrange_calls = 0;

	arrangemon(m);
	ASSERT_EQ(fake_arrange_calls, 1, "arrangemon: invokes layout arrange fn");
	ASSERT(strcmp(m->ltsymbol, "FKE") == 0, "arrangemon: copies layout symbol");

	/* No arrange function (floating layout) must be a safe no-op */
	fake.arrange = NULL;
	arrangemon(m);
	ASSERT_EQ(fake_arrange_calls, 1, "arrangemon: NULL arrange skipped");

	free(m);
}

static void
run_tests(void)
{
	test_arrangemon_updates_symbol_and_layout();
}
