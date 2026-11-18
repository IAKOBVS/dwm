/*
 * test_drw_create.c — per-function drw.c tests for drw_create().
 *
 * Migrated from test_drw_safety.c; includes the real drw.c against
 * the mock X11/Xft stubs (test_drw_common.h) so no X server is needed.
 */
#include "test_drw_common.h"

static void
test_drw_create_zero_dim(void)
{
	Display *dpy = XOpenDisplay(NULL);
	Drw *drw = drw_create(dpy, 0, 100, 0, 0);
	ASSERT(drw != NULL, "drw_create(0x0) returns non-NULL");
	ASSERT(drw->w == 1, "drw_create(0x0): w clamped to 1");
	ASSERT(drw->h == 1, "drw_create(0x0): h clamped to 1");
	ASSERT(drw->drawable != 0, "drw_create(0x0): pixmap created");
	ASSERT(drw->xftd != NULL, "drw_create(0x0): xftd created");
	ASSERT(drw->emoji_cache[0].codepoint == -1, "drw_create: emoji cache init");
	drw_free(drw);
	XCloseDisplay(dpy);
}

static void
test_drw_create_normal(void)
{
	Display *dpy = XOpenDisplay(NULL);
	Drw *drw = drw_create(dpy, 1, 200, 800, 30);
	ASSERT(drw != NULL, "drw_create normal: returns non-NULL");
	ASSERT(drw->w == 800, "drw_create normal: w set");
	ASSERT(drw->h == 30, "drw_create normal: h set");
	ASSERT(drw->dpy == dpy, "drw_create normal: dpy stored");
	ASSERT(drw->screen == 1, "drw_create normal: screen stored");
	ASSERT(drw->root == 200, "drw_create normal: root stored");
	drw_free(drw);
	XCloseDisplay(dpy);
}

static void
run_tests(void)
{
	test_drw_create_zero_dim();
	test_drw_create_normal();
}
