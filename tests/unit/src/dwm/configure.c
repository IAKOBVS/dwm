/*
 * test_configure.c — per-function tests for dwm.c's configure().
 *
 * configure() is pure X11 emission: it must send exactly one ConfigureNotify
 * for the client's current geometry with StructureNotifyMask. The mock records
 * the event so these assertions verify the payload, not just "did not crash".
 */
#include "test_common.h"

/* --- configure --- */
static void
test_configure_basic(void)
{
	reset_index();
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->x = 100; c->y = 110; c->w = 200; c->h = 210; c->bw = 3;
	save_selmon();
	selmon = m;
	mons = m;
	mock_x11_reset(); /* isolate recorded X calls per test */

	configure(c);

	ASSERT_EQ(mock_xsend_event.count, 1, "configure: sends exactly one event");
	ASSERT_EQ(mock_xsend_event_last.type, ConfigureNotify, "configure: event is ConfigureNotify");
	ASSERT_EQ(mock_xsend_event.mask, StructureNotifyMask, "configure: uses StructureNotifyMask");
	ASSERT_EQ(mock_xsend_event_last.xconfigure.window, c->win, "configure: targets the client window");
	ASSERT_EQ(mock_xsend_event_last.xconfigure.x, c->x, "configure: carries c->x");
	ASSERT_EQ(mock_xsend_event_last.xconfigure.y, c->y, "configure: carries c->y");
	ASSERT_EQ(mock_xsend_event_last.xconfigure.width, c->w, "configure: carries c->w");
	ASSERT_EQ(mock_xsend_event_last.xconfigure.height, c->h, "configure: carries c->h");
	ASSERT_EQ(mock_xsend_event_last.xconfigure.border_width, c->bw, "configure: carries c->bw");
	ASSERT_EQ(mock_xsend_event_last.xconfigure.above, None, "configure: above is None");
	ASSERT_EQ(mock_xsend_event_last.xconfigure.override_redirect, False, "configure: override_redirect is False");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

static void
run_tests(void)
{
	test_configure_basic();
}
