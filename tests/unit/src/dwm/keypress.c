/*
 * test_keypress.c — per-function tests for dwm.c's keypress().
 *
 * The mock maps a keycode straight to a keysym, so a test can drive a real
 * configured binding (MODKEY+b toggles the bar) and observe the action instead
 * of merely checking that keypress() survives.  Unbound keysyms must return
 * before touching the cached fast-path masks, and CLEANMASK must strip lock
 * modifiers (NumLock/Lock) before matching.
 */
#include "test_common.h"

/* --- keypress unmapped (early return check) --- */
static void
test_keypress_unmapped(void)
{
	reset_index();
	mock_x11_reset();
	Monitor *m = make_monitor(0);
	m->barwin = 500;
	m->showbar = 0;
	save_selmon();
	selmon = m;
	mons = m;

	cachekeys();
	KeySym keysym_before = key_keysym_used;
	unsigned int mod_before = key_mod_used;

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xkey.keycode = XK_z; /* not present in keys[] */
	ev.xkey.state = MODKEY;

	keypress(&ev);

	ASSERT_EQ(m->showbar, 0,
		"keypress: unmapped keysym invokes no binding");
	ASSERT_EQ(key_keysym_used, keysym_before,
		"keypress: unmapped keysym leaves the keysym fast-path mask untouched");
	ASSERT_EQ(key_mod_used, mod_before,
		"keypress: unmapped keysym leaves the modifier fast-path mask untouched");
	ASSERT_EQ(mock_xmoveresize_window.count, 0,
		"keypress: unmapped keysym performs no action side effects");

	restore_selmon();
	free(m);
}

/* --- keypress matched (lines 972-973) --- */
static void
test_keypress_matched(void)
{
	reset_index();
	mock_x11_reset();
	Monitor *m = make_monitor(0);
	m->barwin = 500;
	m->showbar = 0;
	save_selmon();
	selmon = m;
	mons = m;

	cachekeys();

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	/* mock XKeycodeToKeysym returns the keycode as the keysym */
	ev.xkey.keycode = XK_b; /* MODKEY+b -> togglebar */
	ev.xkey.state = MODKEY;

	keypress(&ev);

	ASSERT_EQ(m->showbar, 1,
		"keypress: MODKEY+b invokes togglebar");
	ASSERT(key_keysym_used & XK_b,
		"keypress: cached fast path includes the matched keysym");
	ASSERT(key_mod_used & MODKEY,
		"keypress: cached fast path includes the matched modifier");
	ASSERT_EQ(mock_xmoveresize_window.count, 1,
		"keypress: togglebar repositions the bar exactly once");
	ASSERT_EQ(mock_xmoveresize_window.win, m->barwin,
		"keypress: togglebar repositions the monitor bar window");

	restore_selmon();
	free(m);
}

/* --- NumLock/Lock are stripped before matching --- */
static void
test_keypress_numlock_cleanmask(void)
{
	reset_index();
	mock_x11_reset();
	Monitor *m = make_monitor(0);
	m->barwin = 500;
	m->showbar = 0;
	save_selmon();
	selmon = m;
	mons = m;

	unsigned int saved_numlockmask = numlockmask;
	numlockmask = Mod2Mask;
	cachekeys();

	XEvent ev;
	memset(&ev, 0, sizeof ev);
	ev.xkey.keycode = XK_b;
	ev.xkey.state = MODKEY | Mod2Mask | LockMask;

	keypress(&ev);

	ASSERT_EQ(m->showbar, 1,
		"keypress: NumLock/Lock variants still match the binding");
	ASSERT_EQ(mock_xmoveresize_window.count, 1,
		"keypress: clean-mask match invokes togglebar once");

	numlockmask = saved_numlockmask;
	restore_selmon();
	free(m);
}

static void
run_tests(void)
{
	test_keypress_unmapped();
	test_keypress_matched();
	test_keypress_numlock_cleanmask();
}
