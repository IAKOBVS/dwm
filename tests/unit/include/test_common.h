/*
 * test_common.h — shared fixture for the per-function dwm test suites.
 *
 * Including this header is equivalent to the boilerplate every legacy suite
 * repeated at the top of its translation unit: pull in the mock X11/Xft
 * headers, include dwm.h and dwm.c exactly once, define the ASSERT helpers,
 * and provide the standard global setup/teardown plus a handful of fixture
 * constructors.
 *
 * Each per-function test file must define exactly one `static void
 * run_tests(void)` after including this header.  The header supplies main()
 * so the per-function files contain only test cases and the list that runs
 * them.  dwm.c is included here, never by the test file, so it is compiled
 * once per translation unit.
 */

#ifndef DWM_TEST_COMMON_H
#define DWM_TEST_COMMON_H 1

#ifndef DWM_TEST
#define DWM_TEST 1
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <locale.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "mock_x11.h"

/* Prevent drw.h from being included; mock_drw.h supplies the same types and
 * no-op drawing helpers so tests never need a real X server. */
#define DRW_H
#include "mock_drw.h"

#include "dwm.h"
#include "dwm.c"

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
#define ASSERT_NE(a, b, msg) do { \
	total++; \
	if ((a) == (b)) { \
		failed++; \
		fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, msg); \
	} \
} while(0)

/* Some handler suites pass a pointer to an all-zero argument. */
static Arg arg_zero;

static Monitor *saved_selmon;

static void
save_selmon(void)
{
	saved_selmon = selmon;
}

static void
restore_selmon(void)
{
	selmon = saved_selmon;
	mons = saved_selmon;
}

/* Unmanage every client on the primary monitor, releasing clients that
 * were created inside dwm (manage/maprequest/scan) rather than by the
 * test fixture. */
static void
drain_clients(void)
{
	while (mons && mons->clients)
		unmanage(mons->clients, 1);
}

/* Release all globals a prior setup() overwrote: monitors, fonts,
 * scheme colors, cursors, drawing context. */
static void
free_global_state(void)
{
	Monitor *m, *mnext;
	Fnt *f, *fnext;
	size_t i;

	m = mons;
	while (m) { mnext = m->next; free(m); m = mnext; }
	mons = selmon = NULL;
	if (drw) {
		f = drw->fonts;
		while (f) { fnext = f->next; free(f); f = fnext; }
		free(drw);
		drw = NULL;
	}
	if (scheme) {
		for (i = 0; i < LENGTH(colors); i++)
			free(scheme[i]);
		free(scheme);
		scheme = NULL;
	}
	for (i = 0; i < CurLast; i++) {
		free(cursor[i]);
		cursor[i] = NULL;
	}
}

/* make_monitor — construct a heap Monitor with the defaults the legacy
 * suites used: one tag selected, tile layout, gaps enabled, 1920x1080. */
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

/* make_client — construct a heap Client attached to the window index so
 * wintoclient()/wintomon() see it exactly like a managed window. */
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
	c->x = 100;
	c->y = 100;
	c->w = 200;
	c->h = 200;
	c->oldw = c->w;
	c->oldh = c->h;
	c->basew = 50;
	c->baseh = 50;
	/* One pixel wider than the default geometry so the size-hint clamp
	 * path is observable when a test resizes a 200x200 fixture. */
	c->minw = 201;
	c->minh = 201;
	c->maxw = 0;
	c->maxh = 0;
	c->incw = 0;
	c->inch = 0;
	c->mina = 0.0f;
	c->maxa = 0.0f;
	c->hintsvalid = 1; /* keep the fixture's hint fields instead of refetching */
	winclient_put(c); /* keep wintoclient's window index consistent */
	return c;
}

/* Reset the window index between test cases. */
static void
reset_index(void)
{
	memset(winhash, 0, sizeof winhash);
	winhash_count = 0;
	winhash_overflowed = 0;
}

/* init_test_globals — reproduce the global fixture legacy main() set up:
 * one monitor, scheme colours, cursors, atoms.  Called once per binary. */
static void
init_test_globals(void)
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
	wmcheckwin = 0;
	lifecycle = LIFE_LIVE;
	bar_clear_on_draw = 0; /* keep dirty segments observable after drawbar() */
	arrange_calls = 0;

	selmon = calloc(1, sizeof(Monitor));
	mons = selmon;
	selmon->tagset[0] = 1;
	selmon->tagset[1] = 1;
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

	scheme = ecalloc(2, sizeof(Clr *));
	for (i = SchemeNorm; i <= SchemeSel; i++)
		scheme[i] = drw_scm_create(drw, colors[i], 3);

	cursor[CurNormal] = drw_cur_create(drw, XC_left_ptr);
	cursor[CurResize] = drw_cur_create(drw, XC_sizing);
	cursor[CurMove]   = drw_cur_create(drw, XC_fleur);

	netatom[NetWMState] = 1;
	netatom[NetWMFullscreen] = 2;
	netatom[NetActiveWindow] = 3;
	netatom[NetWMWindowType] = 4;
	netatom[NetWMWindowTypeDialog] = 5;
	netatom[NetClientList] = 6;
	netatom[NetWMCheck] = 7;
	netatom[NetWMName] = 8;
	netatom[NetSupported] = 9;
	wmatom[WMProtocols] = 100;
	wmatom[WMDelete] = 101;
	wmatom[WMState] = 102;
	wmatom[WMTakeFocus] = 103;

	reset_index();
}

/* Shared xerror fallback used by xerror tests. */
static int mock_xerrorxlib_called;

static int
mock_xerrorxlib_handler(Display *dpy, XErrorEvent *ee)
{
	(void)dpy; (void)ee;
	mock_xerrorxlib_called = 1;
	return 0;
}

/* finish_tests — report the aggregate result and give make a useful exit. */
static int
finish_tests(void)
{
	fprintf(stderr, "=== RESULTS ===\n");
	fprintf(stderr, "Total: %d | Passed: %d | Failed: %d\n",
		total, total - failed, failed);
	if (failed) {
		fprintf(stderr, "*** %d TESTS FAILED ***\n", failed);
		return 1;
	}
	fprintf(stdout, "All tests passed.\n");
	return 0;
}

/* Every per-function test file provides this. */
static void run_tests(void);

int
main(void)
{
	init_test_globals();
	run_tests();
	return finish_tests();
}

#endif /* DWM_TEST_COMMON_H */
