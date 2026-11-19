/*
 * test_wintoclient.c — per-function tests for dwm.c's wintoclient().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through wintoclient(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- wintoclient --- */
static void
test_wintoclient_finds(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(42, m);
	m->clients = c;
	mons = m;

	Client *found = wintoclient(42);
	ASSERT(found == c, "wintoclient: finds client by window");

	winclient_remove(c);
	free(c); free(m);
}

static void
test_wintoclient_notfound(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	m->clients = c;
	mons = m;

	Client *found = wintoclient(99);
	ASSERT(found == NULL, "wintoclient: returns NULL for unknown window");

	winclient_remove(c);
	free(c); free(m);
}

static void
run_tests(void)
{
	test_wintoclient_finds();
	test_wintoclient_notfound();
}
