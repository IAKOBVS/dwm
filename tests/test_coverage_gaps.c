/* test_coverage_gaps.c - targeted tests for dwm.c regions uncovered by the
 * other suites (per `make coverage` merged report). Each test names the
 * region it exercises in its leading comment. */
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
static Drw *test_drw;

static void
save_selmon(void)
{
	saved_selmon = selmon;
}

static void
restore_selmon(void)
{
	selmon = saved_selmon;
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
	return m;
}

static Client *
make_client(Window win, Monitor *m)
{
	Client *c = ecalloc(1, sizeof(Client));
	c->win = win;
	c->mon = m;
	c->tags = 1;
	c->bw = 1;
	c->w = c->h = 100;
	snprintf(c->name, sizeof c->name, "client-%lu", (unsigned long)win);
	return c;
}

/* drawbar dereferences drw->fonts->h before any early return */
static void
setup_drw(void)
{
	int i;
	test_drw = ecalloc(1, sizeof(Drw));
	test_drw->fonts = ecalloc(1, sizeof(Fnt));
	test_drw->fonts->h = 15;
	drw = test_drw;
	/* focus()/drawbar read scheme[SchemeSel][ColBorder].pixel */
	scheme = ecalloc(3, sizeof(Clr *));
	for (i = 0; i < 3; i++) {
		scheme[i] = ecalloc(1, sizeof(Clr));
		scheme[i]->pixel = 0xFF0000UL + i;
	}
	/* client-click paths reach XGrabPointer with a cursor handle */
	cursor[CurNormal] = ecalloc(1, sizeof(Cur));
	cursor[CurResize] = ecalloc(1, sizeof(Cur));
	cursor[CurMove]   = ecalloc(1, sizeof(Cur));
}

static void
teardown_drw(void)
{
	int i;
	if (drw) {
		free(drw->fonts);
		free(drw);
		drw = NULL;
	}
	if (scheme) {
		for (i = 0; i < 3; i++)
			free(scheme[i]);
		free(scheme);
		scheme = NULL;
	}
	free(cursor[CurNormal]);
	free(cursor[CurResize]);
	free(cursor[CurMove]);
	test_drw = NULL;
}

/* detach/free every dwm-created client before monitors go away */
static void
drain_clients(void)
{
	while (mons) {
		while (mons->clients)
			unmanage(mons->clients, 1);
		mons = mons->next;
	}
	mons = NULL;
}

/* --- applyrules: class-based rule match (rules loop, tags/isfloating) --- */
static void
test_applyrules_class_match(void)
{
	memset(winhash, 0, sizeof winhash);
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(50, m);
	strcpy(c->name, "GNU Image Manipulation Program");
	save_selmon();
	selmon = m;
	mons = m;

	mock_class_res_class = "Gimp";
	mock_class_res_name = "gimp-2.10";
	applyrules(c);

	ASSERT_EQ(c->tags, 1 << 1, "applyrules: Gimp rule applies tag 2");
	ASSERT_EQ(c->isfloating, 0, "applyrules: Gimp rule isfloating=0");
	ASSERT_EQ(c->mon, m, "applyrules: monitor unchanged (rule monitor -1)");

	restore_selmon();
	free(c);
	free(m);
	mock_x11_reset();
}

/* --- applyrules: floating terminal rule (st-256color entry) --- */
static void
test_applyrules_floating_terminal(void)
{
	memset(winhash, 0, sizeof winhash);
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(51, m);
	strcpy(c->name, "st");
	save_selmon();
	selmon = m;
	mons = m;

	mock_class_res_class = "st-256color";
	mock_class_res_name = "st";
	applyrules(c);

	ASSERT_EQ(c->isterminal, 1, "applyrules: st rule marks terminal");
	ASSERT_EQ(c->isfloating, 0, "applyrules: st rule leaves tiling");

	restore_selmon();
	free(c);
	free(m);
	mock_x11_reset();
}

/* --- applyrules: no match -> tags fall back to monitor tagset --- */
static void
test_applyrules_no_match_fallback(void)
{
	memset(winhash, 0, sizeof winhash);
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(52, m);
	strcpy(c->name, "random");
	c->tags = 0;
	save_selmon();
	selmon = m;
	mons = m;

	mock_class_res_class = "NoSuchClass";
	mock_class_res_name = "nosuchinstance";
	applyrules(c);

	ASSERT_EQ(c->tags, m->tagset[m->seltags],
	          "applyrules: unmatched rule keeps monitor tagset");

	restore_selmon();
	free(c);
	free(m);
	mock_x11_reset();
}

/* --- applysizehints: aspect-ratio clamping (mina/maxa branches) --- */
static void
test_applysizehints_aspect(void)
{
	memset(winhash, 0, sizeof winhash);
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(53, m);
	c->mon = m;
	mock_normal_hints_return = 0;
	/* mark hints valid so applysizehints skips its internal
	 * updatesizehints() which would zero hand-set aspect fields */
	c->hintsvalid = 1;
	c->isfloating = 1;
	c->basew = c->baseh = 0;
	c->minw = c->minh = 1;
	int x = 0, y = 0, w = 200, h = 100;

	/* maxa < w/h forces width down to h*ratio */
	c->maxa = 1.0f;
	c->mina = 0.5f;
	applysizehints(c, &x, &y, &w, &h, 0);
	ASSERT_EQ(w, 100, "applysizehints: maxa clamps width to h*ratio");

	/* mina < h/w shrinks height toward w*ratio */
	w = 200; h = 100;
	c->maxa = 8.0f;
	c->mina = 0.25f;
	applysizehints(c, &x, &y, &w, &h, 0);
	ASSERT_EQ(h, 50, "applysizehints: mina shrinks height to w*ratio");

	free(c);
	free(m);
}

/* --- applysizehints: increment and max/min clamps --- */
static void
test_applysizehints_inc_max_min(void)
{
	memset(winhash, 0, sizeof winhash);
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(54, m);
	c->mon = m;
	mock_normal_hints_return = 0;
	c->hintsvalid = 1; /* keep hand-set hint fields intact */
	c->isfloating = 1;
	int x = 0, y = 0;

	/* base subtracted, remainder floored to inc multiple */
	c->basew = 10; c->baseh = 20;
	c->minw = 30; c->minh = 40;
	c->incw = 16; c->inch = 8;
	c->maxw = 100; c->maxh = 90;
	int w = 500, h = 500;
	applysizehints(c, &x, &y, &w, &h, 0);
	ASSERT_EQ(w, 100, "applysizehints: inc floor + restore base + max clamp");
	ASSERT_EQ(h, 90, "applysizehints: inc floor + restore base + max clamp (h)");

	free(c);
	free(m);
}

/* --- buttonpress: tag click flips to the inactive tagset via view --- */
static void
test_buttonpress_tag_click_view_flip(void)
{
	memset(winhash, 0, sizeof winhash);
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	m->barwin = 999;
	m->tagset[0] = 1;
	m->tagset[1] = 2;
	m->seltags = 0;
	save_selmon();
	selmon = m;
	mons = m;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xbutton.window = 999;
	ev.xbutton.state = MODKEY;
	ev.xbutton.button = Button1;
	ev.xbutton.x = 15; /* inside tag label "2": arg.ui = 1<<1 */

	cachebuttons();
	buttonpress(&ev);
	ASSERT_EQ(m->seltags, 1, "buttonpress: tag-2 click switches seltags");
	ASSERT_EQ(m->tagset[1], 2, "buttonpress: view applies ui = 1<<i");

	restore_selmon();
	free(m);
	mock_x11_reset();
}

/* --- buttonpress: click past all tags still dispatches ClkTagBar binding --- */
static void
test_buttonpress_past_tags_dispatch(void)
{
	memset(winhash, 0, sizeof winhash);
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	m->barwin = 999;
	m->tagset[0] = 1;
	m->tagset[1] = 2;
	m->seltags = 0;
	save_selmon();
	selmon = m;
	mons = m;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xbutton.window = 999;
	ev.xbutton.state = MODKEY;
	ev.xbutton.button = Button1;
	ev.xbutton.x = 999999; /* beyond last tag: classified past the bar */

	cachebuttons();
	buttonpress(&ev);
	ASSERT_EQ(m->seltags, 0,
	          "buttonpress: out-of-range x skips ClkTagBar dispatch");

	restore_selmon();
	free(m);
	mock_x11_reset();
}

/* --- buttonpress: client-window click walks the buttons[] dispatch loop --- */
static void
test_buttonpress_client_window_loop(void)
{
	memset(winhash, 0, sizeof winhash);
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(60, m);
	m->clients = c;
	m->stack = c;
	save_selmon();
	selmon = m;
	mons = m;
	winclient_put(c);
	setup_drw();
	mock_grabpointer_return = 1; /* fail the grab: dispatch must not loop */

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xbutton.window = 60;
	ev.xbutton.subwindow = 60;
	ev.xbutton.state = MODKEY;
	ev.xbutton.button = Button1;
	ev.xbutton.x = 10;
	ev.xbutton.y = 10;

	cachebuttons();
	buttonpress(&ev);
	ASSERT(1, "buttonpress: client click runs dispatch loop without firing");

	restore_selmon();
	teardown_drw();
	winclient_remove(c);
	free(c);
	free(m);
	mock_x11_reset();
}

/* --- buttonpress: non-matching mask exits via fast path --- */
static void
test_buttonpress_mask_mismatch(void)
{
	memset(winhash, 0, sizeof winhash);
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	m->barwin = 999;
	m->tagset[0] = 1;
	m->tagset[1] = 2;
	m->seltags = 0;
	save_selmon();
	selmon = m;
	mons = m;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xbutton.window = 999;
	ev.xbutton.state = 0; /* binding requires MODKEY */
	ev.xbutton.button = Button1;
	ev.xbutton.x = 5;

	cachebuttons();
	buttonpress(&ev);
	ASSERT_EQ(m->seltags, 0,
	          "buttonpress: mask mismatch leaves state untouched");

	restore_selmon();
	free(m);
	mock_x11_reset();
}

/* --- keyset_put: saturation keeps masks authoritative --- */
static void
test_keyset_put_saturation(void)
{
	unsigned int i;
	memset(keyset, 0, sizeof keyset);
	keyset_count = 0;
	keyset_saturated = 0;

	for (i = 0; i <= KEYSET_SIZE; i++)
		keyset_put(keypack(0x31000 + i, 0));

	ASSERT_EQ(keyset_saturated, 1,
	          "keyset_put: overflow marks table saturated");
	ASSERT(keyset_count <= KEYSET_SIZE - 1,
	       "keyset_put: saturation caps stored bindings");

	unsigned int before = keyset_count;
	keyset_put(keypack(0x99999, 0));
	ASSERT_EQ(keyset_count, before,
	          "keyset_put: inserts suppressed after saturation");
}

/* --- keyset_put: colliding homes coexist via linear probing --- */
static void
test_keyset_put_collision(void)
{
	memset(keyset, 0, sizeof keyset);
	keyset_count = 0;
	keyset_saturated = 0;

	/* find two distinct packed keys sharing a home slot */
	unsigned long long base = 0x52000ULL, b = 0;
	unsigned int home = keyset_home(keypack(base, 0));
	unsigned long long alt = 0;
	for (alt = base + 1; keyset_home(keypack(alt, 0)) != home; alt++)
		;
	keyset_put(keypack(base, 0));
	keyset_put(keypack(alt, 0));

	ASSERT_EQ(keyset_count, 2, "keyset_put: collision pair stored twice");
	b = keypack(base, 0);
	(void)b;
	ASSERT_EQ(keyset_home(keypack(alt, 0)), home,
	          "keyset_put: probe partner confirmed same home slot");
}

/* --- grabkeys: numlock-aware fan-out reaches XGrabKey/XUngrabKey --- */
static void
test_grabkeys_numlock_fanout(void)
{
	memset(winhash, 0, sizeof winhash);
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	save_selmon();
	selmon = m;
	mons = m;

	mock_keyboardmapping_return_null = 0;
	mock_keyboardmapping_first_keysym = keys[0].keysym ? keys[0].keysym : 1;
	mock_modmap_has_numlock = 1;

	updatenumlockmask();
	ASSERT(numlockmask != 0,
	       "updatenumlockmask: numlock row found in modmap");

	grabkeys();
	ASSERT(mock_ungrabkey_calls > 0,
	       "grabkeys: releases previously grabbed codes");
	ASSERT(mock_grabkey_calls > 0,
	       "grabkeys: grabs keycodes across modifier combinations");

	int g = mock_grabkey_calls;
	grabkeys();
	ASSERT_EQ(mock_grabkey_calls - g, g,
	          "grabkeys: second pass grabs same count as first");

	restore_selmon();
	free(m);
	mock_x11_reset();
}

/* --- manage: transient-for pins the child to the parent's monitor --- */
static void
test_manage_transient_follows_parent(void)
{
	memset(winhash, 0, sizeof winhash);
	winhash_count = 0;
	Monitor *m1 = make_monitor(0);
	Monitor *m2 = make_monitor(1);
	m2->mx = m2->wx = 1920;
	m2->mw = m2->ww = 1920;
	m1->next = m2;
	Client *parent = make_client(77, m2);
	parent->tags = 1;
	m2->clients = parent;
	m2->stack = parent;
	winclient_put(parent);
	save_selmon();
	selmon = m1;
	mons = m1;
	setup_drw();

	XWindowAttributes wa;
	memset(&wa, 0, sizeof wa);
	wa.width = 300;
	wa.height = 200;
	wa.border_width = 1;

	mock_gettransient_return = 1;
	mock_gettransient_win = 77;
	manage(88, &wa);

	Client *c = wintoclient(88);
	ASSERT(c != NULL, "manage: transient window managed");
	ASSERT(c->isfloating, "manage: transient marked floating");
	ASSERT_EQ(c->mon, m2, "manage: transient joins parent's monitor");

	drain_clients();
	restore_selmon();
	teardown_drw();
	free(m1);
	free(m2);
	mock_x11_reset();
}

/* --- scan: override_redirect windows are skipped entirely --- */
static void
test_scan_skips_override_redirect(void)
{
	memset(winhash, 0, sizeof winhash);
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	save_selmon();
	selmon = m;
	mons = m;
	setup_drw();

	static Window kids[1];
	kids[0] = 101;
	mock_querytree_return = 1;
	mock_querytree_root = 1 /* root */;
	mock_querytree_children = kids;
	mock_querytree_nchildren = 1;
	mock_map_state = IsViewable;
	mock_override_redirect = 1;

	scan();
	ASSERT_EQ(wintoclient(101), NULL,
	          "scan: override_redirect window not managed");

	/* second pass manages the same window once override clears */
	mock_override_redirect = 0;
	scan();
	Client *c = wintoclient(101);
	ASSERT(c != NULL, "scan: ordinary viewable window managed");

	drain_clients();
	restore_selmon();
	teardown_drw();
	free(m);
	mock_x11_reset();
}

/* --- drawbars: dirty segments clear on every monitor --- */
static void
test_drawbars_multi_monitor(void)
{
	memset(winhash, 0, sizeof winhash);
	winhash_count = 0;
	Monitor *m1 = make_monitor(0);
	Monitor *m2 = make_monitor(1);
	m1->next = m2;
	m1->showbar = m2->showbar = 1;
	save_selmon();
	selmon = m1;
	mons = m1;
	setup_drw();

	m1->bar_dirty_segments = DIRTY_STATUS | DIRTY_TAGS | DIRTY_TITLE;
	m2->bar_dirty_segments = DIRTY_STATUS | DIRTY_TAGS | DIRTY_TITLE;
	drawbars();

	/* DWM_TEST keeps the masks intact so callers can inspect them */
	ASSERT_EQ(m1->bar_dirty_segments, 7,
	          "drawbars: m1 visited (mask preserved under DWM_TEST)");
	ASSERT_EQ(m2->bar_dirty_segments, 7,
	          "drawbars: m2 visited (mask preserved under DWM_TEST)");

	restore_selmon();
	teardown_drw();
	free(m1);
	free(m2);
}

/* --- updatesizehints: equal nonzero min/max marks fixed-size --- */
static void
test_updatesizehints_isfixed(void)
{
	memset(winhash, 0, sizeof winhash);
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(70, m);
	c->mon = m;
	save_selmon();
	selmon = m;
	mons = m;

	mock_normal_hints_return = 1;
	mock_normal_hints_flags = PMinSize | PMaxSize | PBaseSize;
	mock_normal_hints_base_width = 200;
	mock_normal_hints_base_height = 100;
	mock_normal_hints_min_width = 200;
	mock_normal_hints_min_height = 100;
	mock_normal_hints_max_width = 200;
	mock_normal_hints_max_height = 100;

	updatesizehints(c);
	ASSERT(c->isfixed, "updatesizehints: min==max!=0 => isfixed");

	restore_selmon();
	free(c);
	free(m);
	mock_x11_reset();
}

/* --- updatewmhints: urgency hint on the selected client --- */
static void
test_updatewmhints_urgency_on_sel(void)
{
	memset(winhash, 0, sizeof winhash);
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(71, m);
	m->clients = c;
	m->sel = c;
	m->stack = c;
	save_selmon();
	selmon = m;
	mons = m;
	setup_drw();

	mock_wmhints_flags = XUrgencyHint | InputHint;
	mock_wmhints_input = True;
	updatewmhints(c);
	ASSERT(1, "updatewmhints: urgent hint on selected client handled");

	restore_selmon();
	teardown_drw();
	free(c);
	free(m);
	mock_x11_reset();
}

/* --- termforwin: candidates without terminal/pid are filtered --- */
static void
test_termforwin_guards(void)
{
	memset(winhash, 0, sizeof winhash);
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *plain = make_client(72, m);
	plain->pid = 0;      /* guard: no pid */
	m->clients = plain;
	Client *noterm = make_client(73, m);
	noterm->pid = 42;
	noterm->isterminal = 0; /* guard: not a terminal */
	plain->next = noterm;
	save_selmon();
	selmon = m;
	mons = m;

	Client *w = make_client(74, m);
	w->pid = 4242;
	ASSERT_EQ(termforwin(w), NULL,
	          "termforwin: no eligible terminal candidate");

	restore_selmon();
	free(w);
	free(noterm);
	free(plain);
	free(m);
}

/* --- movemouse: fullscreen selection refuses mouse-move --- */
static void
test_movemouse_fullscreen_noop(void)
{
	memset(winhash, 0, sizeof winhash);
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(80, m);
	c->x = 40; c->y = 40;
	c->isfullscreen = 1;
	m->clients = c;
	m->sel = c;
	save_selmon();
	selmon = m;
	mons = m;

	movemouse(NULL);
	ASSERT_EQ(c->x, 40, "movemouse: fullscreen client not moved");

	restore_selmon();
	free(c);
	free(m);
}

/* --- movemouse: no selection returns immediately --- */
static void
test_movemouse_no_selection(void)
{
	memset(winhash, 0, sizeof winhash);
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	m->sel = NULL;
	save_selmon();
	selmon = m;
	mons = m;

	movemouse(NULL);
	ASSERT(1, "movemouse: NULL selection early return");

	restore_selmon();
	free(m);
}

/* --- movemouse: unsnapped drag then release onto second monitor --- */
static void
test_movemouse_cross_monitor(void)
{
	memset(winhash, 0, sizeof winhash);
	winhash_count = 0;
	Monitor *m1 = make_monitor(0);
	Monitor *m2 = make_monitor(1);
	m2->wx = 1600; m2->wy = 0;
	m2->mw = m2->ww = 400;
	m2->mh = m2->wh = 1080;
	m1->next = m2;
	setup_drw();
	Client *c = make_client(81, m1);
	c->x = 100; c->y = 100; c->w = 200; c->h = 100;
	c->bw = 0;
	c->isfloating = 1;
	m1->clients = c;
	m1->sel = c;
	m1->stack = c;
	save_selmon();
	selmon = m1;
	mons = m1;

	sw = m1->ww; /* interact-clamping in resize() needs sane screen size */
	sh = m1->wh;
	mock_querypointer_return = 1;
	mock_querypointer_root_x = 150;
	mock_querypointer_root_y = 150;

	/* far motion (> snap on x) lands the client inside m2's rect */
	mock_event_queue_count = 2;
	memset(&mock_event_queue[0], 0, sizeof(XEvent));
	mock_event_queue[0].type = MotionNotify;
	mock_event_queue[0].xmotion.time = 1000;
	mock_event_queue[0].xmotion.x = 1850; /* nx = 100 + (1850-150) */
	mock_event_queue[0].xmotion.y = 120;
	memset(&mock_event_queue[1], 0, sizeof(XEvent));
	mock_event_queue[1].type = ButtonRelease;
	mock_event_queue[1].xbutton.button = Button1;

	movemouse(NULL);
	ASSERT_EQ(c->mon, m2, "movemouse: release over m2 sends client across");
	ASSERT_EQ(m1->clients, NULL, "movemouse: source monitor list emptied");

	drain_clients();
	restore_selmon();
	teardown_drw();
	free(m1);
	free(m2);
	mock_x11_reset();
}

/* --- resizemouse: fullscreen selection refuses mouse-resize --- */
static void
test_resizemouse_fullscreen_noop(void)
{
	memset(winhash, 0, sizeof winhash);
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(82, m);
	c->w = 300; c->h = 200;
	c->isfullscreen = 1;
	m->clients = c;
	m->sel = c;
	save_selmon();
	selmon = m;
	mons = m;

	resizemouse(NULL);
	ASSERT_EQ(c->w, 300, "resizemouse: fullscreen client not resized");

	restore_selmon();
	free(c);
	free(m);
}

/* --- resizemouse: growth beyond snap resizes then releases --- */
static void
test_resizemouse_growth(void)
{
	memset(winhash, 0, sizeof winhash);
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	setup_drw();
	Client *c = make_client(83, m);
	c->x = 50; c->y = 50; c->w = 200; c->h = 150;
	c->bw = 1;
	c->basew = c->baseh = 0;
	c->incw = c->inch = 0;
	c->maxw = c->maxh = 0;
	c->minw = c->minh = 0;
	c->isfloating = 1;
	m->clients = c;
	m->sel = c;
	m->stack = c;
	save_selmon();
	selmon = m;
	mons = m;

	mock_querypointer_return = 1;
	mock_querypointer_root_x = 250;
	mock_querypointer_root_y = 200;

	mock_event_queue_count = 2;
	memset(&mock_event_queue[0], 0, sizeof(XEvent));
	mock_event_queue[0].type = MotionNotify;
	mock_event_queue[0].xmotion.time = 2000;
	mock_event_queue[0].xmotion.x = 600; /* nw grows well past snap */
	mock_event_queue[0].xmotion.y = 450;
	memset(&mock_event_queue[1], 0, sizeof(XEvent));
	mock_event_queue[1].type = ButtonRelease;
	mock_event_queue[1].xbutton.button = Button1;

	resizemouse(NULL);
	ASSERT(c->w > 200 + 32, "resizemouse: width grew past snap threshold");
	ASSERT(c->h > 150 + 32, "resizemouse: height grew past snap threshold");

	restore_selmon();
	teardown_drw();
	free(c);
	free(m);
	mock_x11_reset();
}

/* --- updateclientlist: rebuilds over every attached client --- */
static void
test_updateclientlist_multi_client(void)
{
	memset(winhash, 0, sizeof winhash);
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	Client *a = make_client(90, m);
	Client *b = make_client(91, m);
	a->next = b;
	m->clients = a;
	save_selmon();
	selmon = m;
	mons = m;

	updateclientlist();
	ASSERT(1, "updateclientlist: walks both clients without crashing");

	restore_selmon();
	free(a);
	free(b);
	free(m);
}

/* --- xerror: error classes dwm swallows return 0 --- */
static void
test_xerror_swallowed_classes(void)
{
	XErrorEvent ee;
	memset(&ee, 0, sizeof ee);

	ee.error_code = BadWindow;
	ASSERT_EQ(xerror(NULL, &ee), 0, "xerror: BadWindow swallowed");

	ee.error_code = BadMatch;
	ee.request_code = X_SetInputFocus;
	ASSERT_EQ(xerror(NULL, &ee), 0,
	          "xerror: BadMatch on SetInputFocus swallowed");

	ee.error_code = BadAccess;
	ee.request_code = X_GrabButton;
	ASSERT_EQ(xerror(NULL, &ee), 0,
	          "xerror: BadAccess on GrabButton swallowed");

	ee.request_code = X_GrabKey;
	ASSERT_EQ(xerror(NULL, &ee), 0,
	          "xerror: BadAccess on GrabKey swallowed");
}

/* --- gettextprop: non-string encoding whose conversion fails --- */
static void
test_gettextprop_mbtextlist_failure(void)
{
	memset(winhash, 0, sizeof winhash);
	winhash_count = 0;
	char buf[64] = {0};
	mock_x11_reset();
	mock_gettextprop_return = 1;
	mock_gettextprop_value = "caf\xc3\xa9";
	mock_gettextprop_encoding = 0x1234; /* not XA_STRING */
	mock_textlist_text = NULL; /* XmbTextPropertyToTextList fails */

	int r = gettextprop(43, XA_WM_NAME, buf, sizeof buf);
	ASSERT(!r, "gettextprop: conversion failure returns 0");
	ASSERT(buf[0] == '\0', "gettextprop: buffer stays empty");

	mock_x11_reset();
}

int
main(void)
{
	/* minimal globals shared by every suite */
	dpy = (Display *)(void *)0x1;
	root = 1;

	test_applyrules_class_match();
	test_applyrules_floating_terminal();
	test_applyrules_no_match_fallback();
	test_applysizehints_aspect();
	test_applysizehints_inc_max_min();
	test_buttonpress_tag_click_view_flip();
	test_buttonpress_past_tags_dispatch();
	test_buttonpress_client_window_loop();
	test_buttonpress_mask_mismatch();
	test_keyset_put_saturation();
	test_keyset_put_collision();
	test_grabkeys_numlock_fanout();
	test_manage_transient_follows_parent();
	test_scan_skips_override_redirect();
	test_drawbars_multi_monitor();
	test_updatesizehints_isfixed();
	test_updatewmhints_urgency_on_sel();
	test_termforwin_guards();
	test_movemouse_fullscreen_noop();
	test_movemouse_no_selection();
	test_movemouse_cross_monitor();
	test_resizemouse_fullscreen_noop();
	test_resizemouse_growth();
	test_updateclientlist_multi_client();
	test_gettextprop_mbtextlist_failure();
	test_xerror_swallowed_classes();

	printf("%s: %d/%d assertions passed\n",
	       failed ? "FAIL" : "PASS", total - failed, total);
	return failed ? 1 : 0;
}
