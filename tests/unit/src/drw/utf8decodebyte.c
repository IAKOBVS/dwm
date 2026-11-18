/*
 * test_utf8decodebyte.c — per-function drw.c tests for utf8decodebyte().
 *
 * Uses the real drw.c with the mock X11/Xft fixture from
 * test_drw_common.h; covers the primary path and an edge case.
 */
#include "test_drw_common.h"

static void
test_utf8decodebyte_classifies(void)
{
	size_t len;

	ASSERT_EQ(utf8decodebyte('A', &len), 'A',
		"utf8decodebyte: ASCII payload");
	ASSERT_EQ(len, 1, "utf8decodebyte: ASCII length 1");

	ASSERT_EQ(utf8decodebyte((char)0x80, &len), 0,
		"utf8decodebyte: bare continuation has no payload");
	ASSERT_EQ(len, 0, "utf8decodebyte: continuation length 0");

	ASSERT_EQ(utf8decodebyte((char)0xC2, &len), 2,
		"utf8decodebyte: two-byte start payload");
	ASSERT_EQ(len, 2, "utf8decodebyte: two-byte length 2");
}

static void
run_tests(void)
{
	test_utf8decodebyte_classifies();
}
