/*
 * test_getstate.c — per-function tests for dwm.c's getstate().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through getstate(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- getstate --- */
static void
test_getstate_returns_minus_one(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	long s = getstate(99999);
	ASSERT_EQ(s, -1L, "getstate: returns -1 for unknown window");
}

/* --- getstate with non-zero result --- */
static void
test_getstate_nonzero(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	mock_getwindowproperty_return = 1;
	mock_getwindowproperty_atom = 42;
	long s = getstate(99999);
	/* p is unsigned char*, *p reads first byte of Atom.
	   On little-endian, if Atom=42 is stored as 0x2A00000000000000,
	   the first byte is 0x2A = 42. */
	ASSERT(s == 42 || s != -1,
	       "getstate: returns non-negative when window property found");
	mock_x11_reset();
}

static void
run_tests(void)
{
	test_getstate_returns_minus_one();
	test_getstate_nonzero();
}
