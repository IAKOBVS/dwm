/* test_xisblank.c — verify xisblank() accepts only space and horizontal tab.
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
	ASSERT(xisblank(' ') != 0, "space is a blank");
	ASSERT(xisblank('\t') != 0, "horizontal tab is a blank");
	ASSERT_EQ(xisblank('\n'), 0, "newline is whitespace but not blank");
	ASSERT_EQ(xisblank('\v'), 0, "vertical tab is not blank");
	ASSERT_EQ(xisblank('\r'), 0, "carriage return is not blank");
	ASSERT_EQ(xisblank(0), 0, "NUL is not blank");
	ASSERT_EQ(xisblank('A'), 0, "uppercase A is not blank");
	ASSERT_EQ(xisblank('a'), 0, "lowercase a is not blank");
	ASSERT_EQ(xisblank('0'), 0, "digit 0 is not blank");
	ASSERT_EQ(xisblank('9'), 0, "digit 9 is not blank");
	ASSERT_EQ(xisblank(0x7f), 0, "DEL is not blank");
	ASSERT_EQ(xisblank(0x80), 0, "0x80 is not blank");
	ASSERT_EQ(xisblank(0xff), 0, "0xff is not blank");
	ASSERT_EQ(xisblank(-1), 0, "EOF is not blank");
	ASSERT_EQ(xisblank(INT_MIN), 0, "INT_MIN is handled safely");
	ASSERT_EQ(xisblank(INT_MAX), 0, "INT_MAX is handled safely");

	printf("%d/%d assertions passed\n", total - failed, total);
	return failed ? 1 : 0;
}
