/*
 * test_resizemouse.c — per-function tests for dwm.c's resizemouse().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through resizemouse(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- resizemouse --- */
static void
test_resizemouse_noop_no_sel(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	drag.phase = DRAG_IDLE;
	pointergrab = 0;
	save_selmon();
	selmon = make_monitor(0);
	mons = selmon;
	selmon->sel = NULL;
	Arg arg = {0};

	resizemouse(&arg);
	/* No selection: return before restack, grab or any X side effects. */
	ASSERT_EQ(mock_xgrabpointer_calls, 0, "resizemouse no-sel: no pointer grab");
	ASSERT_EQ(mock_xungrabpointer_calls, 0, "resizemouse no-sel: no ungrab");
	ASSERT_EQ(mock_xraise_window.count, 0, "resizemouse no-sel: no restack");
	ASSERT_EQ(pointergrab, 0, "resizemouse no-sel: pointergrab stays clear");
	ASSERT_EQ(drag.phase, DRAG_IDLE, "resizemouse no-sel: drag state idle");

{
		Monitor *tmpm = selmon; /* free the fixture, then restore globals */
		restore_selmon();
		free(tmpm);
	}
}

/* --- resizemouse basic (lines 1311-1365) --- */
static void
test_resizemouse_basic(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	drag.phase = DRAG_IDLE;
	pointergrab = 0;
	Monitor *m = make_monitor(0);
	m->wx = 0; m->wy = 0; m->ww = 1920; m->wh = 1080;
	m->showbar = 0;
	Client *c = make_client(1, m);
	c->x = 100; c->y = 100; c->w = 200; c->h = 100; c->bw = 0;
	c->isfloating = 1;
	c->tags = 1;
	m->clients = c;
	m->sel = c;
	m->stack = c;
	m->barwin = 0;

	save_selmon();
	selmon = m;
	mons = m;

	/* Inject a ButtonRelease so the do-while exits */
	mock_event_queue_count = 1;
	memset(&mock_event_queue[0], 0, sizeof(XEvent));
	mock_event_queue[0].type = ButtonRelease;

	resizemouse(&(Arg){0});
	/* An immediate release grabs then ungrabs without changing geometry. */
	ASSERT_EQ(mock_xgrabpointer_calls, 1, "resizemouse basic: pointer grabbed");
	ASSERT_EQ(mock_xungrabpointer_calls, 1, "resizemouse basic: pointer ungrabbed");
	ASSERT_EQ(pointergrab, 0, "resizemouse basic: pointergrab cleared");
	ASSERT_EQ(drag.phase, DRAG_IDLE, "resizemouse basic: drag state idle after release");
	ASSERT_EQ(c->x, 100, "resizemouse basic: x unchanged");
	ASSERT_EQ(c->y, 100, "resizemouse basic: y unchanged");
	ASSERT_EQ(c->w, 200, "resizemouse basic: width unchanged");
	ASSERT_EQ(c->h, 100, "resizemouse basic: height unchanged");
	ASSERT_EQ(selmon, m, "resizemouse basic: stays on same monitor");

	mock_event_queue_count = 0;
	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- resizemouse fullscreen early return (line 1321-1322) --- */
static void
test_resizemouse_fullscreen_early_return(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	drag.phase = DRAG_IDLE;
	pointergrab = 0;
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->isfullscreen = 1;
	m->sel = c;
	save_selmon();
	selmon = m;
	mons = m;

	resizemouse(&(Arg){0});
	ASSERT(c->isfullscreen, "resizemouse: fullscreen returns early, client unchanged");
	ASSERT_EQ(mock_xgrabpointer_calls, 0, "resizemouse fullscreen: no pointer grab");
	ASSERT_EQ(mock_xungrabpointer_calls, 0, "resizemouse fullscreen: no ungrab");
	ASSERT_EQ(mock_xraise_window.count, 0, "resizemouse fullscreen: no restack");
	ASSERT_EQ(pointergrab, 0, "resizemouse fullscreen: pointergrab stays clear");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- resizemouse MotionNotify path (lines 1338-1354) --- */
static void
test_resizemouse_motion(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	drag.phase = DRAG_IDLE;
	pointergrab = 0;
	Monitor *m = make_monitor(0);
	m->wx = 0; m->wy = 0; m->ww = 1920; m->wh = 1080;
	m->showbar = 0;
	Client *c = make_client(1, m);
	c->x = 100; c->y = 100; c->w = 200; c->h = 100; c->bw = 0;
	c->isfloating = 1;
	c->tags = 1;
	m->clients = c;
	m->sel = c;
	m->stack = c;
	m->barwin = 0;

	save_selmon();
	selmon = m;
	mons = m;

	/* Inject MotionNotify then ButtonRelease */
	mock_event_queue_count = 2;
	memset(&mock_event_queue[0], 0, sizeof(XEvent));
	mock_event_queue[0].type = MotionNotify;
	mock_event_queue[0].xmotion.x = 300;
	mock_event_queue[0].xmotion.y = 200;
	mock_event_queue[0].xmotion.time = 10000;
	memset(&mock_event_queue[1], 0, sizeof(XEvent));
	mock_event_queue[1].type = ButtonRelease;

	resizemouse(&(Arg){0});
	/* nw = 300-100+1 = 201, nh = 200-100+1 = 101, but minh=201 clamps h. */
	ASSERT_EQ(c->x, 100, "resizemouse motion: x unchanged");
	ASSERT_EQ(c->y, 100, "resizemouse motion: y unchanged");
	ASSERT_EQ(c->w, 201, "resizemouse motion: width applied");
	ASSERT_EQ(c->h, 201, "resizemouse motion: height clamped to minimum");
	ASSERT_EQ(mock_xconfigure_window.count, 1, "resizemouse motion: one reconfigure");
	ASSERT_EQ(mock_xconfigure_window.win, (unsigned long)c->win, "resizemouse motion: reconfigured client");
	ASSERT_EQ(mock_xconfigure_last.width, 201, "resizemouse motion: configured width");
	ASSERT_EQ(mock_xconfigure_last.height, 201, "resizemouse motion: configured height");
	ASSERT_EQ(mock_xsend_event.count, 1, "resizemouse motion: synthetic ConfigureNotify sent");
	ASSERT_EQ(mock_xungrabpointer_calls, 1, "resizemouse motion: pointer ungrabbed");
	ASSERT_EQ(pointergrab, 0, "resizemouse motion: pointergrab cleared");

	mock_event_queue_count = 0;
	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- resizemouse with MotionNotify that triggers togglefloating (lines 1348-1350) --- */
static void
test_resizemouse_motion_togglefloat(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	drag.phase = DRAG_IDLE;
	pointergrab = 0;
	Monitor *m = make_monitor(0);
	m->wx = 0; m->wy = 0; m->ww = 1920; m->wh = 1080;
	m->showbar = 0;
	m->lt[0] = m->lt[1] = &layouts[0]; /* tile layout has arrange function */
	Client *c = make_client(1, m);
	c->x = 100; c->y = 100; c->w = 200; c->h = 100; c->bw = 0;
	c->isfloating = 0; /* not floating, so togglefloating may be triggered */
	c->tags = 1;
	m->clients = c;
	m->sel = c;
	m->stack = c;
	m->barwin = 0;

	save_selmon();
	selmon = m;
	mons = m;

	/* Inject MotionNotify with large delta (beyond snap threshold) then ButtonRelease */
	mock_event_queue_count = 2;
	memset(&mock_event_queue[0], 0, sizeof(XEvent));
	mock_event_queue[0].type = MotionNotify;
	mock_event_queue[0].xmotion.x = 400; /* large change from initial position */
	mock_event_queue[0].xmotion.y = 300;
	mock_event_queue[0].xmotion.time = 10000;
	memset(&mock_event_queue[1], 0, sizeof(XEvent));
	mock_event_queue[1].type = ButtonRelease;

	resizemouse(&(Arg){0});
	/* The large resize triggers togglefloating since nw-c->w > snap. */
	ASSERT_EQ(c->isfloating, 1, "resizemouse togglefloat: tiled client becomes floating");
	ASSERT_EQ(c->w, 301, "resizemouse togglefloat: width follows motion");
	ASSERT_EQ(c->h, 201, "resizemouse togglefloat: height follows motion");
	ASSERT_EQ(mock_xungrabpointer_calls, 1, "resizemouse togglefloat: pointer ungrabbed");
	ASSERT_EQ(pointergrab, 0, "resizemouse togglefloat: pointergrab cleared");

	mock_event_queue_count = 0;
	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- resizemouse grabpointer fail (line 1328) --- */
static void
test_resizemouse_grabpointer_fail(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	drag.phase = DRAG_IDLE;
	pointergrab = 0;
	Monitor *m = make_monitor(0);
	m->wx = 0; m->wy = 0; m->ww = 1920; m->wh = 1080;
	Client *c = make_client(1, m);
	c->isfloating = 1;
	c->tags = 1;
	m->sel = c;
	m->stack = c;
	m->barwin = 0;

	save_selmon();
	selmon = m;
	mons = m;

	mock_grabpointer_return = AlreadyGrabbed;

	resizemouse(&(Arg){0});
	ASSERT(c->w == 200, "resizemouse grabpointer fail: client size unchanged");
	/* A failed grab must not ungrab someone else's pointer or enter the loop. */
	ASSERT_EQ(mock_xgrabpointer_calls, 1, "resizemouse grabpointer fail: grab attempted");
	ASSERT_EQ(mock_xungrabpointer_calls, 0, "resizemouse grabpointer fail: no ungrab");
	ASSERT_EQ(pointergrab, 0, "resizemouse grabpointer fail: pointergrab stays clear");
	ASSERT_EQ(drag.phase, DRAG_IDLE, "resizemouse grabpointer fail: drag state idle");

	mock_grabpointer_return = 0;
	mock_x11_reset();
	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- resizemouse cross-monitor sendmon (lines 1361-1363) --- */
static void
test_resizemouse_cross_monitor(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	drag.phase = DRAG_IDLE;
	pointergrab = 0;
	Monitor *m1 = make_monitor(0);
	m1->wx = 0; m1->wy = 0; m1->ww = 960; m1->wh = 1080;
	m1->showbar = 0;
	Monitor *m2 = make_monitor(1);
	m2->wx = 960; m2->wy = 0; m2->ww = 960; m2->wh = 1080;
	m2->showbar = 0;
	m1->next = m2;
	m2->next = NULL;

	Client *c = make_client(1, m1);
	c->x = 900; c->y = 100; c->w = 100; c->h = 100; c->bw = 0;
	c->isfloating = 1;
	c->tags = 1;
	m1->clients = c;
	m1->sel = c;
	m1->stack = c;
	m1->barwin = 0;
	m2->barwin = 0;

	save_selmon();
	selmon = m1;
	mons = m1;

	mock_event_queue_count = 2;
	memset(&mock_event_queue[0], 0, sizeof(XEvent));
	mock_event_queue[0].type = MotionNotify;
	/* nw = MAX(ev.x - ocx - 2*bw + 1, 1). ocx=900. ev.x=1400 → nw=501
	 * c->x=900, c->w=501 → spans 900-1401, overlaps m2 (960+) more */
	mock_event_queue[0].xmotion.x = 1400;
	mock_event_queue[0].xmotion.y = 300;
	mock_event_queue[0].xmotion.time = 10000;
	memset(&mock_event_queue[1], 0, sizeof(XEvent));
	mock_event_queue[1].type = ButtonRelease;

	resizemouse(&(Arg){0});
	ASSERT(c->mon == m2, "resizemouse cross-monitor: client moved to second monitor");
	ASSERT_EQ(selmon, m2, "resizemouse cross-monitor: selection follows client");
	ASSERT_EQ(mock_xungrabpointer_calls, 1, "resizemouse cross-monitor: pointer ungrabbed");

	mock_event_queue_count = 0;
	m1->next = NULL;
	restore_selmon();
	winclient_remove(c);
	free(c); free(m1); free(m2);
}

/* --- resizemouse ConfigureRequest handler in loop (lines 1333-1337) --- */
static void
test_resizemouse_configurerequest(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	drag.phase = DRAG_IDLE;
	pointergrab = 0;
	Monitor *m = make_monitor(0);
	m->wx = 0; m->wy = 0; m->ww = 1920; m->wh = 1080;
	m->showbar = 0;
	Client *c = make_client(1, m);
	c->x = 100; c->y = 100; c->w = 200; c->h = 100; c->bw = 0;
	c->isfloating = 1;
	c->tags = 1;
	m->clients = c;
	m->sel = c;
	m->stack = c;
	m->barwin = 0;

	save_selmon();
	selmon = m;
	mons = m;

	/* Inject ConfigureRequest then ButtonRelease */
	mock_event_queue_count = 2;
	memset(&mock_event_queue[0], 0, sizeof(XEvent));
	mock_event_queue[0].type = ConfigureRequest;
	mock_event_queue[0].xconfigurerequest.window = 9999;
	mock_event_queue[0].xconfigurerequest.value_mask = CWX|CWY;
	mock_event_queue[0].xconfigurerequest.x = 50;
	mock_event_queue[0].xconfigurerequest.y = 50;
	memset(&mock_event_queue[1], 0, sizeof(XEvent));
	mock_event_queue[1].type = ButtonRelease;

	resizemouse(&(Arg){0});
	/* The nested loop dispatches ConfigureRequest to configurerequest(). */
	ASSERT_EQ(mock_xconfigure_window.count, 1, "resizemouse ConfigureRequest: XConfigureWindow dispatched");
	ASSERT_EQ(mock_xconfigure_window.win, (unsigned long)9999, "resizemouse ConfigureRequest: requested window");
	ASSERT_EQ(mock_xconfigure_window.mask, (unsigned long)(CWX|CWY), "resizemouse ConfigureRequest: value mask");
	ASSERT_EQ(mock_xconfigure_last.x, 50, "resizemouse ConfigureRequest: x payload");
	ASSERT_EQ(mock_xconfigure_last.y, 50, "resizemouse ConfigureRequest: y payload");
	ASSERT_EQ(mock_xsync_calls >= 1, 1, "resizemouse ConfigureRequest: XSync performed");
	ASSERT_EQ(mock_xungrabpointer_calls, 1, "resizemouse ConfigureRequest: pointer ungrabbed");

	mock_event_queue_count = 0;
	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- resizemouse MotionNotify throttle (line 1340) --- */
static void
test_resizemouse_throttle(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	drag.phase = DRAG_IDLE;
	pointergrab = 0;
	Monitor *m = make_monitor(0);
	m->wx = 0; m->wy = 0; m->ww = 1920; m->wh = 1080;
	m->showbar = 0;
	Client *c = make_client(1, m);
	c->x = 100; c->y = 100; c->w = 200; c->h = 100; c->bw = 0;
	c->isfloating = 1;
	c->tags = 1;
	m->clients = c;
	m->sel = c;
	m->stack = c;
	m->barwin = 0;

	save_selmon();
	selmon = m;
	mons = m;

	/* Two MotionNotify events with close timestamps → second throttled */
	mock_event_queue_count = 3;
	memset(&mock_event_queue[0], 0, sizeof(XEvent));
	mock_event_queue[0].type = MotionNotify;
	mock_event_queue[0].xmotion.x = 300;
	mock_event_queue[0].xmotion.y = 300;
	mock_event_queue[0].xmotion.time = 10000;
	memset(&mock_event_queue[1], 0, sizeof(XEvent));
	mock_event_queue[1].type = MotionNotify;
	mock_event_queue[1].xmotion.x = 400;
	mock_event_queue[1].xmotion.y = 400;
	mock_event_queue[1].xmotion.time = 10005; /* 5ms < 16ms → throttled */
	memset(&mock_event_queue[2], 0, sizeof(XEvent));
	mock_event_queue[2].type = ButtonRelease;

	resizemouse(&(Arg){0});
	/* Only the first motion is applied: 201x201, not the second's 301x301. */
	ASSERT_EQ(c->w, 201, "resizemouse throttle: first motion applied to width");
	ASSERT_EQ(c->h, 201, "resizemouse throttle: first motion applied to height");
	ASSERT_EQ(mock_xconfigure_window.count, 1, "resizemouse throttle: exactly one resize");
	ASSERT_EQ(mock_xconfigure_last.width, 201, "resizemouse throttle: throttled width recorded");
	ASSERT_EQ(mock_xconfigure_last.height, 201, "resizemouse throttle: throttled height recorded");
	ASSERT_EQ(mock_xungrabpointer_calls, 1, "resizemouse throttle: pointer ungrabbed");

	mock_event_queue_count = 0;
	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

static void
run_tests(void)
{
	test_resizemouse_noop_no_sel();
	test_resizemouse_basic();
	test_resizemouse_fullscreen_early_return();
	test_resizemouse_motion();
	test_resizemouse_motion_togglefloat();
	test_resizemouse_grabpointer_fail();
	test_resizemouse_cross_monitor();
	test_resizemouse_configurerequest();
	test_resizemouse_throttle();
}
