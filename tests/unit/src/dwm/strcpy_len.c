/* test_strcpy_len.c — per-function tests for dwm.h's strcpy_len().
 *
 * The copy is length-controlled, so the source need not be NUL-terminated and
 * an embedded NUL is copied like any other byte. The shared fixture in
 * test_common.h provides dwm.h/dwm.c, the mocks, and the ASSERT macros. */
#include "test_common.h"

static void
test_strcpy_len_exact_copy(void)
{
	char dst[8];

	memset(dst, 0xAA, sizeof dst);
	strcpy_len(dst, "hello", 5);

	ASSERT(strcmp(dst, "hello") == 0, "copies exactly src_len bytes");
	ASSERT_EQ(dst[5], '\0', "writes the terminator at src_len");
	ASSERT_EQ((unsigned char)dst[6], 0xAA, "does not touch bytes after NUL");
	ASSERT_EQ((unsigned char)dst[7], 0xAA, "does not touch the tail");
}

static void
test_strcpy_len_nonterminated_source(void)
{
	const char src[3] = { 'a', 'b', 'c' };
	char dst[5];

	memset(dst, 0xAA, sizeof dst);
	strcpy_len(dst, src, sizeof src);

	ASSERT(memcmp(dst, "abc", 3) == 0, "source need not be NUL-terminated");
	ASSERT_EQ(dst[3], '\0', "terminator follows the copied bytes");
	ASSERT_EQ((unsigned char)dst[4], 0xAA, "sentinel beyond terminator is intact");
}

static void
test_strcpy_len_zero_and_embedded_nul(void)
{
	const char src[4] = { 'a', '\0', 'b', 'c' };
	char dst[5];

	memset(dst, 0xAA, sizeof dst);
	strcpy_len(dst, "abc", 0);
	ASSERT_EQ(dst[0], '\0', "zero length writes an empty string");
	ASSERT_EQ((unsigned char)dst[1], 0xAA, "zero length copies no bytes");

	memset(dst, 0xAA, sizeof dst);
	strcpy_len(dst, src, sizeof src);
	ASSERT(memcmp(dst, src, sizeof src) == 0,
		"embedded NUL is copied as ordinary data");
	ASSERT_EQ(dst[1], '\0', "embedded NUL is preserved");
	ASSERT_EQ(dst[4], '\0', "terminator is still written after src_len");
}

static void
run_tests(void)
{
	test_strcpy_len_exact_copy();
	test_strcpy_len_nonterminated_source();
	test_strcpy_len_zero_and_embedded_nul();
}
