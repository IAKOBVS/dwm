/*
 * test_setfocus.c — per-function tests for dwm.c's setfocus().
 *
 * Unless the client is neverfocus, setfocus() must take X input focus and
 * publish _NET_ACTIVE_WINDOW. WM_TAKE_FOCUS is delivered whenever the client
 * advertises it, independently of neverfocus. The mock records all three side
 * effects.
 */
#include "test_common.h"

static Atom takefocus_protos[1];

static void
offer_wm_take_focus(void)
{
	takefocus_protos[0] = wmatom[WMTakeFocus];
	mock_wmprotocols_return = 1;
	mock_wmprotocols_list = takefocus_protos;
	mock_wmprotocols_count = 1;
}

/* --- normal client: focus + active-window property + take-focus event --- */
static void
test_setfocus_basic(void)
{
	reset_index();
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->neverfocus = 0;
	save_selmon();
	selmon = m;
	mons = m;
	mock_x11_reset();
	offer_wm_take_focus();

	setfocus(c);

	ASSERT_EQ(mock_xset_input_focus.count, 1, "setfocus: takes input focus");
	ASSERT_EQ(mock_xset_input_focus.win, c->win, "setfocus: focuses the client window");
	ASSERT_EQ(mock_xchange_property.count, 1, "setfocus: updates active-window property");
	ASSERT_EQ(mock_xchange_property.mask, netatom[NetActiveWindow],
		"setfocus: writes _NET_ACTIVE_WINDOW");
	ASSERT_EQ(mock_xchange_property.aux, XA_WINDOW, "setfocus: property type is XA_WINDOW");
	ASSERT_EQ(mock_xchange_property_last_len, 4, "setfocus: property is one 32-bit value");
	ASSERT_EQ(*(unsigned int *)mock_xchange_property_last, (unsigned int)c->win,
		"setfocus: property value is the client window");
	ASSERT_EQ(mock_xsend_event.count, 1, "setfocus: delivers WM_TAKE_FOCUS");
	ASSERT_EQ(mock_xsend_event_last.type, ClientMessage, "setfocus: take-focus is a ClientMessage");
	ASSERT_EQ(mock_xsend_event.mask, NoEventMask, "setfocus: take-focus uses NoEventMask");
	ASSERT_EQ(mock_xsend_event_last.xclient.data.l[0], wmatom[WMTakeFocus],
		"setfocus: message carries WM_TAKE_FOCUS");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- neverfocus client: no input focus, no active-window update --- */
static void
test_setfocus_neverfocus(void)
{
	reset_index();
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->neverfocus = 1;
	save_selmon();
	selmon = m;
	mons = m;
	mock_x11_reset(); /* no WM_TAKE_FOCUS protocol offered */

	setfocus(c);

	ASSERT_EQ(mock_xset_input_focus.count, 0, "setfocus: neverfocus skips XSetInputFocus");
	ASSERT_EQ(mock_xchange_property.count, 0, "setfocus: neverfocus skips _NET_ACTIVE_WINDOW");
	ASSERT_EQ(mock_xsend_event.count, 0, "setfocus: no take-focus event without protocol");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- neverfocus still receives WM_TAKE_FOCUS when advertised --- */
static void
test_setfocus_neverfocus_takefocus(void)
{
	reset_index();
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->neverfocus = 1;
	save_selmon();
	selmon = m;
	mons = m;
	mock_x11_reset();
	offer_wm_take_focus();

	setfocus(c);

	ASSERT_EQ(mock_xset_input_focus.count, 0, "setfocus: neverfocus skips input focus");
	ASSERT_EQ(mock_xchange_property.count, 0, "setfocus: neverfocus skips property update");
	ASSERT_EQ(mock_xsend_event.count, 1, "setfocus: WM_TAKE_FOCUS still delivered");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

static void
run_tests(void)
{
	test_setfocus_basic();
	test_setfocus_neverfocus();
	test_setfocus_neverfocus_takefocus();
}
