/*
 * test_monocle.c — per-function tests for dwm.c's monocle().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through monocle(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- monocle --- */
static void
test_monocle_no_clients(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor m = { .mx=0, .my=0, .mw=1920, .mh=1080, .wx=0, .wy=0, .ww=1920, .wh=1080,
		.topbar=1, .showbar=0, .lt = {&layouts[0], &layouts[0]} };
	m.clients = NULL;
	monocle(&m);
	ASSERT(1, "monocle: no clients no crash");
}

static void
test_monocle_one_client(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor m = { .mx=0, .my=0, .mw=1920, .mh=1080, .wx=0, .wy=0, .ww=1920, .wh=1080,
		.topbar=1, .showbar=0, .lt = {&layouts[0], &layouts[0]} };
	Client c = { .win=1, .mon=&m, .x=0, .y=0, .w=100, .h=100, .tags=1, .bw=0, .next=NULL };
	m.tagset[0] = 1;
	m.clients = &c;
	monocle(&m);
	ASSERT_EQ(c.x, 0, "monocle: client x set to mx");
}

static void
test_monocle_multiple_clients(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	m->mfact = 0.5f;
	m->nmaster = 1;
	m->lt[0] = &layouts[1]; /* monocle layout */
	m->lt[1] = &layouts[1];
	Client *c1 = make_client(1, m);
	Client *c2 = make_client(2, m);
	c1->next = c2;
	c1->tags = 1; c2->tags = 1;
	m->clients = c1;

	monocle(m);
	ASSERT(c1->w == m->ww - 2*c1->bw, "monocle: c1 fills width");
	ASSERT(c1->h == m->wh - 2*c1->bw, "monocle: c1 fills height");
	ASSERT(c2->w == m->ww - 2*c2->bw, "monocle: c2 fills width");
	ASSERT(c2->h == m->wh - 2*c2->bw, "monocle: c2 fills height");

	winclient_remove(c1);
	winclient_remove(c2);
	winclient_remove(c1);
	free(c1); free(c2); free(m);
}

static void
run_tests(void)
{
	test_monocle_no_clients();
	test_monocle_one_client();
	test_monocle_multiple_clients();
}
