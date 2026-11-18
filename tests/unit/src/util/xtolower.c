/* test_xtolower.c — verify xtolower() maps A-Z and leaves everything else.
 *
 * Self-contained: only util.h is needed, so this suite links without the X11
 * mocks. The wrapper converts through unsigned char, so a negative int keeps
 * its low-byte value instead of the mathematical negative value. */
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
	ASSERT_EQ(xtolower('A'), 'a', "A maps to a");
	ASSERT_EQ(xtolower('Z'), 'z', "Z maps to z");
	ASSERT_EQ(xtolower('M'), 'm', "M maps to m");
	ASSERT_EQ(xtolower('a'), 'a', "lowercase a is unchanged");
	ASSERT_EQ(xtolower('z'), 'z', "lowercase z is unchanged");
	ASSERT_EQ(xtolower('0'), '0', "digit 0 is unchanged");
	ASSERT_EQ(xtolower('9'), '9', "digit 9 is unchanged");
	ASSERT_EQ(xtolower(' '), ' ', "space is unchanged");
	ASSERT_EQ(xtolower('!'), '!', "punctuation is unchanged");
	ASSERT_EQ(xtolower(0), 0, "NUL is unchanged");
	ASSERT_EQ(xtolower(0x7f), 0x7f, "DEL is unchanged");
	ASSERT_EQ(xtolower(0x80), 0x80, "0x80 is unchanged");
	ASSERT_EQ(xtolower(0xff), 0xff, "0xff is unchanged");
	ASSERT_EQ(xtolower(-1), 0xff, "EOF keeps its low-byte value");
	ASSERT_EQ(xtolower(INT_MIN), 0, "INT_MIN truncates to NUL unchanged");
	ASSERT_EQ(xtolower(INT_MAX), 0xff, "INT_MAX keeps its low-byte value");

	printf("%d/%d assertions passed\n", total - failed, total);
	return failed ? 1 : 0;
}
