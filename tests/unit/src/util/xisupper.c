/* test_xisupper.c — verify xisupper() covers A-Z only.
 *
 * Self-contained: only util.h is needed, so this suite links without the X11
 * mocks. The 0x40/0x41 and 0x5a/0x5b boundaries pin down the range. */
#include <limits.h>
#include <stdio.h>

#include "util.h"

static int total = 0, failed = 0;
#define ASSERT(cond, msg) do { \
	total++; \
	if (!(cond)) { \
		failed++; \
		fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, msg); \
	} \
} while(0)
#define ASSERT_EQ(a, b, msg) do { \
	total++; \
	if ((a) != (b)) { \
		failed++; \
		fprintf(stderr, "  FAIL %s:%d: %s (%d != %d)\n", __FILE__, __LINE__, msg, (int)(a), (int)(b)); \
	} \
} while(0)

int
main(void)
{
	ASSERT(xisupper('A') != 0, "uppercase A is uppercase");
	ASSERT(xisupper('Z') != 0, "uppercase Z is uppercase");
	ASSERT_EQ(xisupper('@'), 0, "character before A is not uppercase");
	ASSERT_EQ(xisupper('['), 0, "character after Z is not uppercase");
	ASSERT_EQ(xisupper('a'), 0, "lowercase a is not uppercase");
	ASSERT_EQ(xisupper('z'), 0, "lowercase z is not uppercase");
	ASSERT_EQ(xisupper('0'), 0, "digit 0 is not uppercase");
	ASSERT_EQ(xisupper('9'), 0, "digit 9 is not uppercase");
	ASSERT_EQ(xisupper(0), 0, "NUL is not uppercase");
	ASSERT_EQ(xisupper(' '), 0, "space is not uppercase");
	ASSERT_EQ(xisupper(0x7f), 0, "DEL is not uppercase");
	ASSERT_EQ(xisupper(0x80), 0, "0x80 is not uppercase");
	ASSERT_EQ(xisupper(0xff), 0, "0xff is not uppercase");
	ASSERT_EQ(xisupper(-1), 0, "EOF is not uppercase");
	ASSERT_EQ(xisupper(INT_MIN), 0, "INT_MIN is handled safely");
	ASSERT_EQ(xisupper(INT_MAX), 0, "INT_MAX is handled safely");

	printf("%d/%d assertions passed\n", total - failed, total);
	return failed ? 1 : 0;
}
