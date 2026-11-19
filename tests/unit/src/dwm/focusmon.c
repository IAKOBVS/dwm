/*
 * test_focusmon.c — per-function tests for dwm.c's focusmon().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through focusmon(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- focusmon --- */
static void
test_focusmon_noop_single_monitor(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	selmon = make_monitor(0);
	mons = selmon;
	Arg arg = {.i = 1};

	focusmon(&arg);
	ASSERT(1, "focusmon: single monitor no crash");

{
		Monitor *tmpm = selmon; /* free the fixture, then restore globals */
		restore_selmon();
		free(tmpm);
	}
}

static void
test_focusmon_switches_monitor(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m1 = make_monitor(0);
	Monitor *m2 = make_monitor(1);
	m1->stack = NULL;
	m2->stack = NULL;
	m1->next = m2;
	mons = m1;
	save_selmon();
	selmon = m1;
	Arg arg = {.i = 1};

	focusmon(&arg);
	ASSERT(selmon == m2, "focusmon: switches to next monitor");

	restore_selmon();
	free(m1); /* fixture leaked (ASan-reported) */
	free(m2); /* fixture leaked (ASan-reported) */
}

static void
test_focusmon_switches_focus(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m1 = make_monitor(0);
	Monitor *m2 = make_monitor(1);
	Client *c = make_client(100, m2);
	m2->clients = c;
	m2->stack = c;
	m1->next = m2;
	mons = m1;
	save_selmon();
	selmon = m1;
	m1->sel = NULL;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xbutton.window = 100; /* client window on m2 */
	ev.xbutton.state = 0;
	ev.xbutton.button = Button1;

	cachebuttons();
	buttonpress(&ev);
	/* wintomon(100) returns m2 which != m1, so lines 286-288 execute */
	ASSERT(selmon == m2, "focusmon: buttonpress switches to owning monitor");

	restore_selmon();
	winclient_remove(c);
	free(c); free(m1); free(m2);
}

/* --- focusmon --- */
static void
test_focusmon_switches(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m1 = make_monitor(0);
	Monitor *m2 = make_monitor(1);
	m1->sel = NULL;
	m2->sel = NULL;
	m1->stack = NULL;
	m2->stack = NULL;
	m1->next = m2;
	mons = m1;
	save_selmon();
	selmon = m1;
	Arg arg = { .i = 1 };

	focusmon(&arg);
	ASSERT(selmon == m2, "focusmon: switches to next monitor");

	focusmon(&arg);
	ASSERT(selmon == m1, "focusmon: wraps around to first monitor");

	restore_selmon();
	free(m1); /* fixture leaked (ASan-reported) */
	free(m2); /* fixture leaked (ASan-reported) */
}

/* --- focusmon prev (line 601) --- */
static void
test_focusmon_prev(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m1 = make_monitor(0);
	Monitor *m2 = make_monitor(1);
	m1->sel = NULL;
	m2->sel = NULL;
	m1->stack = NULL;
	m2->stack = NULL;
	m1->next = m2;
	mons = m1;
	save_selmon();
	selmon = m2;
	Arg arg = { .i = -1 };

	focusmon(&arg);
	ASSERT(selmon == m1, "focusmon prev: switches to previous monitor");

	restore_selmon();
	free(m1); /* fixture leaked (ASan-reported) */
	free(m2); /* fixture leaked (ASan-reported) */
}

/* --- focusmon noop when dirtomon returns selmon (line 769) --- */
static void
test_focusmon_noop_same_dirtomon(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Monitor *m = make_monitor(0);
	/* Create a circular self-loop so mons->next != NULL but dirtomon(1) == selmon */
	m->next = m;
	m->sel = NULL;
	m->stack = NULL;
	mons = m;
	save_selmon();
	selmon = m;
	Arg arg = { .i = 1 };

	/* mons->next = m != NULL, so line 766 passes.
	 * dirtomon(1): m = selmon->next = m = selmon, so line 768-769 triggers return */
	focusmon(&arg);
	/* selmon unchanged because dirtomon returned selmon */
	ASSERT(selmon == m, "focusmon noop: selmon unchanged when dirtomon == selmon");

	/* Break circular list before free */
	m->next = NULL;
	restore_selmon();
	free(m);
}

static void
run_tests(void)
{
	test_focusmon_noop_single_monitor();
	test_focusmon_switches_monitor();
	test_focusmon_switches_focus();
	test_focusmon_switches();
	test_focusmon_prev();
	test_focusmon_noop_same_dirtomon();
}
