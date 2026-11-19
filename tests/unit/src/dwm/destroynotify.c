/*
 * test_destroynotify.c — per-function tests for dwm.c's destroynotify().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through destroynotify(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- destroynotify --- */
static void
test_destroynotify_client(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	m->clients = c;
	m->stack = c;
	m->sel = c;
	save_selmon();
	selmon = m;
	mons = m;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xdestroywindow.window = 1;

	destroynotify(&ev);
	ASSERT(m->clients == NULL, "destroynotify: client removed from list");

	restore_selmon();
	free(m);
}

/* --- destroynotify swallowing client (lines 563-564) --- */
static void
test_destroynotify_swallowing(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *term = make_client(10, m);
	Client *child = make_client(11, m);
	term->isterminal = 1;
	term->swallowing = child;
	child->win = 10;   /* windows swapped during swallow */
	term->win = 11;
	child->mon = m;
	m->clients = term;
	term->next = NULL;
	mons = m;
	save_selmon();
	selmon = m;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xdestroywindow.window = child->win; /* 10 — the swallowed window */

	destroynotify(&ev);
	ASSERT(term->swallowing == NULL, "destroynotify: swallowing freed on child destroy");

	/* child was freed by unmanage; only free term */
	restore_selmon();
	winclient_remove(term);
	free(term); free(m);
}

/* --- terminal-window destroy while child swallowed: freed child must be
 * evicted from the winhash (double-free / wintoclient UAF regression) --- */
static void
test_destroynotify_terminal_destroys_parked_child_evicts_hash(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *term = make_client(10, m);
	Client *child = make_client(11, m);
	term->isterminal = 1;
	term->swallowing = child;
	m->clients = term;
	term->next = NULL;

	/* replicate swallow(): ids swapped, child detached from the lists,
	 * hashed lookups re-keyed (term@11, child@10).  The child's only live
	 * references are now term->swallowing and its winhash slot. */
	winclient_remove(term);
	winclient_remove(child);
	term->win = 11;
	child->win = 10;
	winclient_put(term);
	winclient_put(child);
	child->mon = m;
	mons = m;
	save_selmon();
	selmon = m;

	/* terminal dies while its (now parked) id 10 is held by the child:
	 * DestroyNotify resolves through the hash to the child, and
	 * unmanage()'s swallow branch frees it.  Its winhash slot must be
	 * cleared in the same breath — a later window recycling id 10 must
	 * not resolve to freed memory (wintoclient UAF / free(c) double-free). */
	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xdestroywindow.window = 10;
	destroynotify(&ev);

	ASSERT(term->swallowing == NULL, "unmanage: swallowed child freed and detached");
	ASSERT_EQ(wintoclient(10) != NULL, 0, "unmanage: freed child evicted from winhash");

	restore_selmon();
	winclient_remove(term);
	free(term); free(m);
}

static void
run_tests(void)
{
	test_destroynotify_client();
	test_destroynotify_swallowing();
	test_destroynotify_terminal_destroys_parked_child_evicts_hash();
}
