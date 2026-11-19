/*
 * test_grabbuttons.c — per-function tests for dwm.c's grabbuttons().
 *
 * grabbuttons() always ungrabs buttons on the client. An unfocused client also
 * receives a catch-all AnyButton grab, and every ClkClientWin binding is
 * grabbed for all four lock-modifier variants. The mock counts both.
 */
#include "test_common.h"

/* --- unfocused: catch-all grab plus one binding per modifier --- */
static void
test_grabbuttons(void)
{
	reset_index();
	Client c = { .win = 1 };
	mock_x11_reset();

	grabbuttons(&c, 0);

	ASSERT_EQ(mock_ungrabbutton_calls, 1, "grabbuttons: ungrabs buttons first");
	ASSERT_EQ(mock_grabbutton_calls, 5,
		"grabbuttons: AnyButton grab + one ClkClientWin binding x 4 modifiers");
}

/* --- focused: no catch-all grab, bindings only --- */
static void
test_grabbuttons_modifiers(void)
{
	reset_index();
	Client c = { .win = 1 };
	mock_x11_reset();

	grabbuttons(&c, 1);

	ASSERT_EQ(mock_ungrabbutton_calls, 1, "grabbuttons: ungrabs buttons first");
	ASSERT_EQ(mock_grabbutton_calls, 4,
		"grabbuttons: focused client grabs bindings only (4 modifiers)");
}

static void
run_tests(void)
{
	test_grabbuttons();
	test_grabbuttons_modifiers();
}
