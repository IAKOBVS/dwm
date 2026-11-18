/* test_xisprint.c — verify xisprint() covers 0x20-0x7e including space.
 *
 * Self-contained: only util.h is needed, so this suite links without the X11
 * mocks. The 0x1f/0x20 and 0x7e/0x7f boundaries pin down the range. */
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
	ASSERT(xisprint(' ') != 0, "space is printable");
	ASSERT(xisprint(0x7e) != 0, "0x7e is the last printable character");
	ASSERT(xisprint('A') != 0, "uppercase A is printable");
	ASSERT(xisprint('a') != 0, "lowercase a is printable");
	ASSERT(xisprint('Z') != 0, "uppercase Z is printable");
	ASSERT(xisprint('z') != 0, "lowercase z is printable");
	ASSERT(xisprint('0') != 0, "digit 0 is printable");
	ASSERT(xisprint('9') != 0, "digit 9 is printable");
	ASSERT_EQ(xisprint(0), 0, "NUL is not printable");
	ASSERT_EQ(xisprint('\t'), 0, "tab is not printable");
	ASSERT_EQ(xisprint(0x1f), 0, "0x1f is not printable");
	ASSERT_EQ(xisprint(0x7f), 0, "DEL is not printable");
	ASSERT_EQ(xisprint(0x80), 0, "0x80 is not printable");
	ASSERT_EQ(xisprint(0xff), 0, "0xff is not printable");
	ASSERT_EQ(xisprint(-1), 0, "EOF is not printable");
	ASSERT_EQ(xisprint(INT_MIN), 0, "INT_MIN is handled safely");
	ASSERT_EQ(xisprint(INT_MAX), 0, "INT_MAX is handled safely");

	printf("%d/%d assertions passed\n", total - failed, total);
	return failed ? 1 : 0;
}
