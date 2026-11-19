/*
 * test_tile.c — per-function tests for dwm.c's tile().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through tile(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- tile with multiple masters --- */
static void
test_tile_multiple_masters_gap(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	m->nmaster = 2;
	m->ww = 1920; m->wh = 1080;
	m->wx = 0; m->wy = 0;
	m->lt[0] = m->lt[1] = &layouts[0];
	Client *c1 = make_client(1, m);
	Client *c2 = make_client(2, m);
	Client *c3 = make_client(3, m);
	c1->tags = 1; c2->tags = 1; c3->tags = 1;
	c1->next = c2; c2->next = c3;
	c1->bw = c2->bw = c3->bw = 0;
	m->clients = c1;
	m->sel = c1;
	save_selmon();
	selmon = m;
	mons = m;

	tile(m);
	ASSERT(c1->y >= 0, "tile: first master positioned");
	ASSERT(c3->x > c2->x, "tile: stack client right of master");

	restore_selmon();
	winclient_remove(c1);
	winclient_remove(c2);
	winclient_remove(c3);
	winclient_remove(c1);
	winclient_remove(c2);
	free(c1); free(c2); free(c3); free(m);
}

/* --- tile with stack gap --- */
static void
test_tile_stack_gap(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	m->nmaster = 1;
	m->ww = 1920; m->wh = 1080;
	m->wx = 0; m->wy = 0;
	m->lt[0] = m->lt[1] = &layouts[0];
	Client *c1 = make_client(1, m);
	Client *c2 = make_client(2, m);
	Client *c3 = make_client(3, m);
	c1->tags = 1; c2->tags = 1; c3->tags = 1;
	c1->next = c2; c2->next = c3;
	c1->bw = c2->bw = c3->bw = 0;
	m->clients = c1;
	m->sel = c1;
	save_selmon();
	selmon = m;
	mons = m;

	tile(m);
	ASSERT(c2->y > 0, "tile: first stack client y > 0 with gap");

	restore_selmon();
	winclient_remove(c1);
	winclient_remove(c2);
	winclient_remove(c3);
	winclient_remove(c1);
	winclient_remove(c2);
	free(c1); free(c2); free(c3); free(m);
}

/* --- tile with no tiled clients (line 1756-1757) --- */
static void
test_tile_early_return(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	m->clients = NULL;
	tile(m);
	/* n==0, early return at line 1756 */
	ASSERT(1, "tile early return: no clients does not crash");
	free(m);
}

static void
test_tile_nmaster_gt_n(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor m = { .mx=0, .my=0, .mw=1920, .mh=1080, .wx=0, .wy=0, .ww=1920, .wh=1080,
		.nmaster=3, .mfact=0.5f, .lt = {&layouts[0], &layouts[0]} };
	m.gap.isgap = 1;
	m.gap.realgap = 0;
	m.gap.gappx = 0;

	Client c1 = { .win=1, .mon=&m, .tags=1, .bw=0, .w=100, .h=100 };
	Client c2 = { .win=2, .mon=&m, .tags=1, .bw=0, .next=&c1, .w=100, .h=100 };
	m.clients = &c2; c2.next = &c1; c1.next = NULL;

	tile(&m);
	ASSERT(c1.w > 0, "tile: nmaster>n, c1 has positive width");
	ASSERT(c2.w > 0, "tile: nmaster>n, c2 has positive width");

}

static void
run_tests(void)
{
	test_tile_multiple_masters_gap();
	test_tile_stack_gap();
	test_tile_early_return();
	test_tile_nmaster_gt_n();
}
