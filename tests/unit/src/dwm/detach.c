/*
 * test_detach.c — per-function tests for detach.
 *
 * Covers the primary path and at least one edge case, using the
 * shared mock fixture from test_common.h.
 */
#include "test_common.h"

static void
test_detach_unlinks(void)
{
	Monitor m;
	Client a, b, c;

	memset(&m, 0, sizeof m);
	memset(&a, 0, sizeof a);
	memset(&b, 0, sizeof b);
	memset(&c, 0, sizeof c);
	a.mon = b.mon = c.mon = &m;
	m.clients = &a;
	a.next = &b;
	b.next = &c;

	detach(&b);
	ASSERT(m.clients == &a && a.next == &c, "detach: middle client unlinked");

	detach(&a);
	ASSERT(m.clients == &c, "detach: head client unlinked");

	detach(&c);
	ASSERT(m.clients == NULL, "detach: final client leaves empty list");
}

static void
run_tests(void)
{
	test_detach_unlinks();
}
