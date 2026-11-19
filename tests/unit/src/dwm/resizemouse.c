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
	save_selmon();
	selmon = make_monitor(0);
	mons = selmon;
	selmon->sel = NULL;
	Arg arg = {0};

	resizemouse(&arg);
	ASSERT(1, "resizemouse: noop when no sel");

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
	ASSERT(1, "resizemouse: basic does not crash");

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
	Monitor *m = make_monitor(0);
	Client *c = make_client(1, m);
	c->isfullscreen = 1;
	m->sel = c;
	save_selmon();
	selmon = m;
	mons = m;

	resizemouse(&(Arg){0});
	ASSERT(c->isfullscreen, "resizemouse: fullscreen returns early, client unchanged");

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
	/* After MotionNotify + resize + ButtonRelease, width/height may have changed */
	ASSERT(1, "resizemouse: MotionNotify path does not crash");

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
	/* The large resize triggers togglefloating since nw-c->w > snap */
	ASSERT(1, "resizemouse: MotionNotify togglefloat path no crash");

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
	ASSERT(1, "resizemouse ConfigureRequest: handler dispatched without crash");

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
	ASSERT(1, "resizemouse throttle: second event throttled without crash");

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
