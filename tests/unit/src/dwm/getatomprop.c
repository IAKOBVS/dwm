/*
 * test_getatomprop.c — per-function tests for dwm.c's getatomprop().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through getatomprop(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- getatomprop --- */
static void
test_getatomprop(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Client c = { .win = 1 };
	Atom a = getatomprop(&c, 0);
	ASSERT_EQ(a, (Atom)0, "getatomprop: returns None/0 for mock");
}

/* --- getatomprop non-NULL path --- */
static void
test_getatomprop_found(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Client c = { .win = 1 };
	mock_x11_reset();
	mock_getwindowproperty_return = 1;
	mock_getwindowproperty_atom = 42;

	Atom a = getatomprop(&c, 0);
	ASSERT_EQ(a, (Atom)42, "getatomprop found: returns the stored atom");
	mock_x11_reset();
}

static void
run_tests(void)
{
	test_getatomprop();
	test_getatomprop_found();
}
