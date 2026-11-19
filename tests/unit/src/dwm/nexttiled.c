/*
 * test_nexttiled.c — per-function tests for nexttiled.
 *
 * Covers the primary path and at least one edge case, using the
 * shared mock fixture from test_common.h.
 */
#include "test_common.h"

static void
test_nexttiled_skips_floating_and_invisible(void)
{
	Monitor m;
	Client a, b, c;

	memset(&m, 0, sizeof m);
	memset(&a, 0, sizeof a);
	memset(&b, 0, sizeof b);
	memset(&c, 0, sizeof c);
	m.tagset[0] = 1;
	a.mon = b.mon = c.mon = &m;
	a.tags = 1;
	b.tags = 1;
	b.isfloating = 1;
	c.tags = 0; /* not visible under tagset[0] == 1 */
	a.next = &b;
	b.next = &c;

	ASSERT(nexttiled(&a) == &a, "nexttiled: visible tiled head returned");
	ASSERT(nexttiled(&b) == NULL,
		"nexttiled: floating and invisible clients skipped");
	ASSERT(nexttiled(NULL) == NULL, "nexttiled: NULL list returns NULL");
}

static void
run_tests(void)
{
	test_nexttiled_skips_floating_and_invisible();
}
