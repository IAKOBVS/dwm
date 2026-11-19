/*
 * test_updatenumlockmask.c — per-function tests for dwm.c's updatenumlockmask().
 *
 * The mock modifier map places the Num_Lock keycode in slot 2, so the scan must
 * yield (1 << 2); without that keycode numlockmask must be cleared to zero.
 * Repeated scans exercise the XFreeModifiermap() path; the ASan run is what
 * proves the mapping is actually released every time.
 */
#include "test_common.h"

/* --- no Num_Lock keycode in the modifier map --- */
static void
test_updatenumlockmask(void)
{
	reset_index();
	mock_x11_reset(); /* mock_modmap_has_numlock = 0 */
	numlockmask = 0xdead; /* prove the scan resets it rather than keeping it */

	updatenumlockmask();

	ASSERT_EQ(numlockmask, 0,
		"updatenumlockmask: no Num_Lock keycode clears numlockmask");

	mock_x11_reset();
}

/* --- Num_Lock found in the mock modifier map --- */
static void
test_updatenumlockmask_numlock_found(void)
{
	reset_index();
	mock_x11_reset();
	mock_modmap_has_numlock = 1;
	numlockmask = 0;

	updatenumlockmask();

	ASSERT(numlockmask != 0,
		"updatenumlockmask: numlockmask set when Num_Lock is in the modmap");
	/* The mock stores the Num_Lock keycode at modifiermap[2], so the loop
	 * records (1 << 2) for that slot. */
	ASSERT_EQ(numlockmask, (1u << 2),
		"updatenumlockmask: numlockmask matches the Num_Lock modifier slot");
	ASSERT_EQ(numlockmask, 4u,
		"updatenumlockmask: the mock slot yields the expected numeric mask");

	mock_modmap_has_numlock = 0;
	mock_x11_reset();
}

/* --- repeated scans free each mapping (checked by ASan) --- */
static void
test_updatenumlockmask_repeated(void)
{
	int i;

	reset_index();
	mock_x11_reset();
	mock_modmap_has_numlock = 1;
	numlockmask = 0;

	for (i = 0; i < 32; i++)
		updatenumlockmask();

	ASSERT_EQ(numlockmask, (1u << 2),
		"updatenumlockmask: repeated scans leave a stable mask");

	mock_modmap_has_numlock = 0;
	mock_x11_reset();
}

static void
run_tests(void)
{
	test_updatenumlockmask();
	test_updatenumlockmask_numlock_found();
	test_updatenumlockmask_repeated();
}
