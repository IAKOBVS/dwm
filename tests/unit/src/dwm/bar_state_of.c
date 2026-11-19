/*
 * test_bar_state_of.c — per-function tests for bar_state_of().
 *
 * Verifies the explicit bar phase is derived from showbar, fullscreen
 * freeze, dirty segments and expose state.
 */
#include "test_common.h"

static void
test_bar_state_phases(void)
{
	reset_index();
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);

	m->showbar = 1;
	m->sel = NULL;
	m->bar_dirty_segments = 0;
	m->bar_exposed = 0;
	ASSERT_EQ(bar_state_of(m), BAR_CLEAN, "bar_state_of: clean");

	m->bar_exposed = 1;
	ASSERT_EQ(bar_state_of(m), BAR_EXPOSED_ONLY, "bar_state_of: expose only");

	m->bar_dirty_segments = DIRTY_TAGS;
	ASSERT_EQ(bar_state_of(m), BAR_DIRTY, "bar_state_of: dirty segments");

	m->sel = c;
	c->isfullscreen = 1;
	ASSERT_EQ(bar_state_of(m), BAR_FROZEN, "bar_state_of: fullscreen freeze");

	m->showbar = 0;
	ASSERT_EQ(bar_state_of(m), BAR_HIDDEN, "bar_state_of: hidden");

	winclient_remove(c);
	free(c); free(m);
}

static void
run_tests(void)
{
	test_bar_state_phases();
}
