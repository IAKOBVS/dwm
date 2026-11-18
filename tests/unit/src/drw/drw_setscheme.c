/*
 * test_drw_setscheme.c — per-function drw.c tests for drw_setscheme().
 *
 * Migrated from test_drw_safety.c; includes the real drw.c against
 * the mock X11/Xft stubs (test_drw_common.h) so no X server is needed.
 */
#include "test_drw_common.h"

static void
test_drw_setscheme_null(void)
{
	Display *dpy = XOpenDisplay(NULL);
	Drw *drw = drw_create(dpy, 0, 100, 800, 30);
	drw_setscheme(drw, NULL);
	ASSERT(drw->scheme == NULL, "drw_setscheme(NULL): scheme cleared");
	drw_free(drw);
	XCloseDisplay(dpy);
}

static void
run_tests(void)
{
	test_drw_setscheme_null();
}
