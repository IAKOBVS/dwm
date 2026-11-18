/*
 * test_xfont_free.c — per-function drw.c tests for xfont_free().
 *
 * Uses the real drw.c with the mock X11/Xft fixture from
 * test_drw_common.h; covers the primary path and an edge case.
 */
#include "test_drw_common.h"

static void
test_xfont_free_null_and_node(void)
{
	Fnt *font = ecalloc(1, sizeof(Fnt));

	xfont_free(NULL); /* edge: NULL is a no-op */
	xfont_free(font); /* pattern/xfont NULL are safe in the mock */
	ASSERT(1, "xfont_free: NULL and allocated node do not crash");
}

static void
run_tests(void)
{
	test_xfont_free_null_and_node();
}
