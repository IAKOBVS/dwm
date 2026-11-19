/*
 * test_updateclientlist.c — per-function tests for dwm.c's updateclientlist().
 *
 * updateclientlist() must clear then append every managed window to
 * _NET_CLIENT_LIST as 32-bit XA_WINDOW entries.  The mock records both the
 * property calls and the last payload so the tests can verify the atom, type,
 * element width and window contents.
 */
#include "test_common.h"

/* --- updateclientlist --- */
static void
test_updateclientlist(void)
{
	reset_index();
	mock_x11_reset();
	save_selmon();
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	m->clients = c;
	m->stack = c;
	selmon = m;
	mons = m;

	updateclientlist();

	ASSERT_EQ(mock_xdelete_property.count, 1,
		"updateclientlist: clears _NET_CLIENT_LIST once");
	ASSERT_EQ(mock_xdelete_property.win, (Window)root,
		"updateclientlist: clears the property on the root window");
	ASSERT_EQ(mock_xdelete_property.mask, netatom[NetClientList],
		"updateclientlist: deletes the _NET_CLIENT_LIST atom");

	ASSERT_EQ(mock_xchange_property.count, 1,
		"updateclientlist: appends once per managed window");
	ASSERT_EQ(mock_xchange_property.win, (Window)root,
		"updateclientlist: writes the property on the root window");
	ASSERT_EQ(mock_xchange_property.mask, netatom[NetClientList],
		"updateclientlist: writes the _NET_CLIENT_LIST atom");
	ASSERT_EQ(mock_xchange_property.aux, XA_WINDOW,
		"updateclientlist: entries use the XA_WINDOW type");
	ASSERT_EQ(mock_xchange_property_last_len, 4,
		"updateclientlist: each entry is one 32-bit value");
	ASSERT_EQ(*(Window *)mock_xchange_property_last, c->win,
		"updateclientlist: payload contains the managed window id");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- every managed client is appended, in list order --- */
static void
test_updateclientlist_multiple(void)
{
	reset_index();
	mock_x11_reset();
	save_selmon();
	Monitor *m = make_monitor(0);
	Client *c1 = make_client(10, m);
	Client *c2 = make_client(11, m);
	m->clients = c2;
	c2->next = c1;
	c1->next = NULL;
	m->stack = c2;
	c2->snext = c1;
	c1->snext = NULL;
	selmon = m;
	mons = m;

	updateclientlist();

	ASSERT_EQ(mock_xchange_property.count, 2,
		"updateclientlist: appends one property entry per client");
	ASSERT_EQ(mock_xchange_property_last_len, 4,
		"updateclientlist: the last entry is still one 32-bit value");
	ASSERT_EQ(*(Window *)mock_xchange_property_last, c1->win,
		"updateclientlist: the final append carries the list-tail window");

	restore_selmon();
	winclient_remove(c1);
	winclient_remove(c2);
	free(c1); free(c2); free(m);
}

static void
run_tests(void)
{
	test_updateclientlist();
	test_updateclientlist_multiple();
}
