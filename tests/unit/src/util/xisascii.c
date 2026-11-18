/* test_xisascii.c — verify xisascii() tests the low seven bits of the value.
 *
 * Self-contained: only util.h is needed, so this suite links without the X11
 * mocks. Note that the wrapper casts to unsigned char, so 0x100 behaves as
 * 0x00 by design; tests pin down that truncation as well as the 0x7f/0x80
 * boundary. */
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
	ASSERT(xisascii(0) != 0, "NUL is ASCII");
	ASSERT(xisascii('A') != 0, "uppercase A is ASCII");
	ASSERT(xisascii('a') != 0, "lowercase a is ASCII");
	ASSERT(xisascii('z') != 0, "lowercase z is ASCII");
	ASSERT(xisascii('0') != 0, "digit 0 is ASCII");
	ASSERT(xisascii('9') != 0, "digit 9 is ASCII");
	ASSERT(xisascii(' ') != 0, "space is ASCII");
	ASSERT(xisascii(0x7f) != 0, "DEL is still ASCII");
	ASSERT_EQ(xisascii(0x80), 0, "0x80 is outside ASCII");
	ASSERT_EQ(xisascii(0xff), 0, "0xff is outside ASCII");
	ASSERT_EQ(xisascii(-1), 0, "EOF is outside ASCII");
	ASSERT(xisascii(0x100) != 0, "0x100 truncates to NUL and is ASCII");
	ASSERT_EQ(xisascii(0x180), 0, "0x180 truncates to 0x80, not ASCII");
	ASSERT(xisascii(INT_MIN) != 0, "INT_MIN truncates to NUL, which is ASCII");
	ASSERT_EQ(xisascii(INT_MAX), 0, "INT_MAX low byte is not ASCII");

	printf("%d/%d assertions passed\n", total - failed, total);
	return failed ? 1 : 0;
}
