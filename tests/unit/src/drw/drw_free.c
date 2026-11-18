/*
 * test_drw_free.c — per-function drw.c tests for drw_free().
 *
 * Migrated from test_drw_safety.c; includes the real drw.c against
 * the mock X11/Xft stubs (test_drw_common.h) so no X server is needed.
 */
#include "test_drw_common.h"

static void
test_drw_free_normal(void)
{
	Display *dpy = XOpenDisplay(NULL);
	Drw *drw = drw_create(dpy, 0, 100, 800, 30);
	drw_free(drw);
	ASSERT(1, "drw_free normal: no crash");
	XCloseDisplay(dpy);
}

static void
run_tests(void)
{
	test_drw_free_normal();
}
