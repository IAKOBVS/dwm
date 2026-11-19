/*
 * test_swallow.c — per-function tests for dwm.c's swallow().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through swallow(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- swallow / unswallow --- */
static void
test_swallow_basic(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *term = make_client(10, m);
	term->isterminal = 1;
	Client *c = make_client(11, m);
	c->isterminal = 0;
	c->noswallow = 0;
	m->clients = c;
	c->next = NULL;

	swallow(term, c);
	ASSERT(term->swallowing == c, "swallow: term->swallowing set");
	ASSERT_EQ(term->state, CST_HOLDER, "swallow: holder enters HOLDER state");
	ASSERT_EQ(c->state, CST_PLACEHOLDER, "swallow: child enters PLACEHOLDER state");
	ASSERT_EQ(c->holder, term, "swallow: placeholder records its holder");
	ASSERT(c->mon == term->mon, "swallow: client mon updated");
	/* windows swapped */
	ASSERT(term->win == 11, "swallow: term win takes client win");
	ASSERT(c->win == 10, "swallow: client win takes term win");
	winclient_remove(c);
	winclient_remove(term);
	winclient_remove(c);
	free(c); free(term); free(m);
}

static void
test_swallow_noswallow_returns_early(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *term = make_client(10, m);
	Client *c = make_client(11, m);
	c->noswallow = 1;
	swallow(term, c);
	/* When mock X11 returns, the check for c->noswallow is at line 228 */
	ASSERT(term->swallowing == NULL, "swallow: noswallow prevents swallow");
	winclient_remove(c);
	winclient_remove(term);
	winclient_remove(c);
	free(c); free(term); free(m);
}

static void
test_swallow_noswallow_floating(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *term = make_client(10, m);
	term->isterminal = 1;
	Client *c = make_client(11, m);
	c->isterminal = 0;
	c->noswallow = 1;
	c->isfloating = 1;
	m->clients = c;
	c->next = NULL;

	swallow(term, c);
	ASSERT(term->swallowing == NULL, "swallow: noswallow floating prevents swallow");
	ASSERT(term->win == 10, "swallow: term win unchanged");
	ASSERT(c->win == 11, "swallow: client win unchanged");
	winclient_remove(c);
	winclient_remove(term);
	winclient_remove(c);
	free(c); free(term); free(m);
}

static void
test_swallow_floating_rejected_when_not_allowed(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *term = make_client(10, m);
	term->isterminal = 1;
	Client *c = make_client(11, m);
	c->isterminal = 0;
	c->noswallow = 0;
	c->isfloating = 1;
	m->clients = c;
	c->next = NULL;

	/* temporarily set swallowfloating = 0 to test the guard */
	int saved = *(int *)&swallowfloating;
	*(int *)&swallowfloating = 0;

	swallow(term, c);

	*(int *)&swallowfloating = saved;

	ASSERT(term->swallowing == NULL, "swallow: swallowfloating=0 prevents floating swallow");
	ASSERT(term->win == 10, "swallow: term win unchanged");
	ASSERT(c->win == 11, "swallow: client win unchanged");
	winclient_remove(c);
	winclient_remove(term);
	winclient_remove(c);
	free(c); free(term); free(m);
}

static void
run_tests(void)
{
	test_swallow_basic();
	test_swallow_noswallow_returns_early();
	test_swallow_noswallow_floating();
	test_swallow_floating_rejected_when_not_allowed();
}
