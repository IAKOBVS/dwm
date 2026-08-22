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
#define ASSERT_NE(a, b, msg) do { \
	total++; \
	if ((a) == (b)) { \
		failed++; \
		fprintf(stderr, "  FAIL %s:%d: %s (%d == %d)\n", __FILE__, __LINE__, msg, (int)(a), (int)(b)); \
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
cleanup_mon(Monitor *m)
{
	free(m);
}

/* --- togglebar --- */

static void
test_togglebar_toggles_showbar(void)
{
	selmon->showbar = 1;
	togglebar(NULL);
	ASSERT_EQ(selmon->showbar, 0, "togglebar sets showbar from 1 to 0");

	togglebar(NULL);
	ASSERT_EQ(selmon->showbar, 1, "togglebar sets showbar from 0 to 1");
}

static void
test_togglebar_sets_dirty_segments(void)
{
	selmon->bar_dirty_segments = 0;
	selmon->showbar = 1;

	togglebar(NULL);
	ASSERT(selmon->bar_dirty_segments & DIRTY_STATUS, "togglebar sets DIRTY_STATUS");
	ASSERT(selmon->bar_dirty_segments & DIRTY_TAGS, "togglebar sets DIRTY_TAGS");
	ASSERT(selmon->bar_dirty_segments & DIRTY_TITLE, "togglebar sets DIRTY_TITLE");
}

/* --- togglefloating --- */

static void
test_togglefloating_toggles_sel(void)
{
	Client c = { .win = 1, .mon = selmon, .tags = 1 };
	c.isfloating = 0;
	selmon->sel = &c;

	togglefloating(NULL);
	ASSERT_EQ(c.isfloating, 1, "togglefloating sets isfloating from 0 to 1");

	togglefloating(NULL);
	ASSERT_EQ(c.isfloating, 0, "togglefloating sets isfloating from 1 to 0");
}

static void
test_togglefloating_noop_on_fullscreen(void)
{
	Client c = { .win = 1, .mon = selmon, .tags = 1 };
	c.isfloating = 0;
	c.isfullscreen = 1;
	selmon->sel = &c;

	togglefloating(NULL);
	ASSERT_EQ(c.isfloating, 0, "togglefloating is noop on fullscreen window");
}

static void
test_togglefloating_noop_on_nosel(void)
{
	selmon->sel = NULL;
	togglefloating(NULL);
	ASSERT(1, "togglefloating with no selection does not crash");
}

/* --- seturgent --- */

static void
test_seturgent_sets_flag(void)
{
	Client c;
	memset(&c, 0, sizeof c);
	c.isurgent = 0;

	seturgent(&c, 1);
	ASSERT_EQ(c.isurgent, 1, "seturgent(1) sets isurgent");

	seturgent(&c, 0);
	ASSERT_EQ(c.isurgent, 0, "seturgent(0) clears isurgent");
}

/* --- sendmon --- */

static void
test_sendmon_changes_monitor(void)
{
	save_selmon();
	Monitor *m1 = make_monitor(0);
	Monitor *m2 = make_monitor(1);
	mons = m1; m1->next = m2;
	selmon = m1;

	Client c = { .win = 1, .mon = m1, .tags = 1 };
	m1->stack = &c;
	m1->clients = &c;
	m1->sel = &c;

	sendmon(&c, m2);
	ASSERT_EQ(c.mon, m2, "sendmon changes client mon");
	/* client attached to m2's lists */
	ASSERT(m2->clients == &c, "sendmon attaches to target monitor clients");
	ASSERT(m2->stack == &c, "sendmon attaches to target monitor stack");
	/* client removed from m1's lists */
	ASSERT_EQ(m1->clients, NULL, "sendmon detaches from source monitor clients");
	ASSERT_EQ(m1->stack, NULL, "sendmon detaches from source monitor stack");

	/* restore global state */
	mons = selmon = saved_selmon;
	cleanup_mon(m1);
	cleanup_mon(m2);
}

static void
test_sendmon_noop_same_monitor(void)
{
	Client c = { .win = 1, .mon = selmon, .tags = 1 };
	selmon->clients = &c;
	selmon->stack = &c;

	sendmon(&c, selmon);
	ASSERT_EQ(c.mon, selmon, "sendmon to same monitor keeps mon");
	ASSERT(selmon->clients == &c, "sendmon same mon preserves clients");
}

/* --- unmanage --- */

static void
test_unmanage_detaches_client(void)
{
	save_selmon();
	Monitor *m = make_monitor(0);
	/* unmanage uses c->mon, and focus(NULL) -> walks mon->stack */
	Client c = { .win = 1, .mon = m, .tags = 1 };
	m->sel = &c;
	m->clients = &c;
	m->stack = &c;
	/* setup global state for focus(NULL) */
	Monitor *old = selmon;
	mons = selmon = m;

	/* unmanage will call free(c), so use heap alloc */
	Client *cp = ecalloc(1, sizeof(Client));
	*cp = c;
	m->clients = cp;
	m->stack = cp;
	m->sel = cp;

	unmanage(cp, 0);
	ASSERT_EQ(m->clients, NULL, "unmanage removes client from clients list");
	ASSERT_EQ(m->stack, NULL, "unmanage removes client from stack list");
	/* cp was freed by unmanage; don't access it */

	mons = selmon = old;
	cleanup_mon(m);
}

static void
test_unmanage_destroyed_keeps(void)
{
	save_selmon();
	Monitor *m = make_monitor(0);
	Client *cp = ecalloc(1, sizeof(Client));
	cp->win = 1;
	cp->mon = m;
	cp->tags = 1;
	m->clients = cp;
	m->stack = cp;
	m->sel = cp;
	mons = selmon = m;

	unmanage(cp, 1);
	ASSERT_EQ(m->clients, NULL, "unmanage destroyed removes from clients");
	ASSERT_EQ(m->stack, NULL, "unmanage destroyed removes from stack");

	mons = selmon = saved_selmon;
	cleanup_mon(m);
}

/* --- manage --- */

static void
test_manage_clamps_geometry(void)
{
	save_selmon();
	Monitor *m = make_monitor(0);
	mons = selmon = m;

	XWindowAttributes wa = {
		.x = -100, .y = -100,
		.width = 5000, .height = 5000,
		.border_width = 2
	};

	manage(42, &wa);
	/* manage creates a client, attaches it to selmon (m), clamps geometry */
	ASSERT(m->clients != NULL, "manage attaches client to selmon clients");

	/* find the client */
	Client *c = m->clients;
	ASSERT(c->x >= m->wx, "manage clamps x to monitor wx");
	ASSERT(c->y >= m->wy, "manage clamps y to monitor wy");
	ASSERT(c->x + c->w + 2 * c->bw <= m->wx + m->ww,
		"manage clamps right edge to monitor");
	ASSERT(c->y + c->h + 2 * c->bw <= m->wy + m->wh,
		"manage clamps bottom edge to monitor");

	Client *managed = m->clients;
	m->clients = NULL;
	m->stack = NULL;
	m->sel = NULL;
	mons = selmon = saved_selmon;
	free(managed);
	cleanup_mon(m);
}

/* --- setclientstate --- */

static void
test_setclientstate_normal(void)
{
	Client c = { .win = 1 };
	setclientstate(&c, NormalState);
	ASSERT(1, "setclientstate(NormalState) does not crash");
}

static void
test_setclientstate_withdrawn(void)
{
	Client c = { .win = 1 };
	setclientstate(&c, WithdrawnState);
	ASSERT(1, "setclientstate(WithdrawnState) does not crash");
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

	test_togglebar_toggles_showbar();
	test_togglebar_sets_dirty_segments();
	test_togglefloating_toggles_sel();
	test_togglefloating_noop_on_fullscreen();
	test_togglefloating_noop_on_nosel();

	test_seturgent_sets_flag();

	test_sendmon_changes_monitor();
	test_sendmon_noop_same_monitor();

	test_unmanage_detaches_client();
	test_unmanage_destroyed_keeps();

	test_manage_clamps_geometry();

	test_setclientstate_normal();
	test_setclientstate_withdrawn();

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
