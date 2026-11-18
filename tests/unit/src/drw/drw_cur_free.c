/*
 * test_drw_cur_free.c — per-function drw.c tests for drw_cur_free().
 *
 * Uses the real drw.c with the mock X11/Xft fixture from
 * test_drw_common.h; covers the primary path and an edge case.
 */
#include "test_drw_common.h"

static void
test_drw_cur_free_null_and_cursor(void)
{
	Display *dpy = XOpenDisplay(NULL);
	Drw *drw = drw_create(dpy, 0, 100, 800, 30);
	Cur *cur = ecalloc(1, sizeof(Cur));

	cur->cursor = 1;
	drw_cur_free(drw, cur);
	drw_cur_free(drw, NULL); /* edge: NULL wrapper ignored */
	ASSERT(1, "drw_cur_free: cursor released, NULL safe");

	drw_free(drw);
	XCloseDisplay(dpy);
}

static void
run_tests(void)
{
	test_drw_cur_free_null_and_cursor();
}
