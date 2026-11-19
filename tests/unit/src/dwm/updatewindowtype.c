/*
 * test_updatewindowtype.c — per-function tests for dwm.c's updatewindowtype().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through updatewindowtype(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- updatewindowtype sets fullscreen (lines 2122-2124) --- */
static void
test_updatewindowtype_sets_fullscreen(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	mock_getwindowproperty_return = 1;
	mock_getwindowproperty_atom = netatom[NetWMFullscreen];

	save_selmon();
	selmon = make_monitor(0);
	mons = selmon;

	Client c = { .win = 100, .tags = 1, .mon = selmon, .next = NULL,
		.isfullscreen = 0, .bw = 2 };
	selmon->clients = &c;

	updatewindowtype(&c);
	ASSERT(c.isfullscreen, "updatewindowtype: NetWMState fullscreen sets isfullscreen");

	mock_x11_reset();
{
		Monitor *tmpm = selmon; /* free the fixture, then restore globals */
		restore_selmon();
		free(tmpm);
	}
}

/* --- updatewindowtype sets floating for dialog (line 2124) --- */
static void
test_updatewindowtype_sets_floating_dialog(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	mock_getwindowproperty_return = 1;
	mock_getwindowproperty_atom = netatom[NetWMWindowTypeDialog];

	save_selmon();
	selmon = make_monitor(0);
	mons = selmon;

	Client c = { .win = 100, .tags = 1, .mon = selmon, .next = NULL,
		.isfloating = 0, .isfullscreen = 0 };
	selmon->clients = &c;

	updatewindowtype(&c);
	ASSERT(c.isfloating, "updatewindowtype: WMWindowTypeDialog sets isfloating");

	mock_x11_reset();
{
		Monitor *tmpm = selmon; /* free the fixture, then restore globals */
		restore_selmon();
		free(tmpm);
	}
}

static void
run_tests(void)
{
	test_updatewindowtype_sets_fullscreen();
	test_updatewindowtype_sets_floating_dialog();
}
