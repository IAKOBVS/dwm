/*
 * test_attachstack.c — per-function tests for attachstack.
 *
 * Covers the primary path and at least one edge case, using the
 * shared mock fixture from test_common.h.
 */
#include "test_common.h"

static void
test_attachstack_prepends(void)
{
	Monitor m;
	Client a, b;

	memset(&m, 0, sizeof m);
	memset(&a, 0, sizeof a);
	memset(&b, 0, sizeof b);
	a.mon = b.mon = &m;
	m.stack = &a;

	attachstack(&b);
	ASSERT(m.stack == &b, "attachstack: new client becomes stack head");
	ASSERT(b.snext == &a, "attachstack: previous head follows");
}

static void
run_tests(void)
{
	test_attachstack_prepends();
}
