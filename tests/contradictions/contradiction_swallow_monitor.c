/*
 * Reproduction for the swallowed-placeholder contradiction: a child parked
 * by swallow() is off-list, so a monitor move of the holder used to leave the
 * placeholder on the old monitor (dangling c->mon on monitor removal).
 */
#include "test_common.h"

static void
test_placeholder_follows_holder(void)
{
	reset_index();
	Monitor *m1 = make_monitor(0);
	Monitor *m2 = make_monitor(1);
	Client *term = make_client(10, m1);
	Client *ph = make_client(11, m1);

	term->isterminal = 1;
	ph->isterminal = 0;
	m1->clients = ph;
	ph->next = NULL;
	m1->next = m2;

	swallow(term, ph); /* term becomes HOLDER, ph becomes PLACEHOLDER */

	save_selmon();
	selmon = m1;
	mons = m1;

	sendmon(term, m2);

	ASSERT_EQ(term->mon, m2, "repro: holder moved to m2");
	ASSERT_EQ(ph->mon, m2,
		"repro: off-list placeholder did not keep the old monitor");

	restore_selmon();
	winclient_remove(term);
	winclient_remove(ph);
	free(term); free(ph); free(m1); free(m2);
}

static void
run_tests(void)
{
	test_placeholder_follows_holder();
}
