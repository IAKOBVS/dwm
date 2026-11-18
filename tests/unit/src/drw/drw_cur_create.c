/*
 * test_drw_cur_create.c — per-function drw.c tests for drw_cur_create().
 *
 * Uses the real drw.c with the mock X11/Xft fixture from
 * test_drw_common.h; covers the primary path and an edge case.
 */
#include "test_drw_common.h"

static void
test_drw_cur_create_and_null(void)
{
	Display *dpy = XOpenDisplay(NULL);
	Drw *drw = drw_create(dpy, 0, 100, 800, 30);
	Cur *cur = drw_cur_create(drw, 1);

	ASSERT(cur != NULL, "drw_cur_create: allocates cursor wrapper");
	ASSERT(cur->cursor == 1, "drw_cur_create: stores X cursor id");
	drw_cur_free(drw, cur);

	ASSERT(drw_cur_create(NULL, 1) == NULL,
		"drw_cur_create: NULL drw returns NULL");

	drw_free(drw);
	XCloseDisplay(dpy);
}

static void
run_tests(void)
{
	test_drw_cur_create_and_null();
}
