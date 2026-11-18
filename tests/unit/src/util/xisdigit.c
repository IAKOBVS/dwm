/* test_xisdigit.c — verify xisdigit() accepts only ASCII decimal digits.
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
	ASSERT(xisdigit('0') != 0, "digit 0 is a digit");
	ASSERT(xisdigit('5') != 0, "digit 5 is a digit");
	ASSERT(xisdigit('9') != 0, "digit 9 is a digit");
	ASSERT_EQ(xisdigit('/'), 0, "character before 0 is not a digit");
	ASSERT_EQ(xisdigit(':'), 0, "character after 9 is not a digit");
	ASSERT_EQ(xisdigit('A'), 0, "uppercase A is not a digit");
	ASSERT_EQ(xisdigit('z'), 0, "lowercase z is not a digit");
	ASSERT_EQ(xisdigit(0), 0, "NUL is not a digit");
	ASSERT_EQ(xisdigit(' '), 0, "space is not a digit");
	ASSERT_EQ(xisdigit(0x7f), 0, "DEL is not a digit");
	ASSERT_EQ(xisdigit(0x80), 0, "0x80 is not a digit");
	ASSERT_EQ(xisdigit(0xff), 0, "0xff is not a digit");
	ASSERT_EQ(xisdigit(-1), 0, "EOF is not a digit");
	ASSERT_EQ(xisdigit(INT_MIN), 0, "INT_MIN is handled safely");
	ASSERT_EQ(xisdigit(INT_MAX), 0, "INT_MAX is handled safely");

	printf("%d/%d assertions passed\n", total - failed, total);
	return failed ? 1 : 0;
}
