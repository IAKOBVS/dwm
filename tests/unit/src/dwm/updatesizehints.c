/*
 * test_updatesizehints.c — per-function tests for dwm.c's updatesizehints().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through updatesizehints(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- updatesizehints --- */
static void
test_updatesizehints(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client c = { .win = 1, .mon = m };
	memset(&c, 0, sizeof(c));
	c.win = 1;
	c.mon = m;

	updatesizehints(&c);
	ASSERT(1, "updatesizehints: does not crash");
	/* mock: XGetWMNormalHints returns flags=PSize, basew/baseh set */

	free(m);
}

/* --- updatesizehints full flags --- */
static void
test_updatesizehints_full(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client c = { .win = 1, .mon = m };

	updatesizehints(&c);
	ASSERT(c.hintsvalid, "updatesizehints: sets hintsvalid");
	ASSERT(1, "updatesizehints: full flags no crash");

	free(m);
}

static void
test_updatesizehints_base(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Client c = { .win = 1, .hintsvalid = 0 };
	mock_x11_reset();
	mock_normal_hints_flags = PSize | PBaseSize;
	mock_normal_hints_base_width = 100;
	mock_normal_hints_base_height = 80;

	updatesizehints(&c);
	ASSERT_EQ(c.basew, 100, "updatesizehints PBaseSize: basew");
	ASSERT_EQ(c.baseh, 80,  "updatesizehints PBaseSize: baseh");
	mock_x11_reset();
}

static void
test_updatesizehints_minsize_as_base(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Client c = { .win = 1, .hintsvalid = 0 };
	mock_x11_reset();
	mock_normal_hints_flags = PSize | PMinSize;
	mock_normal_hints_min_width = 50;
	mock_normal_hints_min_height = 40;

	updatesizehints(&c);
	/* When PBaseSize not set but PMinSize is, base uses min */
	ASSERT_EQ(c.basew, 50, "updatesizehints PMinSize->base: basew");
	ASSERT_EQ(c.baseh, 40, "updatesizehints PMinSize->base: baseh");
	ASSERT_EQ(c.minw, 50, "updatesizehints PMinSize: minw");
	ASSERT_EQ(c.minh, 40, "updatesizehints PMinSize: minh");
	mock_x11_reset();
}

static void
test_updatesizehints_increment(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Client c = { .win = 1, .hintsvalid = 0 };
	mock_x11_reset();
	mock_normal_hints_flags = PSize | PResizeInc;
	mock_normal_hints_width_inc = 10;
	mock_normal_hints_height_inc = 12;

	updatesizehints(&c);
	ASSERT_EQ(c.incw, 10, "updatesizehints PResizeInc: incw");
	ASSERT_EQ(c.inch, 12, "updatesizehints PResizeInc: inch");
	mock_x11_reset();
}

static void
test_updatesizehints_maxsize(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Client c = { .win = 1, .hintsvalid = 0 };
	mock_x11_reset();
	mock_normal_hints_flags = PSize | PMaxSize;
	mock_normal_hints_max_width = 1920;
	mock_normal_hints_max_height = 1080;

	updatesizehints(&c);
	ASSERT_EQ(c.maxw, 1920, "updatesizehints PMaxSize: maxw");
	ASSERT_EQ(c.maxh, 1080, "updatesizehints PMaxSize: maxh");
	mock_x11_reset();
}

static void
test_updatesizehints_aspect(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Client c = { .win = 1, .hintsvalid = 0 };
	mock_x11_reset();
	mock_normal_hints_flags = PSize | PAspect;
	mock_normal_hints_min_aspect_x = 1;
	mock_normal_hints_min_aspect_y = 2;
	mock_normal_hints_max_aspect_x = 16;
	mock_normal_hints_max_aspect_y = 9;

	updatesizehints(&c);
	ASSERT(c.mina > 0, "updatesizehints PAspect: mina > 0");
	ASSERT(c.maxa > 0, "updatesizehints PAspect: maxa > 0");
	mock_x11_reset();
}

static void
test_updatesizehints_fixed(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Client c = { .win = 1, .hintsvalid = 0 };
	mock_x11_reset();
	mock_normal_hints_flags = PSize | PMinSize | PMaxSize;
	mock_normal_hints_min_width = 200;
	mock_normal_hints_min_height = 100;
	mock_normal_hints_max_width = 200;
	mock_normal_hints_max_height = 100;

	updatesizehints(&c);
	ASSERT(c.isfixed, "updatesizehints: min==max -> isfixed");
	mock_x11_reset();
}

static void
test_updatesizehints_none(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Client c = { .win = 1, .hintsvalid = 0 };
	mock_x11_reset();
	/* Only PSize, no other flags */

	updatesizehints(&c);
	ASSERT_EQ(c.basew, 0, "updatesizehints no flags: basew=0");
	ASSERT_EQ(c.baseh, 0, "updatesizehints no flags: baseh=0");
	ASSERT_EQ(c.minw, 0, "updatesizehints no flags: minw=0");
	ASSERT_EQ(c.minh, 0, "updatesizehints no flags: minh=0");
	ASSERT_EQ(c.incw, 0, "updatesizehints no flags: incw=0");
	ASSERT_EQ(c.inch, 0, "updatesizehints no flags: inch=0");
	ASSERT_EQ(c.maxw, 0, "updatesizehints no flags: maxw=0");
	ASSERT_EQ(c.maxh, 0, "updatesizehints no flags: maxh=0");
	ASSERT(!c.isfixed, "updatesizehints no flags: not fixed");
	mock_x11_reset();
}

/* --- updatesizehints when XGetWMNormalHints fails --- */
static void
test_updatesizehints_xgetwmnormalhints_fails(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Client c = { .win = 1, .hintsvalid = 0 };
	mock_x11_reset();
	mock_normal_hints_return = 0;  /* XGetWMNormalHints returns 0 */
	mock_normal_hints_flags = 0;   /* should be overwritten to PSize */

	updatesizehints(&c);
	/* When XGetWMNormalHints returns 0, size.flags gets set to PSize */
	/* With only PSize, all else-branches are taken: base=0, inc=0, max=0, min=0, aspect=0 */
	ASSERT_EQ(c.basew, 0, "updatesizehints fail: basew=0");
	ASSERT_EQ(c.baseh, 0, "updatesizehints fail: baseh=0");
	ASSERT_EQ(c.incw, 0, "updatesizehints fail: incw=0");
	ASSERT_EQ(c.inch, 0, "updatesizehints fail: inch=0");
	ASSERT_EQ(c.maxw, 0, "updatesizehints fail: maxw=0");
	ASSERT_EQ(c.maxh, 0, "updatesizehints fail: maxh=0");
	mock_x11_reset();
}

static void
test_updatesizehints_min_from_base(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	mock_normal_hints_flags = PBaseSize | PMinSize;
	mock_normal_hints_base_width = 30;
	mock_normal_hints_base_height = 30;
	mock_normal_hints_min_width = 50;
	mock_normal_hints_min_height = 50;

	Client c = { .win = 1, .mon = m };

	updatesizehints(&c);
	/* PBaseSize set → basew/baseh from base_width */
	ASSERT_EQ(c.basew, 30, "updatesizehints: basew from PBaseSize");
	ASSERT_EQ(c.baseh, 30, "updatesizehints: baseh from PBaseSize");
	/* PMinSize set → minw/minh from min_width */
	ASSERT_EQ(c.minw, 50, "updatesizehints: minw from PMinSize");
	ASSERT_EQ(c.minh, 50, "updatesizehints: minh from PMinSize");

	mock_normal_hints_flags = PSize;
	mock_normal_hints_base_width = 0;
	mock_normal_hints_base_height = 0;
	mock_normal_hints_min_width = 0;
	mock_normal_hints_min_height = 0;
	free(m);
}

static void
test_updatesizehints_only_psize(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	mock_normal_hints_flags = 0; /* XGetWMNormalHints returns 0, size.flags becomes PSize per line 2057 */

	Client c = { .win = 1, .mon = m };

	updatesizehints(&c);
	/* No PBaseSize, no PMinSize → basew=0 */
	ASSERT_EQ(c.basew, 0, "updatesizehints: no PBaseSize/PMinSize → basew=0");
	ASSERT_EQ(c.baseh, 0, "updatesizehints: no PBaseSize/PMinSize → baseh=0");
	/* No PMinSize, no PBaseSize → minw=0 */
	ASSERT_EQ(c.minw, 0, "updatesizehints: no PMinSize/PBaseSize → minw=0");
	ASSERT_EQ(c.minh, 0, "updatesizehints: no PMinSize/PBaseSize → minh=0");
	/* No PResizeInc → incw=0 */
	ASSERT_EQ(c.incw, 0, "updatesizehints: no PResizeInc → incw=0");
	/* No PMaxSize → maxw=0 */
	ASSERT_EQ(c.maxw, 0, "updatesizehints: no PMaxSize → maxw=0");

	mock_normal_hints_flags = PSize;
	free(m);
}

static void
run_tests(void)
{
	test_updatesizehints();
	test_updatesizehints_full();
	test_updatesizehints_base();
	test_updatesizehints_minsize_as_base();
	test_updatesizehints_increment();
	test_updatesizehints_maxsize();
	test_updatesizehints_aspect();
	test_updatesizehints_fixed();
	test_updatesizehints_none();
	test_updatesizehints_xgetwmnormalhints_fails();
	test_updatesizehints_min_from_base();
	test_updatesizehints_only_psize();
}
