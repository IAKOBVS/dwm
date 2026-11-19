/*
 * Reproduction for the drw font-state contradiction: the glyph/emoji caches
 * and drw_text's static nomatches/ellipsis state used to survive a font
 * change via drw_setfontset(), reusing metrics from the old font.
 */
#include "test_drw_common.h"

static void
test_setfontset_invalidates_font_state(void)
{
	Display *dpy = XOpenDisplay(NULL);
	Drw *drw = drw_create(dpy, 0, 100, 800, 30);
	const char *fonts[] = { "mono" };
	long cp = 0x1F600;
	unsigned int idx = (unsigned int)cp & (GLYPH_CACHE_SIZE - 1);

	drw_fontset_create(drw, fonts, 1);
	glyph_cache[idx].codepoint = cp;
	glyph_cache[idx].width = 42;
	glyph_cache[idx].generation = font_generation;
	nomatches.codepoint[0] = cp;
	ellipsis_width = 21;

	drw_setfontset(drw, drw->fonts);

	ASSERT_EQ(glyph_cache[idx].codepoint, -1,
		"repro: glyph cache invalidated on font change");
	ASSERT_EQ(nomatches.codepoint[0], 0,
		"repro: missing-glyph memory cleared on font change");
	ASSERT_EQ(ellipsis_width, 0u,
		"repro: ellipsis width recomputed for the new font");

	drw_free(drw);
	XCloseDisplay(dpy);
}

static void
run_tests(void)
{
	test_setfontset_invalidates_font_state();
}
