/*
 * test_run.c — per-function tests for dwm.c's run().
 *
 * Verifies the handler table and that run() actually dispatches injected
 * events, including the event-loop tail flush side effects.
 */
#include "test_common.h"

/* --- handler table maps supported types and leaves others NULL --- */
static void
test_handler_table(void)
{
	ASSERT_EQ(handler[ButtonPress], buttonpress, "handler: ButtonPress");
	ASSERT_EQ(handler[ClientMessage], clientmessage, "handler: ClientMessage");
	ASSERT_EQ(handler[ConfigureRequest], configurerequest, "handler: ConfigureRequest");
	ASSERT_EQ(handler[ConfigureNotify], configurenotify, "handler: ConfigureNotify");
	ASSERT_EQ(handler[DestroyNotify], destroynotify, "handler: DestroyNotify");
	ASSERT_EQ(handler[EnterNotify], enternotify, "handler: EnterNotify");
	ASSERT_EQ(handler[Expose], expose, "handler: Expose");
	ASSERT_EQ(handler[FocusIn], focusin, "handler: FocusIn");
	ASSERT_EQ(handler[KeyPress], keypress, "handler: KeyPress");
	ASSERT_EQ(handler[MappingNotify], mappingnotify, "handler: MappingNotify");
	ASSERT_EQ(handler[MapRequest], maprequest, "handler: MapRequest");
	ASSERT_EQ(handler[MotionNotify], motionnotify, "handler: MotionNotify");
	ASSERT_EQ(handler[PropertyNotify], propertynotify, "handler: PropertyNotify");
	ASSERT_EQ(handler[UnmapNotify], unmapnotify, "handler: UnmapNotify");

	if (LASTEvent > 1)
		ASSERT(handler[LASTEvent - 1] == NULL,
			"handler: unhandled event types map to NULL");
}

/* --- a root ConfigureNotify forces a bar rebuild through run() --- */
static void
test_run_dispatches_configurenotify(void)
{
	reset_index();
	Monitor *m = make_monitor(0);
	m->showbar = 1;
	m->bar_dirty_segments = 0;
	mons = m;
	save_selmon();
	selmon = m;
	mock_x11_reset();
	mock_drw_reset();

	mock_event_queue_count = 1;
	memset(&mock_event_queue[0], 0, sizeof(XEvent));
	mock_event_queue[0].type = ConfigureNotify;
	mock_event_queue[0].xconfigure.window = root;
	mock_event_queue[0].xconfigure.width = sw + 100;
	mock_event_queue[0].xconfigure.height = sh + 100;

	lifecycle = LIFE_LIVE;
	run();

	ASSERT_EQ(mock_xsync_calls, 1, "run: syncs once before the loop");
	ASSERT((m->bar_dirty_segments & DIRTY_ALL) == DIRTY_ALL,
		"run: dispatched ConfigureNotify dirtied the bar");

	restore_selmon();
	free(m);
}

/* --- an Expose event reaches drawbar() through the event loop --- */
static void
test_run_dispatches_expose(void)
{
	reset_index();
	Monitor *m = make_monitor(0);
	m->barwin = 888;
	m->showbar = 1;
	m->bar_dirty_segments = DIRTY_TAGS;
	m->bar_exposed = 0;
	mons = m;
	save_selmon();
	selmon = m;
	mock_x11_reset();
	mock_drw_reset();

	mock_event_queue_count = 1;
	memset(&mock_event_queue[0], 0, sizeof(XEvent));
	mock_event_queue[0].type = Expose;
	mock_event_queue[0].xexpose.window = 888;
	mock_event_queue[0].xexpose.count = 0;

	lifecycle = LIFE_LIVE;
	run();

	ASSERT_EQ(mock_drw_map_calls, 1,
		"run: dispatched Expose redrew the bar pixmap");

	restore_selmon();
	free(m);
}

static void
run_tests(void)
{
	test_handler_table();
	test_run_dispatches_configurenotify();
	test_run_dispatches_expose();
}
