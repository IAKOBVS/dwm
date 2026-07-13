/*
 * test_drw_safety.c — null/zero/edge-case safety tests for drw.c.
 *
 * Includes the real drw.c source; the mock infrastructure (mock_x11.h,
 * include/X11/Xft/Xft.h, mock_x11.c) provides all X11/Xft/Fontconfig
 * stubs needed for compilation.
 *
 * These tests cover paths that dwm.c call-sites never exercise:
 * null pointers, zero dimensions, missing fonts/schemes, etc.
 */

#include "mock_x11.h"
#include "../drw.h"

/* die() is normally in util.h + mock_x11.c, but drw.c references it
 * and we need a version that calls abort() for test failure. */
#undef die
static void
die(const char *file, int line, const char *func, const char *fmt, ...)
{
	(void)file; (void)line; (void)func; (void)fmt;
	abort();
}

#include "../drw.c"

/* ------------------------------------------------------------------ */
/* Test helpers                                                        */
/* ------------------------------------------------------------------ */
static int total = 0, failed = 0;

#define ASSERT(cond, msg) do { \
	total++; \
	if (!(cond)) { \
		failed++; \
		fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, msg); \
	} \
} while(0)

/* ------------------------------------------------------------------ */
/* Tests                                                               */
/* ------------------------------------------------------------------ */

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
}

static void
test_drw_resize_null(void)
{
	drw_resize(NULL, 100, 50);
	ASSERT(1, "drw_resize(NULL): no crash");
}

static void
test_drw_resize_zero(void)
{
	Drw drw;
	memset(&drw, 0, sizeof(drw));
	drw_resize(&drw, 0, 0);
	ASSERT(drw.w == 1, "drw_resize(0x0): w clamped to 1");
	ASSERT(drw.h == 1, "drw_resize(0x0): h clamped to 1");
}

static void
test_drw_rect_null_drw(void)
{
	drw_rect(NULL, 0, 0, 10, 10, 1, 0);
	ASSERT(1, "drw_rect(NULL): no crash");
}

static void
test_drw_rect_zero_dim(void)
{
	Display *dpy = XOpenDisplay(NULL);
	Drw *drw = drw_create(dpy, 0, 100, 800, 30);
	drw_rect(drw, 0, 0, 0, 0, 0, 0);
	ASSERT(1, "drw_rect(0x0): no crash");
	drw_free(drw);
}

static void
test_drw_rect_negative_pos(void)
{
	Display *dpy = XOpenDisplay(NULL);
	Drw *drw = drw_create(dpy, 0, 100, 800, 30);
	drw_rect(drw, -1, -1, 10, 10, 1, 0);
	ASSERT(1, "drw_rect(-1,-1): no crash");
	drw_free(drw);
}

static void
test_drw_map_null_drw(void)
{
	drw_map(NULL, 0, 0, 0, 100, 100);
	ASSERT(1, "drw_map(NULL): no crash");
}

static void
test_drw_map_zero_dim(void)
{
	Display *dpy = XOpenDisplay(NULL);
	Drw *drw = drw_create(dpy, 0, 100, 800, 30);
	drw_map(drw, 200, 0, 0, 0, 0);
	ASSERT(1, "drw_map(0x0): no crash");
	drw_free(drw);
}

static void
test_drw_text_null_drw(void)
{
	int w = drw_text(NULL, 0, 0, 100, 20, 0, "hi", 0);
	ASSERT(w == 0, "drw_text(NULL): returns 0");
}

static void
test_drw_text_null_text(void)
{
	Display *dpy = XOpenDisplay(NULL);
	Drw *drw = drw_create(dpy, 0, 100, 800, 30);
	int w = drw_text(drw, 0, 0, 100, 20, 0, NULL, 0);
	ASSERT(w == 0, "drw_text(NULL text): returns 0");
	drw_free(drw);
}

static void
test_drw_text_empty(void)
{
	Display *dpy = XOpenDisplay(NULL);
	Drw *drw = drw_create(dpy, 0, 100, 800, 30);
	int w = drw_text(drw, 0, 0, 100, 20, 0, "", 0);
	ASSERT(w == 0, "drw_text(empty): returns 0");
	drw_free(drw);
}

static void
test_drw_text_zero_width(void)
{
	Display *dpy = XOpenDisplay(NULL);
	Drw *drw = drw_create(dpy, 0, 100, 800, 30);
	int w = drw_text(drw, 0, 0, 0, 20, 0, "hello", 0);
	ASSERT(w == 0, "drw_text(w=0): returns 0");
	drw_free(drw);
}

static void
test_drw_text_null_scheme(void)
{
	Display *dpy = XOpenDisplay(NULL);
	Drw *drw = drw_create(dpy, 0, 100, 800, 30);
	drw->scheme = NULL;
	int w = drw_text(drw, 0, 0, 100, 20, 0, "hi", 0);
	ASSERT(w == 0, "drw_text(NULL scheme): returns 0");
	drw_free(drw);
}

static void
test_drw_text_null_fonts(void)
{
	Display *dpy = XOpenDisplay(NULL);
	Drw *drw = drw_create(dpy, 0, 100, 800, 30);
	drw->fonts = NULL;
	int w = drw_text(drw, 0, 0, 100, 20, 0, "hi", 0);
	ASSERT(w == 0, "drw_text(NULL fonts): returns 0");
	drw_free(drw);
}

static void
test_drw_fontset_getwidth_null(void)
{
	unsigned int w = drw_fontset_getwidth(NULL, "hello");
	ASSERT(w == 0, "drw_fontset_getwidth(NULL): returns 0");
}

static void
test_drw_fontset_getwidth_null_fonts(void)
{
	Drw drw;
	memset(&drw, 0, sizeof(drw));
	drw.fonts = NULL;
	unsigned int w = drw_fontset_getwidth(&drw, "hello");
	ASSERT(w == 0, "drw_fontset_getwidth(NULL fonts): returns 0");
}

static void
test_drw_fontset_getwidth_null_text(void)
{
	Display *dpy = XOpenDisplay(NULL);
	Drw *drw = drw_create(dpy, 0, 100, 800, 30);
	unsigned int w = drw_fontset_getwidth(drw, NULL);
	ASSERT(w == 0, "drw_fontset_getwidth(NULL text): returns 0");
	drw_free(drw);
}

static void
test_drw_fontset_getwidth_empty(void)
{
	Display *dpy = XOpenDisplay(NULL);
	Drw *drw = drw_create(dpy, 0, 100, 800, 30);
	unsigned int w = drw_fontset_getwidth(drw, "");
	ASSERT(w == 0, "drw_fontset_getwidth(empty): returns 0");
	drw_free(drw);
}

static void
test_drw_setfontset_null(void)
{
	Display *dpy = XOpenDisplay(NULL);
	Drw *drw = drw_create(dpy, 0, 100, 800, 30);
	drw_setfontset(drw, NULL);
	ASSERT(drw->fonts == NULL, "drw_setfontset(NULL): fonts cleared");
	drw_free(drw);
}

static void
test_drw_setscheme_null(void)
{
	Display *dpy = XOpenDisplay(NULL);
	Drw *drw = drw_create(dpy, 0, 100, 800, 30);
	drw_setscheme(drw, NULL);
	ASSERT(drw->scheme == NULL, "drw_setscheme(NULL): scheme cleared");
	drw_free(drw);
}

static void
test_drw_free_normal(void)
{
	Display *dpy = XOpenDisplay(NULL);
	Drw *drw = drw_create(dpy, 0, 100, 800, 30);
	drw_free(drw);
	ASSERT(1, "drw_free normal: no crash");
}

int
main(void)
{
	/* --- drw_create --- */
	test_drw_create_zero_dim();
	test_drw_create_normal();

	/* --- drw_resize --- */
	test_drw_resize_null();
	test_drw_resize_zero();

	/* --- drw_rect --- */
	test_drw_rect_null_drw();
	test_drw_rect_zero_dim();
	test_drw_rect_negative_pos();

	/* --- drw_map --- */
	test_drw_map_null_drw();
	test_drw_map_zero_dim();

	/* --- drw_text --- */
	test_drw_text_null_drw();
	test_drw_text_null_text();
	test_drw_text_empty();
	test_drw_text_zero_width();
	test_drw_text_null_scheme();
	test_drw_text_null_fonts();

	/* --- drw_fontset_getwidth --- */
	test_drw_fontset_getwidth_null();
	test_drw_fontset_getwidth_null_fonts();
	test_drw_fontset_getwidth_null_text();
	test_drw_fontset_getwidth_empty();

	/* --- drw_setfontset / drw_setscheme --- */
	test_drw_setfontset_null();
	test_drw_setscheme_null();

	/* --- drw_free --- */
	test_drw_free_normal();

	fprintf(stderr, "=== RESULTS ===\n");
	fprintf(stderr, "Total: %d | Passed: %d | Failed: %d\n",
		total, total - failed, failed);
	if (failed) {
		fprintf(stderr, "*** %d TESTS FAILED ***\n", failed);
		return 1;
	}
	fprintf(stdout, "All tests passed.\n");
	return 0;
}
