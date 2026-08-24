#define DWM_TEST 1
#define _GNU_SOURCE

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

static void
save_selmon(void)
{
	saved_selmon = selmon;
}

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
	m->sel = NULL;
	m->clients = NULL;
	m->stack = NULL;
	m->next = NULL;
	return m;
}

static void
cleanup_monitor(Monitor *m)
{
	free(m);
}

/* --- arrange --- */

static void
test_arrange_null(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	Monitor *m1 = make_monitor(0);
	Monitor *m2 = make_monitor(1);
	m1->next = m2;
	mons = selmon = m1;

	arrange(NULL);
	ASSERT(1, "arrange(NULL) with two monitors does not crash");
	mons = selmon = saved_selmon;
	cleanup_monitor(m1);
	cleanup_monitor(m2);
}

static void
test_arrange_tile_no_clients(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	Monitor *m = make_monitor(0);
	mons = selmon = m;

	arrange(m);
	ASSERT(1, "arrange with tile layout and no clients does not crash");
	mons = selmon = saved_selmon;
	cleanup_monitor(m);
}

static void
test_arrange_monocle_no_clients(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	Monitor *m = make_monitor(0);
	m->lt[0] = m->lt[1] = &layouts[1];
	strncpy(m->ltsymbol, layouts[1].symbol, sizeof m->ltsymbol);
	mons = selmon = m;

	arrange(m);
	ASSERT(1, "arrange with monocle layout and no clients does not crash");
	mons = selmon = saved_selmon;
	cleanup_monitor(m);
}

/* --- arrangemon --- */

static void
test_arrangemon_tile(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor m;
	memset(&m, 0, sizeof m);
	m.tagset[0] = 1; m.seltags = 0;
	m.mfact = 0.55f; m.nmaster = 1;
	m.mx = m.wx = 0; m.my = m.wy = 0;
	m.mw = m.ww = 1920; m.mh = m.wh = 1080;
	m.lt[0] = &layouts[0]; m.lt[1] = &layouts[0]; m.sellt = 0;
	m.gap.isgap = 1; m.gap.realgap = 17; m.gap.gappx = 17;

	arrangemon(&m);
	ASSERT(strcmp(m.ltsymbol, "[]=") == 0, "arrangemon tile sets ltsymbol to tile symbol");
}

static void
test_arrangemon_floating(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor m;
	memset(&m, 0, sizeof m);
	m.tagset[0] = 1; m.seltags = 0;
	m.mfact = 0.55f; m.nmaster = 1;
	m.mx = m.wx = 0; m.my = m.wy = 0;
	m.mw = m.ww = 1920; m.mh = m.wh = 1080;
	m.lt[0] = &layouts[1]; m.lt[1] = &layouts[1]; m.sellt = 0;
	m.gap.isgap = 1; m.gap.realgap = 17; m.gap.gappx = 17;

	arrangemon(&m);
	ASSERT(strcmp(m.ltsymbol, "><>") == 0, "arrangemon floating sets ltsymbol to floating symbol");
}

/* --- tile with clients --- */

static void
test_tile_one_client(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor m;
	memset(&m, 0, sizeof m);
	m.tagset[0] = 1; m.seltags = 0;
	m.mfact = 0.55f; m.nmaster = 1;
	m.mx = m.wx = 0; m.my = m.wy = 0;
	m.mw = m.ww = 1920; m.mh = m.wh = 1080;
	m.lt[0] = &layouts[0]; m.lt[1] = &layouts[0]; m.sellt = 0;
	m.gap.isgap = 1; m.gap.realgap = 17; m.gap.gappx = 17;

	Client c1 = { .win = 1, .mon = &m, .tags = 1, .bw = 0 };
	m.clients = &c1;
	m.stack = &c1;

	tile(&m);
	ASSERT_EQ(c1.x, m.wx + m.gap.gappx, "tile one client: x = wx + gappx");
	ASSERT_EQ(c1.y, m.wy + m.gap.gappx, "tile one client: y = wy + gappx");
	ASSERT_EQ(c1.w, m.mw - 2 * m.gap.gappx - 2 * c1.bw, "tile one client: w = mw - 2*gappx");
	ASSERT_EQ(c1.h, m.mh - 2 * m.gap.gappx - 2 * c1.bw, "tile one client: h = mh - 2*gappx");
}

static void
test_tile_two_clients_master_stack(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor m;
	memset(&m, 0, sizeof m);
	m.tagset[0] = 1; m.seltags = 0;
	m.mfact = 0.55f; m.nmaster = 1;
	m.mx = m.wx = 0; m.my = m.wy = 0;
	m.mw = m.ww = 1920; m.mh = m.wh = 1080;
	m.lt[0] = &layouts[0]; m.lt[1] = &layouts[0]; m.sellt = 0;
	m.gap.isgap = 1; m.gap.realgap = 17; m.gap.gappx = 17;

	Client c1 = { .win = 1, .mon = &m, .tags = 1, .bw = 0 };
	Client c2 = { .win = 2, .mon = &m, .tags = 1, .bw = 0 };
	/* order in stack determines which is master (stack is checked first by nexttiled via focus) */
	c2.next = &c1;
	c2.snext = &c1;
	m.clients = &c2;
	m.stack = &c2;

	tile(&m);
	/* master area width = mw * mfact = 1920 * 0.55 = 1056 */
	int mw = m.ww * m.mfact;
	ASSERT_EQ(c2.x, m.wx + m.gap.gappx, "tile two: master x = wx + gappx");
	ASSERT_EQ(c2.w, mw - 2 * c2.bw - m.gap.gappx, "tile two: master w = mw - gappx");
	/* stack client starts to the right of master */
	ASSERT_EQ(c1.x, m.wx + mw + m.gap.gappx, "tile two: stack x = wx + mw + gappx");
	ASSERT_EQ(c1.w, m.ww - mw - 2 * c1.bw - 2 * m.gap.gappx, "tile two: stack w = ww - mw - 2*gappx");
}

static void
test_tile_nmaster_zero(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor m;
	memset(&m, 0, sizeof m);
	m.tagset[0] = 1; m.seltags = 0;
	m.mfact = 0.55f; m.nmaster = 0;
	m.mx = m.wx = 0; m.my = m.wy = 0;
	m.mw = m.ww = 1920; m.mh = m.wh = 1080;
	m.lt[0] = &layouts[0]; m.lt[1] = &layouts[0]; m.sellt = 0;
	m.gap.isgap = 1; m.gap.realgap = 17; m.gap.gappx = 17;

	Client c1 = { .win = 1, .mon = &m, .tags = 1, .bw = 0 };
	m.clients = &c1;
	m.stack = &c1;

	tile(&m);
	ASSERT_EQ(c1.x, m.wx + m.gap.gappx, "tile nmaster=0: client x goes to stack area");
	ASSERT_EQ(c1.y, m.wy + m.gap.gappx, "tile nmaster=0: client y = wy + gappx");
}

static void
test_tile_nmaster_exceeds_n(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor m;
	memset(&m, 0, sizeof m);
	m.tagset[0] = 1; m.seltags = 0;
	m.mfact = 0.55f; m.nmaster = 5;
	m.mx = m.wx = 0; m.my = m.wy = 0;
	m.mw = m.ww = 1920; m.mh = m.wh = 1080;
	m.lt[0] = &layouts[0]; m.lt[1] = &layouts[0]; m.sellt = 0;
	m.gap.isgap = 1; m.gap.realgap = 17; m.gap.gappx = 17;

	Client c1 = { .win = 1, .mon = &m, .tags = 1, .bw = 0 };
	m.clients = &c1;
	m.stack = &c1;

	tile(&m);
	/* n=1, nmaster=5, so n <= nmaster -> mw = ww - gappx = 1903 */
	int mw = m.ww - m.gap.gappx;
	ASSERT_EQ(c1.x, m.wx + m.gap.gappx, "tile nmaster > n: client in master area");
	ASSERT_EQ(c1.w, mw - 2 * c1.bw - m.gap.gappx, "tile nmaster > n: full-width");
}

/* --- monocle with clients --- */

static void
test_monocle_one_client(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor m;
	memset(&m, 0, sizeof m);
	m.tagset[0] = 1; m.seltags = 0;
	m.mfact = 0.55f; m.nmaster = 1;
	m.mx = m.wx = 0; m.my = m.wy = 0;
	m.mw = m.ww = 1920; m.mh = m.wh = 1080;
	m.lt[0] = &layouts[1]; m.lt[1] = &layouts[1]; m.sellt = 0;
	m.gap.isgap = 1; m.gap.realgap = 17; m.gap.gappx = 17;

	Client c1 = { .win = 1, .mon = &m, .tags = 1, .bw = 0, .w = 100, .h = 100 };
	m.clients = &c1;

	monocle(&m);
	ASSERT(strcmp(m.ltsymbol, "[1]") == 0, "monocle one client shows [1]");
	ASSERT_EQ(c1.x, m.wx, "monocle: client x = wx");
	ASSERT_EQ(c1.y, m.wy, "monocle: client y = wy");
	ASSERT_EQ(c1.w, m.ww - 2 * c1.bw, "monocle: client w = ww - 2*bw");
	ASSERT_EQ(c1.h, m.wh - 2 * c1.bw, "monocle: client h = wh - 2*bw");
}

static void
test_monocle_two_clients_count(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor m;
	memset(&m, 0, sizeof m);
	m.tagset[0] = 1; m.seltags = 0;
	m.mfact = 0.55f; m.nmaster = 1;
	m.mx = m.wx = 0; m.my = m.wy = 0;
	m.mw = m.ww = 1920; m.mh = m.wh = 1080;
	m.lt[0] = &layouts[1]; m.lt[1] = &layouts[1]; m.sellt = 0;

	Client c1 = { .win = 1, .mon = &m, .tags = 1, .bw = 0 };
	Client c2 = { .win = 2, .mon = &m, .tags = 1, .bw = 0 };
	c1.next = &c2;
	m.clients = &c1;

	monocle(&m);
	ASSERT(strcmp(m.ltsymbol, "[2]") == 0, "monocle two clients shows [2]");
}

/* --- resize / resizeclient --- */

static void
test_resize_updates_geometry(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Client c;
	memset(&c, 0, sizeof c);
	c.x = 10; c.y = 20; c.w = 100; c.h = 200;
	c.oldx = c.oldy = c.oldw = c.oldh = 0;
	c.bw = 2;
	c.minw = c.minh = 1;
	c.mon = selmon;

	resize(&c, 50, 60, 300, 400, 0);
	ASSERT_EQ(c.x, 50, "resize sets x");
	ASSERT_EQ(c.y, 60, "resize sets y");
	ASSERT_EQ(c.w, 300, "resize sets w");
	ASSERT_EQ(c.h, 400, "resize sets h");
	ASSERT_EQ(c.oldx, 10, "resize preserves oldx");
	ASSERT_EQ(c.oldy, 20, "resize preserves oldy");
	ASSERT_EQ(c.oldw, 100, "resize preserves oldw");
	ASSERT_EQ(c.oldh, 200, "resize preserves oldh");
}

static void
test_resize_clamps_h_to_bh(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Client c;
	memset(&c, 0, sizeof c);
	c.x = 0; c.y = 0; c.w = 0; c.h = 0;
	c.bw = 0;
	c.minw = 1; c.minh = 1;
	c.mon = selmon;

	resize(&c, 10, 20, 5, 5, 0);
	/* applysizehints clamps w,h to MAX(1,*), then to bh minimum */
	ASSERT_EQ(c.w, bh, "resize clamps w to bh (22)");
	ASSERT_EQ(c.h, bh, "resize clamps h to bh (22)");
}

/* --- setfocus --- */

static void
test_setfocus_normal(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Client c;
	memset(&c, 0, sizeof c);
	c.win = 1;
	c.neverfocus = 0;

	setfocus(&c);
	ASSERT(1, "setfocus on normal client does not crash");
}

static void
test_setfocus_neverfocus(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Client c;
	memset(&c, 0, sizeof c);
	c.win = 1;
	c.neverfocus = 1;

	setfocus(&c);
	ASSERT(1, "setfocus on neverfocus client does not crash");
}

/* --- showhide --- */

static void
test_showhide_null(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	showhide(NULL);
	ASSERT(1, "showhide(NULL) does not crash");
}

static void
test_showhide_visible(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor m;
	memset(&m, 0, sizeof m);
	m.tagset[0] = 1; m.seltags = 0;
	m.lt[0] = &layouts[0]; m.lt[1] = &layouts[0]; m.sellt = 0;

	Client c = { .win = 1, .mon = &m, .tags = 1 };
	c.snext = NULL;

	showhide(&c);
	ASSERT(1, "showhide visible client does not crash");
}

static void
test_showhide_invisible(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor m;
	memset(&m, 0, sizeof m);
	m.tagset[0] = 1; m.seltags = 0;
	m.lt[0] = &layouts[0]; m.lt[1] = &layouts[0]; m.sellt = 0;

	Client c = { .win = 1, .mon = &m, .tags = 1 };
	c.tags = 0;
	c.snext = NULL;

	showhide(&c);
	ASSERT(1, "showhide invisible client does not crash");
}

/* --- focus --- */

static void
test_focus_sel_changes(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Client *old_sel = selmon->sel;
	Client c = { .win = 2, .mon = selmon, .tags = 1 };
	c.snext = NULL;

	selmon->sel = NULL;
	selmon->stack = &c;
	focus(&c);
	ASSERT(selmon->sel == &c, "focus sets selmon->sel to focused client");
	selmon->sel = old_sel;
}

static void
test_focus_null_finds_next_visible(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Client c = { .win = 2, .mon = selmon, .tags = 1 };
	c.snext = NULL;
	selmon->stack = &c;
	selmon->sel = NULL;

	focus(NULL);
	/* focus(NULL) walks stack to find visible client, selects it */
	ASSERT(selmon->sel == &c, "focus(NULL) selects first visible client from stack");
}

/* --- unfocus --- */

static void
test_unfocus_normal(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Client c;
	memset(&c, 0, sizeof c);
	c.win = 1;
	c.tags = 1;

	unfocus(&c, 1);
	ASSERT(1, "unfocus with setfocus does not crash");
}

static void
test_unfocus_nosetfocus(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Client c;
	memset(&c, 0, sizeof c);
	c.win = 1;
	c.tags = 1;

	unfocus(&c, 0);
	ASSERT(1, "unfocus without setfocus does not crash");
}

/* --- main --- */

int
main(void)
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

	test_arrange_null();
	test_arrange_tile_no_clients();
	test_arrange_monocle_no_clients();

	test_arrangemon_tile();
	test_arrangemon_floating();

	test_tile_one_client();
	test_tile_two_clients_master_stack();
	test_tile_nmaster_zero();
	test_tile_nmaster_exceeds_n();

	test_monocle_one_client();
	test_monocle_two_clients_count();

	test_resize_updates_geometry();
	test_resize_clamps_h_to_bh();

	test_setfocus_normal();
	test_setfocus_neverfocus();

	test_showhide_null();
	test_showhide_visible();
	test_showhide_invisible();

	test_focus_sel_changes();
	test_focus_null_finds_next_visible();

	test_unfocus_normal();
	test_unfocus_nosetfocus();

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
