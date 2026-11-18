/* test_xtoupper.c — verify xtoupper() maps a-z and leaves everything else.
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
	ASSERT_EQ(xtoupper('a'), 'A', "a maps to A");
	ASSERT_EQ(xtoupper('z'), 'Z', "z maps to Z");
	ASSERT_EQ(xtoupper('m'), 'M', "m maps to M");
	ASSERT_EQ(xtoupper('A'), 'A', "uppercase A is unchanged");
	ASSERT_EQ(xtoupper('Z'), 'Z', "uppercase Z is unchanged");
	ASSERT_EQ(xtoupper('0'), '0', "digit 0 is unchanged");
	ASSERT_EQ(xtoupper('9'), '9', "digit 9 is unchanged");
	ASSERT_EQ(xtoupper(' '), ' ', "space is unchanged");
	ASSERT_EQ(xtoupper('!'), '!', "punctuation is unchanged");
	ASSERT_EQ(xtoupper(0), 0, "NUL is unchanged");
	ASSERT_EQ(xtoupper(0x7f), 0x7f, "DEL is unchanged");
	ASSERT_EQ(xtoupper(0x80), 0x80, "0x80 is unchanged");
	ASSERT_EQ(xtoupper(0xff), 0xff, "0xff is unchanged");
	ASSERT_EQ(xtoupper(-1), 0xff, "EOF keeps its low-byte value");
	ASSERT_EQ(xtoupper(INT_MIN), 0, "INT_MIN truncates to NUL unchanged");
	ASSERT_EQ(xtoupper(INT_MAX), 0xff, "INT_MAX keeps its low-byte value");

	printf("%d/%d assertions passed\n", total - failed, total);
	return failed ? 1 : 0;
}
