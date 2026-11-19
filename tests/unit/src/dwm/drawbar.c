/*
 * test_drawbar.c — per-function tests for dwm.c's drawbar().
 *
 * Each case exercises one path through drawbar() and asserts what the
 * function asked the drawing layer (mock_drw) and the X server (mock_x11)
 * to do.  The fixture (global setup, make_monitor/make_client, window-index
 * reset) lives in test_common.h.
 */
#include "test_common.h"

static const int expected_tag_texts = (int)LENGTH(tags);

/* --- hidden bar draws nothing --- */
static void
test_drawbar_fullscreen_freeze(void)
{
	reset_index();
	mock_x11_reset();
	mock_drw_reset();
	Monitor *m = make_monitor(0);
	m->showbar = 1;
	m->barwin = 888;
	m->sel = NULL;
	strcpy(stext, "");
	save_selmon();
	selmon = m;
	mons = m;

	/* all segments dirty: a full render is required */
	selmon->bar_dirty_segments = DIRTY_STATUS | DIRTY_TAGS | DIRTY_TITLE;
	drawbar(m);

	ASSERT_EQ(mock_drw_text_calls, expected_tag_texts + 1,
		"drawbar: status + one text per tag drawn");
	ASSERT_EQ(mock_drw_rect_calls, 1,
		"drawbar: empty title area drawn once as a filled rect");
	ASSERT_EQ(mock_drw_map_calls, 1, "drawbar: full draw maps the pixmap once");
	ASSERT_EQ(mock_drw_last_map_win, (Window)888, "drawbar: maps to m->barwin");
	ASSERT_EQ(mock_drw_last_map_w, m->ww, "drawbar: map spans the monitor width");
	ASSERT_EQ(mock_drw_last_map_h, bh, "drawbar: map spans the bar height");

	restore_selmon();
	free(m);
}

/* --- clean segments still blit when the window was exposed --- */
static void
test_drawbar_clean_bar(void)
{
	reset_index();
	mock_x11_reset();
	mock_drw_reset();
	Monitor *m = make_monitor(0);
	m->showbar = 1;
	m->barwin = 888;
	m->sel = NULL;
	save_selmon();
	selmon = m;
	mons = m;

	selmon->bar_dirty_segments = 0;
	selmon->bar_exposed = 1;
	drawbar(m);

	ASSERT_EQ(mock_drw_text_calls, 0, "drawbar: clean bar re-renders no text");
	ASSERT_EQ(mock_drw_rect_calls, 0, "drawbar: clean bar re-renders no rects");
	ASSERT_EQ(mock_drw_map_calls, 1, "drawbar: clean expose blits the retained pixmap");
	ASSERT_EQ(mock_drw_last_map_win, (Window)888, "drawbar: blit targets m->barwin");
	ASSERT_EQ(selmon->bar_exposed, 0, "drawbar: clears bar_exposed after blitting");

	/* Already clean and not exposed: no pixmap copy either. */
	mock_drw_reset();
	drawbar(m);
	ASSERT_EQ(mock_drw_map_calls, 0,
		"drawbar: clean, unexposed bar performs no X11 copy");
	ASSERT_EQ(mock_drw_text_calls, 0, "drawbar: clean, unexposed bar draws no text");

	restore_selmon();
	free(m);
}

/* --- hidden bar is a complete no-op --- */
static void
test_drawbar_hidden_bar(void)
{
	reset_index();
	mock_x11_reset();
	mock_drw_reset();
	Monitor *m = make_monitor(0);
	m->showbar = 0;
	m->barwin = 888;
	m->sel = NULL;
	save_selmon();
	selmon = m;
	mons = m;

	selmon->bar_dirty_segments = DIRTY_STATUS | DIRTY_TAGS | DIRTY_TITLE;
	drawbar(m);

	ASSERT_EQ(mock_drw_text_calls, 0, "drawbar: hidden bar draws no text");
	ASSERT_EQ(mock_drw_rect_calls, 0, "drawbar: hidden bar draws no rects");
	ASSERT_EQ(mock_drw_map_calls, 0, "drawbar: hidden bar copies no pixmap");
	ASSERT_EQ(selmon->bar_dirty_segments,
		DIRTY_STATUS | DIRTY_TAGS | DIRTY_TITLE,
		"drawbar: hidden bar leaves dirty segments intact");

	restore_selmon();
	free(m);
}

/* --- status-only redraw avoids the tags and title --- */
static void
test_drawbar_segments_status_only(void)
{
	reset_index();
	mock_x11_reset();
	mock_drw_reset();
	Monitor *m = make_monitor(0);
	m->showbar = 1;
	m->barwin = 888;
	m->sel = NULL;
	m->clients = NULL;
	strcpy(stext, "test");
	save_selmon();
	selmon = m;
	mons = m;

	selmon->bar_dirty_segments = DIRTY_STATUS;
	drawbar(m);

	ASSERT_EQ(mock_drw_text_calls, 1, "drawbar: status-only draws exactly one text");
	ASSERT_EQ(mock_drw_last_text_invert, 0, "drawbar: status uses the normal scheme");
	ASSERT(strcmp(mock_drw_last_text, "test") == 0,
		"drawbar: status text payload is stext");
	ASSERT_EQ(mock_drw_rect_calls, 0, "drawbar: status-only draws no rects");
	ASSERT_EQ(mock_drw_map_calls, 1, "drawbar: status-only still maps once");

	restore_selmon();
	free(m);
}

/* --- tags-only redraw avoids the status and title --- */
static void
test_drawbar_segments_tags_only(void)
{
	reset_index();
	mock_x11_reset();
	mock_drw_reset();
	Monitor *m = make_monitor(0);
	m->showbar = 1;
	m->barwin = 888;
	m->sel = NULL;
	m->clients = NULL;
	strcpy(stext, "");
	save_selmon();
	selmon = m;
	mons = m;

	selmon->bar_dirty_segments = DIRTY_TAGS;
	drawbar(m);

	ASSERT_EQ(mock_drw_text_calls, expected_tag_texts,
		"drawbar: tags-only draws one text per tag and no status");
	ASSERT_EQ(mock_drw_rect_calls, 0,
		"drawbar: tags-only without occupancy draws no tag boxes");
	ASSERT_EQ(mock_drw_map_calls, 1, "drawbar: tags-only still maps once");

	restore_selmon();
	free(m);
}

/* --- title redraw with a selected floating client --- */
static void
test_drawbar_segments_title_with_sel(void)
{
	reset_index();
	mock_x11_reset();
	mock_drw_reset();
	Monitor *m = make_monitor(0);
	m->showbar = 1;
	m->barwin = 888;
	Client *c = make_client(1, m);
	c->tags = 1;
	strncpy(c->name, "test-title", sizeof c->name);
	c->isfloating = 1;
	c->isfixed = 0;
	m->sel = c;
	m->clients = c;
	strcpy(stext, "");
	save_selmon();
	selmon = m;
	mons = m;

	selmon->bar_dirty_segments = DIRTY_TITLE;
	drawbar(m);

	ASSERT_EQ(mock_drw_text_calls, 1, "drawbar: title-only draws the client name");
	ASSERT(strcmp(mock_drw_last_text, "test-title") == 0,
		"drawbar: title text payload is the selected client name");
	ASSERT_EQ(mock_drw_rect_calls, 1,
		"drawbar: floating title draws its floating indicator rect");
	ASSERT_EQ(mock_drw_last_rect_filled, 0,
		"drawbar: non-fixed floating client uses an unfilled indicator");
	ASSERT_EQ(mock_drw_map_calls, 1, "drawbar: title-only still maps once");

	restore_selmon();
	winclient_remove(c);
	free(c);
	free(m);
}

/* --- title redraw with no selection --- */
static void
test_drawbar_segments_title_no_sel(void)
{
	reset_index();
	mock_x11_reset();
	mock_drw_reset();
	Monitor *m = make_monitor(0);
	m->showbar = 1;
	m->barwin = 888;
	m->sel = NULL;
	m->clients = NULL;
	strcpy(stext, "");
	save_selmon();
	selmon = m;
	mons = m;

	selmon->bar_dirty_segments = DIRTY_TITLE;
	drawbar(m);

	ASSERT_EQ(mock_drw_text_calls, 0, "drawbar: empty title draws no text");
	ASSERT_EQ(mock_drw_rect_calls, 1, "drawbar: empty title draws one filled rect");
	ASSERT_EQ(mock_drw_last_rect_filled, 1, "drawbar: empty title rect is filled");
	ASSERT_EQ(mock_drw_last_rect_invert, 1, "drawbar: empty title rect is inverted");
	ASSERT_EQ(mock_drw_map_calls, 1, "drawbar: empty title still maps once");

	restore_selmon();
	free(m);
}

/* --- fullscreen freeze --- */
static void
test_drawbar_fullscreen_client(void)
{
	reset_index();
	mock_x11_reset();
	mock_drw_reset();
	Monitor *m = make_monitor(0);
	m->showbar = 1;
	m->barwin = 888;
	Client *c = make_client(1, m);
	c->isfullscreen = 1;
	m->sel = c;
	strcpy(stext, "");
	save_selmon();
	selmon = m;
	mons = m;

	selmon->bar_dirty_segments = DIRTY_STATUS | DIRTY_TAGS | DIRTY_TITLE;
	drawbar(m);

	ASSERT_EQ(mock_drw_text_calls, 0,
		"drawbar: fullscreen freeze renders no text");
	ASSERT_EQ(mock_drw_rect_calls, 0,
		"drawbar: fullscreen freeze renders no rects");
	ASSERT_EQ(mock_drw_map_calls, 0,
		"drawbar: fullscreen freeze performs no pixmap copy");
	ASSERT_EQ(selmon->bar_dirty_segments,
		DIRTY_STATUS | DIRTY_TAGS | DIRTY_TITLE,
		"drawbar: fullscreen freeze retains dirty segments for unfreeze");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- urgent client gets an inverted tag indicator --- */
static void
test_drawbar_urgent_client(void)
{
	reset_index();
	mock_x11_reset();
	mock_drw_reset();
	Monitor *m = make_monitor(0);
	m->showbar = 1;
	m->barwin = 888;
	m->sel = NULL;
	Client *c1 = make_client(1, m);
	c1->tags = 2;
	c1->isurgent = 1;
	m->clients = c1;
	strcpy(stext, "");
	save_selmon();
	selmon = m;
	mons = m;

	selmon->bar_dirty_segments = DIRTY_TAGS;
	drawbar(m);

	ASSERT_EQ(mock_drw_text_calls, expected_tag_texts,
		"drawbar: urgent tags draw one text per tag");
	ASSERT_EQ(mock_drw_rect_calls, 1,
		"drawbar: occupied urgent tag draws one indicator box");
	ASSERT_EQ(mock_drw_last_rect_invert, 2,
		"drawbar: urgent tag box is inverted with the urgency mask");
	ASSERT_EQ(mock_drw_last_rect_filled, 0,
		"drawbar: urgent tag box is unfilled without a selected client");

	restore_selmon();
	winclient_remove(c1);
	free(c1); free(m);
}

/* --- tags loop with occupancy, urgency and a selected client --- */
static void
test_drawbar_tags_loop(void)
{
	reset_index();
	mock_x11_reset();
	mock_drw_reset();
	Monitor *m = make_monitor(0);
	m->showbar = 1;
	m->barwin = 888;
	m->tagset[0] = 1;
	m->seltags = 0;

	/* c1 occupies tag 1, urgent */
	Client *c1 = make_client(20, m);
	c1->tags = 1;
	c1->isurgent = 1;
	strncpy(c1->name, "urgent-title", sizeof c1->name);

	/* c2 occupies tag 2, not urgent */
	Client *c2 = make_client(21, m);
	c2->tags = 2;
	c2->isurgent = 0;
	strncpy(c2->name, "normal-title", sizeof c2->name);

	/* c3 selected client, floating, occupies tag 1 */
	Client *c3 = make_client(22, m);
	c3->tags = 1;
	c3->isfloating = 1;
	c3->isfixed = 0;
	strncpy(c3->name, "sel-title", sizeof c3->name);

	c1->next = c2;
	c2->next = c3;
	c3->next = NULL;
	m->clients = c1;
	m->stack = c3;
	m->sel = c3;
	strcpy(stext, "");

	save_selmon();
	selmon = m;
	mons = m;

	selmon->bar_dirty_segments = DIRTY_TAGS | DIRTY_TITLE;
	drawbar(m);

	/* 9 tag texts + 1 title text */
	ASSERT_EQ(mock_drw_text_calls, expected_tag_texts + 1,
		"drawbar: tags + title draw one text per element");
	ASSERT(strcmp(mock_drw_last_text, "sel-title") == 0,
		"drawbar: title text is the selected client name");
	/* tag 1 + tag 2 occupancy boxes, plus the floating title indicator */
	ASSERT_EQ(mock_drw_rect_calls, 3,
		"drawbar: two occupied tag boxes plus one floating indicator");
	/* the tag-1 box is filled for the selected client and urgent */
	ASSERT_EQ(mock_drw_last_rect_filled, 0,
		"drawbar: last indicator is the unfilled floating title box");

	restore_selmon();
	winclient_remove(c1);
	winclient_remove(c2);
	winclient_remove(c3);
	free(c1); free(c2); free(c3); free(m);
}

static void
run_tests(void)
{
	test_drawbar_fullscreen_freeze();
	test_drawbar_clean_bar();
	test_drawbar_hidden_bar();
	test_drawbar_segments_status_only();
	test_drawbar_segments_tags_only();
	test_drawbar_segments_title_with_sel();
	test_drawbar_segments_title_no_sel();
	test_drawbar_fullscreen_client();
	test_drawbar_urgent_client();
	test_drawbar_tags_loop();
}
