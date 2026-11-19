/*
 * test_swallowingclient.c — per-function tests for swallowingclient.
 *
 * Covers the primary path and at least one edge case, using the
 * shared mock fixture from test_common.h.
 */
#include "test_common.h"

static void
test_swallowingclient_finds_parent(void)
{
	Monitor *m;
	Client *parent;
	Client *child;

	save_selmon();
	reset_index();
	m = make_monitor(0);
	parent = make_client(1, m);
	child = make_client(2, m);
	parent->swallowing = child;
	m->clients = parent;
	selmon = mons = m;

	ASSERT(swallowingclient(child->win) == parent,
		"swallowingclient: resolves child window to parent");
	ASSERT(swallowingclient(9999) == NULL,
		"swallowingclient: unknown window returns NULL");

	reset_index();
	restore_selmon();
	free(parent);
	free(child);
	free(m);
}

static void
run_tests(void)
{
	test_swallowingclient_finds_parent();
}
