/* test_xisalnum.c — verify xisalnum() accepts letters and decimal digits only.
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
	ASSERT(xisalnum('A') != 0, "uppercase A is alphanumeric");
	ASSERT(xisalnum('Z') != 0, "uppercase Z is alphanumeric");
	ASSERT(xisalnum('a') != 0, "lowercase a is alphanumeric");
	ASSERT(xisalnum('z') != 0, "lowercase z is alphanumeric");
	ASSERT(xisalnum('0') != 0, "digit 0 is alphanumeric");
	ASSERT(xisalnum('9') != 0, "digit 9 is alphanumeric");
	ASSERT_EQ(xisalnum(0), 0, "NUL is not alphanumeric");
	ASSERT_EQ(xisalnum(' '), 0, "space is not alphanumeric");
	ASSERT_EQ(xisalnum('!'), 0, "punctuation is not alphanumeric");
	ASSERT_EQ(xisalnum(0x7f), 0, "DEL is not alphanumeric");
	ASSERT_EQ(xisalnum(0x80), 0, "0x80 is not alphanumeric");
	ASSERT_EQ(xisalnum(0xff), 0, "0xff is not alphanumeric");
	ASSERT_EQ(xisalnum(-1), 0, "EOF is not alphanumeric");
	ASSERT_EQ(xisalnum(INT_MIN), 0, "INT_MIN is handled safely");
	ASSERT_EQ(xisalnum(INT_MAX), 0, "INT_MAX is handled safely");

	printf("%d/%d assertions passed\n", total - failed, total);
	return failed ? 1 : 0;
}
