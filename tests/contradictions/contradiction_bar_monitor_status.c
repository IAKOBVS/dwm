/*
 * Reproduction for the cross-monitor status contradiction: status text is
 * only drawn on selmon, but focus() used to dirty only TITLE|TAGS when the
 * selected monitor changed, leaving the new monitor with stale status.
 */
#include "test_common.h"

static void
test_monitor_switch_dirties_status(void)
{
	reset_index();
	Monitor *m1 = make_monitor(0);
	Monitor *m2 = make_monitor(1);
	Client *c = make_client(5, m2);

	m1->next = m2;
	m2->clients = c;
	m2->stack = c;
	mons = m1;
	save_selmon();
	selmon = m1;

	m2->bar_dirty_segments = 0;
	focus(c);

	ASSERT_EQ(selmon, m2, "repro: focus switched selmon");
	ASSERT((m2->bar_dirty_segments & DIRTY_STATUS) != 0,
		"repro: new monitor repaints status after a monitor switch");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m1); free(m2);
}

static void
run_tests(void)
{
	test_monitor_switch_dirties_status();
}
