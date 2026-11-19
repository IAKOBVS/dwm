/*
 * test_xerror.c — per-function tests for dwm.c's xerror().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through xerror(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- xerror / xerrordummy / xerrorstart --- */
static void
test_xerror_swallows_badwindow(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	XErrorEvent ee;
	memset(&ee, 0, sizeof ee);
	ee.error_code = BadWindow;
	int r = xerror(NULL, &ee);
	ASSERT_EQ(r, 0, "xerror: BadWindow returns 0");
}

static void
test_xerror_swallows_badmatch(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	XErrorEvent ee;
	memset(&ee, 0, sizeof ee);
	ee.request_code = X_SetInputFocus;
	ee.error_code = BadMatch;
	int r = xerror(NULL, &ee);
	ASSERT_EQ(r, 0, "xerror: BadMatch on SetInputFocus returns 0");
}

/* --- xerror with other codes --- */
static void
test_xerror_baddrawable(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	XErrorEvent ee;
	memset(&ee, 0, sizeof ee);
	ee.request_code = X_PolyText8;
	ee.error_code = BadDrawable;
	int r = xerror(NULL, &ee);
	ASSERT_EQ(r, 0, "xerror: BadDrawable on PolyText8 returns 0");
}

static void
test_xerror_badmatch_configure(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	XErrorEvent ee;
	memset(&ee, 0, sizeof ee);
	ee.request_code = X_ConfigureWindow;
	ee.error_code = BadMatch;
	int r = xerror(NULL, &ee);
	ASSERT_EQ(r, 0, "xerror: BadMatch on ConfigureWindow returns 0");
}

/* --- xerror: last three conditions (lines 2330-2332) --- */
static void
test_xerror_grabbutton_badaccess(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	XErrorEvent ee;
	memset(&ee, 0, sizeof ee);
	ee.request_code = X_GrabButton;
	ee.error_code = BadAccess;
	int r = xerror(NULL, &ee);
	ASSERT_EQ(r, 0, "xerror: BadAccess on GrabButton returns 0");
}

static void
test_xerror_grabkey_badaccess(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	XErrorEvent ee;
	memset(&ee, 0, sizeof ee);
	ee.request_code = X_GrabKey;
	ee.error_code = BadAccess;
	int r = xerror(NULL, &ee);
	ASSERT_EQ(r, 0, "xerror: BadAccess on GrabKey returns 0");
}

static void
test_xerror_copyarea_baddrawable(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	XErrorEvent ee;
	memset(&ee, 0, sizeof ee);
	ee.request_code = X_CopyArea;
	ee.error_code = BadDrawable;
	int r = xerror(NULL, &ee);
	ASSERT_EQ(r, 0, "xerror: BadDrawable on CopyArea returns 0");
}

static void
test_xerror_polyfill_baddrawable(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	XErrorEvent ee;
	memset(&ee, 0, sizeof ee);
	ee.request_code = X_PolyFillRectangle;
	ee.error_code = BadDrawable;
	int r = xerror(NULL, &ee);
	ASSERT_EQ(r, 0, "xerror: BadDrawable on PolyFillRectangle returns 0");
}

static void
test_xerror_polysegment_baddrawable(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	XErrorEvent ee;
	memset(&ee, 0, sizeof ee);
	ee.request_code = X_PolySegment;
	ee.error_code = BadDrawable;
	int r = xerror(NULL, &ee);
	ASSERT_EQ(r, 0, "xerror: BadDrawable on PolySegment returns 0");
}

static void
test_xerror_fatal_fallthrough(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_xerrorxlib_called = 0;
	xerrorxlib = mock_xerrorxlib_handler;
	XErrorEvent ee;
	memset(&ee, 0, sizeof ee);
	ee.request_code = 99;
	ee.error_code = 99;
	int r = xerror(NULL, &ee);
	ASSERT_EQ(r, 0, "xerror: fatal fallthrough calls xerrorxlib");
	ASSERT(mock_xerrorxlib_called, "xerror: xerrorxlib was invoked");
	xerrorxlib = NULL;
}

static void
run_tests(void)
{
	test_xerror_swallows_badwindow();
	test_xerror_swallows_badmatch();
	test_xerror_baddrawable();
	test_xerror_badmatch_configure();
	test_xerror_grabbutton_badaccess();
	test_xerror_grabkey_badaccess();
	test_xerror_copyarea_baddrawable();
	test_xerror_polyfill_baddrawable();
	test_xerror_polysegment_baddrawable();
	test_xerror_fatal_fallthrough();
}
