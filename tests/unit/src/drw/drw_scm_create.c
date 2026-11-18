/*
 * test_drw_scm_create.c — per-function drw.c tests for drw_scm_create().
 *
 * Uses the real drw.c with the mock X11/Xft fixture from
 * test_drw_common.h; covers the primary path and an edge case.
 */
#include "test_drw_common.h"

static void
test_drw_scm_create_scheme(void)
{
	Display *dpy = XOpenDisplay(NULL);
	Drw *drw = drw_create(dpy, 0, 100, 800, 30);
	const char *names[] = { "#000000", "#ffffff", "#ff0000" };
	Clr *scm = drw_scm_create(drw, names, 3);

	ASSERT(scm != NULL, "drw_scm_create: allocates scheme array");
	ASSERT((scm[0].pixel & 0xff000000UL) == 0xff000000UL,
		"drw_scm_create: entries are opaque");
	free(scm);

	ASSERT(drw_scm_create(NULL, names, 3) == NULL,
		"drw_scm_create: NULL drw rejected");
	ASSERT(drw_scm_create(drw, names, 1) == NULL,
		"drw_scm_create: fewer than 2 colours rejected");

	drw_free(drw);
	XCloseDisplay(dpy);
}

static void
run_tests(void)
{
	test_drw_scm_create_scheme();
}
