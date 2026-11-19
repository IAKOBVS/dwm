/*
 * test_applysizehints.c — per-function tests for dwm.c's applysizehints().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through applysizehints(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- applysizehints --- */
static void
test_applysizehints_min_size(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor m = { .mx=0, .my=0, .mw=1920, .mh=1080, .wx=0, .wy=0, .ww=1920, .wh=1080,
		.lt = {&layouts[0], &layouts[0]} };
	Client c = { .win=1, .mon=&m, .bw=0, .x=0, .y=0, .w=10, .h=10,
		.minw=50, .minh=50, .maxw=0, .maxh=0, .incw=0, .inch=0,
		.mina=0, .maxa=0, .basew=50, .baseh=50, .mina=0, .maxa=0,
		.isfloating=0, .oldw=200, .oldh=200, .hintsvalid=1 };
	int x=c.x, y=c.y, w=c.w, h=c.h;
	int r = applysizehints(&c, &x, &y, &w, &h, 1);
	ASSERT(r, "applysizehints: returns true when changed (resizehints=1)");
	ASSERT(w >= 50, "applysizehints: width clamped to minw");
	ASSERT(h >= 50, "applysizehints: height clamped to minh");
}

static void
test_applysizehints_max_size(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor m = { .mx=0, .my=0, .mw=1920, .mh=1080, .wx=0, .wy=0, .ww=1920, .wh=1080,
		.lt = {&layouts[0], &layouts[0]} };
	Client c = { .win=1, .mon=&m, .bw=0, .x=0, .y=0, .w=3000, .h=3000,
		.minw=50, .minh=50, .maxw=1920, .maxh=1080, .incw=0, .inch=0,
		.mina=0, .maxa=0, .basew=50, .baseh=50, .mina=0, .maxa=0,
		.isfloating=0, .oldw=200, .oldh=200, .hintsvalid=1 };
	int x=c.x, y=c.y, w=c.w, h=c.h;
	int r = applysizehints(&c, &x, &y, &w, &h, 1);
	ASSERT(r, "applysizehints(max): returns true (resizehints=1)");
	ASSERT(w <= 1920, "applysizehints: width clamped to maxw");
	ASSERT(h <= 1080, "applysizehints: height clamped to maxh");
}

static void
test_applysizehints_resizehints_flag(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor m = { .mx=0, .my=0, .mw=1920, .mh=1080, .wx=0, .wy=0, .ww=1920, .wh=1080,
		.lt = {&layouts[0], &layouts[0]} };
	Client c = { .win=1, .mon=&m, .bw=0, .x=0, .y=0, .w=100, .h=100,
		.minw=50, .minh=50, .maxw=0, .maxh=0, .incw=0, .inch=0,
		.mina=0, .maxa=0, .basew=50, .baseh=50, .mina=0, .maxa=0,
		.isfloating=0, .oldw=100, .oldh=100, .hintsvalid=1 };
	int x=c.x, y=c.y, w=c.w, h=c.h;
	int r = applysizehints(&c, &x, &y, &w, &h, 1);
	ASSERT(!r, "applysizehints: resizehints=1, no change -> false");
}

/* --- applysizehints triggers updatesizehints --- */
static void
test_applysizehints_updatesizehints(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor m = { .mx=0, .my=0, .mw=1920, .mh=1080, .wx=0, .wy=0, .ww=1920, .wh=1080,
		.lt = {&layouts[0], &layouts[0]} };
	Client c = { .win=1, .mon=&m, .bw=0, .x=0, .y=0, .w=200, .h=200,
		.minw=0, .minh=0, .maxw=0, .maxh=0, .incw=0, .inch=0,
		.basew=0, .baseh=0, .mina=0, .maxa=0,
		.isfloating=0, .oldw=200, .oldh=200, .hintsvalid=0 };
	int x=c.x, y=c.y, w=c.w, h=c.h;

	/* Set mock to return PSize only (no base/min/max) */
	mock_x11_reset();
	mock_normal_hints_flags = PSize;

	int r = applysizehints(&c, &x, &y, &w, &h, 1);
	ASSERT(!r, "updatesizehints: PSize, no change -> false");
	ASSERT(c.hintsvalid, "updatesizehints: marks hintsvalid");
	mock_x11_reset();
}

/* --- applysizehints aspect/increment/base --- */
static void
test_applysizehints_aspect(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor m = { .mx=0, .my=0, .mw=1920, .mh=1080, .wx=0, .wy=0, .ww=1920, .wh=1080,
		.lt = {&layouts[0], &layouts[0]} };
	Client c = { .win=1, .mon=&m, .bw=0, .x=0, .y=0, .w=400, .h=100,
		.minw=50, .minh=50, .maxw=0, .maxh=0, .incw=0, .inch=0,
		.basew=50, .baseh=50, .mina=1.0, .maxa=2.0,
		.isfloating=0, .oldw=200, .oldh=200, .hintsvalid=1 };
	int x=c.x, y=c.y, w=c.w, h=c.h;
	/* w/h is 400/100 = 4.0. maxa = 2.0, so maxa < w/h -> w = h * maxa = 100 * 2.0 = 200 */
	int r = applysizehints(&c, &x, &y, &w, &h, 0);
	ASSERT(r, "applysizehints aspect: changed");
	ASSERT(w <= 210, "applysizehints aspect: width clamped by max aspect");
}

static void
test_applysizehints_increment(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor m = { .mx=0, .my=0, .mw=1920, .mh=1080, .wx=0, .wy=0, .ww=1920, .wh=1080,
		.lt = {&layouts[0], &layouts[0]} };
	Client c = { .win=1, .mon=&m, .bw=0, .x=0, .y=0, .w=203, .h=105,
		.minw=0, .minh=0, .maxw=0, .maxh=0, .incw=10, .inch=12,
		.basew=50, .baseh=50, .mina=0, .maxa=0,
		.isfloating=0, .oldw=200, .oldh=200, .hintsvalid=1 };
	int x=c.x, y=c.y, w=c.w, h=c.h;
	/* baseismin=true (base=min), so first subtract base, then inc adjustment, then add base back */
	/* w=203, base=50, min=0 => baseismin=true */
	int r = applysizehints(&c, &x, &y, &w, &h, 0);
	ASSERT(r, "applysizehints increment: changed");
	/* w after inc: (203-50) - ((203-50) % 10) = 153 - 3 = 150, +50 = 200 */
	ASSERT_EQ(w, 200, "applysizehints increment: w snapped to increment");
	/* h after inc: (105-50) - ((105-50) % 12) = 55 - 7 = 48, +50 = 98 */
	ASSERT_EQ(h, 98, "applysizehints increment: h snapped to increment");
}

static void
test_applysizehints_nochange_interact(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor m = { .mx=0, .my=0, .mw=1920, .mh=1080, .wx=0, .wy=0, .ww=1920, .wh=1080,
		.lt = {&layouts[0], &layouts[0]} };
	Client c = { .win=1, .mon=&m, .bw=0, .x=100, .y=100, .w=200, .h=200,
		.minw=50, .minh=50, .maxw=0, .maxh=0, .incw=0, .inch=0,
		.basew=50, .baseh=50, .mina=0, .maxa=0,
		.isfloating=0, .oldw=200, .oldh=200, .hintsvalid=1 };
	int x=c.x, y=c.y, w=c.w, h=c.h;
	int r = applysizehints(&c, &x, &y, &w, &h, 1);
	ASSERT(!r, "applysizehints nochange interact: false");
}

/* --- applysizehints interact boundary clamping --- */
static void
test_applysizehints_interact_clamp(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor m = { .mx=0, .my=0, .mw=1920, .mh=1080, .wx=0, .wy=0, .ww=1920, .wh=1080,
		.lt = {&layouts[0], &layouts[0]} };
	Client c = { .win=1, .mon=&m, .bw=0, .x=3000, .y=2000, .w=200, .h=200,
		.minw=50, .minh=50, .maxw=0, .maxh=0, .incw=0, .inch=0,
		.basew=50, .baseh=50, .mina=0, .maxa=0,
		.isfloating=0, .oldw=200, .oldh=200, .hintsvalid=1 };
	int x=c.x, y=c.y, w=c.w, h=c.h;
	int r = applysizehints(&c, &x, &y, &w, &h, 1);
	/* x > sw (1920) so x = sw - WIDTH(c) = 1920 - 200 = 1720 */
	ASSERT(x < 1920, "applysizehints interact: x clamped to screen width");
	/* y > sh (1080) so y = sh - HEIGHT(c) = 1080 - 200 = 880 */
	ASSERT(y < 1080, "applysizehints interact: y clamped to screen height");
	ASSERT(r, "applysizehints interact: changed");
}

static void
test_applysizehints_interact_negative(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor m = { .mx=0, .my=0, .mw=1920, .mh=1080, .wx=0, .wy=0, .ww=1920, .wh=1080,
		.lt = {&layouts[0], &layouts[0]} };
	Client c = { .win=1, .mon=&m, .bw=0, .x=-100, .y=-100, .w=200, .h=200,
		.minw=50, .minh=50, .maxw=0, .maxh=0, .incw=0, .inch=0,
		.basew=50, .baseh=50, .mina=0, .maxa=0,
		.isfloating=0, .oldw=200, .oldh=200, .hintsvalid=1 };
	int x=c.x, y=c.y, w=c.w, h=c.h;
	/* x + w + 2*bw = -100 + 200 + 0 = 100 > 0, so not clamped by that check */
	/* But y + h + 2*bw = -100 + 200 + 0 = 100 > 0, so not clamped */
	/* x,y < 0 but not clamped by simple boundary check... actually these are */
	/* The code checks if *x + *w + 2*c->bw < 0 then *x=0.  -100 + 200 = 100 >= 0 */
	/* So not clamped. The function still works. */
	int r = applysizehints(&c, &x, &y, &w, &h, 1);
	ASSERT(!r, "applysizehints interact negative: no change (x+w >= 0)");
}

/* --- applysizehints interact negative far (lines 134, 136) --- */
static void
test_applysizehints_interact_negative_far(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor m = { .mx=0, .my=0, .mw=1920, .mh=1080, .wx=0, .wy=0, .ww=1920, .wh=1080,
		.lt = {&layouts[0], &layouts[0]} };
	/* x=-500, w=100, bw=0 → x+w+2*bw = -400 < 0 → *x = 0 */
	Client c = { .win=1, .mon=&m, .bw=0, .x=-500, .y=-500, .w=100, .h=100,
		.minw=50, .minh=50, .maxw=0, .maxh=0, .incw=0, .inch=0,
		.basew=50, .baseh=50, .mina=0, .maxa=0,
		.isfloating=0, .oldw=100, .oldh=100, .hintsvalid=1 };
	int x=c.x, y=c.y, w=c.w, h=c.h;
	applysizehints(&c, &x, &y, &w, &h, 1);
	ASSERT_EQ(x, 0, "applysizehints interact neg far: x clamped to 0");
	ASSERT_EQ(y, 0, "applysizehints interact neg far: y clamped to 0");
}

/* --- applysizehints non-interact boundary clamping (lines 138-145) --- */
static void
test_applysizehints_noninteract_clamp(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor m = { .mx=0, .my=0, .mw=1920, .mh=1080, .wx=0, .wy=0, .ww=1920, .wh=1080,
		.lt = {&layouts[0], &layouts[0]} };
	/* Test right/bottom overflow: x=2000 >= wx+ww=1920 → x = 1920 - WIDTH(c) */
	Client c1 = { .win=1, .mon=&m, .bw=0, .x=2000, .y=2000, .w=200, .h=200,
		.minw=50, .minh=50, .maxw=0, .maxh=0, .incw=0, .inch=0,
		.basew=50, .baseh=50, .mina=0, .maxa=0,
		.isfloating=0, .oldw=200, .oldh=200, .hintsvalid=1 };
	int x1=c1.x, y1=c1.y, w1=c1.w, h1=c1.h;
	applysizehints(&c1, &x1, &y1, &w1, &h1, 0);
	/* x clamped: 1920 - WIDTH(c1) = 1920 - 200 = 1720 */
	ASSERT_EQ(x1, 1720, "applysizehints noninteract: x clamped to right edge");
	ASSERT_EQ(y1, 880, "applysizehints noninteract: y clamped to bottom edge");

	/* Test left/top overflow: x+w+2*bw <= wx → x = wx */
	Client c2 = { .win=2, .mon=&m, .bw=0, .x=-300, .y=-300, .w=100, .h=100,
		.minw=50, .minh=50, .maxw=0, .maxh=0, .incw=0, .inch=0,
		.basew=50, .baseh=50, .mina=0, .maxa=0,
		.isfloating=0, .oldw=100, .oldh=100, .hintsvalid=1 };
	int x2=c2.x, y2=c2.y, w2=c2.w, h2=c2.h;
	applysizehints(&c2, &x2, &y2, &w2, &h2, 0);
	ASSERT_EQ(x2, 0, "applysizehints noninteract: x clamped to left edge");
	ASSERT_EQ(y2, 0, "applysizehints noninteract: y clamped to top edge");
}

/* --- applysizehints aspect mina branch (lines 164-165) --- */
static void
test_applysizehints_aspect_mina(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor m = { .mx=0, .my=0, .mw=1920, .mh=1080, .wx=0, .wy=0, .ww=1920, .wh=1080,
		.lt = {&layouts[0], &layouts[0]} };
	/* mina=0.5, maxa=2.0, w=100, h=300
	 * maxa(2.0) < w/h(0.333)? No.
	 * mina(0.5) < h/w(3.0)? Yes → h = w * mina + 0.5 = 50.5 → 50 */
	Client c = { .win=1, .mon=&m, .bw=0, .x=0, .y=0, .w=100, .h=300,
		.minw=0, .minh=0, .maxw=0, .maxh=0, .incw=0, .inch=0,
		.basew=0, .baseh=0, .mina=0.5, .maxa=2.0,
		.isfloating=0, .oldw=100, .oldh=300, .hintsvalid=1 };
	int x=c.x, y=c.y, w=c.w, h=c.h;
	int r = applysizehints(&c, &x, &y, &w, &h, 0);
	ASSERT(r, "applysizehints aspect mina: changed");
	ASSERT_EQ(h, 50, "applysizehints aspect mina: h clamped by min aspect");
}

/* --- edge case tests --- */
static void
test_applysizehints_incw_zero(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor m = { .mx=0, .my=0, .mw=1920, .mh=1080, .wx=0, .wy=0, .ww=1920, .wh=1080,
		.lt = {&layouts[0], &layouts[0]} };
	/* incw=0, inch=0 → inc adjustment skipped; mina=maxa=0 → aspect skipped */
	Client c = { .win=1, .mon=&m, .bw=0, .x=0, .y=0, .w=100, .h=100,
		.minw=50, .minh=50, .maxw=0, .maxh=0, .incw=0, .inch=0,
		.mina=0, .maxa=0, .basew=50, .baseh=50,
		.isfloating=1, .oldw=100, .oldh=100, .hintsvalid=1 };
	int x=c.x, y=c.y, w=c.w, h=c.h;
	int r = applysizehints(&c, &x, &y, &w, &h, 1);
	ASSERT(r == 0, "applysizehints: incw=0/mina=0 returns false (no change)");
	ASSERT_EQ(w, 100, "applysizehints: incw=0 preserves width");
	ASSERT_EQ(h, 100, "applysizehints: incw=0 preserves height");
}

static void
test_applysizehints_baseismin_false(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor m = { .mx=0, .my=0, .mw=1920, .mh=1080, .wx=0, .wy=0, .ww=1920, .wh=1080,
		.lt = {&layouts[0], &layouts[0]} };
	/* basew=30 != minw=50 → baseismin=false → subtract base, then restore */
	Client c = { .win=1, .mon=&m, .bw=0, .x=0, .y=0, .w=100, .h=100,
		.minw=50, .minh=50, .maxw=0, .maxh=0, .incw=0, .inch=0,
		.mina=0, .maxa=0, .basew=30, .baseh=30,
		.isfloating=1, .oldw=100, .oldh=100, .hintsvalid=1 };
	int x=c.x, y=c.y, w=c.w, h=c.h;
	int r = applysizehints(&c, &x, &y, &w, &h, 1);
	/* baseismin=false path: no change to w/h since base+min align after restore.
	 * The path is exercised because basew(30) != minw(50) → !baseismin branch taken. */
	ASSERT(!r, "applysizehints: baseismin=false returns false (w=100 unchanged after restore)");
	ASSERT_EQ(w, 100, "applysizehints: baseismin=false w=100 (70-30+MIN(70+30,50))");
	ASSERT_EQ(h, 100, "applysizehints: baseismin=false h=100");
}

/* Deterministic PRNG (xorshift32) for the invariant fuzz case. */
static unsigned int
fuzz_rand(unsigned int *state)
{
	unsigned int x = *state;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	return (*state = x);
}

/* --- randomized invariants: bounds, clamping and idempotence --- */
static void
test_applysizehints_random_invariants(void)
{
	unsigned int seed = 0xC0FFEEu;
	unsigned int iter;

	for (iter = 0; iter < 500; iter++) {
		Monitor *m = make_monitor(0);
		Client *c = make_client(1, m);
		int x, y, w, h, x2, y2, w2, h2;

		c->isfloating = 1;
		c->hintsvalid = 1;
		c->bw = (int)(fuzz_rand(&seed) % 4);
		c->minw = 1 + (int)(fuzz_rand(&seed) % 80);
		c->minh = 1 + (int)(fuzz_rand(&seed) % 80);
		c->maxw = (fuzz_rand(&seed) & 1) ? 0 : c->minw + (int)(fuzz_rand(&seed) % 400);
		c->maxh = (fuzz_rand(&seed) & 1) ? 0 : c->minh + (int)(fuzz_rand(&seed) % 400);
		c->basew = (int)(fuzz_rand(&seed) % (unsigned int)(c->minw + 1));
		c->baseh = (int)(fuzz_rand(&seed) % (unsigned int)(c->minh + 1));
		c->incw = (int)(fuzz_rand(&seed) % 11); /* 0..10 */
		c->inch = (int)(fuzz_rand(&seed) % 11);
		c->mina = c->maxa = 0.0f;
		if ((fuzz_rand(&seed) % 4) == 0) {
			c->mina = 0.5f + (float)(fuzz_rand(&seed) % 100) / 100.0f;
			c->maxa = c->mina + (float)(fuzz_rand(&seed) % 100) / 100.0f;
		}

		x = (int)(fuzz_rand(&seed) % 5000) - 2000;
		y = (int)(fuzz_rand(&seed) % 5000) - 2000;
		w = 1 + (int)(fuzz_rand(&seed) % 3000);
		h = 1 + (int)(fuzz_rand(&seed) % 3000);

		applysizehints(c, &x, &y, &w, &h, 0);

		ASSERT(w >= 1 && h >= 1, "fuzz: size stays positive");
		ASSERT(w >= c->minw, "fuzz: width respects minw");
		ASSERT(h >= c->minh, "fuzz: height respects minh");
		if (c->maxw)
			ASSERT(w <= c->maxw, "fuzz: width respects maxw");
		if (c->maxh)
			ASSERT(h <= c->maxh, "fuzz: height respects maxh");
		/* Partially off-screen rectangles are legal; only fully off-screen
		 * ones are pulled back inside the monitor's right/bottom edge. */
		ASSERT(x < m->wx + m->ww, "fuzz: x stays left of the monitor edge");
		ASSERT(y < m->wy + m->wh, "fuzz: y stays above the monitor edge");

		/* Re-clamping is stable without aspect rounding or max-size
		 * truncation (both legitimately drift per application). */
		if (c->mina == 0.0f && c->maxa == 0.0f
		&& c->maxw == 0 && c->maxh == 0) {
			x2 = x; y2 = y; w2 = w; h2 = h;
			applysizehints(c, &x2, &y2, &w2, &h2, 0);
			ASSERT_EQ(x2, x, "fuzz: x is idempotent");
			ASSERT_EQ(y2, y, "fuzz: y is idempotent");
			ASSERT_EQ(w2, w, "fuzz: width is idempotent");
			ASSERT_EQ(h2, h, "fuzz: height is idempotent");
		}

		winclient_remove(c);
		free(c);
		free(m);
	}
}

static void
run_tests(void)
{
	test_applysizehints_min_size();
	test_applysizehints_max_size();
	test_applysizehints_resizehints_flag();
	test_applysizehints_updatesizehints();
	test_applysizehints_aspect();
	test_applysizehints_increment();
	test_applysizehints_nochange_interact();
	test_applysizehints_interact_clamp();
	test_applysizehints_interact_negative();
	test_applysizehints_interact_negative_far();
	test_applysizehints_noninteract_clamp();
	test_applysizehints_aspect_mina();
	test_applysizehints_incw_zero();
	test_applysizehints_baseismin_false();
	test_applysizehints_random_invariants();
}
