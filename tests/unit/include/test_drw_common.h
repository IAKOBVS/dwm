/*
 * test_drw_common.h — shared fixture for the per-function drw.c test suites.
 *
 * Includes the real drw.c against the mock X11/Xft/Fontconfig stubs.  No
 * dwm.c is pulled in, so these tests exercise the drawing library directly.
 * Each test file provides a single `static void run_tests(void)`; main()
 * and the assertion counters live here.
 */

#ifndef DWM_TEST_DRW_COMMON_H
#define DWM_TEST_DRW_COMMON_H 1

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "mock_x11.h"
#include "drw.h"

/* drw.c references die() via DIE(); replace it with an abort-on-demand
 * implementation.  mock_die_abort lets a test observe the failure path
 * without terminating the process. */
#undef die
static void
die(const char *file, int line, const char *func, const char *fmt, ...)
{
	(void)file; (void)line; (void)func; (void)fmt;
	if (mock_die_abort)
		mock_die_abort = 2;
	else
		abort();
}

#include "drw.c"

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

#endif /* DWM_TEST_DRW_COMMON_H */
