/*
 * test_isuniquegeom.c — per-function tests for dwm.c's isuniquegeom().
 *
 * isuniquegeom() is compiled only under -DXINERAMA, so this suite enables
 * the macro before including the shared fixture and supplies controllable
 * Xinerama entry points for dwm.c.  It covers the primary path (new
 * geometry), the edge cases (duplicate, empty list, first/last positions),
 * and drives updategeom() through its Xinerama add/update/remove branches
 * so the only caller of isuniquegeom() is exercised end to end.
 */
#define XINERAMA 1

#include "test_common.h"

static int xinerama_active;
static int xinerama_nscreens;
static XineramaScreenInfo xinerama_screens[8];

Bool
XineramaIsActive(Display *dpy)
{
	(void)dpy;
	return xinerama_active ? True : False;
}

XineramaScreenInfo *
XineramaQueryScreens(Display *dpy, int *number)
{
	XineramaScreenInfo *screens = NULL;

	(void)dpy;
	*number = xinerama_nscreens;
	if (xinerama_nscreens > 0) {
		screens = malloc((size_t)xinerama_nscreens * sizeof *screens);
		memcpy(screens, xinerama_screens,
			(size_t)xinerama_nscreens * sizeof *screens);
	}
	return screens;
}

static void
test_isuniquegeom_new(void)
{
	XineramaScreenInfo unique[4];
	XineramaScreenInfo info = { 0, 0, 0, 1920, 1080 };

	ASSERT_EQ(isuniquegeom(unique, 0, &info), 1,
		"isuniquegeom: empty list always reports a new geometry");
}

static void
test_isuniquegeom_duplicate(void)
{
	XineramaScreenInfo unique[4];
	XineramaScreenInfo info = { 0, 0, 0, 1920, 1080 };

	unique[0] = info;
	ASSERT_EQ(isuniquegeom(unique, 1, &info), 0,
		"isuniquegeom: identical geometry is rejected");
}

static void
test_isuniquegeom_differing_field(void)
{
	XineramaScreenInfo unique[4];
	XineramaScreenInfo info = { 0, 0, 0, 1920, 1080 };

	unique[0] = info;
	unique[0].x_org = 1;
	ASSERT_EQ(isuniquegeom(unique, 1, &info), 1,
		"isuniquegeom: differing x_org makes the geometry unique");
	unique[0] = info;
	unique[0].height = 1079;
	ASSERT_EQ(isuniquegeom(unique, 1, &info), 1,
		"isuniquegeom: differing height makes the geometry unique");
}

static void
test_isuniquegeom_scans_all_entries(void)
{
	XineramaScreenInfo unique[4] = {
		{ 0, 0, 0, 1920, 1080 },
		{ 1, 1920, 0, 1280, 1024 },
		{ 2, 0, 1080, 800, 600 },
	};
	XineramaScreenInfo match = { 9, 0, 1080, 800, 600 };
	XineramaScreenInfo miss = { 0, 100, 100, 640, 480 };

	ASSERT_EQ(isuniquegeom(unique, 3, &match), 0,
		"isuniquegeom: finds a match in the last entry");
	ASSERT_EQ(isuniquegeom(unique, 3, &miss), 1,
		"isuniquegeom: reports a miss when no entry matches");
}

/* updategeom() adds monitors for new unique screens, updates cloned
 * geometry, then removes surplus monitors and migrates their clients. */
static void
test_updategeom_xinerama_add_update_remove(void)
{
	Monitor *m;
	Client *c;

	xinerama_active = 1;
	xinerama_nscreens = 3;
	xinerama_screens[0] = (XineramaScreenInfo){ 0, 0, 0, 1920, 1080 };
	xinerama_screens[1] = (XineramaScreenInfo){ 1, 1920, 0, 1280, 1024 };
	xinerama_screens[2] = (XineramaScreenInfo){ 2, 0, 1080, 800, 600 };

	ASSERT_EQ(updategeom(), 1, "updategeom: new screens mark dirty");
	ASSERT(mons && mons->next && mons->next->next,
		"updategeom: a monitor per unique screen");
	ASSERT_EQ(mons->next->mx, 1920, "updategeom: second screen x_org");
	ASSERT_EQ(mons->next->next->mh, 600, "updategeom: third screen height");

	/* Pin a client to the last monitor so the removal loop has work. */
	for (m = mons; m->next; m = m->next);
	c = make_client(77, m);
	attach(c);
	attachstack(c);
	selmon = m;

	xinerama_nscreens = 1;
	ASSERT_EQ(updategeom(), 1,
		"updategeom: removing screens migrates clients and dirties");
	ASSERT_EQ(mons->next, (Monitor *)0, "updategeom: surplus monitors freed");
	ASSERT(c->mon == mons, "updategeom: client migrated to first monitor");
	ASSERT(selmon == mons, "updategeom: selection follows the first monitor");

	drain_clients();
}

/* No monitors yet: the first unique Xinerama screen creates mons directly. */
static void
test_updategeom_xinerama_from_empty(void)
{
	free_global_state();
	reset_index();

	xinerama_active = 1;
	xinerama_nscreens = 1;
	xinerama_screens[0] = (XineramaScreenInfo){ 0, 0, 0, 1024, 768 };

	ASSERT_EQ(updategeom(), 1, "updategeom: empty list creates a monitor");
	ASSERT(mons != NULL && mons->next == NULL,
		"updategeom: exactly one monitor for one unique screen");
	ASSERT_EQ(mons->mw, 1024, "updategeom: created monitor width");
	ASSERT_EQ(mons->mh, 768, "updategeom: created monitor height");
}

static void
run_tests(void)
{
	test_isuniquegeom_new();
	test_isuniquegeom_duplicate();
	test_isuniquegeom_differing_field();
	test_isuniquegeom_scans_all_entries();
	test_updategeom_xinerama_add_update_remove();
	/* Frees the shared globals, so it must run last. */
	test_updategeom_xinerama_from_empty();
}
