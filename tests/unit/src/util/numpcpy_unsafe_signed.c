/*
 * test_numpcpy_unsafe_signed.c — per-function util helper tests for numpcpy_unsafe_signed().
 *
 * util.c is included directly (test_util_common.h); these tests do
 * not use the X11 mocks.
 */
#include "test_util_common.h"

static void
test_numpcpy_signed_negative(void)
{
	char buf[32];
	char *end;

	end = numpcpy_unsafe_signed(buf, -42);
	ASSERT(strcmp(buf, "-42") == 0, "numpcpy_signed: negative rendered");
	ASSERT_EQ(end - buf, 3, "numpcpy_signed: length includes sign");

	end = numpcpy_unsafe_signed(buf, 0);
	ASSERT(strcmp(buf, "0") == 0, "numpcpy_signed: zero rendered");
	ASSERT_EQ(end - buf, 1, "numpcpy_signed: zero length");
}

static void
test_numpcpy_signed_min(void)
{
	char buf[32];

	numpcpy_unsafe_signed(buf, INT64_MIN);
	ASSERT(strcmp(buf, "-9223372036854775808") == 0,
		"numpcpy_signed: INT64_MIN rendered without overflow");
}

static void
run_tests(void)
{
	test_numpcpy_signed_negative();
	test_numpcpy_signed_min();
}
