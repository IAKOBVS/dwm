/*
 * test_cleanupmon.c — per-function tests for dwm.c's cleanupmon().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through cleanupmon(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- cleanup / cleanupmon --- */
static void
test_cleanupmon(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m1 = make_monitor(0);
	Monitor *m2 = make_monitor(1);
	m1->next = m2;
	mons = m1;
	save_selmon();
	selmon = m1;

	cleanupmon(m2);
	ASSERT(mons->next == NULL, "cleanupmon: removes m2 from list");
	ASSERT(mons == m1, "cleanupmon: head unchanged");

	cleanupmon(m1);
	ASSERT(mons == NULL, "cleanupmon: empty list after removing head");

	restore_selmon();
}

/* --- cleanupmon else-branch traversal (lines 408-409) --- */
static void
test_cleanupmon_traverse(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m1 = make_monitor(0);
	Monitor *m2 = make_monitor(1);
	Monitor *m3 = make_monitor(2);
	m1->next = m2;
	m2->next = m3;
	mons = m1;
	save_selmon();
	selmon = m1;

	cleanupmon(m3);
	ASSERT(m1->next == m2, "cleanupmon traverse: m3 removed, m2 remains");
	ASSERT(m2->next == NULL, "cleanupmon traverse: m2->next is NULL after m3 removal");

	cleanupmon(m2);
	ASSERT(m1->next == NULL, "cleanupmon traverse: m2 removed");

	restore_selmon();
	free(m1);
}

static void
run_tests(void)
{
	test_cleanupmon();
	test_cleanupmon_traverse();
}
