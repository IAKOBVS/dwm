/*
 * test_buttonpress.c — per-function tests for dwm.c's buttonpress().
 *
 * Each case drives one click classification path and asserts the observable
 * effect: a tag-bar click changes the view, a client click focuses the client,
 * a mask that cannot match any binding is ignored, and clicks outside the tag
 * strip do not dispatch a tag binding.  The fixture lives in test_common.h.
 */
#include "test_common.h"

/* First tag label width: mock drw_fontset_getwidth returns 10 px per byte. */
#define TAG_W (10 + lrpad)

static unsigned int
tag_mask_at(unsigned int index)
{
	return 1u << index;
}

/* --- buttonpress --- */
static void
test_buttonpress_focus_client(void)
{
	reset_index();
	Monitor *m = make_monitor(0);
	Client *c = make_client(100, m);
	m->clients = c;
	m->sel = c;
	save_selmon();
	selmon = m;
	mons = m;
	mock_x11_reset();

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xbutton.window = 100;
	ev.xbutton.state = 0;
	ev.xbutton.button = Button1;

	cachebuttons();
	buttonpress(&ev);

	ASSERT(selmon->sel == c, "buttonpress: client click selects the client");
	ASSERT_EQ(mock_xset_input_focus.count, 1,
		"buttonpress: client click takes X input focus");
	ASSERT_EQ(mock_xset_input_focus.win, c->win,
		"buttonpress: input focus targets the clicked client");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

static void
test_buttonpress_barwin(void)
{
	reset_index();
	Monitor *m = make_monitor(0);
	m->barwin = 999;
	m->sel = NULL;
	save_selmon();
	selmon = m;
	mons = m;
	mock_x11_reset();

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xbutton.window = 999;
	ev.xbutton.state = 0;
	ev.xbutton.button = Button1;
	ev.xbutton.x = 0;
	ev.xbutton.y = 0;

	cachebuttons();
	cachekeys();
	buttonpress(&ev);

	/* ClkTagBar requires MODKEY, so mask 0 matches no configured binding. */
	ASSERT_EQ(m->tagset[m->seltags], 1,
		"buttonpress: unmodified tag click leaves the view unchanged");
	ASSERT(m->sel == NULL, "buttonpress: bar click does not select a client");
	ASSERT_EQ(mock_xset_input_focus.count, 0,
		"buttonpress: bar click without a binding takes no focus");

	restore_selmon();
	free(m);
}

static void
test_buttonpress_unmapped_button(void)
{
	reset_index();
	Monitor *m = make_monitor(0);
	Client *c = make_client(100, m);
	m->clients = c;
	m->sel = NULL;
	save_selmon();
	selmon = m;
	mons = m;
	mock_x11_reset();

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xbutton.window = 100;
	ev.xbutton.state = Mod1Mask;
	ev.xbutton.button = Button5;

	cachebuttons();
	buttonpress(&ev);

	ASSERT(selmon->sel == c,
		"buttonpress: unbound button still focuses the clicked client");
	ASSERT_EQ(mock_xset_input_focus.win, c->win,
		"buttonpress: unbound button focuses the client window");
	ASSERT_EQ(m->tagset[m->seltags], 1,
		"buttonpress: unbound button does not change the view");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

static void
test_buttonpress_mismatched_mask(void)
{
	reset_index();
	Monitor *m = make_monitor(0);
	m->barwin = 999;
	m->sel = NULL;
	m->ww = 1920;
	strcpy(stext, "");
	save_selmon();
	selmon = m;
	mons = m;
	mock_x11_reset();
	mock_drw_reset();

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xbutton.window = 999;
	ev.xbutton.state = ControlMask; /* no binding uses Control on the bar */
	ev.xbutton.button = Button1;
	ev.xbutton.x = TAG_W + 1; /* second tag */

	cachebuttons();
	buttonpress(&ev);

	ASSERT_EQ(m->tagset[m->seltags], 1,
		"buttonpress: mismatched mask does not change the view");
	ASSERT_EQ(mock_drw_text_calls, 0,
		"buttonpress: mismatched mask returns before calculating tag widths");

	restore_selmon();
	free(m);
}

/* --- buttonpress click types --- */
static void
test_buttonpress_click_layoutsymbol(void)
{
	reset_index();
	Monitor *m = make_monitor(0);
	m->sel = NULL;
	m->barwin = 999;
	m->ww = 1920;
	strcpy(stext, "");
	save_selmon();
	selmon = m;
	mons = m;
	mock_x11_reset();

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xbutton.window = 999;
	ev.xbutton.state = 0;
	ev.xbutton.button = Button1;
	ev.xbutton.x = 220; /* after all tag labels, inside the layout symbol */

	cachebuttons();
	buttonpress(&ev);

	ASSERT_EQ(m->sellt, 0,
		"buttonpress: layout-symbol click without a binding keeps the layout slot");
	ASSERT_EQ(m->tagset[m->seltags], 1,
		"buttonpress: layout-symbol click does not change the view");
	ASSERT_EQ(mock_xset_input_focus.count, 0,
		"buttonpress: bar click does not touch input focus");

	restore_selmon();
	free(m);
}

static void
test_buttonpress_click_statustext(void)
{
	reset_index();
	Monitor *m = make_monitor(0);
	m->sel = NULL;
	m->barwin = 999;
	m->ww = 1920;
	strcpy(stext, "test status");
	save_selmon();
	selmon = m;
	mons = m;
	mock_x11_reset();

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xbutton.window = 999;
	ev.xbutton.state = 0;
	ev.xbutton.button = Button1;
	ev.xbutton.x = 1900; /* in status text area */

	cachebuttons();
	buttonpress(&ev);

	ASSERT_EQ(m->tagset[m->seltags], 1,
		"buttonpress: status click does not change the view");
	ASSERT(m->sel == NULL, "buttonpress: status click selects no client");

	restore_selmon();
	free(m);
}

static void
test_buttonpress_click_wintitle(void)
{
	reset_index();
	Monitor *m = make_monitor(0);
	m->sel = NULL;
	m->barwin = 999;
	m->ww = 1920;
	strcpy(stext, "test status");
	save_selmon();
	selmon = m;
	mons = m;
	mock_x11_reset();

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xbutton.window = 999;
	ev.xbutton.state = 0;
	ev.xbutton.button = Button1;
	ev.xbutton.x = 500; /* in title area */

	cachebuttons();
	buttonpress(&ev);

	ASSERT_EQ(m->tagset[m->seltags], 1,
		"buttonpress: title click does not change the view");
	ASSERT(m->sel == NULL, "buttonpress: title click selects no client");

	restore_selmon();
	free(m);
}

/* --- buttonpress tag iteration --- */
static void
test_buttonpress_tag_iteration(void)
{
	reset_index();
	Monitor *m = make_monitor(0);
	m->barwin = 999;
	m->sel = NULL;
	m->ww = 1920;
	strcpy(stext, "");
	save_selmon();
	selmon = m;
	mons = m;
	mock_x11_reset();

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xbutton.window = 999;
	ev.xbutton.state = MODKEY;
	ev.xbutton.button = Button1;
	ev.xbutton.x = TAG_W + 1; /* second tag */

	cachebuttons();
	buttonpress(&ev);

	ASSERT_EQ(m->tagset[m->seltags], tag_mask_at(1),
		"buttonpress: tag click views the clicked tag");
	ASSERT(m->bar_dirty_segments & DIRTY_TAGS,
		"buttonpress: tag click dirties the tag bar");

	restore_selmon();
	free(m);
}

/* --- buttonpress ClkLtSymbol --- */
static void
test_buttonpress_click_layoutsymbol_only(void)
{
	reset_index();
	Monitor *m = make_monitor(0);
	m->barwin = 999;
	m->sel = NULL;
	m->ww = 1920;
	strcpy(stext, "");
	save_selmon();
	selmon = m;
	mons = m;
	mock_x11_reset();

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xbutton.window = 999;
	ev.xbutton.state = 0;
	ev.xbutton.button = Button1;
	ev.xbutton.x = 220;

	cachebuttons();
	buttonpress(&ev);

	ASSERT_EQ(m->sellt, 0, "buttonpress: layout symbol click keeps sellt");
	ASSERT(m->lt[0] == &layouts[0], "buttonpress: layout symbol click keeps layout 0");
	ASSERT_EQ(m->tagset[m->seltags], 1,
		"buttonpress: layout symbol click keeps the view");

	restore_selmon();
	free(m);
}

/* --- buttonpress ClkLtSymbol (line 307) --- */
static void
test_buttonpress_clicks_ltsymbol(void)
{
	reset_index();
	Monitor *m = make_monitor(0);
	m->sel = NULL;
	m->barwin = 999;
	m->ww = 1920;
	strcpy(stext, "");
	save_selmon();
	selmon = m;
	mons = m;
	mock_x11_reset();

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xbutton.window = 999;
	ev.xbutton.state = 0;
	ev.xbutton.button = Button1;
	/* After 9 tags (each 21px wide = 189), layout symbol area [189, 230) */
	ev.xbutton.x = 220;

	cachebuttons();
	buttonpress(&ev);

	ASSERT_EQ(m->sellt, 0, "buttonpress: ClkLtSymbol keeps the layout slot");
	ASSERT_EQ(m->tagset[m->seltags], 1,
		"buttonpress: ClkLtSymbol does not change the view");

	restore_selmon();
	free(m);
}

/* --- buttonpress dispatch ClkTagBar (lines 318-321) --- */
static void
test_buttonpress_dispatch(void)
{
	reset_index();
	Monitor *m = make_monitor(0);
	m->sel = NULL;
	m->barwin = 999;
	m->ww = 1920;
	strcpy(stext, "");
	save_selmon();
	selmon = m;
	mons = m;
	mock_x11_reset();

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xbutton.window = 999;
	ev.xbutton.state = MODKEY;
	ev.xbutton.button = Button1;
	ev.xbutton.x = 2 * TAG_W + 1; /* third tag */

	cachebuttons();
	buttonpress(&ev);

	ASSERT_EQ(m->tagset[m->seltags], tag_mask_at(2),
		"buttonpress: tag binding dispatches the computed tag mask");

	restore_selmon();
	free(m);
}

static void
test_buttonpress_past_last_tag(void)
{
	reset_index();
	Monitor *m = make_monitor(0);
	m->barwin = 999;
	m->sel = NULL;
	m->ww = 1920;
	strcpy(stext, "");
	save_selmon();
	selmon = m;
	mons = m;
	mock_x11_reset();

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xbutton.window = 999;
	ev.xbutton.state = MODKEY;
	ev.xbutton.button = Button1;
	/* x beyond all tags and the layout symbol; falls through to a bar area
	 * without a configured binding, so no tag dispatch occurs */
	ev.xbutton.x = 9999;

	cachebuttons();
	buttonpress(&ev);

	ASSERT_EQ(m->tagset[m->seltags], 1,
		"buttonpress: out-of-range x does not dispatch a tag binding");
	ASSERT_EQ(m->sellt, 0,
		"buttonpress: out-of-range x does not change the layout");

	restore_selmon();
	free(m);
}

static void
test_buttonpress_clkmastertag(void)
{
	reset_index();
	Monitor *m = make_monitor(0);
	m->barwin = 999;
	m->sel = NULL;
	m->ww = 1920;
	strcpy(stext, "");
	save_selmon();
	selmon = m;
	mons = m;
	mock_x11_reset();

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xbutton.window = 999;
	ev.xbutton.state = MODKEY;
	ev.xbutton.button = Button1;
	ev.xbutton.x = 4 * TAG_W + 1; /* fifth tag */
	ev.xbutton.y = 0;

	cachebuttons();
	buttonpress(&ev);

	/* The view binding stores arg.i == 0, so buttonpress supplies the
	 * computed tag mask instead of the binding's own argument. */
	ASSERT_EQ(m->tagset[m->seltags], tag_mask_at(4),
		"buttonpress: ClkTagBar with arg.i==0 uses the computed tag mask");

	restore_selmon();
	free(m);
}

static void
test_buttonpress_clkrootwin(void)
{
	reset_index();
	Monitor *m = make_monitor(0);
	m->sel = NULL;
	m->barwin = 999;
	m->ww = 1920;
	strcpy(stext, "");
	save_selmon();
	selmon = m;
	mons = m;
	mock_x11_reset();

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xbutton.window = root;
	ev.xbutton.state = 0;
	ev.xbutton.button = Button1;

	cachebuttons();
	buttonpress(&ev);

	ASSERT_EQ(m->tagset[m->seltags], 1,
		"buttonpress: root click does not change the view");
	ASSERT(m->sel == NULL, "buttonpress: root click selects no client");
	ASSERT_EQ(mock_xset_input_focus.count, 0,
		"buttonpress: root click does not touch input focus");

	restore_selmon();
	free(m);
}

static void
run_tests(void)
{
	test_buttonpress_focus_client();
	test_buttonpress_barwin();
	test_buttonpress_unmapped_button();
	test_buttonpress_mismatched_mask();
	test_buttonpress_click_layoutsymbol();
	test_buttonpress_click_statustext();
	test_buttonpress_click_wintitle();
	test_buttonpress_tag_iteration();
	test_buttonpress_click_layoutsymbol_only();
	test_buttonpress_clicks_ltsymbol();
	test_buttonpress_dispatch();
	test_buttonpress_past_last_tag();
	test_buttonpress_clkmastertag();
	test_buttonpress_clkrootwin();
}
