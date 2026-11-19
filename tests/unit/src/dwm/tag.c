/*
 * test_tag.c — per-function tests for dwm.c's tag().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through tag(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- tag --- */
static void
test_tag_basic(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->tags = 1;
	m->sel = c;
	save_selmon();
	selmon = m;
	mons = m;
	Arg arg = { .ui = 2 };

	tag(&arg);
	ASSERT_EQ(c->tags, (unsigned)2, "tag: sets client tag");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

static void
run_tests(void)
{
	test_tag_basic();
}
