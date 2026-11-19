/*
 * test_isanyfullscreen.c — per-function tests for isanyfullscreen.
 *
 * Covers the primary path and at least one edge case, using the
 * shared mock fixture from test_common.h.
 */
#include "test_common.h"

static void
test_isanyfullscreen_scans_all_clients(void)
{
	Monitor *m;
	Client *c;

	save_selmon();
	m = make_monitor(0);
	c = make_client(1, m);
	m->clients = c;
	selmon = mons = m;

	ASSERT_EQ(isanyfullscreen(), 0, "isanyfullscreen: none fullscreen");
	c->isfullscreen = 1;
	ASSERT_EQ(isanyfullscreen(), 1, "isanyfullscreen: detects fullscreen client");

	reset_index();
	restore_selmon();
	free(c);
	free(m);
}

static void
run_tests(void)
{
	test_isanyfullscreen_scans_all_clients();
}
