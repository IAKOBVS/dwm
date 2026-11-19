/*
 * test_resizeclient.c — per-function tests for dwm.c's resizeclient().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through resizeclient(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- resizeclient centering --- */
static void
test_resizeclient_stores_values(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor m = { .mx=100, .my=50, .mw=1800, .mh=1000, .wx=100, .wy=80, .ww=1800, .wh=1000,
		.lt = {&layouts[0], &layouts[0]} };
	Client c = { .win=1, .mon=&m, .bw=0, .x=100, .y=100, .w=200, .h=200,
		.oldw=200, .oldh=200, .basew=50, .baseh=50, .hintsvalid=0 };
	resizeclient(&c, 50, 60, 300, 400);
	ASSERT_EQ(c.x, 50, "resizeclient: stores x");
	ASSERT_EQ(c.y, 60, "resizeclient: stores y");
	ASSERT_EQ(c.w, 300, "resizeclient: stores w");
	ASSERT_EQ(c.h, 400, "resizeclient: stores h");
}

/* --- resizeclient with hintsvalid --- */
static void
test_resizeclient_bw_stored(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor m = { .mx=0, .my=0, .mw=1920, .mh=1080, .wx=0, .wy=0, .ww=1920, .wh=1080,
		.lt = {&layouts[0], &layouts[0]} };
	Client c = { .win=1, .mon=&m, .bw=2, .x=10, .y=10, .w=100, .h=100,
		.oldw=100, .oldh=100, .basew=50, .baseh=50, .minw=50, .minh=50,
		.incw=0, .inch=0, .mina=0, .maxa=0, .hintsvalid=1 };
	resizeclient(&c, 0, 0, 2000, 2000);
	ASSERT_EQ(c.x, 0, "resizeclient: stores x=0");
	ASSERT_EQ(c.y, 0, "resizeclient: stores y=0");
	ASSERT_EQ(c.w, 2000, "resizeclient: stores w=2000");
	ASSERT_EQ(c.h, 2000, "resizeclient: stores h=2000");
}

static void
run_tests(void)
{
	test_resizeclient_stores_values();
	test_resizeclient_bw_stored();
}
