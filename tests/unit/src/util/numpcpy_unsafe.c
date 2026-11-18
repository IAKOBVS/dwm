/*
 * test_numpcpy_unsafe.c — per-function util helper tests for numpcpy_unsafe().
 *
 * util.c is included directly (test_util_common.h); these tests do
 * not use the X11 mocks.
 */
#include "test_util_common.h"

static void
test_numpcpy_unsafe_small(void)
{
	char buf[32];
	char *end;

	end = numpcpy_unsafe(buf, 0);
	ASSERT(strcmp(buf, "0") == 0, "numpcpy_unsafe: zero rendered");
	ASSERT_EQ(end - buf, 1, "numpcpy_unsafe: returns end pointer");

	end = numpcpy_unsafe(buf, 12345);
	ASSERT(strcmp(buf, "12345") == 0, "numpcpy_unsafe: multi-digit rendered");
	ASSERT_EQ(end - buf, 5, "numpcpy_unsafe: end pointer tracks length");
}

static void
test_numpcpy_unsafe_max(void)
{
	char buf[32];
	char *end;

	end = numpcpy_unsafe(buf, UINT64_MAX);
	ASSERT(strcmp(buf, "18446744073709551615") == 0,
		"numpcpy_unsafe: UINT64_MAX rendered");
	ASSERT_EQ(end - buf, 20, "numpcpy_unsafe: max length is 20");
}

static void
run_tests(void)
{
	test_numpcpy_unsafe_small();
	test_numpcpy_unsafe_max();
}
