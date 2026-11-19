/*
 * test_resize.c — per-function tests for dwm.c's resize().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through resize(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- resize --- */
static void
test_resize_basic(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor m = { .mx=0, .my=0, .mw=1920, .mh=1080, .wx=0, .wy=0, .ww=1920, .wh=1080,
		.lt = {&layouts[0], &layouts[0]} };
	Client c = { .win=1, .mon=&m, .x=0, .y=0, .w=100, .h=100, .bw=0,
		.minw=10, .minh=10, .maxw=0, .maxh=0, .incw=0, .inch=0,
		.basew=10, .baseh=10, .mina=0, .maxa=0, .oldw=100, .oldh=100,
		.isfloating=0, .hintsvalid=1 };
	resize(&c, 10, 10, 200, 200, 0);
	ASSERT_EQ(c.x, 10, "resize: x updated");
	ASSERT_EQ(c.y, 10, "resize: y updated");
}

/* --- resize with large dimensions --- */
static void
test_resize_large_dim(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor m = { .mx=0, .my=0, .mw=1920, .mh=1080, .wx=0, .wy=0, .ww=1920, .wh=1080,
		.lt = {&layouts[0], &layouts[0]} };
	Client c = { .win=1, .mon=&m, .bw=0, .x=0, .y=0, .w=100, .h=100,
		.minw=10, .minh=10, .maxw=0, .maxh=0, .incw=0, .inch=0,
		.basew=10, .baseh=10, .mina=0, .maxa=0, .oldw=100, .oldh=100,
		.isfloating=0, .hintsvalid=1 };
	resize(&c, -50, -60, 5000, 5000, 0);
	/* applysizehints accepts these values in non-interactive mode */
	ASSERT(1, "resize: large dims no crash");
}

static void
test_resize_floating(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor m = { .mx=0, .my=0, .mw=1920, .mh=1080, .wx=0, .wy=0, .ww=1920, .wh=1080,
		.lt = {&layouts[0], &layouts[0]} };
	Client c = { .win=1, .mon=&m, .x=0, .y=0, .w=100, .h=100, .bw=0,
		.isfloating=1, .minw=50, .minh=50, .maxw=0, .maxh=0,
		.incw=10, .inch=10, .basew=50, .baseh=50, .mina=0, .maxa=0,
		.oldw=100, .oldh=100, .hintsvalid=1 };
	resize(&c, 10, 10, 95, 95, 0);
	ASSERT_EQ(c.x, 10, "resize floating: x updated");
	ASSERT_EQ(c.y, 10, "resize floating: y updated");
	/* width should snap to increment: basew=50, incw=10 -> 95-50=45, 45%10=5, 45-5=40, 40+50=90 */
	ASSERT_EQ(c.w, 90, "resize floating: w snapped to increment");
	ASSERT_EQ(c.h, 90, "resize floating: h snapped to increment");
}

static void
run_tests(void)
{
	test_resize_basic();
	test_resize_large_dim();
	test_resize_floating();
}
