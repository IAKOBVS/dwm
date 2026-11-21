/*
 * test_drw_resize.c — per-function drw.c tests for drw_resize().
 *
 * Migrated from test_drw_safety.c; includes the real drw.c against
 * the mock X11/Xft stubs (test_drw_common.h) so no X server is needed.
 */
#include "test_drw_common.h"

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

/* --- resizing invalidates emoji pixmaps captured at the old height --- */
static void
test_drw_resize_invalidates_emoji_cache(void)
{
	Drw drw;

	memset(&drw, 0, sizeof drw);
	mock_x11_reset();
	for (int i = 0; i < EMOJI_CACHE_SIZE; i++)
		drw.emoji_cache[i].codepoint = -1;
	drw.emoji_cache[0].codepoint = 0x1F600;
	drw.emoji_cache[0].pixmap = 123;
	drw.emoji_cache[0].w = 10;

	drw_resize(&drw, 100, 40);

	ASSERT_EQ(drw.emoji_cache[0].codepoint, -1,
		"drw_resize: emoji cache slot cleared");
	ASSERT_EQ(drw.emoji_cache[0].pixmap, 0,
		"drw_resize: cached pixmap reference cleared");
	ASSERT_EQ(mock_xfreepixmap_calls, 1,
		"drw_resize: stale cached pixmap is freed");
}

static void
run_tests(void)
{
	test_drw_resize_null();
	test_drw_resize_zero();
	test_drw_resize_invalidates_emoji_cache();
}
