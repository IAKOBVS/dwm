/*
 * test_mappingnotify.c — per-function tests for dwm.c's mappingnotify().
 *
 * Every MappingNotify refreshes Xlib's keyboard mapping; only a keyboard
 * request additionally re-grabs the configured keys. The mock records the
 * refresh and grab counts so the branch is observable.
 */
#include "test_common.h"

/* --- keyboard mapping: refresh + grabkeys --- */
static void
test_mappingnotify(void)
{
	reset_index();
	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xmapping.request = MappingKeyboard;
	mock_x11_reset();

	mappingnotify(&ev);

	ASSERT_EQ(mock_xrefresh_keyboard_mapping.count, 1,
		"mappingnotify: always refreshes the keyboard mapping");
	ASSERT_EQ(mock_ungrabkey_calls, 1,
		"mappingnotify: keyboard request re-runs grabkeys (ungrab)");
}

/* --- non-keyboard request: refresh only --- */
static void
test_mappingnotify_not_keyboard(void)
{
	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xmapping.request = MappingPointer; /* not MappingKeyboard */
	mock_x11_reset();

	mappingnotify(&ev);

	ASSERT_EQ(mock_xrefresh_keyboard_mapping.count, 1,
		"mappingnotify: non-keyboard request still refreshes");
	ASSERT_EQ(mock_ungrabkey_calls, 0,
		"mappingnotify: non-keyboard request does not re-grab keys");
}

static void
run_tests(void)
{
	test_mappingnotify();
	test_mappingnotify_not_keyboard();
}
