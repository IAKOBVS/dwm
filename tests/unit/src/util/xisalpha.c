/* test_xisalpha.c — verify xisalpha() classifies C-locale letters.
 *
 * Self-contained: only util.h is needed, so this suite links without the X11
 * mocks. Out-of-range ints prove the wrapper's unsigned char cast is safe. */
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
	ASSERT(xisalpha('A') != 0, "uppercase A is alphabetic");
	ASSERT(xisalpha('Z') != 0, "uppercase Z is alphabetic");
	ASSERT(xisalpha('a') != 0, "lowercase a is alphabetic");
	ASSERT(xisalpha('z') != 0, "lowercase z is alphabetic");
	ASSERT_EQ(xisalpha('0'), 0, "digit 0 is not alphabetic");
	ASSERT_EQ(xisalpha('9'), 0, "digit 9 is not alphabetic");
	ASSERT_EQ(xisalpha(0), 0, "NUL is not alphabetic");
	ASSERT_EQ(xisalpha(' '), 0, "space is not alphabetic");
	ASSERT_EQ(xisalpha(0x7f), 0, "DEL is not alphabetic");
	ASSERT_EQ(xisalpha(0x80), 0, "0x80 is not alphabetic");
	ASSERT_EQ(xisalpha(0xff), 0, "0xff is not alphabetic");
	ASSERT_EQ(xisalpha(-1), 0, "EOF does not classify as alphabetic");
	ASSERT_EQ(xisalpha(INT_MIN), 0, "INT_MIN is handled safely");
	ASSERT_EQ(xisalpha(INT_MAX), 0, "INT_MAX is handled safely");

	printf("%d/%d assertions passed\n", total - failed, total);
	return failed ? 1 : 0;
}
