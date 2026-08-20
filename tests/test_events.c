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

static void
restore_selmon(void)
{
	selmon = saved_selmon;
	mons = saved_selmon;
}

/* --- focusin --- */

static void
test_focusin_sets_focus_on_sel(void)
{
	Client *c = ecalloc(1, sizeof(Client));
	c->win = 42; c->mon = selmon; c->tags = 1; c->neverfocus = 0;
	selmon->sel = c;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xfocus.window = 1;

	focusin(&ev);
	ASSERT(1, "focusin with different window calls setfocus on sel");
	free(c);
	selmon->sel = NULL;
}

static void
test_focusin_ignores_own_window(void)
{
	Client *c = ecalloc(1, sizeof(Client));
	c->win = 42; c->mon = selmon; c->tags = 1; c->neverfocus = 0;
	selmon->sel = c;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xfocus.window = 42;

	focusin(&ev);
	ASSERT(1, "focusin with own window does not refocus");
	free(c);
	selmon->sel = NULL;
}

static void
test_focusin_no_sel_ignores(void)
{
	selmon->sel = NULL;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xfocus.window = 42;

	focusin(&ev);
	ASSERT(1, "focusin with no sel does not crash");
}

/* --- clientmessage: fullscreen --- */

static void
test_clientmessage_fullscreen_add(void)
{
	Client c = { .win = 42, .mon = selmon, .tags = 1, .isfullscreen = 0 };
	selmon->clients = &c;
	selmon->stack = &c;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xclient.message_type = netatom[NetWMState];
	ev.xclient.data.l[0] = 1; /* _NET_WM_STATE_ADD */
	ev.xclient.data.l[1] = netatom[NetWMFullscreen];
	ev.xclient.window = 42;

	clientmessage(&ev);
	ASSERT_EQ(c.isfullscreen, 1, "clientmessage fullscreen add sets isfullscreen");
	selmon->clients = NULL;
	selmon->stack = NULL;
}

static void
test_clientmessage_fullscreen_remove(void)
{
	Client c = { .win = 42, .mon = selmon, .tags = 1, .isfullscreen = 1 };
	selmon->clients = &c;
	selmon->stack = &c;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xclient.message_type = netatom[NetWMState];
	ev.xclient.data.l[0] = 0; /* _NET_WM_STATE_REMOVE */
	ev.xclient.data.l[1] = netatom[NetWMFullscreen];
	ev.xclient.window = 42;

	clientmessage(&ev);
	ASSERT_EQ(c.isfullscreen, 0, "clientmessage fullscreen remove clears isfullscreen");
	selmon->clients = NULL;
	selmon->stack = NULL;
}

static void
test_clientmessage_fullscreen_toggle(void)
{
	Client c = { .win = 42, .mon = selmon, .tags = 1, .isfullscreen = 0 };
	selmon->clients = &c;
	selmon->stack = &c;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xclient.message_type = netatom[NetWMState];
	ev.xclient.data.l[0] = 2; /* _NET_WM_STATE_TOGGLE */
	ev.xclient.data.l[1] = netatom[NetWMFullscreen];
	ev.xclient.window = 42;

	clientmessage(&ev);
	ASSERT_EQ(c.isfullscreen, 1, "clientmessage fullscreen toggle sets isfullscreen");
	selmon->clients = NULL;
	selmon->stack = NULL;
}

static void
test_clientmessage_noop_on_unknown_window(void)
{
	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xclient.message_type = netatom[NetWMState];
	ev.xclient.data.l[0] = 1;
	ev.xclient.data.l[1] = netatom[NetWMFullscreen];
	ev.xclient.window = 9999; /* no such client */

	clientmessage(&ev);
	ASSERT(1, "clientmessage for unknown window does not crash");
}

/* --- clientmessage: NetActiveWindow --- */

static void
test_clientmessage_activewindow_sets_urgent(void)
{
	Client c = { .win = 42, .mon = selmon, .tags = 1, .isurgent = 0 };
	selmon->clients = &c;
	selmon->stack = &c;
	selmon->sel = NULL;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xclient.message_type = netatom[NetActiveWindow];
	ev.xclient.window = 42;

	clientmessage(&ev);
	ASSERT_EQ(c.isurgent, 1, "clientmessage activewindow sets urgent");
	selmon->clients = NULL;
	selmon->stack = NULL;
}

static void
test_clientmessage_activewindow_skips_sel(void)
{
	Client c = { .win = 42, .mon = selmon, .tags = 1, .isurgent = 0 };
	selmon->clients = &c;
	selmon->stack = &c;
	selmon->sel = &c;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xclient.message_type = netatom[NetActiveWindow];
	ev.xclient.window = 42;

	clientmessage(&ev);
	ASSERT_EQ(c.isurgent, 0, "clientmessage activewindow skips if already selected");
	selmon->clients = NULL;
	selmon->stack = NULL;
	selmon->sel = NULL;
}

/* --- unmapnotify --- */

static void
test_unmapnotify_send_event_withdraws(void)
{
	Client c = { .win = 42, .mon = selmon, .tags = 1, .neverfocus = 0 };
	Client *old_clients = selmon->clients;
	Client *old_stack = selmon->stack;
	Client *old_sel = selmon->sel;
	selmon->clients = &c;
	selmon->stack = &c;
	selmon->sel = &c;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xunmap.window = 42;
	ev.xunmap.send_event = 1;

	unmapnotify(&ev);

	selmon->clients = old_clients;
	selmon->stack = old_stack;
	selmon->sel = old_sel;
	ASSERT(1, "unmapnotify with send_event does not crash");
}

static void
test_unmapnotify_normal_unmanages(void)
{
	saved_selmon = selmon;
	Monitor *m = ecalloc(1, sizeof(Monitor));
	m->tagset[0] = 1; m->tagset[1] = 1;
	m->mx = m->wx = 0; m->my = m->wy = 0;
	m->mw = m->ww = 1920; m->mh = m->wh = 1080;
	m->lt[0] = m->lt[1] = &layouts[0];
	m->gap = ecalloc(1, sizeof(Gap));
	m->gap->isgap = 1; m->gap->realgap = 17; m->gap->gappx = 17;

	Client *cp = ecalloc(1, sizeof(Client));
	cp->win = 42; cp->mon = m; cp->tags = 1;
	cp->oldbw = 2;
	m->sel = cp;
	m->clients = cp;
	m->stack = cp;
	mons = selmon = m;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xunmap.window = 42;
	ev.xunmap.send_event = 0;

	unmapnotify(&ev);
	/* client was unmanaged (freed), lists are empty */
	ASSERT_EQ(m->clients, NULL, "unmapnotify without send_event unmanages client");
	ASSERT_EQ(m->stack, NULL, "unmapnotify without send_event clears stack");

	selmon = mons = saved_selmon;
	free(m->gap);
	free(m);
}

/* --- destroynotify --- */

static void
test_destroynotify_unmanages(void)
{
	saved_selmon = selmon;
	Monitor m;
	memset(&m, 0, sizeof m);
	m.tagset[0] = 1; m.tagset[1] = 1;
	m.mx = m.wx = 0; m.my = m.wy = 0;
	m.mw = m.ww = 1920; m.mh = m.wh = 1080;
	m.lt[0] = m.lt[1] = &layouts[0];
	m.gap = ecalloc(1, sizeof(Gap));
	m.gap->isgap = 1; m.gap->realgap = 17; m.gap->gappx = 17;

	Client *cp = ecalloc(1, sizeof(Client));
	cp->win = 42; cp->mon = &m; cp->tags = 1;
	m.sel = cp;
	m.clients = cp;
	m.stack = cp;
	mons = selmon = &m;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xdestroywindow.window = 42;

	destroynotify(&ev);
	ASSERT_EQ(m.clients, NULL, "destroynotify unmanages client");

	free(m.gap);
	mons = selmon = saved_selmon;
}

/* --- configurerequest: floating client --- */

static void
test_configurerequest_updates_floating_geometry(void)
{
	saved_selmon = selmon;
	Monitor m;
	memset(&m, 0, sizeof m);
	m.tagset[0] = 1; m.seltags = 0;
	m.mfact = 0.55f; m.nmaster = 1;
	m.mx = 0; m.my = 0;
	m.mw = 1920; m.mh = 1080;
	m.wx = 0; m.wy = 0;
	m.ww = 1920; m.wh = 1080;
	m.lt[0] = &layouts[0]; m.lt[1] = &layouts[0]; m.sellt = 0;
	m.gap = ecalloc(1, sizeof(Gap));

	Client c = { .win = 42, .mon = &m, .tags = 1, .bw = 2,
		.isfloating = 1, .oldx = 0, .oldy = 0, .oldw = 0, .oldh = 0,
		.x = 100, .y = 200, .w = 300, .h = 400 };

	mons = selmon = &m;
	m.clients = &c;
	m.stack = &c;
	c.next = NULL;
	c.snext = NULL;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xconfigurerequest.window = 42;
	ev.xconfigurerequest.value_mask = CWX | CWY | CWWidth | CWHeight;
	ev.xconfigurerequest.x = 50;
	ev.xconfigurerequest.y = 60;
	ev.xconfigurerequest.width = 500;
	ev.xconfigurerequest.height = 600;

	configurerequest(&ev);

	ASSERT_EQ(c.x, m.mx + 50, "configurerequest sets x relative to mon mx");
	ASSERT_EQ(c.y, m.my + 60, "configurerequest sets y relative to mon my");
	ASSERT_EQ(c.w, 500, "configurerequest sets w");
	ASSERT_EQ(c.h, 600, "configurerequest sets h");
	ASSERT_EQ(c.oldx, 100, "configurerequest preserves oldx before change");
	ASSERT_EQ(c.oldy, 200, "configurerequest preserves oldy before change");

	free(m.gap);
	mons = selmon = saved_selmon;
}

static void
test_configurerequest_partial_mask(void)
{
	saved_selmon = selmon;
	Monitor m;
	memset(&m, 0, sizeof m);
	m.tagset[0] = 1; m.seltags = 0;
	m.mfact = 0.55f; m.nmaster = 1;
	m.mx = 0; m.my = 0;
	m.mw = 1920; m.mh = 1080;
	m.wx = 0; m.wy = 0;
	m.ww = 1920; m.wh = 1080;
	m.lt[0] = &layouts[0]; m.lt[1] = &layouts[0]; m.sellt = 0;
	m.gap = ecalloc(1, sizeof(Gap));

	Client c = { .win = 42, .mon = &m, .tags = 1, .bw = 2,
		.isfloating = 1,
		.x = 100, .y = 200, .w = 300, .h = 400,
		.oldx = 100, .oldy = 200, .oldw = 300, .oldh = 400 };

	mons = selmon = &m;
	m.clients = &c;
	m.stack = &c;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xconfigurerequest.window = 42;
	ev.xconfigurerequest.value_mask = CWWidth | CWHeight;
	ev.xconfigurerequest.width = 800;
	ev.xconfigurerequest.height = 600;

	configurerequest(&ev);
	ASSERT_EQ(c.x, 100, "configurerequest partial mask preserves x");
	ASSERT_EQ(c.y, 200, "configurerequest partial mask preserves y");
	ASSERT_EQ(c.w, 800, "configurerequest partial mask updates w");
	ASSERT_EQ(c.h, 600, "configurerequest partial mask updates h");

	free(m.gap);
	mons = selmon = saved_selmon;
}

static void
test_configurerequest_nonclient_configures(void)
{
	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xconfigurerequest.window = 9999; /* no client for this */
	ev.xconfigurerequest.value_mask = CWX | CWY;

	configurerequest(&ev);
	ASSERT(1, "configurerequest for unknown window does not crash");
}

/* --- expose --- */

static void
test_expose_draws_bar(void)
{
	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xexpose.window = selmon->barwin;

	expose(&ev);
	ASSERT(1, "expose on barwin does not crash");
}

static void
test_expose_ignores_other(void)
{
	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xexpose.window = 9999;

	expose(&ev);
	ASSERT(1, "expose on unknown window does not crash");
}

/* --- propertynotify --- */

static void
test_propertynotify_root_wmname(void)
{
	/* reset stext so updatestatus() sees a change and sets dirty */
	stext[0] = '\0';
	stext_len = 0;
	selmon->bar_dirty_segments = 0;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xproperty.window = root;
	ev.xproperty.atom = XA_WM_NAME;

	propertynotify(&ev);

	ASSERT(selmon->bar_dirty_segments & DIRTY_STATUS, "propertynotify root XA_WM_NAME sets DIRTY_STATUS");
	ASSERT(selmon->bar_dirty_segments & DIRTY_TITLE, "propertynotify root XA_WM_NAME sets DIRTY_TITLE");
}

static void
test_propertynotify_propertydelete_ignored(void)
{
	selmon->bar_dirty_segments = 0;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xproperty.window = 9999;  /* not root, not a client */
	ev.xproperty.atom = XA_WM_NAME;
	ev.xproperty.state = PropertyDelete;

	propertynotify(&ev);

	ASSERT_EQ(selmon->bar_dirty_segments, 0, "propertynotify PropertyDelete returns early, no dirty set");
}

static void
test_propertynotify_client_normal_hints(void)
{
	Client c;
	memset(&c, 0, sizeof c);
	c.win = 43;
	c.mon = selmon;
	c.tags = 1;
	c.hintsvalid = 1;
	selmon->clients = &c;
	selmon->stack = &c;
	c.next = NULL;
	c.snext = NULL;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xproperty.window = 43;
	ev.xproperty.atom = XA_WM_NORMAL_HINTS;
	ev.xproperty.state = 0;

	propertynotify(&ev);

	ASSERT_EQ(c.hintsvalid, 0, "propertynotify XA_WM_NORMAL_HINTS clears hintsvalid");
	selmon->clients = NULL;
	selmon->stack = NULL;
}

static void
test_propertynotify_client_wm_hints(void)
{
	selmon->bar_dirty_segments = 0;

	Client *c = ecalloc(1, sizeof(Client));
	c->win = 44;
	c->mon = selmon;
	c->tags = 1;
	c->neverfocus = 1;
	c->isurgent = 0;
	selmon->clients = c;
	selmon->stack = c;
	c->next = NULL;
	c->snext = NULL;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xproperty.window = 44;
	ev.xproperty.atom = XA_WM_HINTS;
	ev.xproperty.state = 0;

	propertynotify(&ev);

	ASSERT(selmon->bar_dirty_segments & DIRTY_TAGS, "propertynotify XA_WM_HINTS sets DIRTY_TAGS");
	ASSERT_EQ(c->neverfocus, 0, "propertynotify XA_WM_HINTS calls updatewmhints, neverfocus=0");

	selmon->clients = NULL;
	selmon->stack = NULL;
	free(c);
}

static void
test_propertynotify_client_wmname_selected(void)
{
	selmon->bar_dirty_segments = 0;

	Client *c = ecalloc(1, sizeof(Client));
	c->win = 45;
	c->mon = selmon;
	c->tags = 1;
	strcpy(c->name, "old");
	selmon->clients = c;
	selmon->stack = c;
	selmon->sel = c;
	c->next = NULL;
	c->snext = NULL;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xproperty.window = 45;
	ev.xproperty.atom = XA_WM_NAME;
	ev.xproperty.state = 0;

	propertynotify(&ev);

	ASSERT_EQ(strcmp(c->name, "broken"), 0, "propertynotify XA_WM_NAME calls updatetitle");
	ASSERT(selmon->bar_dirty_segments & DIRTY_TITLE, "propertynotify XA_WM_NAME on sel sets DIRTY_TITLE");

	selmon->clients = NULL;
	selmon->stack = NULL;
	selmon->sel = NULL;
	free(c);
}

static void
test_propertynotify_client_wmname_not_selected(void)
{
	selmon->bar_dirty_segments = 0;

	Client *c = ecalloc(1, sizeof(Client));
	c->win = 46;
	c->mon = selmon;
	c->tags = 1;
	strcpy(c->name, "old");
	selmon->clients = c;
	selmon->stack = c;
	selmon->sel = NULL;
	c->next = NULL;
	c->snext = NULL;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xproperty.window = 46;
	ev.xproperty.atom = XA_WM_NAME;
	ev.xproperty.state = 0;

	propertynotify(&ev);

	ASSERT_EQ(strcmp(c->name, "broken"), 0, "propertynotify XA_WM_NAME on non-sel calls updatetitle");
	ASSERT(!(selmon->bar_dirty_segments & DIRTY_TITLE),
		"propertynotify XA_WM_NAME on non-sel does not set DIRTY_TITLE");

	selmon->clients = NULL;
	selmon->stack = NULL;
	free(c);
}

/* --- optimizefullscreen propertynotify skip --- */

static void
test_propertynotify_root_wmname_fullscreen_skip(void)
{
	/* optimizefullscreen=1 (default config) + fullscreen client -> skip updatestatus */
	save_selmon();
	selmon = ecalloc(1, sizeof(Monitor));
	mons = selmon;
	selmon->tagset[0] = selmon->tagset[1] = 1;
	selmon->mfact = 0.55f;
	selmon->nmaster = 1;
	selmon->showbar = 1;
	selmon->topbar = 1;
	selmon->lt[0] = selmon->lt[1] = &layouts[0];
	selmon->gap = ecalloc(1, sizeof(Gap));

	Client c = { .win = 1, .isfullscreen = 1 };
	selmon->sel = &c;

	selmon->bar_dirty_segments = 0;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xproperty.window = root;
	ev.xproperty.atom = XA_WM_NAME;

	propertynotify(&ev);

	ASSERT_EQ(selmon->bar_dirty_segments, 0,
		"propertynotify root XA_WM_NAME skipped when optimizefullscreen + fullscreen");

	free(selmon->gap);
	free(selmon);
	restore_selmon();
}

static void
test_propertynotify_root_wmname_no_fullscreen_not_skipped(void)
{
	/* optimizefullscreen=1 but no fullscreen client -> updatestatus called */
	/* reset stext so updatestatus() sees a change and sets dirty */
	stext[0] = '\0';
	selmon->sel = NULL;
	selmon->bar_dirty_segments = 0;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xproperty.window = root;
	ev.xproperty.atom = XA_WM_NAME;

	propertynotify(&ev);

	ASSERT(selmon->bar_dirty_segments & DIRTY_STATUS,
		"propertynotify root XA_WM_NAME not skipped when no fullscreen client");
}

/* --- setfullscreen --- */

static void
test_setfullscreen_enter(void)
{
	Client c;
	memset(&c, 0, sizeof c);
	c.win = 42;
	c.mon = selmon;
	c.isfloating = 0;
	c.isfullscreen = 0;
	c.oldstate = 0;
	c.bw = 2;
	c.x = 100; c.y = 200; c.w = 300; c.h = 400;
	c.oldx = 100; c.oldy = 200; c.oldw = 300; c.oldh = 400;

	setfullscreen(&c, 1);

	ASSERT_EQ(c.isfullscreen, 1, "setfullscreen(1) sets isfullscreen=1");
	ASSERT_EQ(c.isfloating, 1, "setfullscreen(1) sets isfloating=1");
	ASSERT_EQ(c.bw, 0, "setfullscreen(1) clears border width");
	ASSERT_EQ(c.oldstate, 0, "setfullscreen(1) preserves oldstate before change");
	ASSERT_EQ(c.oldbw, 2, "setfullscreen(1) preserves oldbw before change");
}

static void
test_setfullscreen_exit(void)
{
	selmon->bar_dirty_segments = 0;

	Client c;
	memset(&c, 0, sizeof c);
	c.win = 42;
	c.mon = selmon;
	c.isfloating = 1;
	c.isfullscreen = 1;
	c.oldstate = 0;
	c.bw = 0;
	c.oldbw = 2;
	c.x = 0; c.y = 0; c.w = 1920; c.h = 1080;
	c.oldx = 100; c.oldy = 200; c.oldw = 300; c.oldh = 400;

	setfullscreen(&c, 0);

	ASSERT_EQ(c.isfullscreen, 0, "setfullscreen(0) sets isfullscreen=0");
	ASSERT_EQ(c.isfloating, 0, "setfullscreen(0) restores oldstate");
	ASSERT_EQ(c.bw, 2, "setfullscreen(0) restores border width");
	ASSERT_EQ(c.x, 100, "setfullscreen(0) restores old x");
	ASSERT_EQ(c.y, 200, "setfullscreen(0) restores old y");
	ASSERT_EQ(c.w, 300, "setfullscreen(0) restores old w");
	ASSERT_EQ(c.h, 400, "setfullscreen(0) restores old h");
	ASSERT(selmon->bar_dirty_segments & DIRTY_STATUS, "setfullscreen(0) sets DIRTY_STATUS");
	ASSERT(selmon->bar_dirty_segments & DIRTY_TAGS, "setfullscreen(0) sets DIRTY_TAGS");
	ASSERT(selmon->bar_dirty_segments & DIRTY_TITLE, "setfullscreen(0) sets DIRTY_TITLE");
}

static void
test_setfullscreen_idempotent(void)
{
	Client c;
	memset(&c, 0, sizeof c);
	c.win = 42;
	c.isfloating = 0;
	c.isfullscreen = 0;
	c.bw = 2;
	c.x = 100; c.y = 200; c.w = 300; c.h = 400;

	setfullscreen(&c, 0);
	ASSERT_EQ(c.isfullscreen, 0, "setfullscreen(0) on non-fullscreen is noop");
	ASSERT_EQ(c.bw, 2, "setfullscreen(0) on non-fullscreen preserves bw");
}

static void
test_togglefullscr_with_sel(void)
{
	Client c;
	memset(&c, 0, sizeof c);
	c.win = 42;
	c.mon = selmon;
	c.isfloating = 0;
	c.isfullscreen = 0;
	c.bw = 2;
	c.x = 100; c.y = 200; c.w = 300; c.h = 400;
	c.oldx = 100; c.oldy = 200; c.oldw = 300; c.oldh = 400;

	selmon->sel = &c;

	togglefullscr(NULL);
	ASSERT_EQ(c.isfullscreen, 1, "togglefullscr toggles isfullscreen to 1");

	togglefullscr(NULL);
	ASSERT_EQ(c.isfullscreen, 0, "togglefullscr toggles isfullscreen back to 0");

	selmon->sel = NULL;
}

static void
test_togglefullscr_no_sel(void)
{
	selmon->sel = NULL;
	togglefullscr(NULL);
	ASSERT(1, "togglefullscr with no sel does not crash");
}

/* --- enternotify --- */

static void
test_enternotify_non_normal_mode(void)
{
	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xcrossing.window = 9999;
	ev.xcrossing.mode = NotifyNormal + 1;  /* not NotifyNormal */
	ev.xcrossing.detail = NotifyNonlinear;

	enternotify(&ev);
	ASSERT(1, "enternotify with non-Normal mode does not crash");
}

static void
test_enternotify_notify_inferior(void)
{
	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xcrossing.window = 9999;
	ev.xcrossing.mode = NotifyNormal;
	ev.xcrossing.detail = NotifyInferior;

	enternotify(&ev);
	ASSERT(1, "enternotify with NotifyInferior detail does not crash");
}

static void
test_enternotify_enter_sel_returns_early(void)
{
	Client c;
	memset(&c, 0, sizeof c);
	c.win = 47;
	c.mon = selmon;
	c.tags = 1;
	selmon->clients = &c;
	selmon->stack = &c;
	selmon->sel = &c;
	c.next = NULL;
	c.snext = NULL;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xcrossing.window = 47;
	ev.xcrossing.mode = NotifyNormal;
	ev.xcrossing.detail = NotifyNonlinear;

	enternotify(&ev);
	ASSERT(1, "enternotify entering sel returns early");
	selmon->clients = NULL;
	selmon->stack = NULL;
	selmon->sel = NULL;
}

static void
test_enternotify_enter_barwin_returns_early(void)
{
	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xcrossing.window = selmon->barwin;
	ev.xcrossing.mode = NotifyNormal;
	ev.xcrossing.detail = NotifyNonlinear;

	enternotify(&ev);
	ASSERT(1, "enternotify entering barwin returns early (no client)");
}

/* --- motionnotify --- */

static void
test_motionnotify_no_crash_single_monitor(void)
{
	/* mons->next = NULL so the early return "!mons->next" triggers */
	mons->next = NULL;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xmotion.window = root;
	ev.xmotion.x_root = 100;
	ev.xmotion.y_root = 200;

	motionnotify(&ev);
	ASSERT(1, "motionnotify with single monitor does not crash");
}

static void
test_motionnotify_no_crash_no_mons(void)
{
	Monitor *saved_mons = mons;
	mons = NULL;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xmotion.window = root;

	motionnotify(&ev);
	ASSERT(1, "motionnotify with NULL mons does not crash");
	mons = saved_mons;
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
	selmon->barwin = 123;
	selmon->gap = ecalloc(1, sizeof(Gap));
	selmon->gap->isgap = 1;
	selmon->gap->realgap = 17;
	selmon->gap->gappx = 17;

	scheme = ecalloc(2, sizeof(Clr *));
	for (i = SchemeNorm; i <= SchemeSel; i++)
		scheme[i] = drw_scm_create(drw, colors[i], 3);
	netatom[NetWMState] = 1;
	netatom[NetWMFullscreen] = 2;
	netatom[NetActiveWindow] = 3;

	test_focusin_sets_focus_on_sel();
	test_focusin_ignores_own_window();
	test_focusin_no_sel_ignores();
	test_clientmessage_fullscreen_add();
	test_clientmessage_fullscreen_remove();
	test_clientmessage_fullscreen_toggle();
	test_clientmessage_noop_on_unknown_window();
	test_clientmessage_activewindow_sets_urgent();
	test_clientmessage_activewindow_skips_sel();

	test_unmapnotify_send_event_withdraws();
	test_unmapnotify_normal_unmanages();

	test_destroynotify_unmanages();

	test_configurerequest_updates_floating_geometry();
	test_configurerequest_partial_mask();
	test_configurerequest_nonclient_configures();

	test_expose_draws_bar();
	test_expose_ignores_other();

	test_propertynotify_root_wmname();
	test_propertynotify_propertydelete_ignored();
	test_propertynotify_client_normal_hints();
	test_propertynotify_client_wm_hints();
	test_propertynotify_client_wmname_selected();
	test_propertynotify_client_wmname_not_selected();

	test_propertynotify_root_wmname_fullscreen_skip();
	test_propertynotify_root_wmname_no_fullscreen_not_skipped();

	test_setfullscreen_enter();
	test_setfullscreen_exit();
	test_setfullscreen_idempotent();
	test_togglefullscr_with_sel();
	test_togglefullscr_no_sel();

	test_enternotify_non_normal_mode();
	test_enternotify_notify_inferior();
	test_enternotify_enter_sel_returns_early();
	test_enternotify_enter_barwin_returns_early();

	test_motionnotify_no_crash_single_monitor();
	test_motionnotify_no_crash_no_mons();

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
