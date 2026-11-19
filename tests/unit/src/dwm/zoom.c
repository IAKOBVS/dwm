/*
 * test_zoom.c — per-function tests for dwm.c's zoom().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through zoom(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- zoom --- */
static void
test_zoom_noop_no_sel(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	Monitor *m = make_monitor(0);
	m->sel = NULL;
	selmon = m;
	mons = m;
	Arg arg = {0};

	zoom(&arg);
	ASSERT(1, "zoom: noop when no sel");

	restore_selmon();
	free(m); /* fixture leaked (ASan-reported) */
}

static void
test_zoom_noop_floating(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->isfloating = 1;
	m->sel = c;
	m->clients = c;
	save_selmon();
	selmon = m;
	mons = m;
	Arg arg = {0};

	zoom(&arg);
	ASSERT(1, "zoom: noop on floating client");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- zoom second client --- */
static void
test_zoom_second_tiled(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c1 = make_client(1, m);
	Client *c2 = make_client(2, m);
	c1->tags = 1; c2->tags = 1;
	c1->next = c2;
	m->clients = c1;
	m->sel = c1;
	m->stack = c1; c1->snext = c2; c2->snext = NULL;
	save_selmon();
	selmon = m;
	mons = m;
	Arg arg = {0};

	zoom(&arg);
	ASSERT(selmon->sel == c2, "zoom: switches to second client when first is master");

	restore_selmon();
	winclient_remove(c1);
	winclient_remove(c2);
	winclient_remove(c1);
	free(c1); free(c2); free(m);
}

/* --- zoom with single tiled client (line 2361-2362) --- */
static void
test_zoom_single_client_returns(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->tags = 1;
	c->isfloating = 0;
	m->clients = c;
	m->stack = c;
	m->sel = c;
	save_selmon();
	selmon = m;
	mons = m;
	Arg arg = {0};

	/* c == nexttiled(m->clients) == c, nexttiled(c->next) == NULL */
	zoom(&arg);
	/* Should return early at line 2362, no crash, c stays in place */
	ASSERT(selmon->sel == c, "zoom single client: returns early, sel unchanged");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

static void
run_tests(void)
{
	test_zoom_noop_no_sel();
	test_zoom_noop_floating();
	test_zoom_second_tiled();
	test_zoom_single_client_returns();
}
