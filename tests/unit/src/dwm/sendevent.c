/*
 * test_sendevent.c — per-function tests for dwm.c's sendevent().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through sendevent(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- sendevent --- */
static void
test_sendevent(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Client c = { .win = 1 };
	int r = sendevent(&c, 0);
	ASSERT_EQ(r, 0, "sendevent: returns 0 (mock XSendEvent returns 0)");
}

/* --- sendevent protocol found (lines 1470-1481) --- */
static void
test_sendevent_protocol_found(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Client c = { .win = 1 };
	Atom proto = 42;
	Atom protocols[] = { 42, 99 };
	mock_wmprotocols_return = 1;
	mock_wmprotocols_list = protocols;
	mock_wmprotocols_count = 2;
	int r = sendevent(&c, proto);
	/* lines 1469-1472: XGetWMProtocols succeeds, loop finds proto==42,
	 * lines 1475-1481: ClientMessage sent via XSendEvent */
	ASSERT_EQ(r, 1, "sendevent protocol found: returns 1");
	mock_x11_reset();
}

/* --- sendevent protocol not found (lines 1470-1472) --- */
static void
test_sendevent_protocol_not_found(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Client c = { .win = 2 };
	Atom proto = 42;
	Atom protocols[] = { 99, 100 };
	mock_wmprotocols_return = 1;
	mock_wmprotocols_list = protocols;
	mock_wmprotocols_count = 2;
	int r = sendevent(&c, proto);
	/* lines 1469-1472: XGetWMProtocols succeeds, loop finds no match */
	ASSERT_EQ(r, 0, "sendevent protocol not found: returns 0");
	mock_x11_reset();
}

/* --- sendevent no protocols (XGetWMProtocols fails) --- */
static void
test_sendevent_no_protocols(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Client c = { .win = 3 };
	Atom proto = 42;
	mock_wmprotocols_return = 0;
	int r = sendevent(&c, proto);
	/* line 1469: XGetWMProtocols fails, exists stays 0 */
	ASSERT_EQ(r, 0, "sendevent no protocols: returns 0");
	mock_x11_reset();
}

static void
run_tests(void)
{
	test_sendevent();
	test_sendevent_protocol_found();
	test_sendevent_protocol_not_found();
	test_sendevent_no_protocols();
}
