/*
 * test_grabkeys.c — per-function tests for dwm.c's grabkeys().
 *
 * grabkeys() always ungrabs every key on the root and re-grabs each bound
 * keycode for all four lock-modifier variants. The mock counts XGrabKey and
 * XUngrabKey calls, so the fan-out is asserted exactly.
 */
#include "test_common.h"

/* --- no matching keysyms: ungrab only --- */
static void
test_grabkeys(void)
{
	reset_index();
	mock_x11_reset();

	grabkeys();

	ASSERT_EQ(mock_ungrabkey_calls, 1, "grabkeys: ungrabs all keys once");
	ASSERT_EQ(mock_grabkey_calls, 0, "grabkeys: default keymap matches no binding");
}

/* --- NULL keyboard mapping: early return after the ungrab --- */
static void
test_grabkeys_early_return(void)
{
	reset_index();
	mock_x11_reset();
	mock_keyboardmapping_return_null = 1;

	grabkeys();

	ASSERT_EQ(mock_ungrabkey_calls, 1, "grabkeys: ungrabs before the NULL check");
	ASSERT_EQ(mock_grabkey_calls, 0, "grabkeys: NULL syms returns without grabbing");
}

/* --- a matching keysym is grabbed for all four modifier variants --- */
static void
test_grabkeys_modifier_loop(void)
{
	reset_index();
	mock_x11_reset();
	/* XK_p matches keys[0] in config.h (spawn/dmenu). */
	mock_keyboardmapping_first_keysym = XK_p;

	grabkeys();

	ASSERT_EQ(mock_ungrabkey_calls, 1, "grabkeys: ungrabs all keys first");
	ASSERT_EQ(mock_grabkey_calls, 4,
		"grabkeys: one binding grabbed for 0/Lock/NumLock/NumLock+Lock");
}

static void
run_tests(void)
{
	test_grabkeys();
	test_grabkeys_early_return();
	test_grabkeys_modifier_loop();
}
