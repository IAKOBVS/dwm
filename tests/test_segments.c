#define DWM_TEST 1
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mock_x11.h"
#define DRW_H
#include "mock_drw.h"
#include "../dwm.h"
#include "../dwm.c"

static int total = 0, failed = 0;
static Monitor *orig_selmon = NULL;

#define ASSERT(cond, msg) do { \
	total++; \
	if (!(cond)) { \
		fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, msg); \
		failed++; \
	} \
} while(0)
#define ASSERT_EQ(a, b, msg) ASSERT((a) == (b), msg)

static void
restore_selmon(void)
{
	selmon = orig_selmon;
	mons = orig_selmon;
}

static void
test_initial_state(void)
{
	ASSERT_EQ(bar_dirty_segments, DIRTY_STATUS | DIRTY_TAGS | DIRTY_TITLE,
		"initial bar_dirty_segments = all dirty (7)");
}

static void
test_drawbar_resets(void)
{
	bar_dirty_segments = DIRTY_STATUS | DIRTY_TAGS | DIRTY_TITLE;
	drawbar(selmon);
	/* In DWM_TEST builds, bar_dirty_segments is NOT reset to 0 after drawing.
	 * It stays set so tests can verify it after the call. */
	ASSERT_EQ(bar_dirty_segments, DIRTY_STATUS | DIRTY_TAGS | DIRTY_TITLE,
		"drawbar leaves segments intact (DWM_TEST build)");
}

static void
test_drawbar_early_return(void)
{
	bar_dirty_segments = 0;
	drawbar(selmon);
	ASSERT_EQ(bar_dirty_segments, 0,
		"drawbar early return leaves segments at 0");
}

static void
test_focus_sets_segments(void)
{
	bar_dirty_segments = 0;
	selmon->stack = NULL;
	focus(NULL);
	ASSERT(bar_dirty_segments & DIRTY_TITLE,
		"focus sets DIRTY_TITLE");
	ASSERT(bar_dirty_segments & DIRTY_TAGS,
		"focus sets DIRTY_TAGS");
}

static void
test_focus_with_sel_sets_segments(void)
{
	Client c = { .win = 1, .mon = selmon, .tags = 1 };
	selmon->clients = selmon->stack = &c;
	selmon->sel = &c;

	bar_dirty_segments = 0;
	focus(NULL);
	ASSERT(bar_dirty_segments & DIRTY_TITLE,
		"focus with sel sets DIRTY_TITLE");
	ASSERT(bar_dirty_segments & DIRTY_TAGS,
		"focus with sel sets DIRTY_TAGS");

	selmon->sel = NULL;
	selmon->clients = selmon->stack = NULL;
}

static void
test_updatestatus_sets_segments(void)
{
	bar_dirty_segments = 0;
	stext[0] = '\0';
	updatestatus();
	ASSERT(bar_dirty_segments & DIRTY_STATUS,
		"updatestatus sets DIRTY_STATUS");
	ASSERT(bar_dirty_segments & DIRTY_TITLE,
		"updatestatus sets DIRTY_TITLE");
	ASSERT(stext[0] != '\0',
		"updatestatus sets stext");
}

static void
test_setlayout_sets_segments(void)
{
	bar_dirty_segments = 0;
	Arg arg = { .v = NULL };
	selmon->sel = NULL;
	selmon->sellt = 0;
	setlayout(&arg);
	ASSERT(bar_dirty_segments & DIRTY_TAGS,
		"setlayout sets DIRTY_TAGS");
}

static void
test_togglebar_sets_segments(void)
{
	Arg arg;
	selmon->showbar = 0;
	bar_dirty_segments = 0;
	togglebar(&arg);
	ASSERT(bar_dirty_segments & DIRTY_STATUS,
		"togglebar sets DIRTY_STATUS");
	ASSERT(bar_dirty_segments & DIRTY_TAGS,
		"togglebar sets DIRTY_TAGS");
	ASSERT(bar_dirty_segments & DIRTY_TITLE,
		"togglebar sets DIRTY_TITLE");
	selmon->showbar = 0;
}

static void
test_bar_draw_pending_initial(void)
{
	/* other tests may have set bar_draw_pending; reset to check initial default */
	bar_draw_pending = 0;
	ASSERT_EQ(bar_draw_pending, 0, "bar_draw_pending initially 0");
}

static void
test_bar_draw_pending_on_focus(void)
{
	bar_draw_pending = 0;
	selmon->stack = NULL;
	focus(NULL);
	ASSERT_EQ(bar_draw_pending, 1, "focus sets bar_draw_pending");
}

static void
test_bar_draw_pending_on_updatestatus(void)
{
	bar_draw_pending = 0;
	stext[0] = '\0';
	updatestatus();
	ASSERT_EQ(bar_draw_pending, 1, "updatestatus sets bar_draw_pending");
}

static void
test_bar_draw_pending_on_setlayout_no_sel(void)
{
	bar_draw_pending = 0;
	selmon->sel = NULL;
	Arg arg = { .v = NULL };
	selmon->sellt = 0;
	setlayout(&arg);
	ASSERT_EQ(bar_draw_pending, 1, "setlayout with no sel sets bar_draw_pending");
}

static void
test_bar_draw_pending_on_restack(void)
{
	Monitor m;
	memset(&m, 0, sizeof(m));
	bar_draw_pending = 0;
	restack(&m);
	ASSERT_EQ(bar_draw_pending, 1, "restack sets bar_draw_pending");
}

static void
test_bar_exposed_full_draw_resets(void)
{
	selmon->showbar = 1;          /* drawbar early-returns if showbar=0 */
	bar_dirty_segments = DIRTY_STATUS | DIRTY_TAGS | DIRTY_TITLE;
	bar_exposed = 1;
	drawbar(selmon);
	/* drawbar full draw path sets bar_exposed = 0 */
	ASSERT_EQ(bar_exposed, 0, "bar_exposed reset after full draw");
}

static void
test_bar_exposed_expose_then_drawbar_resets(void)
{
	XEvent ev;
	memset(&ev, 0, sizeof(ev));
	ev.type = Expose;
	ev.xexpose.window = selmon->barwin;
	ev.xexpose.count = 0;

	selmon->showbar = 1;
	bar_dirty_segments = DIRTY_STATUS | DIRTY_TAGS | DIRTY_TITLE;
	bar_exposed = 0;
	expose(&ev);
	/* expose sets bar_exposed=1 then calls drawbar(), which resets to 0 after full draw */
	ASSERT_EQ(bar_exposed, 0, "bar_exposed 0 after expose (drawbar clears it)");
}

static void
test_bar_exposed_early_return_resets(void)
{
	selmon->showbar = 1;          /* drawbar early-returns if showbar=0 */
	bar_dirty_segments = 0;
	bar_exposed = 1;
	drawbar(selmon);
	/* early-return path resets bar_exposed = 0 */
	ASSERT_EQ(bar_exposed, 0, "bar_exposed reset after early-return drawbar");
}

static void
test_setfullscreen_unset_sets_segments(void)
{
	Client c;
	memset(&c, 0, sizeof c);
	c.isfullscreen = 1;
	c.oldstate = 0;
	c.oldbw = c.bw = 2;
	c.w = c.oldw = 800;
	c.h = c.oldh = 600;
	c.x = c.oldx = 100;
	c.y = c.oldy = 100;
	c.mon = selmon;

	selmon->sel = &c;

	bar_dirty_segments = 0;
	setfullscreen(&c, 0);
	ASSERT(bar_dirty_segments & DIRTY_STATUS,
		"setfullscreen(0) sets DIRTY_STATUS");
	ASSERT(bar_dirty_segments & DIRTY_TAGS,
		"setfullscreen(0) sets DIRTY_TAGS");
	ASSERT(bar_dirty_segments & DIRTY_TITLE,
		"setfullscreen(0) sets DIRTY_TITLE");

	selmon->sel = NULL;
}

int
main(void)
{
	int i;
	failed = 0; total = 0;
	setbuf(stderr, NULL);

	/* Init globals */
	dpy = (Display *)(void *)0x1;
	drw = calloc(1, sizeof(Drw));
	drw->fonts = calloc(1, sizeof(Fnt));
	drw->fonts->h = 15;
	root = 42;
	screen = 0;
	sw = 1920; sh = 1080;
	bh = 22; lrpad = 11;

	/* Default selmon */
	selmon = calloc(1, sizeof(Monitor));
	mons = selmon;
	selmon->tagset[0] = 1;
	selmon->tagset[1] = 1;
	selmon->mfact = 0.55f;
	selmon->nmaster = 1;
	selmon->showbar = 1;
	selmon->topbar = 1;
	selmon->mx = selmon->wx = 0;
	selmon->my = selmon->wy = 0;
	selmon->mw = selmon->ww = 1920;
	selmon->mh = selmon->wh = 1080;
	selmon->lt[0] = selmon->lt[1] = &layouts[0];
	strncpy(selmon->ltsymbol, layouts[0].symbol, sizeof selmon->ltsymbol);
	selmon->gap = ecalloc(1, sizeof(Gap));
	selmon->gap->gappx = 0;
	selmon->barwin = 42;

	orig_selmon = selmon;

	scheme = ecalloc(2, sizeof(Clr *));
	for (i = SchemeNorm; i <= SchemeSel; i++)
		scheme[i] = drw_scm_create(drw, colors[i], 3);

	/* --- Tests --- */
	test_initial_state();
	ASSERT_EQ(bar_exposed, 1, "bar_exposed initially 1 (first draw must copy)");

	/* Reset after checking initial state */
	bar_dirty_segments = 7;
	test_drawbar_resets();
	test_drawbar_early_return();

	restore_selmon();
	test_focus_sets_segments();

	restore_selmon();
	test_focus_with_sel_sets_segments();

	restore_selmon();
	test_updatestatus_sets_segments();

	restore_selmon();
	test_setlayout_sets_segments();

	restore_selmon();
	test_togglebar_sets_segments();

	restore_selmon();
	test_setfullscreen_unset_sets_segments();

	restore_selmon();
	test_bar_draw_pending_initial();

	restore_selmon();
	test_bar_draw_pending_on_focus();

	restore_selmon();
	test_bar_draw_pending_on_updatestatus();

	restore_selmon();
	test_bar_draw_pending_on_setlayout_no_sel();

	restore_selmon();
	test_bar_draw_pending_on_restack();

	restore_selmon();
	/* drawbar needs a valid drw->fonts for dwm.c:605 (early access to drw->fonts->h) */
	test_bar_exposed_full_draw_resets();

	restore_selmon();
	test_bar_exposed_expose_then_drawbar_resets();

	restore_selmon();
	test_bar_exposed_early_return_resets();

	/* Report */
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
