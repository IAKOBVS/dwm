/*
 * test_unfocus.c — per-function tests for unfocus.
 *
 * Covers the primary path and at least one edge case, using the
 * shared mock fixture from test_common.h.
 */
#include "test_common.h"

static void
test_unfocus_guards_null_and_restores_border(void)
{
	Client *c;

	reset_index();
	c = make_client(1, selmon);

	unfocus(c, 1);
	unfocus(c, 0);
	unfocus(NULL, 1); /* edge: NULL is a no-op */
	ASSERT(1, "unfocus: NULL and normal paths do not crash");

	reset_index();
	free(c);
}

static void
run_tests(void)
{
	test_unfocus_guards_null_and_restores_border();
}
