/* test_xiscntrl.c — verify xiscntrl() covers 0x00-0x1f and DEL only.
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
	ASSERT(xiscntrl(0) != 0, "NUL is a control character");
	ASSERT(xiscntrl('\t') != 0, "tab is a control character");
	ASSERT(xiscntrl('\n') != 0, "newline is a control character");
	ASSERT(xiscntrl(0x1f) != 0, "0x1f is the last low control character");
	ASSERT(xiscntrl(0x7f) != 0, "DEL is a control character");
	ASSERT_EQ(xiscntrl(' '), 0, "space is not a control character");
	ASSERT_EQ(xiscntrl('A'), 0, "uppercase A is not a control character");
	ASSERT_EQ(xiscntrl('a'), 0, "lowercase a is not a control character");
	ASSERT_EQ(xiscntrl('0'), 0, "digit 0 is not a control character");
	ASSERT_EQ(xiscntrl('9'), 0, "digit 9 is not a control character");
	ASSERT_EQ(xiscntrl(0x80), 0, "0x80 is not a control character");
	ASSERT_EQ(xiscntrl(0xff), 0, "0xff is not a control character");
	ASSERT_EQ(xiscntrl(-1), 0, "EOF is not a control character");
	ASSERT(xiscntrl(INT_MIN) != 0, "INT_MIN truncates to NUL, a control");
	ASSERT_EQ(xiscntrl(INT_MAX), 0, "INT_MAX low byte is not a control");

	printf("%d/%d assertions passed\n", total - failed, total);
	return failed ? 1 : 0;
}
