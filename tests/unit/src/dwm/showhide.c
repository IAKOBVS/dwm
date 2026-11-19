/*
 * test_showhide.c — per-function tests for dwm.c's showhide().
 *
 * Visible clients are moved to their geometry (and resized when floating);
 * invisible clients are parked twice their width off-screen. The mock records
 * XMoveWindow/XConfigureWindow so these are real side-effect assertions.
 */
#include "test_common.h"

/* --- hidden client is parked off-screen --- */
static void
test_showhide_not_visible(void)
{
	reset_index();
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->tags = 2; /* not visible on tag 1 */
	c->w = 200;
	c->bw = 0;
	m->tagset[0] = 1;
	m->sel = NULL;
	save_selmon();
	selmon = m;
	mons = m;
	mock_x11_reset();

	showhide(c);

	ASSERT_EQ(mock_xmove_window.count, 1, "showhide: hidden client moved once");
	ASSERT_EQ(mock_xmove_window.win, c->win, "showhide: moves the client window");
	ASSERT_EQ(mock_xmove_window.x, WIDTH(c) * -2, "showhide: parks at -2*WIDTH");
	ASSERT_EQ(mock_xmove_window.y, c->y, "showhide: preserves y when hiding");
	ASSERT_EQ(mock_xconfigure_window.count, 0, "showhide: hidden client is not resized");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- visible tiled client is only moved --- */
static void
test_showhide_visible_tiled(void)
{
	reset_index();
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->tags = 1;
	c->isfloating = 0;
	m->tagset[0] = 1;
	m->lt[0] = m->lt[1] = &layouts[0]; /* tile: arrange != NULL */
	m->sel = NULL;
	save_selmon();
	selmon = m;
	mons = m;
	mock_x11_reset();

	showhide(c);

	ASSERT_EQ(mock_xmove_window.count, 1, "showhide: visible tiled client moved once");
	ASSERT_EQ(mock_xmove_window.x, c->x, "showhide: moves to c->x");
	ASSERT_EQ(mock_xmove_window.y, c->y, "showhide: moves to c->y");
	ASSERT_EQ(mock_xconfigure_window.count, 0, "showhide: tile layout does not resize");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- visible floating client is moved and resized --- */
static void
test_showhide_visible_floating(void)
{
	reset_index();
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->tags = 1;
	c->isfloating = 1;
	m->tagset[0] = 1;
	m->sel = NULL;
	save_selmon();
	selmon = m;
	mons = m;
	mock_x11_reset();

	showhide(c);

	ASSERT_EQ(mock_xmove_window.count, 1, "showhide: floating client moved once");
	ASSERT_EQ(mock_xconfigure_window.count, 1, "showhide: floating client resized once");
	ASSERT_EQ(mock_xconfigure_window.win, c->win, "showhide: configure targets the client");
	ASSERT_EQ(mock_xconfigure_last.x, c->x, "showhide: configure x matches geometry");
	ASSERT_EQ(mock_xconfigure_last.y, c->y, "showhide: configure y matches geometry");
	ASSERT_EQ(mock_xconfigure_last.width, c->w, "showhide: configure width matches geometry");
	ASSERT_EQ(mock_xconfigure_last.height, c->h, "showhide: configure height matches geometry");
	ASSERT_EQ(mock_xsend_event.count, 1, "showhide: resize emits one ConfigureNotify");
	ASSERT_EQ(mock_xsend_event_last.type, ConfigureNotify, "showhide: notify is ConfigureNotify");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

static void
run_tests(void)
{
	test_showhide_not_visible();
	test_showhide_visible_tiled();
	test_showhide_visible_floating();
}
