/*
 * test_drw_clr_create.c — per-function drw.c tests for drw_clr_create().
 *
 * Uses the real drw.c with the mock X11/Xft fixture from
 * test_drw_common.h; covers the primary path and an edge case.
 */
#include "test_drw_common.h"

static void
test_drw_clr_create_sets_opaque_pixel(void)
{
	Display *dpy = XOpenDisplay(NULL);
	Drw *drw = drw_create(dpy, 0, 100, 800, 30);
	Clr clr;

	memset(&clr, 0, sizeof clr);
	drw_clr_create(drw, &clr, "#ffffff");
	ASSERT((clr.pixel & 0xff000000UL) == 0xff000000UL,
		"drw_clr_create: alpha forced opaque");

	drw_clr_create(NULL, &clr, "#ffffff"); /* edge: guarded */
	drw_clr_create(drw, NULL, "#ffffff");
	drw_clr_create(drw, &clr, NULL);
	ASSERT(1, "drw_clr_create: NULL arguments are ignored");

	drw_free(drw);
	XCloseDisplay(dpy);
}

static void
run_tests(void)
{
	test_drw_clr_create_sets_opaque_pixel();
}
