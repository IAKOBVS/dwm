/* test_xisspace.c — verify xisspace() covers space and 0x09-0x0d.
 *
 * Self-contained: only util.h is needed, so this suite links without the X11
 * mocks. ISSPACE spans the five ASCII control whitespace characters, so a
 * newline is whitespace; the 0x08/0x09 and 0x0d/0x0e boundaries prove it. */
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
	ASSERT(xisspace(' ') != 0, "space is whitespace");
	ASSERT(xisspace('\t') != 0, "tab is whitespace");
	ASSERT(xisspace('\n') != 0, "newline is whitespace");
	ASSERT(xisspace('\v') != 0, "vertical tab is whitespace");
	ASSERT(xisspace('\f') != 0, "form feed is whitespace");
	ASSERT(xisspace('\r') != 0, "carriage return is whitespace");
	ASSERT_EQ(xisspace(0x08), 0, "backspace is below the whitespace range");
	ASSERT_EQ(xisspace(0x0e), 0, "0x0e is above the whitespace range");
	ASSERT_EQ(xisspace(0), 0, "NUL is not whitespace");
	ASSERT_EQ(xisspace('A'), 0, "uppercase A is not whitespace");
	ASSERT_EQ(xisspace('a'), 0, "lowercase a is not whitespace");
	ASSERT_EQ(xisspace('Z'), 0, "uppercase Z is not whitespace");
	ASSERT_EQ(xisspace('z'), 0, "lowercase z is not whitespace");
	ASSERT_EQ(xisspace('0'), 0, "digit 0 is not whitespace");
	ASSERT_EQ(xisspace('9'), 0, "digit 9 is not whitespace");
	ASSERT_EQ(xisspace(0x7f), 0, "DEL is not whitespace");
	ASSERT_EQ(xisspace(0x80), 0, "0x80 is not whitespace");
	ASSERT_EQ(xisspace(0xff), 0, "0xff is not whitespace");
	ASSERT_EQ(xisspace(-1), 0, "EOF is not whitespace");
	ASSERT_EQ(xisspace(INT_MIN), 0, "INT_MIN is handled safely");
	ASSERT_EQ(xisspace(INT_MAX), 0, "INT_MAX is handled safely");

	printf("%d/%d assertions passed\n", total - failed, total);
	return failed ? 1 : 0;
}
