/* test_stpncpy_len.c — per-function tests for dwm.h's stpncpy_len().
 *
 * The helper copies at most dstsize - 1 bytes, always NUL-terminates, and
 * never pads the remainder like strncpy() would. The shared fixture in
 * test_common.h provides dwm.h/dwm.c, the mocks, and the ASSERT macros. */
#include "test_common.h"

static void
test_stpncpy_len_exact_fit(void)
{
	char dst[6];
	char *end;

	memset(dst, 0xAA, sizeof dst);
	end = stpncpy_len(dst, sizeof dst, "hello", 5);

	ASSERT(end == dst + 5, "exact fit returns the terminator position");
	ASSERT(strcmp(dst, "hello") == 0, "exact fit copies the whole source");
	ASSERT_EQ(dst[5], '\0', "exact fit terminates at src_len");
}

static void
test_stpncpy_len_truncation(void)
{
	char dst[6];
	char *end;

	memset(dst, 0xAA, sizeof dst);
	end = stpncpy_len(dst, 4, "abcdef", 6);

	ASSERT(end == dst + 3, "truncation returns dst + dstsize - 1");
	ASSERT(strcmp(dst, "abc") == 0, "truncates to dstsize - 1 bytes");
	ASSERT_EQ(dst[3], '\0', "truncated result is NUL-terminated");
	ASSERT_EQ((unsigned char)dst[4], 0xAA,
		"no padding past the terminator");
	ASSERT_EQ((unsigned char)dst[5], 0xAA,
		"tail past the destination is untouched");
}

static void
test_stpncpy_len_size_one(void)
{
	char dst[3];
	char *end;

	memset(dst, 0xAA, sizeof dst);
	end = stpncpy_len(dst, 1, "abcdef", 6);

	ASSERT(end == dst, "dstsize 1 returns dst");
	ASSERT_EQ(dst[0], '\0', "dstsize 1 yields an empty string");
	ASSERT_EQ((unsigned char)dst[1], 0xAA, "dstsize 1 copies no source bytes");
	ASSERT_EQ((unsigned char)dst[2], 0xAA, "dstsize 1 writes only the NUL");
}

static void
test_stpncpy_len_no_padding(void)
{
	char dst[8];
	char *end;

	memset(dst, 0xAA, sizeof dst);
	end = stpncpy_len(dst, sizeof dst, "hi", 2);

	ASSERT(end == dst + 2, "sufficient capacity returns src_len");
	ASSERT(strcmp(dst, "hi") == 0, "source fits without truncation");
	ASSERT_EQ(dst[2], '\0', "result is NUL-terminated");
	ASSERT_EQ((unsigned char)dst[3], 0xAA,
		"remainder is not padded with NULs");
	ASSERT_EQ((unsigned char)dst[7], 0xAA,
		"tail sentinel proves no padding");
}

static void
run_tests(void)
{
	test_stpncpy_len_exact_fit();
	test_stpncpy_len_truncation();
	test_stpncpy_len_size_one();
	test_stpncpy_len_no_padding();
}
