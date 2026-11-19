/*
 * test_restack.c — per-function tests for dwm.c's restack().
 *
 * Asserts the stacking side effects rather than just survival: the selected
 * floating client is raised, visible tiled clients are chained below one
 * another with the bar underneath, and the no-selection edge performs no X11
 * work at all.
 */
#include "test_common.h"

/* --- restack --- */
static void
test_restack_noop_no_sel(void)
{
	reset_index();
	mock_x11_reset();
	Monitor *m = make_monitor(0);
	m->barwin = 999;
	m->sel = NULL;
	m->clients = NULL;
	m->stack = NULL;
	save_selmon();
	selmon = m;
	mons = m;
	bar_draw_pending = 0;

	restack(m);

	ASSERT_EQ(mock_xraise_window.count, 0,
		"restack: no selection raises no window");
	ASSERT_EQ(mock_xconfigure_window.count, 0,
		"restack: no selection configures no window");
	ASSERT_EQ(mock_xsync_calls, 0,
		"restack: no selection returns before XSync");
	ASSERT_EQ(bar_draw_pending, 1,
		"restack: still schedules the bar redraw");

	restore_selmon();
	free(m);
}

/* --- floating selection is raised above the tiled stack --- */
static void
test_restack_floating_raised(void)
{
	reset_index();
	mock_x11_reset();
	Monitor *m = make_monitor(0);
	m->barwin = 999;
	Client *floating = make_client(10, m);
	Client *tiled = make_client(11, m);
	floating->isfloating = 1;
	floating->tags = 1;
	tiled->isfloating = 0;
	tiled->tags = 1;
	m->sel = floating;
	m->stack = tiled;
	tiled->snext = NULL;
	save_selmon();
	selmon = m;
	mons = m;
	bar_draw_pending = 0;

	restack(m);

	ASSERT_EQ(mock_xraise_window.count, 1,
		"restack: floating selection is raised exactly once");
	ASSERT_EQ(mock_xraise_window.win, floating->win,
		"restack: the raised window is the selected floating client");
	ASSERT_EQ(mock_xconfigure_window.count, 1,
		"restack: the visible tiled client is configured once");
	ASSERT_EQ(mock_xconfigure_window.win, tiled->win,
		"restack: the configured window is the tiled client");
	ASSERT_EQ(mock_xconfigure_last.stack_mode, Below,
		"restack: tiled clients are placed below their sibling");
	ASSERT_EQ(mock_xconfigure_last.sibling, (Window)m->barwin,
		"restack: the first tiled client sits below the bar");
	ASSERT_EQ(mock_xsync_calls, 1,
		"restack: flushes the stacking changes once");

	restore_selmon();
	winclient_remove(floating);
	winclient_remove(tiled);
	free(floating); free(tiled); free(m);
}

/* --- tiled selection chains clients below one another --- */
static void
test_restack_tiled_chain(void)
{
	reset_index();
	mock_x11_reset();
	Monitor *m = make_monitor(0);
	m->barwin = 999;
	Client *c1 = make_client(20, m);
	Client *c2 = make_client(21, m);
	c1->isfloating = c2->isfloating = 0;
	c1->tags = c2->tags = 1;
	m->sel = c1;
	m->stack = c1;
	c1->snext = c2;
	c2->snext = NULL;
	save_selmon();
	selmon = m;
	mons = m;
	bar_draw_pending = 0;

	restack(m);

	ASSERT_EQ(mock_xraise_window.count, 0,
		"restack: tiled selection is not explicitly raised");
	ASSERT_EQ(mock_xconfigure_window.count, 2,
		"restack: each visible tiled client is configured once");
	ASSERT_EQ(mock_xconfigure_window.win, c2->win,
		"restack: the last configured window is the list tail");
	ASSERT_EQ(mock_xconfigure_last.stack_mode, Below,
		"restack: chained clients use Below stacking");
	ASSERT_EQ(mock_xconfigure_last.sibling, c1->win,
		"restack: each client is chained below the previous one");
	ASSERT_EQ(mock_xsync_calls, 1,
		"restack: flushes the stacking changes once");

	restore_selmon();
	winclient_remove(c1);
	winclient_remove(c2);
	free(c1); free(c2); free(m);
}

static void
run_tests(void)
{
	test_restack_noop_no_sel();
	test_restack_floating_raised();
	test_restack_tiled_chain();
}
