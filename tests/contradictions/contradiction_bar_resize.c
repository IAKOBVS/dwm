/*
 * Reproduction for the blank-bar contradiction: configurenotify() used to
 * call drw_resize() (destroying the backing pixmap) without marking any bar
 * segment dirty, so the next Expose copied a blank pixmap to the bar.
 */
#include "test_common.h"

static void
test_root_resize_marks_bar_dirty(void)
{
	reset_index();
	Monitor *m = make_monitor(0);
	XEvent ev;

	m->showbar = 1;
	m->bar_dirty_segments = 0;
	m->bar_exposed = 0;
	mons = m;
	save_selmon();
	selmon = m;

	memset(&ev, 0, sizeof ev);
	ev.xconfigure.window = root;
	ev.xconfigure.width = sw + 100;
	ev.xconfigure.height = sh + 100;
	configurenotify(&ev);

	ASSERT((m->bar_dirty_segments & DIRTY_ALL) == DIRTY_ALL,
		"repro: root resize forces a full bar rebuild");

	restore_selmon();
	free(m);
}

static void
run_tests(void)
{
	test_root_resize_marks_bar_dirty();
}
