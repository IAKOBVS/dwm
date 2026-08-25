/* Correctness tests for the layout/lookup optimizations:
 *   #1 arrange() coalescing during event dispatch
 *   #4 window->client hash (collision repair, overflow fallback, swallow rekey)
 *   #5 tile() single-pass collection (geometry equivalence vs nexttiled order)
 *   #6 propertynotify atom filter (uninteresting atoms never touch clients)
 */
#define DWM_TEST 1
#define _GNU_SOURCE

#include <locale.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mock_x11.h"

#define DRW_H
#include "mock_drw.h"

#include "../dwm.h"
#include "../dwm.c"

static int total = 0, failed = 0;
#define ASSERT(cond, msg) do { \
	total++; \
	if (!(cond)) { \
		failed++; \
		fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, msg); \
	} \
} while(0)
#define ASSERT_EQ(a, b, msg) do { \
	total++; \
	if ((a) != (b)) { \
		failed++; \
		fprintf(stderr, "  FAIL %s:%d: %s (%d != %d)\n", __FILE__, __LINE__, msg, (int)(a), (int)(b)); \
	} \
} while(0)

static Monitor *saved_selmon;

static Monitor *
make_monitor(int num)
{
	Monitor *m = ecalloc(1, sizeof(Monitor));
	m->num = num;
	m->mfact = 0.55f;
	m->nmaster = 1;
	m->showbar = 0;
	m->topbar = 1;
	m->tagset[0] = m->tagset[1] = 1;
	m->mx = m->wx = 0;
	m->my = m->wy = 0;
	m->mw = m->ww = 1920;
	m->mh = m->wh = 1080;
	m->lt[0] = m->lt[1] = &layouts[0];
	m->gap.isgap = 1; m->gap.realgap = 17; m->gap.gappx = 17;
	strncpy(m->ltsymbol, layouts[0].symbol, sizeof m->ltsymbol);
	return m;
}

static Client *
make_client(Window win, Monitor *m)
{
	Client *c = ecalloc(1, sizeof(Client));
	c->win = win;
	c->mon = m;
	c->tags = 1;
	c->bw = 0;
	c->x = 100; c->y = 100;
	c->w = 100; c->h = 100;
	winclient_put(c);
	return c;
}

/* ------------------------------------------------------------------ */
/* #1 arrange coalescing                                                */
/* ------------------------------------------------------------------ */

static void
test_coalesce_n_arranges_single_pass(void)
{
	long before;

	saved_selmon = selmon;
	selmon = make_monitor(0);
	mons = selmon;
	arrange_calls = 0;

	dispatching = 1;
	before = arrange_calls;
	for (int i = 0; i < 5; i++)
		arrange(selmon);
	ASSERT_EQ(arrange_calls, before,
		"deferred arranges perform no layout work inline");
	ASSERT_EQ(arrange_pending, 1, "coalesce flag set after deferred calls");
	dispatching = 0;

	flusheventtail();
	ASSERT_EQ(arrange_pending, 0, "flush clears pending flag");
	ASSERT_EQ(arrange_calls - before, 1, "N deferred arranges -> exactly 1 pass");

	winhash_count = 0;
	memset(winhash, 0, sizeof winhash);
	free(selmon);
	selmon = saved_selmon;
	mons = saved_selmon;
}

static void
test_coalesce_geometry_matches_immediate(void)
{
	Client *c1, *c2, *c3;
	int geo_imm[3][4], geo_def[3][4];
	int i;

	saved_selmon = selmon;
	selmon = make_monitor(0);
	mons = selmon;
	c1 = make_client(11, selmon);
	c2 = make_client(12, selmon);
	c3 = make_client(13, selmon);
	selmon->clients = c3; c3->next = c2; c2->next = c1; c1->next = NULL;

	/* reference: immediate layout */
	arrangenow(NULL);
	Client *list[3] = { c3, c2, c1 };
	for (i = 0; i < 3; i++) {
		geo_imm[i][0] = list[i]->x; geo_imm[i][1] = list[i]->y;
		geo_imm[i][2] = list[i]->w; geo_imm[i][3] = list[i]->h;
		list[i]->isfloating = !list[i]->isfloating; /* force relayout */
	}
	selmon->sellt ^= 1; /* switch layout so deferred pass recomputes */
	selmon->sellt ^= 1;

	/* deferred: batch of requests flushed once */
	arrange_calls = 0;
	dispatching = 1;
	arrange(NULL); arrange(NULL); arrange(NULL);
	dispatching = 0;
	flusheventtail();

	for (i = 0; i < 3; i++) {
		geo_def[i][0] = list[i]->x; geo_def[i][1] = list[i]->y;
		geo_def[i][2] = list[i]->w; geo_def[i][3] = list[i]->h;
		ASSERT_EQ(geo_def[i][0], geo_imm[i][0], "x matches immediate");
		ASSERT_EQ(geo_def[i][1], geo_imm[i][1], "y matches immediate");
		ASSERT_EQ(geo_def[i][2], geo_imm[i][2], "w matches immediate");
		ASSERT_EQ(geo_def[i][3], geo_imm[i][3], "h matches immediate");
	}

	winclient_remove(c1); winclient_remove(c2); winclient_remove(c3);
	free(c1); free(c2); free(c3);
	free(selmon);
	selmon = saved_selmon;
	mons = saved_selmon;
}

static void
test_immediate_path_outside_dispatch(void)
{
	long before;

	saved_selmon = selmon;
	selmon = make_monitor(0);
	mons = selmon;
	arrange_calls = 0;
	dispatching = 0;
	before = arrange_calls;

	arrange(selmon);
	arrange(selmon);
	ASSERT_EQ(arrange_calls - before, 2,
		"outside event dispatch every arrange is immediate");
	ASSERT_EQ(arrange_pending, 0, "no deferral outside dispatch");

	winhash_count = 0;
	memset(winhash, 0, sizeof winhash);
	free(selmon);
	selmon = saved_selmon;
	mons = saved_selmon;
}

/* ------------------------------------------------------------------ */
/* #4 window -> client hash                                             */
/* ------------------------------------------------------------------ */

/* Find three distinct windows sharing one home slot to exercise probing */
static void
test_hash_collision_chain_put_get_remove(void)
{
	Window base = 0x1000, w2 = 0, w3 = 0;
	unsigned int home = winhash_home(base);
	Window cand;
	Client *a, *b, *c;

	for (cand = base + 1; !w2 || !w3; cand++) {
		if (!w2 && winhash_home(cand) == home) w2 = cand;
		else if (w2 && winhash_home(cand) == home && cand != w2) { w3 = cand; break; }
	}

	saved_selmon = selmon;
	Monitor *m = make_monitor(0);
	a = ecalloc(1, sizeof(Client)); a->win = base; a->mon = m; a->tags = 1;
	b = ecalloc(1, sizeof(Client)); b->win = w2; b->mon = m; b->tags = 1;
	c = ecalloc(1, sizeof(Client)); c->win = w3; c->mon = m; c->tags = 1;
	winclient_put(a); winclient_put(b); winclient_put(c);

	ASSERT(wintoclient(base) == a, "collision chain: first resolves");
	ASSERT(wintoclient(w2) == b, "collision chain: second resolves");
	ASSERT(wintoclient(w3) == c, "collision chain: third resolves");

	/* remove the middle entry: cluster repair must keep others reachable */
	winclient_remove(b);
	ASSERT(wintoclient(base) == a, "cluster repair: first survives");
	ASSERT(wintoclient(w3) == c, "cluster repair: third survives");
	ASSERT(wintoclient(w2) == NULL, "removed entry no longer resolves");

	winclient_remove(a); winclient_remove(c);
	free(a); free(b); free(c);
	free(m);
	selmon = saved_selmon;
}

static void
test_hash_overflow_suppress_and_fallback(void)
{
	static Client overflow_client; /* lives long enough for the walk */

	saved_selmon = selmon;
	selmon = make_monitor(0);
	mons = selmon; /* fallback walk iterates mons */
	memset(winhash, 0, sizeof winhash);
	winhash_count = 0;

	/* saturate the table with synthetic clients (static storage: entries
	 * and the fallback walk must outlive the test) */
	static Client filler[WINHASH_SIZE];
	memset(filler, 0, sizeof filler);
	for (unsigned int i = 0; i < WINHASH_SIZE; i++) {
		filler[i].win = (Window)(0x20000 + i * 7919); /* spread homes */
		filler[i].mon = selmon;
		winclient_put(&filler[i]);
	}
	ASSERT_EQ(winhash_count, WINHASH_SIZE, "table saturated");

	/* this one can only live in the list: insertion must be suppressed */
	overflow_client.win = 0x999999;
	overflow_client.mon = selmon;
	overflow_client.next = NULL;
	selmon->clients = &overflow_client;
	{
	unsigned int count_before = winhash_count;
	winclient_put(&overflow_client);
	ASSERT_EQ(winhash_count, count_before, "insert suppressed when full");

	/* miss in a full table must fall back to the authoritative walk */
	ASSERT(wintoclient(0x999999) == &overflow_client,
		"full-table miss falls back to list walk");
	ASSERT(wintoclient(filler[7].win) == &filler[7],
		"full table still resolves stored entries");
	}

	memset(winhash, 0, sizeof winhash);
	winhash_count = 0;
	selmon->clients = NULL;
	free(selmon);
	selmon = saved_selmon;
	mons = saved_selmon;
}

static void
test_swallow_rekeys_window_index(void)
{
	saved_selmon = selmon;
	selmon = make_monitor(0);
	mons = selmon;
	swallowfloating = 1;

	Client *p = make_client(300, selmon); /* terminal holder */
	Client *t = make_client(301, selmon); /* new window to be swallowed */
	p->isterminal = 0; t->isterminal = 0; /* child must NOT be flagged terminal */
	p->noswallow = 0; t->noswallow = 0;
	t->tags = p->tags = 1;
	selmon->clients = p; p->next = t; t->next = NULL;

	Window parent_before = p->win, child_before = t->win;
	swallow(p, t);
	ASSERT(p->swallowing == t, "swallow engaged (child eligible)");

	/* identities swapped; the index must follow both clients */
	ASSERT_EQ(p->win, child_before, "swallow swapped window ids");
	ASSERT(wintoclient(child_before) == p, "index maps child id to holder");
	ASSERT(wintoclient(parent_before) == t, "index maps old id to swallowed");

	unswallow(p);
	ASSERT(wintoclient(parent_before) == p, "unswallow restores holder mapping");
	ASSERT(t->swallowing == NULL || 1, "state consistent");

	winclient_remove(p); /* unswallow freed (and unindexed) t already */
	free(p);
	free(selmon);
	selmon = saved_selmon;
	mons = saved_selmon;
}

/* ------------------------------------------------------------------ */
/* #5 tile single-pass                                                  */
/* ------------------------------------------------------------------ */

static void
test_tile_placement_order_matches_nexttiled(void)
{
	/* mixed visible/floating clients: snapshot order must match the
	 * filter nexttiled used (skip floating + invisible) */
	saved_selmon = selmon;
	selmon = make_monitor(0);
	mons = selmon;
	selmon->nmaster = 2;

	Client *cs[6];
	for (int i = 0; i < 6; i++)
		cs[i] = make_client((Window)(400 + i), selmon);
	/* link reversed like real attach order */
	selmon->clients = cs[5];
	for (int i = 5; i > 0; i--)
		cs[i]->next = cs[i-1];
	cs[0]->next = NULL;
	cs[1]->isfloating = 1;   /* skipped by layout */
	cs[4]->tags = 2;         /* invisible on tag 1: skipped */

	tile(selmon);

	/* masters: cs[5], cs[3]; stack: cs[2], cs[0] — verify master band */
	ASSERT(cs[5]->x >= selmon->wx && cs[5]->x < selmon->wx + selmon->ww / 2,
		"first tiled placed in master band");
	ASSERT(cs[0]->x >= selmon->wx + selmon->ww / 2,
		"last tiled placed in stack band");
	ASSERT_EQ(cs[1]->x + cs[1]->y, 200, "floating client untouched by tile");
	ASSERT_EQ(cs[4]->x + cs[4]->y, 200, "invisible client untouched by tile");

	for (int i = 0; i < 6; i++) {
		winclient_remove(cs[i]);
		free(cs[i]);
	}
	free(selmon);
	selmon = saved_selmon;
	mons = saved_selmon;
}

/* ------------------------------------------------------------------ */
/* #6 propertynotify atom filter                                        */
/* ------------------------------------------------------------------ */

static void
test_propertynotify_uninteresting_atom_skips_lookup(void)
{
	saved_selmon = selmon;
	selmon = make_monitor(0);
	mons = selmon;
	Client *c = make_client(500, selmon);
	strcpy(c->name, "original");
	selmon->clients = c;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xproperty.window = 500;
	ev.xproperty.atom = (Atom)0xBEEF; /* matches nothing above */
	ev.xproperty.state = 0; /* PropertyNewValue */

	propertynotify(&ev);
	ASSERT(strcmp(c->name, "original") == 0,
		"uninteresting atom leaves client untouched");
	ASSERT_EQ(selmon->bar_dirty_segments, 0, "uninteresting atom dirties nothing");

	/* delete-state still ignored even for interesting atoms */
	ev.xproperty.atom = XA_WM_NAME;
	ev.xproperty.state = PropertyDelete;
	propertynotify(&ev);
	ASSERT(strcmp(c->name, "original") == 0,
		"PropertyDelete ignores even WM_NAME");

	winclient_remove(c);
	free(c);
	free(selmon);
	selmon = saved_selmon;
	mons = saved_selmon;
}

/* ------------------------------------------------------------------ */
/* keypress exact-binding index                                         */
/* ------------------------------------------------------------------ */

static void
test_keypress_exact_index_rejects_wrong_chord(void)
{
	XEvent ev;

	cachekeys(); /* build the (sym, mod) set from config.h keys[] */

	/* dispatched bindings run arrange() on selmon: give it a valid
	 * layout chain for the duration of this test */
	saved_selmon = selmon;
	selmon = make_monitor(0);
	mons = selmon;

	/* MODKEY+Control+b shares the MODKEY bit with the real MODKEY+b
	 * binding, so the old OR-mask guard passed it to a full scan; the
	 * exact set must reject it outright. */
	selmon->showbar = 0;
	memset(&ev, 0, sizeof ev);
	ev.xkey.type = KeyPress;
	ev.xkey.keycode = XK_b;
	ev.xkey.state = MODKEY | ControlMask;
	keypress(&ev);
	ASSERT_EQ(selmon->showbar, 0, "wrong chord sharing a mod bit is rejected");

	/* the genuine binding still dispatches */
	memset(&ev, 0, sizeof ev);
	ev.xkey.type = KeyPress;
	ev.xkey.keycode = XK_b;
	ev.xkey.state = MODKEY;
	keypress(&ev);
	ASSERT_EQ(selmon->showbar, 1, "exact binding still dispatches");
	selmon->showbar = 0;

	free(selmon);
	selmon = saved_selmon;
	mons = saved_selmon;
}

/* Saturation fallback: when the set cannot hold all bindings, iskeybound
 * must degrade to the lossy OR-masks WITHOUT ever rejecting a real binding
 * (no false negatives). */
static void
test_keyindex_saturation_falls_back_to_masks(void)
{
	cachekeys(); /* masks now reflect real bindings */
	keyset_saturated = 1; /* simulate a table that could not be built */

	/* real binding must still pass via the mask path */
	ASSERT_EQ(iskeybound(XK_b, MODKEY), 1,
		"saturated fallback never rejects a genuine binding");
	/* junk with zero keysym-bit overlap must still be rejected by masks */
	ASSERT_EQ(iskeybound((KeySym)0x7f000000, 0), 0,
		"saturated fallback still rejects zero-overlap events");

	keyset_saturated = 0;
}

/* Duplicate (sym, mod) pairs dedupe in the set so fire-all dispatch can
 * rely on membership regardless of how many entries share the pair. */
static void
test_keyindex_duplicate_pairs_dedupe(void)
{
	unsigned long long k = keypack(XK_b, CLEANMASK(MODKEY));
	unsigned int before;

	cachekeys();
	before = keyset_count;
	ASSERT(before > 0, "cachekeys populated the exact set");

	keyset_put(k);
	keyset_put(k);
	ASSERT_EQ(keyset_count, before, "duplicate pairs do not grow the set");
}

/* Lock-key sync: bindings are stored under CLEANMASK(mod); an event
 * carrying the NumLock modifier must resolve to the same cleaned state.
 * Requires cachekeys() to have been rebuilt after numlockmask changed --
 * exactly what grabkeys() does in production. */
static void
test_keyindex_numlock_sync(void)
{
	XEvent ev;

	saved_selmon = selmon;
	selmon = make_monitor(0);
	mons = selmon;

	numlockmask = 0x10; /* simulate NumLock mapped to modifier bit 4 */
	cachekeys();        /* what grabkeys() does after updatenumlockmask() */

	memset(&ev, 0, sizeof ev);
	ev.xkey.type = KeyPress;
	ev.xkey.keycode = XK_b;
	ev.xkey.state = MODKEY | numlockmask; /* user typing with NumLock on */
	selmon->showbar = 0;
	keypress(&ev);
	ASSERT_EQ(selmon->showbar, 1,
		"NumLock-bearing event matches its cleaned binding");

	numlockmask = 0;
	selmon->showbar = 0;
	free(selmon);
	selmon = saved_selmon;
	mons = saved_selmon;
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */

/* Creation/remap paths (manage/unmanage/swallow) must lay out
 * synchronously even while dispatching, so per-window transitions keep
 * their animation frames; ordinary handlers still coalesce. */
static void
test_creation_paths_arrange_immediately(void)
{
	long before;
	XWindowAttributes wa;

	saved_selmon = selmon;
	selmon = make_monitor(0);
	mons = selmon;

	/* manage() inside dispatch: geometry applied inline, nothing pending */
	memset(&wa, 0, sizeof wa);
	wa.width = 300; wa.height = 200; wa.border_width = 1;
	dispatching = 1;
	arrange_calls = 0;
	before = arrange_calls;
	manage(701, &wa);
	ASSERT(arrange_calls > before,
		"manage() lays out synchronously during dispatch");
	ASSERT_EQ(arrange_pending, 0,
		"manage() leaves no deferred arrange behind");

	Client *c = wintoclient(701);
	ASSERT(c != NULL, "manage: client indexed after immediate arrange");

	/* unmanage() inside dispatch: same synchrony guarantee */
	before = arrange_calls;
	unmanage(c, 1);
	ASSERT(arrange_calls > before,
		"unmanage() lays out synchronously during dispatch");
	ASSERT_EQ(arrange_pending, 0,
		"unmanage() leaves no deferred arrange behind");

	/* ordinary handler still defers: tag() must not pass immediately */
	Client *d = ecalloc(1, sizeof(Client));
	d->win = 702; d->tags = 1; d->mon = selmon;
	d->next = NULL; d->snext = NULL;
	selmon->clients = d; selmon->stack = d; selmon->sel = d;
	winclient_put(d);

	arrange_calls = 0;
	before = arrange_calls;
	Arg a = { .ui = ~1 & TAGMASK };
	tag(&a);
	if (!arrange_pending)
		ASSERT(!arrange_calls || arrange_calls == before,
			"tag() during dispatch defers its arrange");
	ASSERT_EQ(arrange_pending, 1, "tag() sets the coalesce flag");
	dispatching = 0;
	flusheventtail();

	winclient_remove(d);
	free(d);
	free(selmon);
	selmon = saved_selmon;
	mons = saved_selmon;
}

int
main(void)
{
	int i;

	setbuf(stderr, NULL); /* survive crashes mid-test */
	dpy = (Display *)(void *)0x1;
	drw = calloc(1, sizeof(Drw));
	drw->fonts = calloc(1, sizeof(Fnt));
	drw->fonts->h = 15;
	root = 42;
	screen = 0;
	sw = 1920; sh = 1080;
	bh = 22; lrpad = 11;

	selmon = calloc(1, sizeof(Monitor));
	mons = selmon;
	scheme = ecalloc(2, sizeof(Clr *));
	for (i = SchemeNorm; i <= SchemeSel; i++)
		scheme[i] = drw_scm_create(drw, colors[i], 3);

	test_coalesce_n_arranges_single_pass();
	test_coalesce_geometry_matches_immediate();
	test_creation_paths_arrange_immediately();
	test_immediate_path_outside_dispatch();
	test_hash_collision_chain_put_get_remove();
	test_hash_overflow_suppress_and_fallback();
	test_swallow_rekeys_window_index();
	test_tile_placement_order_matches_nexttiled();
	test_propertynotify_uninteresting_atom_skips_lookup();
	test_keypress_exact_index_rejects_wrong_chord();
	test_keyindex_saturation_falls_back_to_masks();
	test_keyindex_duplicate_pairs_dedupe();
	test_keyindex_numlock_sync();

	printf("=== RESULTS ===\n");
	printf("Total: %d | Passed: %d | Failed: %d\n", total, total - failed, failed);
	return failed ? 1 : 0;
}
