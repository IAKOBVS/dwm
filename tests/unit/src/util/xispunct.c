/* test_xispunct.c — verify xispunct() is graph AND NOT alphanumeric.
 *
 * Self-contained: only util.h is needed, so this suite links without the X11
 * mocks. The 0x20/0x21 and 0x7e/0x7f boundaries pin down the range. */
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
	ASSERT(xispunct('!') != 0, "exclamation mark is punctuation");
	ASSERT(xispunct('/') != 0, "slash is punctuation");
	ASSERT(xispunct('@') != 0, "at sign is punctuation");
	ASSERT(xispunct('~') != 0, "tilde is punctuation");
	ASSERT_EQ(xispunct('A'), 0, "uppercase A is not punctuation");
	ASSERT_EQ(xispunct('a'), 0, "lowercase a is not punctuation");
	ASSERT_EQ(xispunct('Z'), 0, "uppercase Z is not punctuation");
	ASSERT_EQ(xispunct('z'), 0, "lowercase z is not punctuation");
	ASSERT_EQ(xispunct('0'), 0, "digit 0 is not punctuation");
	ASSERT_EQ(xispunct('9'), 0, "digit 9 is not punctuation");
	ASSERT_EQ(xispunct(0), 0, "NUL is not punctuation");
	ASSERT_EQ(xispunct(' '), 0, "space is not punctuation");
	ASSERT_EQ(xispunct(0x7f), 0, "DEL is not punctuation");
	ASSERT_EQ(xispunct(0x80), 0, "0x80 is not punctuation");
	ASSERT_EQ(xispunct(0xff), 0, "0xff is not punctuation");
	ASSERT_EQ(xispunct(-1), 0, "EOF is not punctuation");
	ASSERT_EQ(xispunct(INT_MIN), 0, "INT_MIN is handled safely");
	ASSERT_EQ(xispunct(INT_MAX), 0, "INT_MAX is handled safely");

	printf("%d/%d assertions passed\n", total - failed, total);
	return failed ? 1 : 0;
}
