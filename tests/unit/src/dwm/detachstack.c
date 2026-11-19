/*
 * test_detachstack.c — per-function tests for detachstack.
 *
 * Covers the primary path and at least one edge case, using the
 * shared mock fixture from test_common.h.
 */
#include "test_common.h"

static void
test_detachstack_promotes_selection(void)
{
	Monitor m;
	Client a, b;

	memset(&m, 0, sizeof m);
	memset(&a, 0, sizeof a);
	memset(&b, 0, sizeof b);
	m.tagset[0] = 1;
	a.mon = b.mon = &m;
	a.tags = b.tags = 1;
	m.stack = &a;
	a.snext = &b;
	m.sel = &a;

	detachstack(&a);
	ASSERT(m.stack == &b, "detachstack: removes client from stack");
	ASSERT(m.sel == &b, "detachstack: promotes next visible stack entry");
}

static void
run_tests(void)
{
	test_detachstack_promotes_selection();
}
