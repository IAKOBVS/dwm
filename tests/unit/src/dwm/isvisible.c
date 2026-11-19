/*
 * test_isvisible.c — per-function tests for isvisible.
 *
 * Covers the primary path and at least one edge case, using the
 * shared mock fixture from test_common.h.
 */
#include "test_common.h"

static void
test_isvisible_tag_match(void)
{
	Monitor m;
	Client client;
	Client *c = &client;

	memset(&m, 0, sizeof m);
	memset(&client, 0, sizeof client);
	m.tagset[0] = 1;
	c->mon = &m;
	c->tags = 1;
	ASSERT(ISVISIBLE(c), "ISVISIBLE: matching tag is visible");

	c->tags = 2; /* not in tagset[0] */
	ASSERT(!ISVISIBLE(c), "ISVISIBLE: non-matching tag hidden");
}

static void
run_tests(void)
{
	test_isvisible_tag_match();
}
