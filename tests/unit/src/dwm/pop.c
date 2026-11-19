/*
 * test_pop.c — per-function tests for dwm.c's pop().
 *
 * pop() must detach the client and re-attach it at the head of both the client
 * list and the focus stack, then select it.  Assertions check the resulting
 * pointer identities rather than only the absence of a crash.
 */
#include "test_common.h"

/* --- pop --- */
static void
test_pop_basic(void)
{
	reset_index();
	mock_x11_reset();
	Monitor *m = make_monitor(0);
	m->barwin = 999;
	Client *c1 = make_client(1, m);
	Client *c2 = make_client(2, m);
	m->clients = c1; c1->next = c2;
	m->stack = c1; c1->snext = c2; c2->snext = NULL;
	m->sel = c1; /* c2 is not selected yet */

	save_selmon();
	selmon = m;
	mons = m;

	pop(c2);

	ASSERT(m->clients == c2,
		"pop: client becomes the head of the client list");
	ASSERT(c2->next == c1,
		"pop: previous list head follows the promoted client");
	ASSERT(c1->next == NULL,
		"pop: promoted client is unlinked from its old position");

	ASSERT(m->stack == c2,
		"pop: client becomes the head of the focus stack");
	ASSERT(c2->snext == c1,
		"pop: previous stack head follows the promoted client");
	ASSERT(c1->snext == NULL,
		"pop: promoted client is unlinked from its old stack position");

	ASSERT(m->sel == c2, "pop: promoted client becomes the selection");
	ASSERT_EQ(mock_xset_input_focus.count, 1,
		"pop: focusing the promoted client takes X input focus");
	ASSERT_EQ(mock_xset_input_focus.win, c2->win,
		"pop: input focus targets the promoted client");

	restore_selmon();
	winclient_remove(c1);
	winclient_remove(c2);
	free(c1); free(c2); free(m);
}

static void
run_tests(void)
{
	test_pop_basic();
}
