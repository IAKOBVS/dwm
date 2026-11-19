/*
 * test_spawn.c — per-function tests for dwm.c's spawn().
 *
 * Migrated from the monolithic test_comprehensive.c suite.  Each case
 * below exercises one path through spawn(); the fixture (global setup,
 * make_monitor/make_client, window-index reset) lives in test_common.h.
 */
#include "test_common.h"

/* --- spawn --- */
static void
test_spawn(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	/* We can't easily test execvp, but we can verify it doesn't crash
	 * in the parent process. The mock X11 doesn't have a real display,
	 * so fork will still work. */
	lifecycle = LIFE_QUIT;
	Arg arg = { .v = (void *)(char *[]){(char *)"true", NULL} };
	spawn(&arg);
	ASSERT(1, "spawn: does not crash in parent");
}

/* --- spawn with NULL arg (lines 1720-1727) --- */
static void
test_spawn_null_arg(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	selmon->num = 0;

	Arg arg = { .v = NULL };
	spawn(&arg);

	/* Lines 1720-1721: arg->v != dmenucmd (NULL != dmenucmd)
	 * Lines 1722-1727: fork() creates child, child crashes on NULL deref,
	 * but parent continues without issue. */
	ASSERT(1, "spawn: NULL arg does not crash parent");

	restore_selmon();
}

/* --- spawn with mock_fork child path (lines 1722-1728) --- */
static void
test_spawn_mock_fork_child(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	mock_fork_return = 0;  /* child process */
	mock_die_abort = 1;    /* prevent DIE from calling exit */

	/* Use non-existent binary so execvp fails → DIE is called */
	static char *argv[] = { "/nonexistent_binary_xyz", NULL };
	Arg arg = { .v = argv };

	spawn(&arg);
	/*
	 * mock_fork_return=0 means fork() returns 0 in the test process.
	 * The test process runs child code: close(), setsid(), execvp() fails
	 * (binary doesn't exist), DIE() is called but intercepted by mock.
	 * After DIE returns, the test process continues past spawn().
	 */
	ASSERT(mock_die_abort == 2, "spawn child: DIE called after execvp failure");

	mock_fork_return = -1;
	mock_die_abort = 0;
	mock_x11_reset();
}

/* --- spawn mock fork parent path (lines 1720-1728) --- */
static void
test_spawn_mock_fork_parent(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	mock_x11_reset();
	mock_fork_return = 1234;  /* pretend parent; fork returns child PID */
	/* In parent: fork() returns 1234 != 0, so child block is skipped */
	/* But arg->v is still dereferenced in dmenucmd check at line 1720 */
	/* dmenucmd is config.h defined; arg->v != dmenucmd, so skip */

	static char *argv[] = { "/bin/true", NULL };
	Arg arg = { .v = argv };
	spawn(&arg);

	/* fork() returned 1234 (non-zero), so we're in the parent path */
	ASSERT(1, "spawn mock fork parent: does not crash");

	mock_fork_return = -1;
	mock_x11_reset();
}

/* --- spawn with dmenucmd (line 1721) --- */
static void
test_spawn_dmenucmd(void)
{
	memset(winhash, 0, sizeof winhash); /* isolate window index per test */
	winhash_count = 0;
	save_selmon();
	Monitor *m = make_monitor(0);
	selmon = m;
	mons = m;

	mock_fork_return = 1234;  /* parent path, skip child block */

	/* arg->v == dmenucmd triggers line 1721 */
	Arg arg = { .v = dmenucmd };
	spawn(&arg);

	ASSERT(dmenumon[0] == '0' + m->num, "spawn dmenucmd: dmenumon set from selmon->num");

	mock_fork_return = -1;
	mock_x11_reset();
	restore_selmon();
	free(m);
}

static void
run_tests(void)
{
	test_spawn();
	test_spawn_null_arg();
	test_spawn_mock_fork_child();
	test_spawn_mock_fork_parent();
	test_spawn_dmenucmd();
}
