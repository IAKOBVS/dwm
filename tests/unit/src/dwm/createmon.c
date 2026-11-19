/*
 * test_createmon.c — per-function tests for dwm.c's createmon().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through createmon(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- createmon --- */
static void
test_createmon_defaults(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = createmon();
	ASSERT(m != NULL, "createmon: returns non-NULL");
	ASSERT(m->tagset[0] == 1, "createmon: tagset[0]=1");
	ASSERT(m->tagset[1] == 1, "createmon: tagset[1]=1");
	ASSERT(m->mfact > 0, "createmon: mfact set");
	ASSERT(m->nmaster == 1, "createmon: nmaster=1");
	free(m);
}

static void
run_tests(void)
{
	test_createmon_defaults();
}
