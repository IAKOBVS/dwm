/*
 * test_focusstack.c — per-function tests for dwm.c's focusstack().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through focusstack(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- focusstack --- */
static void
test_focusstack_forward(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c1 = make_client(1, m);
	Client *c2 = make_client(2, m);
	c1->tags = 1; c2->tags = 1;
	m->clients = c1; c1->next = c2;
	m->stack = c2; c2->snext = c1; c1->snext = NULL;
	m->sel = c1;
	save_selmon();
	selmon = m;
	mons = m;
	Arg arg = { .i = 1 };

	focusstack(&arg);
	ASSERT(selmon->sel == c2, "focusstack: forward to c2");

	restore_selmon();
	free(c1); /* fixture leaked (ASan-reported) */
	free(c2); /* fixture leaked (ASan-reported) */
	free(m); /* fixture leaked (ASan-reported) */
}

/* --- focusstack reverse --- */
static void
test_focusstack_reverse(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c1 = make_client(1, m);
	Client *c2 = make_client(2, m);
	c1->tags = 1; c2->tags = 1;
	m->clients = c1; c1->next = c2;
	m->stack = c2; c2->snext = c1; c1->snext = NULL;
	m->sel = c2;
	save_selmon();
	selmon = m;
	mons = m;
	Arg arg = { .i = -1 };

	focusstack(&arg);
	ASSERT(selmon->sel == c1, "focusstack: reverse to c1");

	restore_selmon();
	free(c1); /* fixture leaked (ASan-reported) */
	free(c2); /* fixture leaked (ASan-reported) */
	free(m); /* fixture leaked (ASan-reported) */
}

static void
test_focusstack_single_client(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->tags = 1;
	m->clients = c;
	m->stack = c;
	m->sel = c;
	save_selmon();
	selmon = m;
	mons = m;
	Arg arg = { .i = 1 };

	focusstack(&arg);
	ASSERT(selmon->sel == c, "focusstack: single client stays focused");

	restore_selmon();
	free(c); /* fixture leaked (ASan-reported) */
	free(m); /* fixture leaked (ASan-reported) */
}

static void
test_focusstack_fullscreen_locked(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->isfullscreen = 1;
	m->sel = c;
	save_selmon();
	selmon = m;
	mons = m;
	Arg arg = { .i = 1 };

	focusstack(&arg);
	ASSERT(selmon->sel == c, "focusstack: fullscreen locked, sel unchanged");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- focusstack reverse wrap --- */
static void
test_focusstack_reverse_wrap(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c1 = make_client(1, m);
	Client *c2 = make_client(2, m);
	c1->tags = 1; c2->tags = 1;
	m->clients = c1; c1->next = c2;
	m->stack = c2; c2->snext = c1; c1->snext = NULL;
	m->sel = c1;
	save_selmon();
	selmon = m;
	mons = m;
	Arg arg = { .i = -1 };

	focusstack(&arg);
	ASSERT(selmon->sel == c2, "focusstack: reverse wrap finds last visible");

	restore_selmon();
	free(c1); /* fixture leaked (ASan-reported) */
	free(c2); /* fixture leaked (ASan-reported) */
	free(m); /* fixture leaked (ASan-reported) */
}

/* --- focusstack forward wrap (lines 783-785) --- */
static void
test_focusstack_forward_wrap(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c1 = make_client(1, m);
	Client *c2 = make_client(2, m);
	c1->tags = 1; c2->tags = 1;
	m->clients = c1; c1->next = c2;
	m->stack = c2; c2->snext = c1; c1->snext = NULL;
	m->sel = c2;
	save_selmon();
	selmon = m;
	mons = m;
	Arg arg = { .i = 1 };

	focusstack(&arg);
	ASSERT(selmon->sel == c1, "focusstack: forward wrap to first client");

	restore_selmon();
	winclient_remove(c1);
	winclient_remove(c2);
	winclient_remove(c1);
	free(c1); free(c2); free(m);
}

/* --- focusstack with selmon->sel == NULL (line 780) --- */
static void
test_focusstack_no_sel(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	Monitor *m = make_monitor(0);
	selmon = m;
	mons = m;
	m->sel = NULL;

	Arg arg = { .i = +1 };
	focusstack(&arg);
	/* Early return at line 780: !selmon->sel is true */
	ASSERT(m->sel == NULL, "focusstack no sel: sel remains NULL");

	restore_selmon();
	free(m);
}

static void
test_focusstack_all_invisible(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c1 = make_client(1, m);
	Client *c2 = make_client(2, m);
	c1->tags = 2; /* tag 2, not visible on tag 1 */
	c2->tags = 2;
	c1->next = c2;
	m->clients = c1;
	m->stack = c1; c1->snext = c2; c2->snext = NULL;
	m->sel = c1;
	m->tagset[0] = 1;
	m->tagset[1] = 1;
	selmon = m;
	mons = m;
	Arg arg = { .i = 1 };

	focusstack(&arg);
	ASSERT(selmon->sel == c1, "focusstack: all invisible, sel unchanged");

	/* restore globals BEFORE freeing m */
	restore_selmon();
	winclient_remove(c1);
	winclient_remove(c2);
	winclient_remove(c1);
	free(c1); free(c2); free(m);
}

static void
run_tests(void)
{
	test_focusstack_forward();
	test_focusstack_reverse();
	test_focusstack_single_client();
	test_focusstack_fullscreen_locked();
	test_focusstack_reverse_wrap();
	test_focusstack_forward_wrap();
	test_focusstack_no_sel();
	test_focusstack_all_invisible();
}
