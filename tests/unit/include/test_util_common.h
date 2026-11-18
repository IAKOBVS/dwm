/*
 * test_util_common.h — shared fixture for the util.c / util.h helper tests.
 *
 * These helpers have no X11 dependency, so the tests include util.c directly
 * and are deliberately not linked against mock_x11.o (which also defines
 * die()/ecalloc()).  Each test file supplies `static void run_tests(void)`.
 */

#ifndef DWM_TEST_UTIL_COMMON_H
#define DWM_TEST_UTIL_COMMON_H 1

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "util.h"
#include "util.c"

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

static int
finish_tests(void)
{
	fprintf(stderr, "=== RESULTS ===\n");
	fprintf(stderr, "Total: %d | Passed: %d | Failed: %d\n",
		total, total - failed, failed);
	if (failed) {
		fprintf(stderr, "*** %d TESTS FAILED ***\n", failed);
		return 1;
	}
	fprintf(stdout, "All tests passed.\n");
	return 0;
}

static void run_tests(void);

int
main(void)
{
	run_tests();
	return finish_tests();
}

#endif /* DWM_TEST_UTIL_COMMON_H */
