/*
 * test_ecalloc.c — per-function util helper tests for ecalloc().
 *
 * util.c is included directly (test_util_common.h); these tests do
 * not use the X11 mocks.
 */
#include "test_util_common.h"

static void
test_ecalloc_zeroes(void)
{
	int *p = ecalloc(4, sizeof *p);
	size_t i;

	ASSERT(p != NULL, "ecalloc: returns non-NULL");
	for (i = 0; i < 4; i++)
		ASSERT_EQ(p[i], 0, "ecalloc: memory is zero-filled");
	free(p);
}

static void
test_ecalloc_zero_count(void)
{
	void *p = ecalloc(0, sizeof(int));

	ASSERT(p != NULL, "ecalloc: zero-sized request still returns a pointer");
	free(p);
}

static void
run_tests(void)
{
	test_ecalloc_zeroes();
	test_ecalloc_zero_count();
}
