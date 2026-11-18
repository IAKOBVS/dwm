/* test_xisgraph.c — verify xisgraph() covers printable non-space ASCII.
 *
 * Self-contained: only util.h is needed, so this suite links without the X11
 * mocks. The 0x20/0x21 and 0x7e/0x7f boundaries pin down the range. */
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
	ASSERT(xisgraph(0x21) != 0, "0x21 is the first graph character");
	ASSERT(xisgraph('A') != 0, "uppercase A is a graph character");
	ASSERT(xisgraph('a') != 0, "lowercase a is a graph character");
	ASSERT(xisgraph('Z') != 0, "uppercase Z is a graph character");
	ASSERT(xisgraph('z') != 0, "lowercase z is a graph character");
	ASSERT(xisgraph('0') != 0, "digit 0 is a graph character");
	ASSERT(xisgraph('9') != 0, "digit 9 is a graph character");
	ASSERT(xisgraph('!') != 0, "punctuation is a graph character");
	ASSERT(xisgraph(0x7e) != 0, "0x7e is the last graph character");
	ASSERT_EQ(xisgraph(0), 0, "NUL is not a graph character");
	ASSERT_EQ(xisgraph(' '), 0, "space is not a graph character");
	ASSERT_EQ(xisgraph('\t'), 0, "tab is not a graph character");
	ASSERT_EQ(xisgraph(0x7f), 0, "DEL is not a graph character");
	ASSERT_EQ(xisgraph(0x80), 0, "0x80 is not a graph character");
	ASSERT_EQ(xisgraph(0xff), 0, "0xff is not a graph character");
	ASSERT_EQ(xisgraph(-1), 0, "EOF is not a graph character");
	ASSERT_EQ(xisgraph(INT_MIN), 0, "INT_MIN is handled safely");
	ASSERT_EQ(xisgraph(INT_MAX), 0, "INT_MAX is handled safely");

	printf("%d/%d assertions passed\n", total - failed, total);
	return failed ? 1 : 0;
}
