/* test_xislower.c — verify xislower() covers a-z only.
 *
 * Self-contained: only util.h is needed, so this suite links without the X11
 * mocks. The 0x60/0x61 and 0x7a/0x7b boundaries pin down the range. */
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
	ASSERT(xislower('a') != 0, "lowercase a is lowercase");
	ASSERT(xislower('z') != 0, "lowercase z is lowercase");
	ASSERT_EQ(xislower('`'), 0, "character before a is not lowercase");
	ASSERT_EQ(xislower('{'), 0, "character after z is not lowercase");
	ASSERT_EQ(xislower('A'), 0, "uppercase A is not lowercase");
	ASSERT_EQ(xislower('Z'), 0, "uppercase Z is not lowercase");
	ASSERT_EQ(xislower('0'), 0, "digit 0 is not lowercase");
	ASSERT_EQ(xislower('9'), 0, "digit 9 is not lowercase");
	ASSERT_EQ(xislower(0), 0, "NUL is not lowercase");
	ASSERT_EQ(xislower(' '), 0, "space is not lowercase");
	ASSERT_EQ(xislower(0x7f), 0, "DEL is not lowercase");
	ASSERT_EQ(xislower(0x80), 0, "0x80 is not lowercase");
	ASSERT_EQ(xislower(0xff), 0, "0xff is not lowercase");
	ASSERT_EQ(xislower(-1), 0, "EOF is not lowercase");
	ASSERT_EQ(xislower(INT_MIN), 0, "INT_MIN is handled safely");
	ASSERT_EQ(xislower(INT_MAX), 0, "INT_MAX is handled safely");

	printf("%d/%d assertions passed\n", total - failed, total);
	return failed ? 1 : 0;
}
