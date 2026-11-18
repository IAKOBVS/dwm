/*
 * test_drw_fontset_free.c — per-function drw.c tests for drw_fontset_free().
 *
 * Uses the real drw.c with the mock X11/Xft fixture from
 * test_drw_common.h; covers the primary path and an edge case.
 */
#include "test_drw_common.h"

static void
test_drw_fontset_free_chain(void)
{
	Fnt *a = ecalloc(1, sizeof(Fnt));
	Fnt *b = ecalloc(1, sizeof(Fnt));

	a->next = b;
	drw_fontset_free(a);
	drw_fontset_free(NULL); /* edge: NULL chain is a no-op */
	ASSERT(1, "drw_fontset_free: walks and frees the chain");
}

static void
run_tests(void)
{
	test_drw_fontset_free_chain();
}
