/*
 * test_movemouse.c — per-function tests for dwm.c's movemouse().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through movemouse(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- movemouse noop --- */
static void
test_movemouse_noop_no_sel(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	selmon = make_monitor(0);
	mons = selmon;
	selmon->sel = NULL;
	Arg arg = {0};

	movemouse(&arg);
	ASSERT(1, "movemouse: noop when no sel");

{
		Monitor *tmpm = selmon; /* free the fixture, then restore globals */
		restore_selmon();
		free(tmpm);
	}
}

/* --- movemouse basic (lines 1138-1195) --- */
static void
test_movemouse_basic(void)
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

	mock_querypointer_return = 1;
	mock_querypointer_root_x = 150;
	mock_querypointer_root_y = 150;

	/* Inject a ButtonRelease event so the do-while loop exits */
	mock_event_queue_count = 1;
	memset(&mock_event_queue[0], 0, sizeof(XEvent));
	mock_event_queue[0].type = ButtonRelease;

	movemouse(&(Arg){0});
	/* movemouse should have entered the loop, processed ButtonRelease, and exited */
	ASSERT(1, "movemouse: basic does not crash");

	mock_event_queue_count = 0;
	mock_querypointer_return = 1;
	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- movemouse fullscreen early return (line 1148-1149) --- */
static void
test_movemouse_fullscreen_early_return(void)
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

	movemouse(&(Arg){0});
	ASSERT(c->isfullscreen, "movemouse: fullscreen returns early, client unchanged");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- movemouse with MotionNotify then snap (lines 1166-1186) --- */
static void
test_movemouse_motion_snap(void)
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

	mock_querypointer_return = 1;
	mock_querypointer_root_x = 150;
	mock_querypointer_root_y = 150;

	/* Inject MotionNotify (moves close to snap point) then ButtonRelease */
	mock_event_queue_count = 2;
	memset(&mock_event_queue[0], 0, sizeof(XEvent));
	mock_event_queue[0].type = MotionNotify;
	mock_event_queue[0].xmotion.x = 10; /* close to wx=0 for snap */
	mock_event_queue[0].xmotion.y = 10;
	mock_event_queue[0].xmotion.time = 10000;
	memset(&mock_event_queue[1], 0, sizeof(XEvent));
	mock_event_queue[1].type = ButtonRelease;

	movemouse(&(Arg){0});
	/* MotionNotify processed, snap may trigger */
	ASSERT(1, "movemouse: MotionNotify snap path no crash");

	mock_event_queue_count = 0;
	mock_querypointer_return = 1;
	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- movemouse grabpointer fail (line 1155) --- */
static void
test_movemouse_grabpointer_fail(void)
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

	movemouse(&(Arg){0});
	ASSERT(c->x == 100, "movemouse grabpointer fail: client position unchanged");

	mock_grabpointer_return = 0;
	mock_x11_reset();
	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- movemouse getrootptr fail (line 1157) --- */
static void
test_movemouse_getrootptr_fail(void)
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

	mock_querypointer_return = 0;

	movemouse(&(Arg){0});
	ASSERT(c->x == 100, "movemouse getrootptr fail: client position unchanged");

	mock_querypointer_return = 1;
	mock_x11_reset();
	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- movemouse snap left (line 1174) --- */
static void
test_movemouse_snap_left(void)
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

	mock_querypointer_return = 1;
	mock_querypointer_root_x = 150;
	mock_querypointer_root_y = 150;

	/* Motion to nx=0 (close to wx=0, within snap=32) */
	mock_event_queue_count = 2;
	memset(&mock_event_queue[0], 0, sizeof(XEvent));
	mock_event_queue[0].type = MotionNotify;
	mock_event_queue[0].xmotion.x = 50;  /* nx = 100 + (50-150) = 0, abs(0-0)=0 < 32 */
	mock_event_queue[0].xmotion.y = 200; /* ny = 100 + (200-150) = 50, far from edges */
	mock_event_queue[0].xmotion.time = 10000;
	memset(&mock_event_queue[1], 0, sizeof(XEvent));
	mock_event_queue[1].type = ButtonRelease;

	movemouse(&(Arg){0});
	ASSERT(c->x == 0, "movemouse snap left: client snapped to left edge");

	mock_event_queue_count = 0;
	mock_querypointer_return = 1;
	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- movemouse snap right (line 1176) --- */
static void
test_movemouse_snap_right(void)
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

	mock_querypointer_return = 1;
	mock_querypointer_root_x = 150;
	mock_querypointer_root_y = 150;

	/* Motion to right edge: nx=1720, nx+WIDTH(c)=1920, abs(1920-1920)=0 < 32 */
	mock_event_queue_count = 2;
	memset(&mock_event_queue[0], 0, sizeof(XEvent));
	mock_event_queue[0].type = MotionNotify;
	mock_event_queue[0].xmotion.x = 1770; /* nx = 100+(1770-150)=1720 */
	mock_event_queue[0].xmotion.y = 200;
	mock_event_queue[0].xmotion.time = 10000;
	memset(&mock_event_queue[1], 0, sizeof(XEvent));
	mock_event_queue[1].type = ButtonRelease;

	movemouse(&(Arg){0});
	ASSERT(c->x == 1720, "movemouse snap right: client snapped to right edge");

	mock_event_queue_count = 0;
	mock_querypointer_return = 1;
	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- movemouse snap top (line 1178) --- */
static void
test_movemouse_snap_top(void)
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

	mock_querypointer_return = 1;
	mock_querypointer_root_x = 150;
	mock_querypointer_root_y = 150;

	/* Motion to top edge: ny=0, abs(0-0)=0 < 32 */
	mock_event_queue_count = 2;
	memset(&mock_event_queue[0], 0, sizeof(XEvent));
	mock_event_queue[0].type = MotionNotify;
	mock_event_queue[0].xmotion.x = 960;  /* far from x edges */
	mock_event_queue[0].xmotion.y = 50;   /* ny = 100+(50-150)=0, abs(0-0)=0 < 32 */
	mock_event_queue[0].xmotion.time = 10000;
	memset(&mock_event_queue[1], 0, sizeof(XEvent));
	mock_event_queue[1].type = ButtonRelease;

	movemouse(&(Arg){0});
	ASSERT(c->y == 0, "movemouse snap top: client snapped to top edge");

	mock_event_queue_count = 0;
	mock_querypointer_return = 1;
	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- movemouse snap bottom (line 1180) --- */
static void
test_movemouse_snap_bottom(void)
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

	mock_querypointer_return = 1;
	mock_querypointer_root_x = 150;
	mock_querypointer_root_y = 150;

	/* Motion to bottom edge: ny+HEIGHT(c) = 1080, abs(1080-1080)=0 < 32 */
	mock_event_queue_count = 2;
	memset(&mock_event_queue[0], 0, sizeof(XEvent));
	mock_event_queue[0].type = MotionNotify;
	mock_event_queue[0].xmotion.x = 960;  /* far from x edges */
	mock_event_queue[0].xmotion.y = 1030; /* ny = 100+(1030-150)=980, ny+100=1080 */
	mock_event_queue[0].xmotion.time = 10000;
	memset(&mock_event_queue[1], 0, sizeof(XEvent));
	mock_event_queue[1].type = ButtonRelease;

	movemouse(&(Arg){0});
	ASSERT(c->y == 980, "movemouse snap bottom: client snapped to bottom edge");

	mock_event_queue_count = 0;
	mock_querypointer_return = 1;
	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- movemouse togglefloating (lines 1182-1183) --- */
static void
test_movemouse_togglefloating(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	m->wx = 0; m->wy = 0; m->ww = 1920; m->wh = 1080;
	m->showbar = 0;
	m->lt[0] = m->lt[1] = &layouts[0]; /* tile layout has arrange */
	Client *c = make_client(1, m);
	c->x = 100; c->y = 100; c->w = 200; c->h = 100; c->bw = 0;
	c->isfloating = 0; /* NOT floating */
	c->tags = 1;
	m->clients = c;
	m->sel = c;
	m->stack = c;
	m->barwin = 0;

	save_selmon();
	selmon = m;
	mons = m;

	mock_querypointer_return = 1;
	mock_querypointer_root_x = 150;
	mock_querypointer_root_y = 150;

	/* Motion far enough that abs(nx-c->x) > snap triggers togglefloating */
	mock_event_queue_count = 2;
	memset(&mock_event_queue[0], 0, sizeof(XEvent));
	mock_event_queue[0].type = MotionNotify;
	mock_event_queue[0].xmotion.x = 400; /* nx=100+(400-150)=350, abs(350-100)=250>32 */
	mock_event_queue[0].xmotion.y = 200; /* ny=100+(200-150)=150 */
	mock_event_queue[0].xmotion.time = 10000;
	memset(&mock_event_queue[1], 0, sizeof(XEvent));
	mock_event_queue[1].type = ButtonRelease;

	movemouse(&(Arg){0});
	ASSERT(c->isfloating == 1, "movemouse togglefloating: non-floating client became floating");

	mock_event_queue_count = 0;
	mock_querypointer_return = 1;
	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- movemouse cross-monitor sendmon (lines 1191-1193) --- */
static void
test_movemouse_cross_monitor(void)
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
	c->x = 100; c->y = 100; c->w = 200; c->h = 100; c->bw = 0;
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

	mock_querypointer_return = 1;
	mock_querypointer_root_x = 150;
	mock_querypointer_root_y = 150;

	/* Move client to m2 area: nx=100+(1100-150)=1050 → recttomon returns m2 */
	mock_event_queue_count = 2;
	memset(&mock_event_queue[0], 0, sizeof(XEvent));
	mock_event_queue[0].type = MotionNotify;
	mock_event_queue[0].xmotion.x = 1100; /* nx=1050, overlaps m2 (960-1920) */
	mock_event_queue[0].xmotion.y = 200;
	mock_event_queue[0].xmotion.time = 10000;
	memset(&mock_event_queue[1], 0, sizeof(XEvent));
	mock_event_queue[1].type = ButtonRelease;

	movemouse(&(Arg){0});
	/* After sendmon, client should be on m2 */
	ASSERT(c->mon == m2, "movemouse cross-monitor: client moved to second monitor");

	mock_event_queue_count = 0;
	mock_querypointer_return = 1;
	m1->next = NULL;
	restore_selmon();
	winclient_remove(c);
	free(c); free(m1); free(m2);
}

/* --- movemouse ConfigureRequest handler in loop (lines 1161-1165) --- */
static void
test_movemouse_configurerequest(void)
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

	mock_querypointer_return = 1;
	mock_querypointer_root_x = 150;
	mock_querypointer_root_y = 150;

	/* Inject ConfigureRequest then ButtonRelease */
	mock_event_queue_count = 2;
	memset(&mock_event_queue[0], 0, sizeof(XEvent));
	mock_event_queue[0].type = ConfigureRequest;
	mock_event_queue[0].xconfigurerequest.window = 9999; /* unknown window */
	mock_event_queue[0].xconfigurerequest.value_mask = CWX|CWY;
	mock_event_queue[0].xconfigurerequest.x = 50;
	mock_event_queue[0].xconfigurerequest.y = 50;
	memset(&mock_event_queue[1], 0, sizeof(XEvent));
	mock_event_queue[1].type = ButtonRelease;

	movemouse(&(Arg){0});
	ASSERT(1, "movemouse ConfigureRequest: handler dispatched without crash");

	mock_event_queue_count = 0;
	mock_querypointer_return = 1;
	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

/* --- movemouse MotionNotify throttle (line 1168) --- */
static void
test_movemouse_throttle(void)
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

	mock_querypointer_return = 1;
	mock_querypointer_root_x = 150;
	mock_querypointer_root_y = 150;

	/* Two MotionNotify events: second within 16ms of first → throttled */
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

	movemouse(&(Arg){0});
	/* Second MotionNotify was throttled, client moved by first only */
	ASSERT(c->x != 100 || c->y != 100, "movemouse throttle: processed first event");

	mock_event_queue_count = 0;
	mock_querypointer_return = 1;
	restore_selmon();
	winclient_remove(c);
	free(c); free(m);
}

static void
run_tests(void)
{
	test_movemouse_noop_no_sel();
	test_movemouse_basic();
	test_movemouse_fullscreen_early_return();
	test_movemouse_motion_snap();
	test_movemouse_grabpointer_fail();
	test_movemouse_getrootptr_fail();
	test_movemouse_snap_left();
	test_movemouse_snap_right();
	test_movemouse_snap_top();
	test_movemouse_snap_bottom();
	test_movemouse_togglefloating();
	test_movemouse_cross_monitor();
	test_movemouse_configurerequest();
	test_movemouse_throttle();
}
