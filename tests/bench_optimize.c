/* bench_optimize.c — Micro-benchmarks for proposed optimizations (Roadmap §1–9).
 *
 * Measures the cost of each optimization scenario using clock_gettime().
 * All benchmarks are mock-based (no real X server needed).
 *
 * Build:  make bench_optimize     (from tests/)
 * Run:    ./bench_optimize
 *
 * Each benchmark reports ns/call for the current (unoptimized) code.
 * After implementing an optimization, re-run to measure improvement.
 */
#define DWM_TEST 1
#define _GNU_SOURCE

#include <locale.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "mock_x11.h"

#define DRW_H
#include "mock_drw.h"

#include "../dwm.h"
#include "../dwm.c"

/* ── timing ────────────────────────────────────────────────────────── */

static long
nanos_mono(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (long)ts.tv_sec * 1000000000L + ts.tv_nsec;
}

/* ── helpers ───────────────────────────────────────────────────────── */

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
	strncpy(m->ltsymbol, layouts[0].symbol, sizeof m->ltsymbol);
	m->gap.isgap = 1;
	m->gap.realgap = 17;
	m->gap.gappx = 17;
	m->sel = NULL;
	m->clients = NULL;
	m->stack = NULL;
	m->next = NULL;
	m->barwin = 0;
	m->by = 0;
	return m;
}

static Client *
make_client(Window win, Monitor *mon)
{
	Client *c = ecalloc(1, sizeof(Client));
	c->win = win;
	c->mon = mon;
	c->tags = 1;
	c->next = NULL;
	c->snext = NULL;
	c->isfloating = 0;
	c->isfullscreen = 0;
	c->neverfocus = 0;
	c->isurgent = 0;
	c->isterminal = 0;
	c->noswallow = 0;
	c->swallowing = NULL;
	c->pid = 0;
	c->bw = 0;
	c->x = 100; c->y = 100;
	c->w = 200; c->h = 200;
	c->oldw = c->w; c->oldh = c->h;
	c->basew = 50; c->baseh = 50;
	c->minw = 50; c->minh = 50;
	c->maxw = 0; c->maxh = 0;
	c->incw = 0; c->inch = 0;
	c->mina = 0.0f; c->maxa = 0.0f;
	return c;
}

static void
init_globals(void)
{
	int i;

	dpy = (Display *)(void *)0x1;
	drw = calloc(1, sizeof(Drw));
	drw->fonts = calloc(1, sizeof(Fnt));
	drw->fonts->h = 15;

	root = 42;
	screen = 0;
	sw = 1920; sh = 1080;
	bh = 22; lrpad = 11;
	wmcheckwin = 42;
	running = 1;

	selmon = make_monitor(0);
	mons = selmon;

	scheme = ecalloc(2, sizeof(Clr *));
	for (i = SchemeNorm; i <= SchemeSel; i++)
		scheme[i] = drw_scm_create(drw, colors[i], 3);

	netatom[NetWMState] = 1;
	netatom[NetWMFullscreen] = 2;
	netatom[NetActiveWindow] = 3;
	netatom[NetWMName] = 4;
	netatom[NetWMWindowType] = 5;
	netatom[NetWMWindowTypeDialog] = 6;
	wmatom[WMProtocols] = 100;
	wmatom[WMDelete] = 101;
	wmatom[WMState] = 102;
	wmatom[WMTakeFocus] = 103;

	mock_gettextprop_return = 1;
	mock_gettextprop_value = "dwm-6.4";
}

static void
cleanup_clients(void)
{
	Client *c, *next;
	Monitor *m;
	for (m = mons; m; m = m->next) {
		for (c = m->clients; c; c = next) {
			next = c->next;
			free(c);
		}
		m->clients = NULL;
		m->stack = NULL;
		m->sel = NULL;
	}
}

static void
add_clients(Monitor *m, int n, int base_win)
{
	int i;
	for (i = 0; i < n; i++) {
		Client *c = make_client(base_win + i, m);
		attach(c);
		attachstack(c);
	}
}

/* ── benchmark infrastructure ──────────────────────────────────────── */

static int bench_count = 0;

static void
report(const char *label, long total_ns, int n)
{
	long per_call = total_ns / n;
	double ms = total_ns / 1e6;
	bench_count++;
	fprintf(stderr, "  [%d] %-52s %8ld ns/call  (%d iters, %.3f ms total)\n",
	        bench_count, label, per_call, n, ms);
}

/* ── BENCH 1: arrange() coalescing ────────────────────────────────── */

static void
bench_arrange_coalescing(void)
{
	int N = 50000;
	int i;
	long t0, t1;

	fprintf(stderr, "\n=== 1. arrange() coalescing ===\n");
	fprintf(stderr, "  Measures cost of arrange(selmon) with 10 tiled clients.\n");
	fprintf(stderr, "  After optimization: N calls should coalesce to 1.\n\n");

	/* baseline: call arrange() N times */
	t0 = nanos_mono();
	for (i = 0; i < N; i++)
		arrange(selmon);
	t1 = nanos_mono();
	report("arrange(selmon) x N (10 clients)", t1 - t0, N);

	cleanup_clients();
	add_clients(selmon, 10, 1000);
	selmon->showbar = 0;

	/* simulate burst: 5 arrange() calls per event */
	t0 = nanos_mono();
	for (i = 0; i < N; i++) {
		arrange(selmon);
		arrange(selmon);
		arrange(selmon);
		arrange(selmon);
		arrange(selmon);
	}
	t1 = nanos_mono();
	report("arrange(selmon) x 5N (burst, 10 clients)", t1 - t0, N * 5);

	/* with more clients */
	cleanup_clients();
	add_clients(selmon, 20, 2000);
	selmon->showbar = 0;

	t0 = nanos_mono();
	for (i = 0; i < N; i++)
		arrange(selmon);
	t1 = nanos_mono();
	report("arrange(selmon) x N (20 clients)", t1 - t0, N);

	cleanup_clients();
	add_clients(selmon, 20, 3000);
	selmon->showbar = 0;

	t0 = nanos_mono();
	for (i = 0; i < N; i++) {
		arrange(selmon);
		arrange(selmon);
		arrange(selmon);
	}
	t1 = nanos_mono();
	report("arrange(selmon) x 3N (burst, 20 clients)", t1 - t0, N * 3);
}

/* ── BENCH 2: updatestatus() comparison ────────────────────────────── */

static void
bench_updatestatus_comparison(void)
{
	int N = 50000;
	int i;
	long t0, t1;

	fprintf(stderr, "\n=== 2. updatestatus() comparison ===\n");
	fprintf(stderr, "  Measures cost of updatestatus() when text is identical.\n");
	fprintf(stderr, "  After optimization: identical text returns early.\n\n");

	/* baseline: updatestatus() always dirties segments */
	t0 = nanos_mono();
	for (i = 0; i < N; i++) {
		updatestatus();
		selmon->bar_dirty_segments = 0;
		bar_draw_pending = 0;
	}
	t1 = nanos_mono();
	report("updatestatus() x N (identical text)", t1 - t0, N);

	/* with different text each time */
	t0 = nanos_mono();
	for (i = 0; i < N; i++) {
		mock_gettextprop_value = (i % 2) ? "text-a" : "text-b";
		updatestatus();
		selmon->bar_dirty_segments = 0;
		bar_draw_pending = 0;
	}
	t1 = nanos_mono();
	report("updatestatus() x N (alternating text)", t1 - t0, N);

	mock_gettextprop_value = "dwm-6.4";
}

/* ── BENCH 3: focus() idempotent guard ─────────────────────────────── */

static void
bench_focus_idempotent(void)
{
	int N = 50000;
	int i;
	long t0, t1;
	Client *c;

	fprintf(stderr, "\n=== 3. focus() idempotent guard ===\n");
	fprintf(stderr, "  Measures cost of focus() when client is already focused.\n");
	fprintf(stderr, "  After optimization: same-client focus returns early.\n\n");

	cleanup_clients();
	add_clients(selmon, 5, 1000);
	c = selmon->clients;
	selmon->sel = c;
	selmon->bar_dirty_segments = 0;

	/* baseline: focus() always sets dirty segments */
	t0 = nanos_mono();
	for (i = 0; i < N; i++) {
		focus(c);
		selmon->bar_dirty_segments = 0;
		bar_draw_pending = 0;
	}
	t1 = nanos_mono();
	report("focus(same_client) x N (5 clients)", t1 - t0, N);

	/* focus with NULL (find visible) */
	t0 = nanos_mono();
	for (i = 0; i < N; i++) {
		focus(NULL);
		selmon->bar_dirty_segments = 0;
		bar_draw_pending = 0;
	}
	t1 = nanos_mono();
	report("focus(NULL) x N (5 clients, finds sel)", t1 - t0, N);

	/* focus with different client */
	cleanup_clients();
	add_clients(selmon, 5, 2000);
	c = selmon->clients;
	selmon->sel = c->next;

	t0 = nanos_mono();
	for (i = 0; i < N; i++) {
		focus(c);
		selmon->bar_dirty_segments = 0;
		bar_draw_pending = 0;
	}
	t1 = nanos_mono();
	report("focus(different_client) x N (5 clients)", t1 - t0, N);
}

/* ── BENCH 4: wintoclient() hash table ─────────────────────────────── */

static void
bench_wintoclient_hash(void)
{
	int N = 50000;
	int i;
	long t0, t1;

	fprintf(stderr, "\n=== 4. wintoclient() O(n) → O(1) ===\n");
	fprintf(stderr, "  Measures cost of wintoclient() linear walk.\n");
	fprintf(stderr, "  After optimization: hash table lookup is O(1).\n\n");

	/* 10 clients */
	cleanup_clients();
	add_clients(selmon, 10, 1000);

	t0 = nanos_mono();
	for (i = 0; i < N; i++)
		wintoclient(1005);  /* middle of list */
	t1 = nanos_mono();
	report("wintoclient() x N (10 clients, hit)", t1 - t0, N);

	t0 = nanos_mono();
	for (i = 0; i < N; i++)
		wintoclient(9999);  /* not found */
	t1 = nanos_mono();
	report("wintoclient() x N (10 clients, miss)", t1 - t0, N);

	/* 20 clients */
	cleanup_clients();
	add_clients(selmon, 20, 2000);

	t0 = nanos_mono();
	for (i = 0; i < N; i++)
		wintoclient(2010);  /* middle of list */
	t1 = nanos_mono();
	report("wintoclient() x N (20 clients, hit)", t1 - t0, N);

	t0 = nanos_mono();
	for (i = 0; i < N; i++)
		wintoclient(9999);  /* not found */
	t1 = nanos_mono();
	report("wintoclient() x N (20 clients, miss)", t1 - t0, N);

	/* 50 clients */
	cleanup_clients();
	add_clients(selmon, 50, 3000);

	t0 = nanos_mono();
	for (i = 0; i < N; i++)
		wintoclient(3025);  /* middle of list */
	t1 = nanos_mono();
	report("wintoclient() x N (50 clients, hit)", t1 - t0, N);

	t0 = nanos_mono();
	for (i = 0; i < N; i++)
		wintoclient(9999);  /* not found */
	t1 = nanos_mono();
	report("wintoclient() x N (50 clients, miss)", t1 - t0, N);
}

/* ── BENCH 5: cached visible-client count ──────────────────────────── */

static void
bench_tile_monocle_count(void)
{
	int N = 50000;
	int i;
	long t0, t1;

	fprintf(stderr, "\n=== 5. tile()/monocle() visible-client count ===\n");
	fprintf(stderr, "  Measures cost of client list walk to count n.\n");
	fprintf(stderr, "  After optimization: cached count avoids O(n) walk.\n\n");

	/* tile with 10 clients */
	cleanup_clients();
	add_clients(selmon, 10, 1000);
	selmon->showbar = 0;

	t0 = nanos_mono();
	for (i = 0; i < N; i++)
		tile(selmon);
	t1 = nanos_mono();
	report("tile(selmon) x N (10 clients)", t1 - t0, N);

	/* tile with 20 clients */
	cleanup_clients();
	add_clients(selmon, 20, 2000);
	selmon->showbar = 0;

	t0 = nanos_mono();
	for (i = 0; i < N; i++)
		tile(selmon);
	t1 = nanos_mono();
	report("tile(selmon) x N (20 clients)", t1 - t0, N);

	/* tile with 50 clients */
	cleanup_clients();
	add_clients(selmon, 50, 3000);
	selmon->showbar = 0;

	t0 = nanos_mono();
	for (i = 0; i < N; i++)
		tile(selmon);
	t1 = nanos_mono();
	report("tile(selmon) x N (50 clients)", t1 - t0, N);

	/* monocle with 10 clients */
	cleanup_clients();
	add_clients(selmon, 10, 4000);
	selmon->showbar = 0;

	t0 = nanos_mono();
	for (i = 0; i < N; i++)
		monocle(selmon);
	t1 = nanos_mono();
	report("monocle(selmon) x N (10 clients)", t1 - t0, N);

	/* monocle with 20 clients */
	cleanup_clients();
	add_clients(selmon, 20, 5000);
	selmon->showbar = 0;

	t0 = nanos_mono();
	for (i = 0; i < N; i++)
		monocle(selmon);
	t1 = nanos_mono();
	report("monocle(selmon) x N (20 clients)", t1 - t0, N);

	/* monocle with 50 clients */
	cleanup_clients();
	add_clients(selmon, 50, 6000);
	selmon->showbar = 0;

	t0 = nanos_mono();
	for (i = 0; i < N; i++)
		monocle(selmon);
	t1 = nanos_mono();
	report("monocle(selmon) x N (50 clients)", t1 - t0, N);
}

/* ── BENCH 6: propertynotify() atom filter ─────────────────────────── */

static void
bench_propertynotify_filter(void)
{
	int N = 50000;
	int i;
	long t0, t1;
	XPropertyEvent ev;

	fprintf(stderr, "\n=== 6. propertynotify() atom filter ===\n");
	fprintf(stderr, "  Measures cost of propertynotify() for uninteresting atoms.\n");
	fprintf(stderr, "  After optimization: early return before wintoclient().\n\n");

	cleanup_clients();
	add_clients(selmon, 10, 1000);

	/* uninteresting atom on a client window — goes through full chain */
	ev.type = PropertyNotify;
	ev.window = 1005;  /* client window */
	ev.atom = 9999;    /* uninteresting atom */
	ev.state = 0;

	t0 = nanos_mono();
	for (i = 0; i < N; i++)
		propertynotify((XEvent *)&ev);
	t1 = nanos_mono();
	report("propertynotify(uninteresting) x N (10 clients)", t1 - t0, N);

	/* root WM_NAME — triggers updatestatus() */
	ev.window = root;
	ev.atom = XA_WM_NAME;

	t0 = nanos_mono();
	for (i = 0; i < N; i++) {
		propertynotify((XEvent *)&ev);
		selmon->bar_dirty_segments = 0;
		bar_draw_pending = 0;
	}
	t1 = nanos_mono();
	report("propertynotify(root WM_NAME) x N", t1 - t0, N);

	/* PropertyDelete — early return */
	ev.state = PropertyDelete;
	ev.window = 1005;
	ev.atom = XA_WM_NAME;

	t0 = nanos_mono();
	for (i = 0; i < N; i++)
		propertynotify((XEvent *)&ev);
	t1 = nanos_mono();
	report("propertynotify(PropertyDelete) x N", t1 - t0, N);
}

/* ── BENCH 7: setlayout() dirty guard ──────────────────────────────── */

static void
bench_setlayout_guard(void)
{
	int N = 50000;
	int i;
	long t0, t1;
	Arg arg;

	fprintf(stderr, "\n=== 7. setlayout() dirty guard ===\n");
	fprintf(stderr, "  Measures cost of setlayout() with same layout.\n");
	fprintf(stderr, "  After optimization: same layout skips dirty.\n\n");

	/* same layout — currently still sets DIRTY_TAGS */
	arg.v = &layouts[0];

	t0 = nanos_mono();
	for (i = 0; i < N; i++) {
		setlayout(&arg);
		selmon->bar_dirty_segments = 0;
		bar_draw_pending = 0;
	}
	t1 = nanos_mono();
	report("setlayout(same) x N", t1 - t0, N);

	/* toggling layout — should always dirty */
	t0 = nanos_mono();
	for (i = 0; i < N; i++) {
		setlayout(NULL);  /* NULL arg toggles sellt */
		selmon->bar_dirty_segments = 0;
		bar_draw_pending = 0;
	}
	t1 = nanos_mono();
	report("setlayout(toggle) x N", t1 - t0, N);

	/* restore layout */
	arg.v = &layouts[0];
	setlayout(&arg);
	selmon->bar_dirty_segments = 0;
	bar_draw_pending = 0;
}

/* ── BENCH 8: enternotify() single-monitor guard ───────────────────── */

static void
bench_enternotify_guard(void)
{
	int N = 50000;
	int i;
	long t0, t1;
	XCrossingEvent ev;
	Monitor *second;

	fprintf(stderr, "\n=== 8. enternotify() single-monitor guard ===\n");
	fprintf(stderr, "  Measures cost of enternotify() on single monitor.\n");
	fprintf(stderr, "  After optimization: single-monitor returns early.\n\n");

	/* single monitor */
	cleanup_clients();
	add_clients(selmon, 5, 1000);
	selmon->sel = selmon->clients;

	ev.type = EnterNotify;
	ev.window = 1002;
	ev.mode = NotifyNormal;
	ev.detail = NotifyNonlinear;
	ev.root = root;

	t0 = nanos_mono();
	for (i = 0; i < N; i++)
		enternotify((XEvent *)&ev);
	t1 = nanos_mono();
	report("enternotify() x N (single monitor, 5 clients)", t1 - t0, N);

	/* add second monitor */
	second = make_monitor(1);
	selmon->next = second;
	second->tagset[0] = second->tagset[1] = 1;
	second->lt[0] = second->lt[1] = &layouts[0];
	second->gap.isgap = 1;
	second->gap.realgap = 17;
	second->gap.gappx = 17;

	t0 = nanos_mono();
	for (i = 0; i < N; i++)
		enternotify((XEvent *)&ev);
	t1 = nanos_mono();
	report("enternotify() x N (2 monitors, 5 clients)", t1 - t0, N);

	/* restore single monitor */
	selmon->next = NULL;
	free(second);
}

/* ── BENCH 9: gaming mode (fullscreen) ─────────────────────────────── */

static void
bench_gaming_mode(void)
{
	int N = 50000;
	int i;
	long t0, t1;
	Client *c;

	fprintf(stderr, "\n=== 9. gaming mode (fullscreen overhead) ===\n");
	fprintf(stderr, "  Measures overhead of handlers during fullscreen.\n");
	fprintf(stderr, "  After optimization: propertynotify/updatestatus skipped.\n\n");

	cleanup_clients();
	add_clients(selmon, 5, 1000);
	c = selmon->clients;
	selmon->sel = c;
	c->isfullscreen = 1;

	/* propertynotify during fullscreen — currently still runs */
	{
		XPropertyEvent pev;
		pev.type = PropertyNotify;
		pev.window = 1002;
		pev.atom = 9999;
		pev.state = 0;

		t0 = nanos_mono();
		for (i = 0; i < N; i++)
			propertynotify((XEvent *)&pev);
		t1 = nanos_mono();
		report("propertynotify(uninteresting) during fullscreen x N", t1 - t0, N);
	}

	/* updatestatus during fullscreen — currently guarded */
	t0 = nanos_mono();
	for (i = 0; i < N; i++) {
		updatestatus();
		selmon->bar_dirty_segments = 0;
		bar_draw_pending = 0;
	}
	t1 = nanos_mono();
	report("updatestatus() during fullscreen x N (already guarded)", t1 - t0, N);

	/* enternotify during fullscreen — currently still runs */
	{
		XCrossingEvent cev;
		cev.type = EnterNotify;
		cev.window = 1002;
		cev.mode = NotifyNormal;
		cev.detail = NotifyNonlinear;
		cev.root = root;

		t0 = nanos_mono();
		for (i = 0; i < N; i++)
			enternotify((XEvent *)&cev);
		t1 = nanos_mono();
		report("enternotify() during fullscreen x N", t1 - t0, N);
	}

	c->isfullscreen = 0;
}

/* ── main ──────────────────────────────────────────────────────────── */

/* BENCH 10: arrange() coalescing under event dispatch */
static void
bench_arrange_coalesce(void)
{
	int N = 50000;
	int i;
	long t0, t1;

	fprintf(stderr, "\n=== 10. arrange() event-loop coalescing ===\n");
	fprintf(stderr, "  Immediate path serves grabs; dispatched events defer\n");
	fprintf(stderr, "  to one layout pass per event batch.\n\n");

	cleanup_clients();
	add_clients(selmon, 10, 1000);
	selmon->showbar = 0;

	/* baseline: every request lays out immediately (grab-path cost) */
	t0 = nanos_mono();
	for (i = 0; i < N; i++)
		arrangenow(selmon);
	t1 = nanos_mono();
	report("arrangenow x N (immediate, 10 clients)", t1 - t0, N);

	/* optimized burst: 3 requests coalesce into a single tail flush */
	t0 = nanos_mono();
	for (i = 0; i < N; i++) {
		dispatching = 1;
		arrange(selmon);
		arrange(selmon);
		arrange(selmon);
		dispatching = 0;
		flusheventtail();
	}
	t1 = nanos_mono();
	report("event batch: 3 defers + flush x N", t1 - t0, N);

	cleanup_clients();
}

/* BENCH 11: keypress() dispatch cost */
/* Answers "would sorting keys[] by sym/mod help?": measures the masked
 * fast-path reject vs the full-scan dispatch for an actual binding. */
static void
bench_keypress(void)
{
	int N = 50000;
	int i;
	long t0, t1;
	XEvent ev;

	fprintf(stderr, "\n=== 11. keypress() dispatch ===\n");
	fprintf(stderr, "  Fast-path masks reject unmatched events without\n");
	fprintf(stderr, "  scanning keys[]; matched events pay the full scan.\n\n");

	/* unmatched modifier: both masks reject before the loop */
	memset(&ev, 0, sizeof ev);
	ev.xkey.type = KeyPress;
	ev.xkey.keycode = XK_a;
	ev.xkey.state = 0; /* no binding uses bare state */

	t0 = nanos_mono();
	for (i = 0; i < N; i++)
		keypress(&ev);
	t1 = nanos_mono();
	report("keypress(unmatched mod, mask reject) x N", t1 - t0, N);

	/* wrong chord sharing a mod bit: old OR-mask guard passed this to a
	 * full keys[] scan; the exact set rejects it in O(1) */
	selmon->showbar = 0;
	memset(&ev, 0, sizeof ev);
	ev.xkey.type = KeyPress;
	ev.xkey.keycode = XK_b;
	ev.xkey.state = MODKEY | ControlMask;

	t0 = nanos_mono();
	for (i = 0; i < N; i++)
		keypress(&ev);
	t1 = nanos_mono();
	report("keypress(wrong chord, exact-set reject) x N", t1 - t0, N);

	/* matched: MODKEY+b -> togglebar, pays the full keys[] scan */
	selmon->showbar = 0;
	memset(&ev, 0, sizeof ev);
	ev.xkey.type = KeyPress;
	ev.xkey.keycode = XK_b;
	ev.xkey.state = MODKEY;

	t0 = nanos_mono();
	for (i = 0; i < N; i++) {
		keypress(&ev);
		selmon->showbar = 0;                    /* undo toggle */
		selmon->bar_dirty_segments = 0;
		bar_draw_pending = 0;
	}
	t1 = nanos_mono();
	report("keypress(MODKEY+b, full scan + dispatch) x N", t1 - t0, N);
}

int main(void)
{
	init_globals();

	fprintf(stderr, "=== bench_optimize: Baseline Performance ===\n");
	fprintf(stderr, "All measurements use mock X11 (no real server).\n");
	fprintf(stderr, "Reported as nanoseconds per call.\n");

	bench_arrange_coalescing();
	bench_updatestatus_comparison();
	bench_focus_idempotent();
	bench_wintoclient_hash();
	bench_tile_monocle_count();
	bench_propertynotify_filter();
	bench_setlayout_guard();
	bench_enternotify_guard();
	bench_gaming_mode();
	bench_arrange_coalesce();
	bench_keypress();

	fprintf(stderr, "\n=== Summary ===\n");
	fprintf(stderr, "Total benchmarks: %d\n", bench_count);
	fprintf(stderr, "\nAfter implementing optimizations, re-run to measure improvement.\n");
	fprintf(stderr, "Expected improvements per optimization:\n");
	fprintf(stderr, "  1. arrange() coalescing:     3–5%% (burst events)\n");
	fprintf(stderr, "  2. updatestatus() compare:   2–3%% (identical status text)\n");
	fprintf(stderr, "  3. focus() idempotent:       2–4%% (redundant focus calls)\n");
	fprintf(stderr, "  4. wintoclient() hash:       0.5–1%% (O(n) → O(1))\n");
	fprintf(stderr, "  5. tile/monocle count cache: 1–2%% (O(n) walk eliminated)\n");
	fprintf(stderr, "  6. propertynotify filter:    1–2%% (skip uninteresting atoms)\n");
	fprintf(stderr, "  7. setlayout() guard:        <0.5%% (skip redundant dirty)\n");
	fprintf(stderr, "  8. enternotify() guard:      0.5–1%% (single-monitor)\n");
	fprintf(stderr, "  9. gaming mode:              5–10%% (during fullscreen)\n");

	return 0;
}
