/*
 * test_killclient.c — per-function tests for dwm.c's killclient().
 *
 * With a selected client, killclient() sends WM_DELETE_WINDOW when the client
 * advertises it; otherwise it falls back to XKillClient. The mock records both
 * paths so the fallback selection is asserted, not just exercised.
 */
#include "test_common.h"

static Atom delete_protos[1];

/* --- no selection: nothing happens --- */
static void
test_killclient_noop_no_sel(void)
{
	reset_index();
	save_selmon();
	selmon = make_monitor(0);
	mons = selmon;
	selmon->sel = NULL;
	Arg arg = {0};
	mock_x11_reset();

	killclient(&arg);

	ASSERT_EQ(mock_xsend_event.count, 0, "killclient: no event without a selection");
	ASSERT_EQ(mock_xkill_client.count, 0, "killclient: no kill without a selection");

	{
		Monitor *tmpm = selmon; /* free the fixture, then restore globals */
		restore_selmon();
		free(tmpm);
	}
}

/* --- graceful path: WM_DELETE_WINDOW is delivered, XKillClient is not used --- */
static void
test_killclient_calls_sendevent(void)
{
	reset_index();
	Monitor *m = make_monitor(0);
	Client *c = make_client(42, m);
	m->sel = c;
	save_selmon();
	selmon = m;
	mons = m;
	mock_x11_reset();
	delete_protos[0] = wmatom[WMDelete];
	mock_wmprotocols_return = 1;
	mock_wmprotocols_list = delete_protos;
	mock_wmprotocols_count = 1;

	{
		Arg arg = {0};

		killclient(&arg);
	}

	ASSERT_EQ(mock_xsend_event.count, 1, "killclient: sends WM_DELETE_WINDOW");
	ASSERT_EQ(mock_xsend_event_last.type, ClientMessage, "killclient: delete is a ClientMessage");
	ASSERT_EQ(mock_xsend_event_last.xclient.window, c->win, "killclient: targets the selected client");
	ASSERT_EQ(mock_xsend_event_last.xclient.data.l[0], wmatom[WMDelete],
		"killclient: carries WM_DELETE_WINDOW");
	ASSERT_EQ(mock_xkill_client.count, 0, "killclient: does not force-kill a cooperative client");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- fallback path: no list of protocols, so XKillClient is used --- */
static void
test_killclient_fallback_kill(void)
{
	Arg arg = {0};

	reset_index();
	Monitor *m = make_monitor(0);
	Client *c = make_client(43, m);
	m->sel = c;
	save_selmon();
	selmon = m;
	mons = m;
	mock_x11_reset(); /* mock_wmprotocols_return defaults to 0 */

	killclient(&arg);

	ASSERT_EQ(mock_xsend_event.count, 0, "killclient: no delete event without protocol support");
	ASSERT_EQ(mock_xkill_client.count, 1, "killclient: falls back to XKillClient");
	ASSERT_EQ(mock_xkill_client.win, c->win, "killclient: kills the selected window");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

static void
run_tests(void)
{
	test_killclient_noop_no_sel();
	test_killclient_calls_sendevent();
	test_killclient_fallback_kill();
}
