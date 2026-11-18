/*
 * test_drw_fontset_create.c — per-function drw.c tests for drw_fontset_create().
 *
 * Uses the real drw.c with the stateful mock Xft fixture from
 * test_drw_common.h.
 */
#include "test_drw_common.h"

static void
test_drw_fontset_create_chain(void)
{
	Display *dpy = XOpenDisplay(NULL);
	Drw *drw = drw_create(dpy, 0, 100, 800, 30);
	const char *fonts[] = { "mono", "emoji" };
	Fnt *head;

	mock_xft_font_fail = 0;
	head = drw_fontset_create(drw, fonts, 2);
	ASSERT(head != NULL, "drw_fontset_create: builds a chain");
	ASSERT(drw->fonts == head, "drw_fontset_create: stores chain in drw");
	ASSERT(head->next != NULL, "drw_fontset_create: keeps both fonts");
	ASSERT_EQ(head->h, 15, "drw_fontset_create: chain head has metrics");
	ASSERT_EQ(head->next->h, 15, "drw_fontset_create: fallback has metrics");

	drw_free(drw);
	XCloseDisplay(dpy);
}

static void
test_drw_fontset_create_guards(void)
{
	Display *dpy = XOpenDisplay(NULL);
	Drw *drw = drw_create(dpy, 0, 100, 800, 30);
	const char *fonts[] = { "mono" };

	ASSERT(drw_fontset_create(NULL, fonts, 1) == NULL,
		"drw_fontset_create: NULL drw rejected");
	ASSERT(drw_fontset_create(drw, NULL, 1) == NULL,
		"drw_fontset_create: NULL font list rejected");
	ASSERT(drw_fontset_create(drw, fonts, 0) == NULL,
		"drw_fontset_create: empty list yields no chain");
	ASSERT(drw->fonts == NULL,
		"drw_fontset_create: empty list clears active fonts");

	drw_free(drw);
	XCloseDisplay(dpy);
}

static void
test_drw_fontset_create_skips_failures(void)
{
	Display *dpy = XOpenDisplay(NULL);
	Drw *drw = drw_create(dpy, 0, 100, 800, 30);
	const char *fonts[] = { "mono" };

	mock_xft_font_fail = 1;
	ASSERT(drw_fontset_create(drw, fonts, 1) == NULL,
		"drw_fontset_create: all fonts failing yields NULL");
	mock_xft_font_fail = 0;

	drw_free(drw);
	XCloseDisplay(dpy);
}

static void
run_tests(void)
{
	test_drw_fontset_create_chain();
	test_drw_fontset_create_guards();
	test_drw_fontset_create_skips_failures();
}
