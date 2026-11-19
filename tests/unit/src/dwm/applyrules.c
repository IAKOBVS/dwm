/*
 * test_applyrules.c — per-function tests for dwm.c's applyrules().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through applyrules(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- applyrules --- */
static void
test_applyrules_matches_class(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->name[0] = '\0';
	/* XGetClassHint stubs: res_class=NULL, res_name=NULL -> class="broken" */
	applyrules(c);
	ASSERT(c->tags != 0, "applyrules: client gets default tag");
	ASSERT(c->mon == m, "applyrules: client stays on same monitor");
	winclient_remove(c);
	free(c); free(m);
}

static void
test_applyrules_matches_rule(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->name[0] = '\0';
	mock_class_res_class = "st-256color";
	mock_class_res_name = "test";
	applyrules(c);
	/* rule: { "st-256color", NULL, NULL, 0, 0, 1, 0, 1 } sets isterminal=1 */
	ASSERT(c->isterminal, "applyrules: st-256color rule sets isterminal");
	ASSERT(c->isfloating == 0, "applyrules: st-256color rule does not set floating");
	ASSERT(c->noswallow == 0, "applyrules: st-256color rule noswallow=0");
	ASSERT_EQ(c->tags, 1, "applyrules: tags default to 1 when rule tags=0");
	winclient_remove(c);
	free(c); free(m);
	mock_x11_reset();
}

static void
test_applyrules_instance_matching(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->name[0] = '\0';
	mock_class_res_class = "TestClass";
	mock_class_res_name = "TestInstance";
	applyrules(c);
	/* TestClass+TestInstance rule sets tags=1<<2 */
	ASSERT(c->tags & (1 << 2), "applyrules: instance match sets tag 2");
	winclient_remove(c);
	free(c); free(m);
	mock_x11_reset();
}

/* --- applyrules with class/name matching (exercises XFree path) --- */
static void
test_applyrules_with_class(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->name[0] = '\0';
	mock_class_res_class = "Terminal";
	mock_class_res_name = "urxvt";
	applyrules(c);
	ASSERT(c->tags != 0, "applyrules with class: gets default tag");
	ASSERT(c->mon == m, "applyrules with class: stays on same monitor");
	/* XFree called on class/name strings inside applyrules */
	winclient_remove(c);
	free(c); free(m);
	mock_x11_reset();
}

/* --- applyrules monitor branch (line 108: c->mon = m) --- */
static void
test_applyrules_monitor_branch(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	Monitor *m0 = make_monitor(0);
	Monitor *m1 = make_monitor(1);
	m0->next = m1;

	save_selmon();
	selmon = m0;
	mons = m0;

	Client *c = make_client(1, m0);
	c->name[0] = '\0';
	mock_class_res_class = "st-256color";
	mock_class_res_name = "test";
	applyrules(c);
	/* st-256color rule has monitor=1, so c->mon should be m1 */
	ASSERT(c->mon == m1, "applyrules monitor: c->mon set to matching monitor");

	winclient_remove(c);
	free(c);
	free(m0);
	free(m1);
	mock_x11_reset();
	restore_selmon();
}

static void
run_tests(void)
{
	test_applyrules_matches_class();
	test_applyrules_matches_rule();
	test_applyrules_instance_matching();
	test_applyrules_with_class();
	test_applyrules_monitor_branch();
}
