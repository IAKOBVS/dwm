/*
 * test_setup.c — per-function tests for dwm.c's setup().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through setup(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- setup function (lines 1590-1670) --- */
static void
test_setup_die_on_font_fail(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	/* Force drw_fontset_create to fail, verify DIE path is taken.
	 * mock_die_abort makes die() set flag=2 and return instead of abort,
	 * and the mock provides a safe drw->fonts fallback so setup()
	 * completes without crashing after the DIE returns. */
	mock_die_abort = 1;
	mock_fontset_fail = 1;
	free_global_state(); /* release state from any prior setup() */
	setup();
	ASSERT(mock_die_abort == 2, "setup: DIE called on font fail");
	mock_die_abort = 0;
	mock_fontset_fail = 0;
}

static void
test_setup_function(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	/* setup() overwrites all globals, so call it and verify */
	free_global_state(); /* release state from any prior setup() */
	setup();

	ASSERT(dpy != NULL, "setup: dpy initialized");
	ASSERT(drw != NULL, "setup: drw initialized");
	ASSERT(drw->fonts != NULL, "setup: fonts initialized");
	ASSERT(bh > 0, "setup: bh set");
	ASSERT(lrpad >= 0, "setup: lrpad set");
	ASSERT(mons != NULL, "setup: mons created");
	ASSERT(selmon != NULL, "setup: selmon set");
	ASSERT(scheme != NULL, "setup: scheme allocated");
	ASSERT(cursor[CurNormal] != NULL, "setup: CurNormal cursor created");
	ASSERT(cursor[CurResize] != NULL, "setup: CurResize cursor created");
	ASSERT(cursor[CurMove] != NULL, "setup: CurMove cursor created");
	ASSERT(wmcheckwin != 0, "setup: wmcheckwin created");
	ASSERT(wmatom[WMProtocols] != 0, "setup: WMProtocols atom set");
	ASSERT(wmatom[WMDelete] != 0, "setup: WMDelete atom set");
	ASSERT(wmatom[WMState] != 0, "setup: WMState atom set");
	ASSERT(wmatom[WMTakeFocus] != 0, "setup: WMTakeFocus atom set");
	ASSERT(netatom[NetActiveWindow] != 0, "setup: NetActiveWindow atom set");
	ASSERT(netatom[NetSupported] != 0, "setup: NetSupported atom set");
	ASSERT(netatom[NetWMName] != 0, "setup: NetWMName atom set");
	ASSERT(netatom[NetWMState] != 0, "setup: NetWMState atom set");
	ASSERT(netatom[NetWMCheck] != 0, "setup: NetWMCheck atom set");
	ASSERT(netatom[NetWMFullscreen] != 0, "setup: NetWMFullscreen atom set");
	ASSERT(netatom[NetWMWindowType] != 0, "setup: NetWMWindowType atom set");
	ASSERT(netatom[NetWMWindowTypeDialog] != 0, "setup: NetWMWindowTypeDialog atom set");
	ASSERT(netatom[NetClientList] != 0, "setup: NetClientList atom set");
}

static void
run_tests(void)
{
	test_setup_die_on_font_fail();
	test_setup_function();
}
