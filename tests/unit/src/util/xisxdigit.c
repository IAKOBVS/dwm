/* test_xisxdigit.c — verify xisxdigit() accepts 0-9, A-F, and a-f.
 *
 * Self-contained: only util.h is needed, so this suite links without the X11
 * mocks. The g/G and f/F boundaries prove the letter range stops at six. */
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
	ASSERT(xisxdigit('0') != 0, "digit 0 is hexadecimal");
	ASSERT(xisxdigit('9') != 0, "digit 9 is hexadecimal");
	ASSERT(xisxdigit('a') != 0, "lowercase a is hexadecimal");
	ASSERT(xisxdigit('f') != 0, "lowercase f is hexadecimal");
	ASSERT(xisxdigit('A') != 0, "uppercase A is hexadecimal");
	ASSERT(xisxdigit('F') != 0, "uppercase F is hexadecimal");
	ASSERT_EQ(xisxdigit('g'), 0, "lowercase g is not hexadecimal");
	ASSERT_EQ(xisxdigit('G'), 0, "uppercase G is not hexadecimal");
	ASSERT_EQ(xisxdigit('z'), 0, "lowercase z is not hexadecimal");
	ASSERT_EQ(xisxdigit('Z'), 0, "uppercase Z is not hexadecimal");
	ASSERT_EQ(xisxdigit('/'), 0, "character before 0 is not hexadecimal");
	ASSERT_EQ(xisxdigit(':'), 0, "character after 9 is not hexadecimal");
	ASSERT_EQ(xisxdigit(0), 0, "NUL is not hexadecimal");
	ASSERT_EQ(xisxdigit(' '), 0, "space is not hexadecimal");
	ASSERT_EQ(xisxdigit(0x7f), 0, "DEL is not hexadecimal");
	ASSERT_EQ(xisxdigit(0x80), 0, "0x80 is not hexadecimal");
	ASSERT_EQ(xisxdigit(0xff), 0, "0xff is not hexadecimal");
	ASSERT_EQ(xisxdigit(-1), 0, "EOF is not hexadecimal");
	ASSERT_EQ(xisxdigit(INT_MIN), 0, "INT_MIN is handled safely");
	ASSERT_EQ(xisxdigit(INT_MAX), 0, "INT_MAX is handled safely");

	printf("%d/%d assertions passed\n", total - failed, total);
	return failed ? 1 : 0;
}
