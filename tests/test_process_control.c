/* Tests for process-control behavior: the killall indirection seam,
 * killatfullscreen STOP/CONT/HUP sequencing, setfullscreen hook wiring,
 * SIGHUP/SIGTERM restart semantics, and getrootptr True/False paths. */
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

/* ------------------------------------------------------------------ */
/* killall recorder                                                     */
/* ------------------------------------------------------------------ */
#define MAX_CALLS 8
static int ncalls;
static const char *call_name[MAX_CALLS];
static const char *call_signal[MAX_CALLS];

static void
record_killall(const char *name, const char *sig)
{
	if (ncalls < MAX_CALLS) {
		call_name[ncalls] = name;
		call_signal[ncalls] = sig;
	}
	ncalls++;
}

static void
install_recorder(void)
{
	ncalls = 0;
	memset(call_name, 0, sizeof call_name);
	memset(call_signal, 0, sizeof call_signal);
	killall_impl = record_killall;
}

static void
restore_killall(void)
{
	killall_impl = killall;
}

/* ------------------------------------------------------------------ */
/* Tests                                                                */
/* ------------------------------------------------------------------ */

/* The seam defaults to the real killall so production behavior is intact */
static void
test_seam_defaults_to_real_killall(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	ASSERT_EQ(killall_impl, killall, "killall_impl defaults to real killall");
}

/* killatfullscreen_stop sends -STOP once per configured target */
static void
test_stop_sends_sigstop_to_each_target(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	install_recorder();
	killatfullscreen_stop();
	restore_killall();

	ASSERT_EQ(ncalls, 1, "one target configured under DWM_TEST");
	ASSERT(strcmp(call_name[0], "test-target") == 0,
	       "target name comes from killatfullscreen[]");
	ASSERT(strcmp(call_signal[0], "-STOP") == 0, "-STOP signal used");
}

/* start body resumes targets (-CONT), waits, then terminates them (-HUP),
 * in that order */
static void
test_start_body_cont_then_hup_in_order(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	install_recorder();
	killatfullscreen_start_body();
	restore_killall();

	ASSERT_EQ(ncalls, 2, "CONT and HUP per target");
	ASSERT(strcmp(call_signal[0], "-CONT") == 0, "first invocation is -CONT");
	ASSERT(strcmp(call_signal[1], "-HUP") == 0, "second invocation is -HUP");
	ASSERT(strcmp(call_name[0], "test-target") == 0, "CONT hits target");
	ASSERT(strcmp(call_name[1], "test-target") == 0, "HUP hits target");
}

/* Entering fullscreen suspends the configured processes */
static void
test_setfullscreen_enter_calls_stop(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Client c;

	install_recorder();
	memset(&c, 0, sizeof c);
	c.win = 42;
	c.mon = selmon;
	c.isfloating = 0;
	c.isfullscreen = 0;
	c.bw = 2;
	c.x = 100; c.y = 200; c.w = 300; c.h = 400;
	c.oldx = 100; c.oldy = 200; c.oldw = 300; c.oldh = 400;

	setfullscreen(&c, 1);
	restore_killall();

	ASSERT_EQ(c.isfullscreen, 1, "client became fullscreen");
	ASSERT_EQ(ncalls, 1, "exactly one stop invocation on enter");
	ASSERT(strcmp(call_signal[0], "-STOP") == 0, "enter sends -STOP");
}

/* Leaving fullscreen runs the CONT → HUP restart sequence synchronously */
static void
test_setfullscreen_exit_calls_start(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Client c;

	install_recorder();
	memset(&c, 0, sizeof c);
	c.win = 42;
	c.mon = selmon;
	c.isfloating = 1;
	c.isfullscreen = 1;
	c.bw = 0;
	c.oldbw = 2;
	c.x = 0; c.y = 0; c.w = 1920; c.h = 1080;
	c.oldx = 100; c.oldy = 200; c.oldw = 300; c.oldh = 400;

	setfullscreen(&c, 0);
	restore_killall();

	ASSERT_EQ(c.isfullscreen, 0, "client left fullscreen");
	ASSERT_EQ(ncalls, 2, "restart sequence ran synchronously");
	ASSERT(strcmp(call_signal[0], "-CONT") == 0, "exit first resumes (-CONT)");
	ASSERT(strcmp(call_signal[1], "-HUP") == 0, "exit then terminates (-HUP)");
}

/* Idempotent setfullscreen(1) must not re-suspend (no duplicate STOP) */
static void
test_setfullscreen_enter_idempotent_no_extra_stop(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	Client c;

	memset(&c, 0, sizeof c);
	c.win = 42;
	c.mon = selmon;
	c.isfloating = 1;
	c.isfullscreen = 1;
	c.bw = 0;

	install_recorder();
	setfullscreen(&c, 1);
	restore_killall();

	ASSERT_EQ(ncalls, 0, "already-fullscreen client triggers nothing");
}

/* Destroying a fullscreen client must resume when it was the last one */
static void
test_unmanage_fullscreen_last_resumes(void)
{
	memset(winhash, 0, sizeof winhash);
	winhash_count = 0;

	/* single fullscreen client on selmon */
	Client *c = ecalloc(1, sizeof(Client));
	c->win = 100;
	c->mon = selmon;
	c->isfullscreen = 1;
	c->bw = 0;
	c->oldbw = 2;
	attach(c);
	attachstack(c);
	selmon->sel = c;
	winclient_put(c);

	/* another non-fullscreen client to keep monitor non-empty */
	Client *other = ecalloc(1, sizeof(Client));
	other->win = 101;
	other->mon = selmon;
	other->isfullscreen = 0;
	attach(other);
	attachstack(other);
	winclient_put(other);

	install_recorder();
	unmanage(c, 1); /* destroyed while fullscreen */
	restore_killall();

	ASSERT_EQ(ncalls, 2, "destroy of last fullscreen triggers CONT+HUP");
	ASSERT(strcmp(call_signal[0], "-CONT") == 0, "destroy resumes (-CONT)");
	ASSERT(strcmp(call_signal[1], "-HUP") == 0, "destroy then HUP");
	/* other remains, correctly not fullscreen */
	ASSERT(!isanyfullscreen(), "no fullscreen remains after destroy");

	/* cleanup */
	unmanage(other, 1);
}

/* Destroying a non-fullscreen client must not resume when a fullscreen remains */
static void
test_unmanage_non_fullscreen_no_resume_when_fullscreen_remains(void)
{
	memset(winhash, 0, sizeof winhash);
	winhash_count = 0;

	Client *fs = ecalloc(1, sizeof(Client));
	fs->win = 200;
	fs->mon = selmon;
	fs->isfullscreen = 1;
	fs->bw = 0;
	attach(fs);
	attachstack(fs);
	winclient_put(fs);

	Client *victim = ecalloc(1, sizeof(Client));
	victim->win = 201;
	victim->mon = selmon;
	victim->isfullscreen = 0;
	attach(victim);
	attachstack(victim);
	winclient_put(victim);

	install_recorder();
	unmanage(victim, 1);
	restore_killall();

	ASSERT_EQ(ncalls, 0, "destroy of non-fullscreen with fullscreen present does not resume");
	ASSERT(isanyfullscreen(), "fullscreen still present");

	/* cleanup - destroy the fullscreen last, should resume */
	install_recorder();
	unmanage(fs, 1);
	restore_killall();
	ASSERT_EQ(ncalls, 2, "destroy of last fullscreen now resumes");
}

/* Multiple monitors: exiting one fullscreen while another remains stays stopped */
static void
test_multi_fullscreen_exit_one_stays_stopped(void)
{
	memset(winhash, 0, sizeof winhash);
	winhash_count = 0;

	Monitor *m2 = createmon();
	m2->num = 1;
	m2->mx = m2->wx = 1920;
	m2->my = m2->wy = 0;
	m2->mw = m2->ww = 1920;
	m2->mh = m2->wh = 1080;
	m2->next = NULL;
	mons->next = m2;

	Client *a = ecalloc(1, sizeof(Client));
	a->win = 300;
	a->mon = selmon;
	a->isfullscreen = 0;
	a->bw = 2;
	attach(a);
	attachstack(a);
	winclient_put(a);

	Client *b = ecalloc(1, sizeof(Client));
	b->win = 301;
	b->mon = m2;
	b->isfullscreen = 0;
	b->bw = 2;
	attach(b);
	attachstack(b);
	winclient_put(b);

	/* enter fullscreen on both */
	install_recorder();
	setfullscreen(a, 1);
	restore_killall();
	ASSERT_EQ(ncalls, 1, "first fullscreen enters -> STOP");

	install_recorder();
	setfullscreen(b, 1);
	restore_killall();
	ASSERT_EQ(ncalls, 0, "second fullscreen enter does not duplicate STOP");
	ASSERT(isanyfullscreen(), "at least one fullscreen after both enters");

	/* exit one - should NOT resume because the other remains */
	install_recorder();
	setfullscreen(a, 0);
	restore_killall();
	ASSERT_EQ(ncalls, 0, "exit of one of two fullscreen stays stopped");
	ASSERT(isanyfullscreen(), "one fullscreen still remains");

	/* exit the last - should resume */
	install_recorder();
	setfullscreen(b, 0);
	restore_killall();
	ASSERT_EQ(ncalls, 2, "exit of last fullscreen resumes");

	/* cleanup monitors */
	mons->next = NULL;
	/* a and b already detached via setfullscreen exit? a was removed from fullscreen but still attached;
	 * free manually for test isolation */
	detach(a); detachstack(a); winclient_remove(a); free(a);
	detach(b); detachstack(b); winclient_remove(b); free(b);
	free(m2);
}

/* isanyfullscreen reflects global state */
static void
test_isanyfullscreen_global(void)
{
	memset(winhash, 0, sizeof winhash);
	winhash_count = 0;

	ASSERT(!isanyfullscreen(), "no fullscreen initially");

	Client *c = ecalloc(1, sizeof(Client));
	c->win = 400;
	c->mon = selmon;
	c->isfullscreen = 1;
	attach(c);
	attachstack(c);
	winclient_put(c);

	ASSERT(isanyfullscreen(), "one fullscreen -> true");

	c->isfullscreen = 0;
	ASSERT(!isanyfullscreen(), "cleared flag -> false again");

	detach(c); detachstack(c); winclient_remove(c); free(c);
}

/* SIGHUP requests a restart: quit({1}) sets restart=1 and stops running */
static void
test_sighup_sets_restart_flag(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	running = 1;
	restart = 0;
	sighup(0);
	ASSERT_EQ(running, 0, "sighup stops the event loop");
	ASSERT_EQ(restart, 1, "sighup requests restart via execvp");
}

/* SIGTERM exits without restart: quit({0}) leaves restart untouched */
static void
test_sigterm_does_not_restart(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	running = 1;
	restart = 0;
	sigterm(0);
	ASSERT_EQ(running, 0, "sigterm stops the event loop");
	ASSERT_EQ(restart, 0, "sigterm does not request restart");
}

/* getrootptr returns True and writes coordinates when X reports pointer
 * inside root */
static void
test_getrootptr_true_returns_coords(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	int x = -1, y = -1, ret;

	mock_querypointer_return = 1;
	mock_querypointer_root_x = 777;
	mock_querypointer_root_y = 333;
	ret = getrootptr(&x, &y);
	ASSERT_EQ(ret, 1, "returns True when XQueryPointer succeeds");
	ASSERT_EQ(x, 777, "x receives root_x from XQueryPointer");
	ASSERT_EQ(y, 333, "y receives root_y from XQueryPointer");
}

/* getrootptr returns False when the query fails; output coords are
 * undefined by X (the mock zeroes them), so only the return value
 * and caller-side early-return behavior are contract here */
static void
test_getrootptr_false_on_query_failure(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	int x = -12345, y = -67890, ret;

	mock_querypointer_return = 0;
	ret = getrootptr(&x, &y);
	ASSERT_EQ(ret, 0, "returns False when XQueryPointer fails");
	mock_querypointer_return = 1;
}

/* ------------------------------------------------------------------ */
/* Main                                                                  */
/* ------------------------------------------------------------------ */
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
	selmon->tagset[0] = selmon->tagset[1] = 1;
	selmon->mfact = 0.55f;
	selmon->nmaster = 1;
	selmon->showbar = 0;
	selmon->topbar = 1;
	selmon->mx = selmon->wx = 0;
	selmon->my = selmon->wy = 20;
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

	test_seam_defaults_to_real_killall();
	test_stop_sends_sigstop_to_each_target();
	test_start_body_cont_then_hup_in_order();
	test_setfullscreen_enter_calls_stop();
	test_setfullscreen_exit_calls_start();
	test_setfullscreen_enter_idempotent_no_extra_stop();
	test_unmanage_fullscreen_last_resumes();
	test_unmanage_non_fullscreen_no_resume_when_fullscreen_remains();
	test_multi_fullscreen_exit_one_stays_stopped();
	test_isanyfullscreen_global();
	test_sighup_sets_restart_flag();
	test_sigterm_does_not_restart();
	test_getrootptr_true_returns_coords();
	test_getrootptr_false_on_query_failure();

	printf("=== RESULTS ===\n");
	printf("Total: %d | Passed: %d | Failed: %d\n", total, total - failed, failed);

	free(selmon);
	return failed ? 1 : 0;
}
