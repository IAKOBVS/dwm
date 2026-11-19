/* test_stpcpy_len.c — per-function tests for dwm.h's stpcpy_len().
 *
 * The helper must return dst + src_len and produce the same bytes as
 * strcpy_len(), enabling callers to append without rescanning. The shared
 * fixture in test_common.h provides dwm.h/dwm.c, the mocks, and ASSERT. */
#include "test_common.h"

static void
test_stpcpy_len_matches_strcpy_len(void)
{
	char dst[8], ref[8];
	char *end;

	memset(dst, 0xAA, sizeof dst);
	memset(ref, 0xAA, sizeof ref);
	end = stpcpy_len(dst, "hello", 5);
	strcpy_len(ref, "hello", 5);

	ASSERT(end == dst + 5, "returns dst + src_len");
	ASSERT_EQ(*end, '\0', "returned pointer addresses the terminator");
	ASSERT(memcmp(dst, ref, sizeof dst) == 0,
		"destination bytes match strcpy_len()");
	ASSERT_EQ((unsigned char)dst[6], 0xAA, "bytes after NUL are untouched");
}

static void
test_stpcpy_len_zero_length(void)
{
	char dst[3];
	char *end;

	memset(dst, 0xAA, sizeof dst);
	end = stpcpy_len(dst, "xyz", 0);

	ASSERT(end == dst, "zero length returns dst unchanged");
	ASSERT_EQ(dst[0], '\0', "zero length writes an empty string");
	ASSERT_EQ((unsigned char)dst[1], 0xAA, "zero length copies no bytes");
	ASSERT_EQ((unsigned char)dst[2], 0xAA, "zero length copies no bytes");
}

static void
test_stpcpy_len_embedded_nul_and_chaining(void)
{
	const char src[3] = { 'a', '\0', 'b' };
	char dst[6];
	char *end;

	memset(dst, 0xAA, sizeof dst);
	end = stpcpy_len(dst, src, sizeof src);
	ASSERT(end == dst + 3, "embedded NUL does not shorten the copy");
	ASSERT(memcmp(dst, src, sizeof src) == 0,
		"embedded NUL is copied as ordinary data");
	ASSERT_EQ(dst[3], '\0', "terminator follows all src_len bytes");

	end = stpcpy_len(end, "cd", 2);
	ASSERT(end == dst + 5, "chained call returns the new terminator");
	ASSERT(strcmp(dst, "a") == 0, "chained copy appends after the first batch");
	ASSERT_EQ(dst[4], 'd', "chained copy writes the appended byte");
	ASSERT_EQ(dst[5], '\0', "chained copy NUL-terminates");
}

static void
run_tests(void)
{
	test_stpcpy_len_matches_strcpy_len();
	test_stpcpy_len_zero_length();
	test_stpcpy_len_embedded_nul_and_chaining();
}
