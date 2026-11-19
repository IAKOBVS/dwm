/*
 * test_gettextprop.c — per-function tests for dwm.c's gettextprop().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through gettextprop(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- gettextprop --- */
static void
test_gettextprop_returns_zero(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	char buf[256] = {0};
	int r = gettextprop(99999, XA_WM_NAME, buf, sizeof buf);
	/* XGetTextProperty mock returns 0, so function returns 0 */
	ASSERT_EQ(r, 0, "gettextprop: returns 0 for unknown window");
}

/* --- gettextprop XA_STRING path --- */
static void
test_gettextprop_xastring(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	char buf[256] = {0};
	mock_x11_reset();
	mock_gettextprop_return = 1;
	mock_gettextprop_value = "hello status";
	mock_gettextprop_encoding = XA_STRING;

	int r = gettextprop(42, XA_WM_NAME, buf, sizeof buf);
	ASSERT(r, "gettextprop XA_STRING: returns 1");
	ASSERT_EQ(strcmp(buf, "hello status"), 0, "gettextprop XA_STRING: text matches");
	mock_x11_reset();
}

static void
test_gettextprop_empty(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	char buf[256] = {0};
	mock_x11_reset();
	mock_gettextprop_return = 1;
	mock_gettextprop_value = "";

	int r = gettextprop(42, XA_WM_NAME, buf, sizeof buf);
	ASSERT(!r, "gettextprop empty: returns 0 (nitems == 0)");
	mock_x11_reset();
}

/* --- gettextprop null text --- */
static void
test_gettextprop_null_text(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	int r = gettextprop(42, XA_WM_NAME, NULL, 256);
	ASSERT_EQ(r, 0, "gettextprop null text: returns 0");
}

static void
test_gettextprop_zero_size(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	char buf[256] = {0};
	int r = gettextprop(42, XA_WM_NAME, buf, 0);
	ASSERT_EQ(r, 0, "gettextprop zero size: returns 0");
}

/* --- gettextprop compound text (non-XA_STRING encoding) --- */
static void
test_gettextprop_compound_text(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	char buf[256] = {0};
	mock_x11_reset();
	mock_gettextprop_return = 1;
	mock_gettextprop_value = "hello";
	mock_gettextprop_encoding = XA_CARDINAL;
	/* Make XmbTextPropertyToTextList return a real list so lines 868-870 fire */
	mock_textlist_text = "compound hello";
	mock_textlist_count = 1;

	int r = gettextprop(42, XA_WM_NAME, buf, sizeof buf);
	ASSERT(r, "gettextprop compound text: returns 1 (else-if path taken)");
	ASSERT_EQ(strcmp(buf, "compound hello"), 0, "gettextprop compound text: text filled from XmbTextPropertyToTextList");
	mock_x11_reset();
}

static void
run_tests(void)
{
	test_gettextprop_returns_zero();
	test_gettextprop_xastring();
	test_gettextprop_empty();
	test_gettextprop_null_text();
	test_gettextprop_zero_size();
	test_gettextprop_compound_text();
}
