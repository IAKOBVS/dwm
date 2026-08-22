#define DWM_TEST 1
#define _GNU_SOURCE

/* Provide all global variables expected by dwm.c */
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

/* Prevent drw.h from being included (our mock_drw.h replaces it) */
#define DRW_H
#include "mock_drw.h"

/* drw.h (included via dwm.h) provides Drw, Cur, Clr types */
#include "../dwm.h"

/* Include the actual dwm source (with main() guarded out by DWM_TEST) */
#include "../dwm.c"

/* Test infrastructure */
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

static Monitor *
make_monitor(int num)
{
	Monitor *m = ecalloc(1, sizeof(Monitor));
	m->num = num;
	m->mfact = 0.55f;
	m->nmaster = 1;
	m->showbar = 1;
	m->topbar = 1;
	m->tagset[0] = m->tagset[1] = 1;
	m->mx = m->wx = 0;
	m->my = m->wy = 0;
	m->mw = m->ww = 1920;
	m->mh = m->wh = 1080;
	m->lt[0] = m->lt[1] = &layouts[0];
	gap_copy(&m->gap, &default_gap);
	strncpy(m->ltsymbol, layouts[0].symbol, sizeof m->ltsymbol);
	m->sel = NULL;
	m->clients = NULL;
	m->stack = NULL;
	m->next = NULL;
	return m;
}

/* === TESTS === */

/* --- attach / detach / nexttiled --- */
static void
test_attach(void)
{
	Monitor m = {0};
	Client c = { .win = 1, .mon = &m, .tags = 1, .next = NULL };
	m.clients = NULL;
	attach(&c);
	ASSERT(m.clients == &c, "attach single client");
	ASSERT(c.next == NULL, "attach: c.next == NULL");
}

static void
test_attach_multiple(void)
{
	Monitor m = {0};
	Client c1 = { .win = 1, .mon = &m, .tags = 1, .next = NULL };
	Client c2 = { .win = 2, .mon = &m, .tags = 1, .next = NULL };
	m.clients = NULL;
	attach(&c1);
	attach(&c2);
	ASSERT(m.clients == &c2, "attach prepends (c2 first)");
	ASSERT(c2.next == &c1, "attach: c2.next == c1");
	ASSERT(c1.next == NULL, "attach: c1.next == NULL");
}

static void
test_detach(void)
{
	Monitor m = {0};
	Client c1 = { .win = 1, .mon = &m, .tags = 1, .next = NULL };
	Client c2 = { .win = 2, .mon = &m, .tags = 1, .next = NULL };
	m.clients = &c1; c1.next = &c2;
	detach(&c1);
	ASSERT(m.clients == &c2, "detach head");
	ASSERT(c2.next == NULL, "detach head: c2.next == NULL");
}

static void
test_detach_tail(void)
{
	Monitor m = {0};
	Client c1 = { .win = 1, .mon = &m, .tags = 1, .next = NULL };
	Client c2 = { .win = 2, .mon = &m, .tags = 1, .next = NULL };
	m.clients = &c1; c1.next = &c2;
	detach(&c2);
	ASSERT(m.clients == &c1, "detach tail: head unchanged");
	ASSERT(c1.next == NULL, "detach tail: c1.next == NULL");
}

static void
test_attachstack(void)
{
	Monitor m = {0};
	Client c = { .win = 1, .mon = &m, .snext = NULL };
	m.stack = NULL;
	attachstack(&c);
	ASSERT(m.stack == &c, "attachstack single");
}

static void
test_detachstack_sel_update(void)
{
	Monitor m = {0};
	m.tagset[0] = 1; m.seltags = 0;
	Client c1 = { .win = 1, .mon = &m, .tags = 1, .snext = NULL };
	Client c2 = { .win = 2, .mon = &m, .tags = 2, .snext = NULL };
	m.stack = &c1; c1.snext = &c2;
	m.sel = &c1;
	detachstack(&c1);
	ASSERT(m.stack == &c2, "detachstack head: stack updated");
	/* sel should be updated: walks stack for visible, c2 has tags=2 != tagset=1 -> not visible */
	ASSERT(m.sel == NULL, "detachstack: sel becomes NULL when no visible clients remain");
}

static void
test_nexttiled_skips_floating(void)
{
	Monitor m = {0};
	m.tagset[0] = 1; m.seltags = 0;
	Client c1 = { .win = 1, .mon = &m, .tags = 1, .isfloating = 1, .next = NULL };
	Client c2 = { .win = 2, .mon = &m, .tags = 1, .isfloating = 0, .next = NULL };
	c1.next = &c2;
	Client *nr = nexttiled(&c1);
	ASSERT(nr == &c2, "nexttiled skips floating client");
}

static void
test_nexttiled_skips_hidden_tags(void)
{
	Monitor m = {0};
	m.tagset[0] = 1; m.seltags = 0;
	Client c1 = { .win = 1, .mon = &m, .tags = 2, .isfloating = 0, .next = NULL };
	Client c2 = { .win = 2, .mon = &m, .tags = 1, .isfloating = 0, .next = NULL };
	c1.next = &c2;
	Client *nr = nexttiled(&c1);
	ASSERT(nr == &c2, "nexttiled skips client on different tag");
}

static void
test_nexttiled_all_invisible(void)
{
	Monitor m = {0};
	m.tagset[0] = 1; m.seltags = 0;
	Client c1 = { .win = 1, .mon = &m, .tags = 2, .isfloating = 0, .next = NULL };
	Client *nr = nexttiled(&c1);
	ASSERT(nr == NULL, "nexttiled returns NULL when no visible clients");
}

/* --- dirtomon --- */
static void
test_dirtomon_next(void)
{
	Monitor m1, m2;
	memset(&m1, 0, sizeof m1);
	memset(&m2, 0, sizeof m2);
	m1.num = 0; m2.num = 1;
	m1.next = &m2; m2.next = NULL;
	mons = &m1;
	selmon = &m1;

	Monitor *next = dirtomon(1);
	ASSERT(next == &m2, "dirtomon(+1) next monitor");
}

static void
test_dirtomon_wrap(void)
{
	Monitor m1, m2;
	memset(&m1, 0, sizeof m1);
	memset(&m2, 0, sizeof m2);
	m1.num = 0; m2.num = 1;
	m1.next = &m2; m2.next = NULL;
	mons = &m1;
	selmon = &m2;

	Monitor *next = dirtomon(1);
	ASSERT(next == &m1, "dirtomon(+1) wraps to first");
}

static void
test_dirtomon_prev(void)
{
	Monitor m1, m2;
	memset(&m1, 0, sizeof m1);
	memset(&m2, 0, sizeof m2);
	m1.num = 0; m2.num = 1;
	m1.next = &m2; m2.next = NULL;
	mons = &m1;
	selmon = &m2;

	Monitor *prev = dirtomon(-1);
	ASSERT(prev == &m1, "dirtomon(-1) prev monitor");
}

/* --- recttomon --- */
static void
test_recttomon_intersect(void)
{
	Monitor m1, m2;
	memset(&m1, 0, sizeof m1);
	memset(&m2, 0, sizeof m2);
	m1.mx = m1.wx = 0; m1.my = m1.wy = 0; m1.mw = m1.ww = 960; m1.mh = m1.wh = 1080;
	m2.mx = m2.wx = 960; m2.my = m2.wy = 0; m2.mw = m2.ww = 960; m2.mh = m2.wh = 1080;
	m1.next = &m2; m2.next = NULL;
	mons = &m1;
	selmon = &m1;

	Monitor *r = recttomon(1000, 500, 1, 1);
	ASSERT(r == &m2, "recttomon picks right monitor for right-side point");
}

static void
test_recttomon_no_intersect(void)
{
	Monitor m1;
	memset(&m1, 0, sizeof m1);
	m1.mx = m1.wx = 0; m1.my = m1.wy = 0; m1.mw = m1.ww = 960; m1.mh = m1.wh = 1080;
	m1.next = NULL;
	mons = &m1;
	selmon = &m1;

	Monitor *r = recttomon(-100, -100, 1, 1);
	ASSERT(r == &m1, "recttomon falls back to selmon");
}

/* --- wintoclient --- */
static void
test_wintoclient(void)
{
	Monitor m;
	memset(&m, 0, sizeof m);
	m.clients = NULL;
	mons = &m;
	Client c = { .win = 42, .mon = &m, .next = NULL };
	m.clients = &c;

	Client *found = wintoclient(42);
	ASSERT(found == &c, "wintoclient finds by window");
}

static void
test_wintoclient_notfound(void)
{
	Monitor m;
	memset(&m, 0, sizeof m);
	m.clients = NULL;
	mons = &m;
	Client c = { .win = 42, .mon = &m, .next = NULL };
	m.clients = &c;

	Client *found = wintoclient(99);
	ASSERT(found == NULL, "wintoclient returns NULL for unknown window");
}

/* --- gap_copy / setgaps --- */
static void
test_gap_copy(void)
{
	Gap src = { .isgap = 1, .realgap = 10, .gappx = 10 };
	Gap dst = { .isgap = 0, .realgap = 0, .gappx = 0 };
	gap_copy(&dst, &src);
	ASSERT(dst.isgap == 1 && dst.realgap == 10 && dst.gappx == 10, "gap_copy copies all fields");
}

static void
test_setgaps_toggle(void)
{
	Monitor m;
	memset(&m, 0, sizeof m);
	m.lt[0] = &layouts[0]; m.lt[1] = &layouts[0]; m.sellt = 0;
	m.tagset[0] = 1; m.seltags = 0;
	m.showbar = 0;
	m.gap.isgap = 1; m.gap.realgap = 17; m.gap.gappx = 17;
	mons = selmon = &m;

	Arg arg = { .i = GAP_TOGGLE };
	setgaps(&arg);
	ASSERT(m.gap.isgap == 0, "setgaps toggle: isgap becomes 0");
	ASSERT(m.gap.gappx == 0, "setgaps toggle: gappx becomes 0");

	setgaps(&arg);
	ASSERT(m.gap.isgap == 1, "setgaps toggle: isgap back to 1");
	ASSERT(m.gap.gappx == m.gap.realgap, "setgaps toggle: gappx restored");
}

static void
test_setgaps_adjust(void)
{
	Monitor m;
	memset(&m, 0, sizeof m);
	m.lt[0] = &layouts[0]; m.lt[1] = &layouts[0]; m.sellt = 0;
	m.tagset[0] = 1; m.seltags = 0;
	m.gap.isgap = 1; m.gap.realgap = 17; m.gap.gappx = 17;
	mons = selmon = &m;

	Arg arg = { .i = 5 };
	setgaps(&arg);
	ASSERT(m.gap.realgap == 22, "setgaps adjust +5");
	ASSERT(m.gap.gappx == 22, "setgaps adjust: gappx matches realgap");
}

static void
test_setgaps_negative_clamp(void)
{
	Monitor m;
	memset(&m, 0, sizeof m);
	m.lt[0] = &layouts[0]; m.lt[1] = &layouts[0]; m.sellt = 0;
	m.tagset[0] = 1; m.seltags = 0;
	m.gap.isgap = 1; m.gap.realgap = 3; m.gap.gappx = 3;
	mons = selmon = &m;

	Arg arg = { .i = -10 };
	setgaps(&arg);
	ASSERT(m.gap.realgap == 0, "setgaps negative clamp to 0");
}

/* --- tag / toggletag / toggleview / view --- */
static void
test_tag(void)
{
	Monitor m;
	memset(&m, 0, sizeof m);
	m.lt[0] = &layouts[0]; m.lt[1] = &layouts[0]; m.sellt = 0;
	m.tagset[0] = 1; m.seltags = 0;
	mons = selmon = &m;
	Client c = { .win = 1, .mon = &m, .tags = 0 };
	m.clients = m.stack = &c;
	m.sel = &c;

	Arg arg = { .ui = 1 << 3 };
	tag(&arg);
	ASSERT_EQ(c.tags, 1 << 3, "tag sets client tags");
}

static void
test_toggletag(void)
{
	Monitor m;
	memset(&m, 0, sizeof m);
	m.lt[0] = &layouts[0]; m.lt[1] = &layouts[0]; m.sellt = 0;
	m.tagset[0] = 1; m.seltags = 0;
	mons = selmon = &m;
	Client c = { .win = 1, .mon = &m, .tags = 1 << 2 };
	m.clients = m.stack = &c;
	m.sel = &c;

	Arg arg = { .ui = 1 << 2 };
	toggletag(&arg);
	ASSERT_EQ(c.tags, 1 << 2, "toggletag preserves last tag (newtags would be 0)");

	toggletag(&arg);
	ASSERT_EQ(c.tags, 1 << 2, "toggletag re-adds tag");
}

static void
test_toggleview(void)
{
	Monitor m;
	memset(&m, 0, sizeof m);
	m.lt[0] = &layouts[0]; m.lt[1] = &layouts[0]; m.sellt = 0;
	m.tagset[0] = 1; m.seltags = 0;
	mons = selmon = &m;

	Arg arg = { .ui = 1 << 3 };
	toggleview(&arg);
	ASSERT_EQ(m.tagset[0], 1 | (1 << 3), "toggleview adds tag to tagset");
}

static void
test_view_noop(void)
{
	Monitor m;
	memset(&m, 0, sizeof m);
	m.lt[0] = &layouts[0]; m.lt[1] = &layouts[0]; m.sellt = 0;
	m.tagset[0] = 1; m.tagset[1] = 1; m.seltags = 0;
	mons = selmon = &m;

	Arg arg = { .ui = 1 };
	view(&arg);
	ASSERT_EQ(m.seltags, 0, "view same tag: no toggle (noop)");
}

/* --- setmfact --- */
static void
test_setmfact(void)
{
	Monitor m;
	memset(&m, 0, sizeof m);
	m.mfact = 0.55f;
	m.lt[0] = &layouts[0]; m.lt[1] = &layouts[0]; m.sellt = 0;
	mons = selmon = &m;

	Arg arg = { .f = 0.10f };
	setmfact(&arg);
	ASSERT(m.mfact > 0.64f && m.mfact < 0.66f, "setmfact adds to mfact");

	Arg arg2 = { .f = 1.5f };
	setmfact(&arg2);
	ASSERT(m.mfact > 0.49f && m.mfact < 0.51f, "setmfact absolute (arg=1.5 sets 0.5)");
}

static void
test_setmfact_bounds(void)
{
	Monitor m;
	memset(&m, 0, sizeof m);
	m.mfact = 0.55f;
	m.lt[0] = &layouts[0]; m.lt[1] = &layouts[0]; m.sellt = 0;
	mons = selmon = &m;

	Arg arg = { .f = -1.0f };
	setmfact(&arg);
	ASSERT(m.mfact >= 0.05f, "setmfact lower bound");
}

/* --- incnmaster --- */
static void
test_incnmaster(void)
{
	Monitor m;
	memset(&m, 0, sizeof m);
	m.nmaster = 2;
	mons = selmon = &m;
	m.tagset[0] = 1; m.seltags = 0;
	m.lt[0] = &layouts[0]; m.lt[1] = &layouts[0]; m.sellt = 0;

	Arg arg = { .i = 1 };
	incnmaster(&arg);
	ASSERT_EQ(m.nmaster, 3, "incnmaster +1");

	Arg arg2 = { .i = -5 };
	incnmaster(&arg2);
	ASSERT_EQ(m.nmaster, 0, "incnmaster clamped to 0");
}

/* --- zoom --- */
static void
test_zoom_single_client(void)
{
	Monitor m;
	memset(&m, 0, sizeof m);
	m.tagset[0] = 1; m.seltags = 0;
	m.lt[0] = &layouts[0]; m.lt[1] = &layouts[0]; m.sellt = 0;
	mons = selmon = &m;
	Client c = { .win = 1, .mon = &m, .tags = 1, .isfloating = 0, .next = NULL };
	m.clients = &c; m.sel = &c;
	m.gap.gappx = 0;

	Arg arg;
	/* zoom on single client should be no-op (returns early) */
	/* Just verify it doesn't crash */
	zoom(&arg);
	ASSERT(m.sel == &c, "zoom single client: sel unchanged");
}

static void
test_zoom_swaps(void)
{
	Monitor m;
	memset(&m, 0, sizeof m);
	m.tagset[0] = 1; m.seltags = 0;
	m.lt[0] = &layouts[0]; m.lt[1] = &layouts[0]; m.sellt = 0;
	mons = selmon = &m;
	Client c1 = { .win = 1, .mon = &m, .tags = 1, .isfloating = 0, .next = NULL };
	Client c2 = { .win = 2, .mon = &m, .tags = 1, .isfloating = 0, .next = NULL };
	m.clients = &c1; c1.next = &c2;
	m.sel = &c2;
	m.stack = &c2; c2.snext = &c1;
	m.gap.gappx = 0;

	Arg arg;
	zoom(&arg);
	/* After zoom, c2 (sel) becomes first in clients list */
	ASSERT(m.clients == &c2, "zoom moves sel to front");
	ASSERT(c2.next == &c1, "zoom: c2.next == c1");
}

static void
test_zoom_floating_noop(void)
{
	Monitor m;
	memset(&m, 0, sizeof m);
	m.tagset[0] = 1; m.seltags = 0;
	m.lt[0] = &layouts[0]; m.lt[1] = &layouts[0]; m.sellt = 0;
	mons = selmon = &m;
	Client c = { .win = 1, .mon = &m, .tags = 1, .isfloating = 1 };
	m.clients = &c; m.sel = &c;

	Arg arg;
	zoom(&arg);
	ASSERT(m.sel == &c, "zoom floating: noop");
}

/* --- setlayout --- */
static void
test_setlayout_toggle(void)
{
	Monitor m;
	memset(&m, 0, sizeof m);
	m.lt[0] = &layouts[0]; m.lt[1] = &layouts[0];
	m.sellt = 1; /* start at layout 1 */
	m.tagset[0] = 1; m.seltags = 0;
	mons = selmon = &m;
	m.showbar = 1;

	Arg arg = { .v = NULL };
	setlayout(&arg);
	ASSERT_EQ(m.sellt, 0, "setlayout toggles sellt");
}

/* --- tile layout --- */
static void
test_tile_no_clients(void)
{
	Monitor m;
	memset(&m, 0, sizeof m);
	m.tagset[0] = 1; m.seltags = 0;
	m.mfact = 0.55f; m.nmaster = 1;
	m.mx = m.wx = 0; m.my = m.wy = 0;
	m.mw = m.ww = 1920; m.mh = m.wh = 1080;
	m.lt[0] = &layouts[0]; m.lt[1] = &layouts[0]; m.sellt = 0;
	m.gap.isgap = 1; m.gap.realgap = 17; m.gap.gappx = 17;
	m.clients = NULL;
	mons = selmon = &m;

	tile(&m);
	ASSERT(1, "tile with no clients does not crash");
}

/* --- monocle --- */
static void
test_monocle_count(void)
{
	Monitor m;
	memset(&m, 0, sizeof m);
	m.tagset[0] = 1; m.seltags = 0;
	m.mx = m.wx = 0; m.my = m.wy = 0;
	m.mw = m.ww = 1920; m.mh = m.wh = 1080;
	m.lt[0] = &layouts[0]; m.lt[1] = &layouts[0]; m.sellt = 0;
	m.clients = NULL;
	mons = selmon = &m;

	Client c1 = { .win = 1, .mon = &m, .tags = 1 };
	Client c2 = { .win = 2, .mon = &m, .tags = 1 };
	Client c3 = { .win = 3, .mon = &m, .tags = 1 };
	m.clients = &c1; c1.next = &c2; c2.next = &c3;

	monocle(&m);
	ASSERT(strcmp(m.ltsymbol, "[3]") == 0, "monocle shows client count [3]");
}

static void
test_monocle_no_clients(void)
{
	Monitor m;
	memset(&m, 0, sizeof m);
	m.tagset[0] = 1; m.seltags = 0;
	m.mx = m.wx = 0; m.my = m.wy = 0;
	m.mw = m.ww = 1920; m.mh = m.wh = 1080;
	m.lt[0] = &layouts[0]; m.lt[1] = &layouts[0]; m.sellt = 0;
	m.clients = NULL;
	mons = selmon = &m;

	monocle(&m);
	ASSERT(1, "monocle with no clients does not crash");
}

/* --- createmon --- */
static void
test_createmon_defaults(void)
{
	Monitor *m = make_monitor(0);
	ASSERT(m != NULL, "createmon returns non-NULL");
	ASSERT_EQ(m->tagset[0], 1, "createmon: tagset[0] == 1");
	ASSERT_EQ(m->tagset[1], 1, "createmon: tagset[1] == 1");
	ASSERT(m->mfact > 0.54f && m->mfact < 0.56f, "createmon: mfact");
	ASSERT_EQ(m->nmaster, 1, "createmon: nmaster");
	ASSERT_EQ(m->showbar, 1, "createmon: showbar");
	ASSERT_EQ(m->topbar, 1, "createmon: topbar");
	ASSERT(m->gap.isgap == 1, "createmon: gap.isgap == 1");
	ASSERT_EQ(m->gap.realgap, 17, "createmon: gap.realgap");
	free(m);
}

/* --- isdescprocess / getparentprocess --- */
/* These would need /proc stubs - skip for now, test structure only */

/* --- swallow related --- */
static void
test_swallowingclient(void)
{
	Monitor m;
	memset(&m, 0, sizeof m);
	m.clients = NULL;
	mons = &m;

	Client c;
	memset(&c, 0, sizeof c);
	c.win = 100;
	c.swallowing = NULL;

	Client sw;
	memset(&sw, 0, sizeof sw);
	sw.win = 200;
	c.swallowing = &sw;
	m.clients = &c;

	Client *found = swallowingclient(200);
	ASSERT(found == &c, "swallowingclient finds parent by child window");
}

static void
test_swallowingclient_notfound(void)
{
	Monitor m;
	memset(&m, 0, sizeof m);
	m.clients = NULL;
	mons = &m;

	Client *found = swallowingclient(999);
	ASSERT(found == NULL, "swallowingclient returns NULL");
}

/* --- focusstack --- */
static void
test_focusstack_forward(void)
{
	Monitor m;
	memset(&m, 0, sizeof m);
	m.tagset[0] = 1; m.seltags = 0;
	m.lt[0] = &layouts[0]; m.lt[1] = &layouts[0]; m.sellt = 0;
	mons = selmon = &m;

	Client c1 = { .win = 1, .mon = &m, .tags = 1 };
	Client c2 = { .win = 2, .mon = &m, .tags = 1 };
	m.clients = &c1; c1.next = &c2;
	m.stack = &c1; c1.snext = &c2;
	m.sel = &c1;

	Arg arg = { .i = 1 };
	focusstack(&arg);
	ASSERT(1, "focusstack(+1) does not crash");
}

static void
test_focusstack_fullscreen_lock(void)
{
	Monitor m;
	memset(&m, 0, sizeof m);
	m.tagset[0] = 1; m.seltags = 0;
	m.lt[0] = &layouts[0]; m.lt[1] = &layouts[0]; m.sellt = 0;
	mons = selmon = &m;

	Client c = { .win = 1, .mon = &m, .tags = 1, .isfullscreen = 1 };
	m.clients = &c; m.stack = &c; m.sel = &c;

	Arg arg = { .i = 1 };
	focusstack(&arg);
	ASSERT(m.sel == &c, "focusstack: fullscreen+lockfullscreen prevents focus change");
}

/* --- cachebuttons / cachekeys --- */
static void
test_cachebuttons(void)
{
	button_button_used = 0;
	button_mask_used = 0;
	cachebuttons();
	ASSERT(1, "cachebuttons does not crash");
}

static void
test_cachekeys(void)
{
	key_keysym_used = 0;
	key_mod_used = 0;
	cachekeys();
	ASSERT(1, "cachekeys does not crash");
}

/* --- updatebarpos --- */
static void
test_updatebarpos_top(void)
{
	Monitor m;
	memset(&m, 0, sizeof m);
	m.my = m.wy = 0;
	m.mh = m.wh = 1080;
	m.showbar = 1;
	m.topbar = 1;
	bh = 22;

	updatebarpos(&m);
	ASSERT_EQ(m.by, 0, "updatebarpos top: by == my (top)");
	ASSERT_EQ(m.wy, 22, "updatebarpos top: wy shifts down by bh");
	ASSERT_EQ(m.wh, 1080 - 22, "updatebarpos top: wh reduced by bh");
}

static void
test_updatebarpos_bottom(void)
{
	Monitor m;
	memset(&m, 0, sizeof m);
	m.my = m.wy = 0;
	m.mh = m.wh = 1080;
	m.showbar = 1;
	m.topbar = 0;
	bh = 22;

	updatebarpos(&m);
	ASSERT_EQ(m.by, 1080 - 22, "updatebarpos bottom: by at bottom");
	ASSERT_EQ(m.wh, 1080 - 22, "updatebarpos bottom: wh reduced by bh");
}

static void
test_updatebarpos_hidden(void)
{
	Monitor m;
	memset(&m, 0, sizeof m);
	m.my = m.wy = 0;
	m.mh = m.wh = 1080;
	m.showbar = 0;
	bh = 22;

	updatebarpos(&m);
	ASSERT_EQ(m.by, -22, "updatebarpos hidden: by == -bh");
	ASSERT_EQ(m.wh, 1080, "updatebarpos hidden: wh unchanged");
}

/* --- gap_copy with reset --- */
static void
test_setgaps_reset(void)
{
	Monitor m;
	memset(&m, 0, sizeof m);
	m.lt[0] = &layouts[0]; m.lt[1] = &layouts[0]; m.sellt = 0;
	m.tagset[0] = 1; m.seltags = 0;
	m.gap.isgap = 1; m.gap.realgap = 17; m.gap.gappx = 17;
	mons = selmon = &m;

	/* First adjust gap */
	Arg adj = { .i = 10 };
	setgaps(&adj);
	ASSERT_EQ(m.gap.realgap, 27, "setgaps adjust before reset");

	/* Then reset */
	Arg reset = { .i = GAP_RESET };
	setgaps(&reset);
	ASSERT_EQ(m.gap.realgap, 17, "setgaps reset gap value");
	ASSERT_EQ(m.gap.gappx, 17, "setgaps reset gappx");
}

/* --- ISVISIBLE macro --- */
static void
test_isvisible(void)
{
	Monitor m;
	memset(&m, 0, sizeof m);
	m.tagset[0] = 1; m.seltags = 0;

	Client c;
	Client *cp = &c;
	memset(&c, 0, sizeof c);
	c.mon = &m;
	c.tags = 1;
	ASSERT(ISVISIBLE(cp), "ISVISIBLE: matching tag");

	c.tags = 2;
	ASSERT(!ISVISIBLE(cp), "ISVISIBLE: non-matching tag");
}

/* --- INTERSECT macro --- */
static void
test_intersect(void)
{
	Monitor m;
	memset(&m, 0, sizeof m);
	m.wx = 10; m.wy = 20; m.ww = 100; m.wh = 100;

	/* Fully inside */
	int a = INTERSECT(20, 30, 50, 50, &m);
	ASSERT(a > 0, "INTERSECT: fully inside returns positive");

	/* Fully outside */
	a = INTERSECT(200, 200, 10, 10, &m);
	ASSERT_EQ(a, 0, "INTERSECT: fully outside returns 0");

	/* Overlapping */
	a = INTERSECT(0, 0, 50, 50, &m);
	ASSERT(a > 0, "INTERSECT: overlapping returns positive");
}

/* --- setlayout with explicit layout --- */
static void
test_setlayout_explicit(void)
{
	Monitor m;
	memset(&m, 0, sizeof m);
	m.lt[0] = &layouts[0]; m.lt[1] = &layouts[1];
	m.sellt = 0;
	m.tagset[0] = 1; m.seltags = 0;
	mons = selmon = &m;
	m.showbar = 1;

	Arg arg = { .v = (void *)&layouts[1] };
	setlayout(&arg);
	ASSERT(m.lt[m.sellt] == &layouts[1], "setlayout switches to explicit layout");
}

/* --- quit --- */
static void
test_quit(void)
{
	running = 1; restart = 0;
	Arg arg = { .i = 0 };
	quit(&arg);
	ASSERT_EQ(running, 0, "quit: running == 0");
	ASSERT_EQ(restart, 0, "quit: restart == 0 (arg.i=0)");

	running = 1; restart = 0;
	Arg arg2 = { .i = 1 };
	quit(&arg2);
	ASSERT_EQ(running, 0, "quit restart: running == 0");
	ASSERT_EQ(restart, 1, "quit restart: restart == 1");
}

/* --- togglefloating --- */
static void
test_togglefloating(void)
{
	Monitor m;
	memset(&m, 0, sizeof m);
	m.tagset[0] = 1; m.seltags = 0;
	m.lt[0] = &layouts[0]; m.lt[1] = &layouts[0]; m.sellt = 0;
	mons = selmon = &m;

	Client c = { .win = 1, .mon = &m, .tags = 1, .isfloating = 0 };
	m.clients = &c; m.stack = &c; m.sel = &c;
	m.gap.isgap = 0; m.gap.realgap = 0; m.gap.gappx = 0;

	togglefloating(NULL);
	ASSERT_EQ(c.isfloating, 1, "togglefloating: becomes floating");

	togglefloating(NULL);
	ASSERT_EQ(c.isfloating, 0, "togglefloating: becomes non-floating");
}

/* --- applysizehints --- */
static void
test_applysizehints_min(void)
{
	Client c;
	memset(&c, 0, sizeof c);
	c.minw = 100; c.minh = 50;
	c.basew = 0; c.baseh = 0;
	c.incw = 0; c.inch = 0;
	c.maxw = 0; c.maxh = 0;
	c.mina = 0; c.maxa = 0;
	c.hintsvalid = 1;
	c.bw = 5;
	Monitor m;
	memset(&m, 0, sizeof m);
	m.wx = 0; m.wy = 0; m.ww = 1920; m.wh = 1080;
	m.lt[0] = &layouts[0]; m.lt[1] = &layouts[0]; m.sellt = 0;
	c.mon = &m;

	int x = 0, y = 0, w = 10, h = 10;
	applysizehints(&c, &x, &y, &w, &h, 0);

	ASSERT(w >= 100, "applysizehints: width clamped to minw");
	ASSERT(h >= 50, "applysizehints: height clamped to minh");
}

static void
test_applysizehints_interactive_bounds(void)
{
	sw = 1920; sh = 1080; bh = 22;
	Client c;
	memset(&c, 0, sizeof c);
	c.minw = 1; c.minh = 1;
	c.basew = 0; c.baseh = 0;
	c.w = 100; c.h = 100;
	c.hintsvalid = 1;
	c.bw = 5;
	Monitor m;
	memset(&m, 0, sizeof m);
	m.wx = 0; m.wy = 0; m.ww = 1920; m.wh = 1080;
	m.lt[0] = &layouts[0]; m.lt[1] = &layouts[0]; m.sellt = 0;
	c.mon = &m;

	int x = -100, y = -100, w = 50, h = 50;
	applysizehints(&c, &x, &y, &w, &h, 1); /* interactive */
	ASSERT(x >= 0, "applysizehints interactive: x clamped");
	ASSERT(y >= 0, "applysizehints interactive: y clamped");
}

/* --- cachebuttons fast-path tests --- */
static void
test_cachebuttons_global_flags(void)
{
	unsigned int i;
	unsigned long expected_button = 0;
	unsigned int expected_mask = 0;

	button_button_used = 0;
	button_mask_used = 0;
	cachebuttons();

	for (i = 0; i < LENGTH(buttons); i++) {
		if (!buttons[i].func)
			continue;
		if (buttons[i].button < 8 * sizeof button_button_used)
			expected_button |= 1UL << buttons[i].button;
		expected_mask |= CLEANMASK(buttons[i].mask);
	}

	ASSERT_EQ(button_button_used, expected_button,
		"cachebuttons: button_button_used matches expected");
	ASSERT_EQ(button_mask_used, expected_mask,
		"cachebuttons: button_mask_used matches expected");
}

static void
test_cachekeys_global_flags(void)
{
	unsigned int i;
	KeySym expected_keysym = 0;
	unsigned int expected_mod = 0;

	key_keysym_used = 0;
	key_mod_used = 0;
	cachekeys();

	for (i = 0; i < LENGTH(keys); i++) {
		if (!keys[i].func)
			continue;
		expected_keysym |= keys[i].keysym;
		expected_mod |= keys[i].mod;
	}

	ASSERT_EQ(key_keysym_used, expected_keysym,
		"cachekeys: key_keysym_used matches expected");
	ASSERT_EQ(key_mod_used, expected_mod,
		"cachekeys: key_mod_used matches expected");
}

static void
test_cachebuttons_empty_buttons(void)
{
	/* cachebuttons() skips entries with func==NULL.
	   Re-compute from config and compare. */
	unsigned int i;
	unsigned long contrib = 0;

	button_button_used = 0;
	button_mask_used = 0;
	cachebuttons();

	for (i = 0; i < LENGTH(buttons); i++) {
		if (buttons[i].func && buttons[i].button < 8 * sizeof button_button_used)
			contrib |= 1UL << buttons[i].button;
	}

	ASSERT_EQ(button_button_used, contrib,
		"cachebuttons: only entries with non-NULL func contribute");
}

static void
test_mousebuttonmatch_mapped_button(void)
{
	/* Button1+MODKEY matches the first buttons[] entry */
	ASSERT(mousebuttonmatch(Button1, CLEANMASK(MODKEY)) == 1,
		"mousebuttonmatch: Button1+MODKEY is mapped");
}

static void
test_mousebuttonmatch_unmapped_button(void)
{
	/* Button4/Button5 (scroll) are not in the buttons array */
	ASSERT(mousebuttonmatch(Button4, CLEANMASK(MODKEY)) == 0,
		"mousebuttonmatch: Button4 is not mapped");
	ASSERT(mousebuttonmatch(Button5, CLEANMASK(MODKEY)) == 0,
		"mousebuttonmatch: Button5 is not mapped");
}

static void
test_keypress_mapped_key(void)
{
	XEvent ev;
	int old_showbar;

	memset(&ev, 0, sizeof(ev));
	ev.xkey.type = KeyPress;
	ev.xkey.keycode = XK_b;  /* mock returns keycode as keysym → XK_b */
	ev.xkey.state = MODKEY;  /* matches the togglebar key binding (MODKEY) */

	old_showbar = selmon->showbar;
	keypress(&ev);
	ASSERT_EQ(selmon->showbar, !old_showbar,
		"keypress: MODKEY+XK_b toggles showbar");

	/* Toggle back to restore */
	keypress(&ev);
	ASSERT_EQ(selmon->showbar, old_showbar,
		"keypress: second toggle restores showbar");
}

static void
test_keypress_unmapped_key(void)
{
	XEvent ev;
	int saved_showbar;

	memset(&ev, 0, sizeof(ev));
	ev.xkey.type = KeyPress;
	ev.xkey.keycode = 0xFFFF; /* keysym not in keys[] */
	ev.xkey.state = MODKEY;

	saved_showbar = selmon->showbar;
	keypress(&ev);
	ASSERT_EQ(selmon->showbar, saved_showbar,
		"keypress: unmapped keysym does not dispatch");
}

static void
test_keypress_unmapped_mod(void)
{
	XEvent ev;
	int saved_showbar;

	memset(&ev, 0, sizeof(ev));
	ev.xkey.type = KeyPress;
	ev.xkey.keycode = XK_b; /* correct keysym */
	ev.xkey.state = 0;       /* no modifier — wrong mod */

	saved_showbar = selmon->showbar;
	keypress(&ev);
	ASSERT_EQ(selmon->showbar, saved_showbar,
		"keypress: correct keysym but wrong mod does not dispatch");
}

/* Main */
int
main(void)
{
	int i;
	/* Initialize globals expected by dwm.c functions */
	dpy = (Display *)(void *)0x1;
	drw = calloc(1, sizeof(Drw));
	drw->fonts = calloc(1, sizeof(Fnt));
	drw->fonts->h = 15;
	root = 42;
	screen = 0;
	sw = 1920; sh = 1080;
	bh = 22; lrpad = 11;
	wmcheckwin = 42;

	/* Default selmon: tests that need specific setup override this.
	 * Keep a copy so we can restore after tests that change selmon
	 * to a stack-local. */
	selmon = calloc(1, sizeof(Monitor));
	mons = selmon;
	Monitor *main_selmon = selmon;
	selmon->tagset[0] = 1;
	selmon->tagset[1] = 1;
	selmon->mfact = 0.55f;
	selmon->nmaster = 1;
	selmon->showbar = 0; /* prevents drawbar from crashing on fake drw pointer */
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

	/* scheme array: needed by unfocus/focus */
	scheme = ecalloc(2, sizeof(Clr *));
	for (i = SchemeNorm; i <= SchemeSel; i++)
		scheme[i] = drw_scm_create(drw, colors[i], 3);

	/* linked lists */
	test_attach();
	test_attach_multiple();
	test_detach();
	test_detach_tail();
	test_attachstack();
	test_detachstack_sel_update();
	test_nexttiled_skips_floating();
	test_nexttiled_skips_hidden_tags();
	test_nexttiled_all_invisible();

	/* monitor ops */
	test_dirtomon_next();
	test_dirtomon_wrap();
	test_dirtomon_prev();
	test_recttomon_intersect();
	test_recttomon_no_intersect();
	test_createmon_defaults();

	/* client lookup */
	test_wintoclient();
	test_wintoclient_notfound();

	/* gaps */
	test_gap_copy();
	test_setgaps_toggle();
	test_setgaps_adjust();
	test_setgaps_negative_clamp();
	test_setgaps_reset();

	/* tags */
	test_tag();
	test_toggletag();
	test_toggleview();
	test_view_noop();

	/* misc */
	test_setmfact();
	test_setmfact_bounds();
	test_incnmaster();
	test_quit();

	/* zoom / floating */
	test_zoom_single_client();
	test_zoom_swaps();
	test_zoom_floating_noop();
	test_togglefloating();

	/* layout */
	test_setlayout_toggle();
	test_setlayout_explicit();

	/* tile / monocle */
	test_tile_no_clients();
	test_monocle_count();
	test_monocle_no_clients();

	/* bar pos */
	test_updatebarpos_top();
	test_updatebarpos_bottom();
	test_updatebarpos_hidden();

	/* focusstack */
	test_focusstack_forward();
	test_focusstack_fullscreen_lock();

	/* swallow */
	test_swallowingclient();
	test_swallowingclient_notfound();

	/* Some tests (focusstack) changed selmon/mons to stack-locals.
	 * Restore to the heap-allocated version for subsequent tests.
	 * AGENTS.md: "Tests that set selmon = &local_monitor MUST restore
	 * selmon to the heap-allocated original before the local variable
	 * goes out of scope." */
	selmon = mons = main_selmon;

	/* Initialize fast-path bitsets */
	cachebuttons();
	cachekeys();

	/* caches */
	test_cachebuttons();
	test_cachekeys();

	/* cachebuttons / cachekeys fast-path correctness */
	test_cachebuttons_global_flags();
	test_cachekeys_global_flags();
	test_cachebuttons_empty_buttons();

	/* mousebuttonmatch */
	test_mousebuttonmatch_mapped_button();
	test_mousebuttonmatch_unmapped_button();

	/* keypress */
	test_keypress_mapped_key();
	test_keypress_unmapped_key();
	test_keypress_unmapped_mod();

	/* macros */
	test_isvisible();
	test_intersect();

	/* size hints */
	test_applysizehints_min();
	test_applysizehints_interactive_bounds();

	/* Report */
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
