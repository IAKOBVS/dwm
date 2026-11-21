/*
 * test_setclientstate.c — per-function tests for dwm.c's setclientstate().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through setclientstate(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- setclientstate --- */
static void
test_setclientstate_normal(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	Client c = { .win = 1 };
	setclientstate(&c, NormalState);

	/* WM_STATE is written on the client window with WM_STATE as both the
	 * property and its type, 32-bit format, two elements. */
	ASSERT_EQ(mock_xchange_property.count, 1, "setclientstate(NormalState): one property write");
	ASSERT_EQ(mock_xchange_property.win, (unsigned long)c.win, "setclientstate(NormalState): targets client window");
	ASSERT_EQ(mock_xchange_property.mask, (unsigned long)wmatom[WMState], "setclientstate(NormalState): WM_STATE property");
	ASSERT_EQ(mock_xchange_property.aux, (unsigned long)wmatom[WMState], "setclientstate(NormalState): WM_STATE type");
	ASSERT_EQ(mock_xchange_property_last_len, 8, "setclientstate(NormalState): two 32-bit words");
	{
		long observed = -1;
		memcpy(&observed, mock_xchange_property_last, sizeof observed);
		ASSERT_EQ(observed, NormalState, "setclientstate(NormalState): NormalState data word");
	}
}

static void
test_setclientstate_withdrawn(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	Client c = { .win = 1 };
	setclientstate(&c, WithdrawnState);

	ASSERT_EQ(mock_xchange_property.count, 1, "setclientstate(WithdrawnState): one property write");
	ASSERT_EQ(mock_xchange_property.win, (unsigned long)c.win, "setclientstate(WithdrawnState): targets client window");
	ASSERT_EQ(mock_xchange_property.mask, (unsigned long)wmatom[WMState], "setclientstate(WithdrawnState): WM_STATE property");
	ASSERT_EQ(mock_xchange_property_last_len, 8, "setclientstate(WithdrawnState): two 32-bit words");
	{
		long observed = -1;
		memcpy(&observed, mock_xchange_property_last, sizeof observed);
		ASSERT_EQ(observed, WithdrawnState, "setclientstate(WithdrawnState): WithdrawnState data word");
	}
}

/* --- setclientstate (line 1569 comment: arrange is in setlayout) --- */
static void
test_setclientstate_arrange(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	m->sel = c;
	m->clients = c;
	save_selmon();
	selmon = m;
	mons = m;

	setclientstate(c, NormalState);
	ASSERT_EQ(mock_xchange_property.count, 1, "setclientstate arrange: first write recorded");
	{
		long observed = -1;
		memcpy(&observed, mock_xchange_property_last, sizeof observed);
		ASSERT_EQ(observed, NormalState, "setclientstate arrange: NormalState first");
	}

	setclientstate(c, WithdrawnState);
	ASSERT_EQ(mock_xchange_property.count, 2, "setclientstate arrange: second write recorded");
	ASSERT_EQ(mock_xchange_property.win, (unsigned long)c->win, "setclientstate arrange: both writes target client");
	{
		long observed = -1;
		memcpy(&observed, mock_xchange_property_last, sizeof observed);
		ASSERT_EQ(observed, WithdrawnState, "setclientstate arrange: WithdrawnState overwrites last");
	}

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

static void
run_tests(void)
{
	test_setclientstate_normal();
	test_setclientstate_withdrawn();
	test_setclientstate_arrange();
}
