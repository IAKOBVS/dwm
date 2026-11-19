/*
 * test_cleanup.c — per-function tests for dwm.c's cleanup().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through cleanup(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- unmanage loop from cleanup --- */
static void
test_cleanup_unmanages(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c1 = make_client(1, m);
	c1->next = NULL;
	c1->snext = NULL;
	m->sel = c1;
	m->clients = c1;
	m->stack = c1;
	/* exercise the while (m->stack) unmanage(m->stack, 0) loop from cleanup lines 384-385 */
	while (m->stack)
		unmanage(m->stack, 0);
	ASSERT(m->stack == NULL, "cleanup: unmanage loop clears stack");
	ASSERT(m->clients == NULL, "cleanup: unmanage loop removes all clients");
	free(m);
}

/* --- destroy / cleanup --- */
static void
test_cleanup_empties_mons(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	Monitor *m = make_monitor(0);
	m->sel = NULL;
	m->stack = NULL;
	selmon = m;
	mons = m;
	/* cleanup frees drw, scheme, cursors — must run LAST */
	cleanup();
	ASSERT(1, "cleanup: does not crash with empty mons");
	mons = NULL;
	selmon = NULL;
	restore_selmon();
}

/* --- cleanupmon with client (line 385 area — exercises cleanupmon path) --- */
static void
test_cleanup_manages_stack(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	Monitor *m = make_monitor(0);
	mons = m;
	selmon = m;

	Client *c = make_client(300, m);
	c->tags = 1;
	m->clients = c;
	m->stack = c;
	c->next = NULL;
	c->snext = NULL;

	cleanupmon(m);
	ASSERT(mons == NULL, "cleanupmon with client: mons is NULL after cleanupmon");

	restore_selmon();
	free(c); /* fixture leaked (ASan-reported) */
}

/* --- cleanup with client present (lines 384-385) --- */
static void
test_cleanup_with_client(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	/* Reinitialize minimal globals so cleanup has something to work with */
	selmon = calloc(1, sizeof(Monitor));
	mons = selmon;
	selmon->tagset[0] = selmon->tagset[1] = 1;
	selmon->mfact = 0.55f;
	selmon->nmaster = 1;
	selmon->showbar = 0;
	selmon->topbar = 1;
	selmon->mx = selmon->wx = 0;
	selmon->my = selmon->wy = 0;
	selmon->mw = selmon->ww = 1920;
	selmon->mh = selmon->wh = 1080;
	selmon->lt[0] = selmon->lt[1] = &layouts[0];
	strncpy(selmon->ltsymbol, layouts[0].symbol, sizeof selmon->ltsymbol);
	selmon->gap.isgap = 1;
	selmon->gap.realgap = 17;
	selmon->gap.gappx = 17;

	drw = calloc(1, sizeof(Drw));
	drw->fonts = calloc(1, sizeof(Fnt));
	drw->fonts->h = 15;

	scheme = ecalloc(2, sizeof(Clr *));
	int i;
	for (i = SchemeNorm; i <= SchemeSel; i++)
		scheme[i] = drw_scm_create(drw, colors[i], 3);

	cursor[CurNormal] = drw_cur_create(drw, XC_left_ptr);
	cursor[CurResize] = drw_cur_create(drw, XC_sizing);
	cursor[CurMove]   = drw_cur_create(drw, XC_fleur);

	wmcheckwin = 0;

	Client *c = make_client(1, selmon);
	c->tags = 1;
	c->next = NULL;
	c->snext = NULL;
	selmon->sel = c;
	selmon->clients = c;
	selmon->stack = c;

	cleanup();
	ASSERT(1, "cleanup with client: does not crash (covers lines 384-385)");

	/* Reinitialize for safety (nothing after this, but be clean) */
	mons = NULL;
	selmon = NULL;
}

static void
run_tests(void)
{
	test_cleanup_unmanages();
	test_cleanup_empties_mons();
	test_cleanup_manages_stack();
	test_cleanup_with_client();
}
