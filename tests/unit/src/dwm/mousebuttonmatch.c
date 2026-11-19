/*
 * test_mousebuttonmatch.c — per-function tests for mousebuttonmatch.
 *
 * Covers the primary path and at least one edge case, using the
 * shared mock fixture from test_common.h.
 */
#include "test_common.h"

static void
test_mousebuttonmatch_mapped(void)
{
	cachebuttons();
	ASSERT(mousebuttonmatch(Button1, CLEANMASK(MODKEY)),
		"mousebuttonmatch: configured Button1+MODKEY matches");
}

static void
test_mousebuttonmatch_unmapped(void)
{
	cachebuttons();
	ASSERT(!mousebuttonmatch(Button4, CLEANMASK(MODKEY)),
		"mousebuttonmatch: unconfigured Button4 rejected");
	ASSERT(!mousebuttonmatch(Button1, ShiftMask),
		"mousebuttonmatch: unexpected modifier rejected");
}

static void
run_tests(void)
{
	test_mousebuttonmatch_mapped();
	test_mousebuttonmatch_unmapped();
}
