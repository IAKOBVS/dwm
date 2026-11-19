/*
 * test_attach.c — per-function tests for attach.
 *
 * Covers the primary path and at least one edge case, using the
 * shared mock fixture from test_common.h.
 */
#include "test_common.h"

static void
test_attach_prepends(void)
{
	Monitor m;
	Client a, b;

	memset(&m, 0, sizeof m);
	memset(&a, 0, sizeof a);
	memset(&b, 0, sizeof b);
	a.mon = b.mon = &m;
	m.clients = &a;

	attach(&b);
	ASSERT(m.clients == &b, "attach: new client becomes list head");
	ASSERT(b.next == &a, "attach: previous head follows new client");

	attach(&a);
	ASSERT(m.clients == &a, "attach: duplicate re-prepend is harmless");
}

static void
run_tests(void)
{
	test_attach_prepends();
}
