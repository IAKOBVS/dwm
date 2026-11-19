/* See LICENSE file for copyright and license details.
 *
 * dynamic window manager is designed like any other X client as well. It is
 * driven through handling X events. In contrast to other X clients, a window
 * manager selects for SubstructureRedirectMask on the root window, to receive
 * events about window (dis-)appearance. Only one X connection at a time is
 * allowed to select for this event mask.
 *
 * The event handlers of dwm are organized in an array which is accessed
 * whenever a new event has been fetched. This allows event dispatching
 * in O(1) time.
 *
 * Each child of the root window is called a client, except windows which have
 * set the override_redirect flag. Clients are organized in a linked client
 * list on each monitor, the focus history is remembered through a stack list
 * on each monitor. Each client contains a bit array to indicate the tags of a
 * client.
 *
 * Keys and tagging rules are organized as arrays and defined in config.h.
 *
 * To understand everything else, start reading main().
 */
#include <fcntl.h>
#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif /* XINERAMA */
#include <X11/Xft/Xft.h>
#include <X11/Xlib-xcb.h>
#include <xcb/res.h>
#ifdef __OpenBSD__
#include <sys/sysctl.h>
#include <kvm.h>
#endif /* __OpenBSD */

#include "dwm.h"
#include "drw.h"
#include "util.h"

/* macros */
#define BUTTONMASK              (ButtonPressMask|ButtonReleaseMask)
#define CLEANMASK(mask)         (mask & ~(numlockmask|LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask)) /* strip lock bits, keep only modifiers */
#define INTERSECT(x,y,w,h,m)    (MAX(0, MIN((x)+(w),(m)->wx+(m)->ww) - MAX((x),(m)->wx)) /* pixel overlap of rect and monitor m */\
                               * MAX(0, MIN((y)+(h),(m)->wy+(m)->wh) - MAX((y),(m)->wy)))
/**
 * @brief True when a client's tags intersect the monitor's current view.
 * @param C  Client with a valid `mon`, `tags`, and monitor tag state.
 * @anchor spec_isvisible
 */
#define ISVISIBLE(C)            ((C->tags & C->mon->tagset[C->mon->seltags])) /* true if client tagged in current view */
#define LENGTH(X)               (sizeof X / sizeof X[0])
#define MOUSEMASK               (BUTTONMASK|PointerMotionMask)
#define WIDTH(X)                ((X)->w + 2 * (X)->bw)
#define HEIGHT(X)               ((X)->h + 2 * (X)->bw)
#define TAGMASK                 ((1 << LENGTH(tags)) - 1)
/**
 * @brief Pixel width of a text string including horizontal padding.
 * @param X  NUL-terminated string.
 * @anchor spec_textw
 */
#define TEXTW(X)                (drw_fontset_getwidth(drw, (X)) + lrpad)
#define S_LEN(s)     (sizeof(s) - 1)
#define S_LITERAL(s) (s), (sizeof(s) - 1)
static const int lrpad_modifier = 1;
static int bar_draw_pending = 0;     /* deferred drawbar requested; executed at run() loop tail */
static int arrange_pending = 0;      /* deferred arrange requested; executed at run() loop tail */
static int dispatching = 0;          /* true while run() dispatches an event handler; enables arrange coalescing */
static int pointergrab = 0;          /* true inside movemouse/resizemouse; force immediate reflow */

/** @brief Phases of an interactive mouse drag. */
typedef enum {
	DRAG_IDLE = 0,   /**< no grab owned */
	DRAG_GRABBED,    /**< XGrabPointer succeeded, loop not entered */
	DRAG_ACTIVE,     /**< motion loop running */
	DRAG_RELEASING   /**< ungrab in progress */
} DragPhase;

/** @brief State owned by a single mouse drag; no overlapping drags are allowed. */
typedef struct {
	Client *c;        /**< client being dragged */
	Monitor *origin;  /**< monitor the drag started on */
	Time lasttime;    /**< throttling timestamp */
	DragPhase phase;  /**< current phase */
} DragState;
static DragState drag = { .phase = DRAG_IDLE };

/* Window -> Client lookup cache.
 * wintoclient() is called on every event delivery (enter/notify, property,
 * configure...); the linear walk over all monitors' client lists showed
 * O(n) cost growth in benchmarks.  Open-addressing hash with linear
 * probing; entries are Client pointers (key = c->win read back on probe).
 * When the table is full, insertion is suppressed and lookups fall back to
 * the authoritative list walk, so overflow degrades gracefully instead of
 * corrupting results. */
#define WINHASH_SIZE 256   /* power of two */
static Client *winhash[WINHASH_SIZE];
static unsigned int winhash_count = 0;
static int winhash_overflowed = 0;   /* an insert was suppressed; lists are authoritative */
static unsigned int winhash_home(Window w);
static void winclient_put(Client *c);
static void winclient_remove(Client *c);

#ifndef DWM_TEST
static
#endif
long arrange_calls = 0;              /* test observability: layout passes performed */

#ifndef VERSION
#	define VERSION
#endif

/* configuration, allows nested code to access above variables */
#include "config.h"

/** @brief Logical phase of a monitor's bar, derived from its flag fields. */
typedef enum {
	BAR_HIDDEN = 0,    /**< bar not shown */
	BAR_FROZEN,        /**< fullscreen policy suppresses drawing */
	BAR_CLEAN,         /**< pixmap matches screen and content */
	BAR_EXPOSED_ONLY,  /**< only the retained pixmap needs a blit */
	BAR_DIRTY          /**< one or more content segments changed */
} BarState;

/* Tests observe the dirty mask after drawbar(); production clears it. */
#ifdef DWM_TEST
static int bar_clear_on_draw = 0;
#else
static int bar_clear_on_draw = 1;
#endif

/**
 * @brief Derive the explicit bar phase from a monitor's current flags.
 *
 * @param[in] m  Monitor whose bar state is inspected.
 * @return The current BarState.
 * @note Pure read of m->showbar, m->sel, m->bar_dirty_segments and
 *       m->bar_exposed.
 * @anchor spec_bar_state_of
 */
static BarState
bar_state_of(Monitor *m)
{
	if (!m->showbar)
		return BAR_HIDDEN;
	if (optimizefullscreen && m->sel && m->sel->isfullscreen)
		return BAR_FROZEN;
	if (m->bar_dirty_segments)
		return BAR_DIRTY;
	if (m->bar_exposed)
		return BAR_EXPOSED_ONLY;
	return BAR_CLEAN;
}

/**
 * @brief Mark bar segments dirty and request a coalesced redraw.
 *
 * This is the single writer for bar_dirty_segments and bar_draw_pending;
 * direct writes elsewhere are replaced by calls to this helper.
 *
 * @param[in,out] m     Monitor whose bar changes; NULL is ignored.
 * @param[in]     bits  DIRTY_* segment mask.
 * @post m->bar_dirty_segments includes @p bits and bar_draw_pending is set.
 * @note Writes per-monitor bar state and the global pending flag.
 * @anchor spec_bar_mark_dirty
 */
static void
bar_mark_dirty(Monitor *m, unsigned int bits)
{
	if (!m)
		return;
	m->bar_dirty_segments |= bits;
	bar_draw_pending = 1;
}


/**
 * @brief Compile-time guard that all tags fit in an unsigned int bitmask.
 */
struct NumTags { char limitexceeded[LENGTH(tags) > 31 ? -1 : 1]; };

/**
 * @brief Apply matching rules[] entries to a client.
 *
 * Resets c->isfloating and c->tags, pulls WM_CLASS from the X server, and
 * applies every matching rule's floating, tag, monitor, terminal and noswallow
 * fields. When no rule supplies tags the client inherits the current view's
 * tags.
 *
 * @param[in,out]  c  Client whose fields are updated; c->win and c->mon must already be set.
 *
 * @pre c != NULL, c->win is a valid X window and c->mon is a valid monitor.
 * @post c->tags is non-zero and masked to TAGMASK; c->mon points into mons.
 * @note Reads rules[]/broken; writes c->tags, c->isfloating, c->isterminal, c->noswallow and c->mon; calls XGetClassHint() and XFree().
 * @warning The tag fallback uses c->mon, so c->mon must be initialized even when no rule matches.
 * @see manage, termforwin, applysizehints
 * @anchor spec_applyrules
 */
void
applyrules(Client *c)
{
	const char *class, *instance;
	unsigned int i;
	const Rule *r;
	Monitor *m;
	XClassHint ch = { NULL, NULL };

	/* rule matching */
	c->isfloating = 0;
	c->tags = 0;
	XGetClassHint(dpy, c->win, &ch);
	class    = ch.res_class ? ch.res_class : broken;
	instance = ch.res_name  ? ch.res_name  : broken;

	for (i = 0; i < LENGTH(rules); i++) {
		r = &rules[i];
		if ((!r->title || strstr(c->name, r->title))
		&& (!r->class || strstr(class, r->class))
		&& (!r->instance || strstr(instance, r->instance)))
		{
			c->isterminal = r->isterminal;
			c->noswallow  = r->noswallow;
			c->isfloating = r->isfloating;
			c->tags |= r->tags;
			for (m = mons; m && m->num != r->monitor; m = m->next);
			if (m)
				c->mon = m;
		}
	}
	if (ch.res_class)
		XFree(ch.res_class);
	if (ch.res_name)
		XFree(ch.res_name);
	c->tags = c->tags & TAGMASK ? c->tags & TAGMASK : c->mon->tagset[c->mon->seltags];
}

/**
 * @brief Clamp geometry to ICCCM size hints and workarea bounds.
 *
 * Forces at least a 1-pixel size and the bar height as a minimum, keeps the
 * rectangle on-screen (interact uses the full screen, otherwise the client's
 * monitor workarea), then applies base/min/max size, aspect ratio and resize
 * increment hints when hints are honored. Returns whether the clamped geometry
 * differs from the client's current geometry.
 *
 * @param[in]  c  Client whose cached hints are consulted.
 * @param[in,out]  x  Proposed X coordinate, clamped in place.
 * @param[in,out]  y  Proposed Y coordinate, clamped in place.
 * @param[in,out]  w  Proposed width, clamped in place.
 * @param[in,out]  h  Proposed height, clamped in place.
 * @param[in]  interact  Non-zero for mouse-driven geometry, zero for layout geometry.
 *
 * @return Non-zero when any resulting coordinate or size differs from c->x/y/w/h; zero when the geometry is unchanged.
 *
 * @pre c != NULL, c->mon is valid, and x/y/w/h point to initialized integers.
 * @post The pointed-to values hold the clamped geometry; size-hint fields may have been refreshed via updatesizehints().
 * @note Reads resizehints, sw, sh and bh; may call updatesizehints() (which issues XGetWMNormalHints()) when c->hintsvalid is clear.
 * @see updatesizehints, resize, resizeclient, tile
 * @anchor spec_applysizehints
 */
int
applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact)
{
	int baseismin;
	Monitor *m = c->mon;

	/* set minimum possible */
	*w = MAX(1, *w);
	*h = MAX(1, *h);
	if (interact) {
		if (*x > sw)
			*x = sw - WIDTH(c);
		if (*y > sh)
			*y = sh - HEIGHT(c);
		if (*x + *w + 2 * c->bw < 0)
			*x = 0;
		if (*y + *h + 2 * c->bw < 0)
			*y = 0;
	} else {
		if (*x >= m->wx + m->ww)
			*x = m->wx + m->ww - WIDTH(c);
		if (*y >= m->wy + m->wh)
			*y = m->wy + m->wh - HEIGHT(c);
		if (*x + *w + 2 * c->bw <= m->wx)
			*x = m->wx;
		if (*y + *h + 2 * c->bw <= m->wy)
			*y = m->wy;
	}
	if (*h < bh)
		*h = bh;
	if (*w < bh)
		*w = bh;
	if (resizehints || c->isfloating || !c->mon->lt[c->mon->sellt]->arrange) {
		if (!c->hintsvalid)
			updatesizehints(c);
		/* see last two sentences in ICCCM 4.1.2.3 */
		baseismin = c->basew == c->minw && c->baseh == c->minh;
		if (!baseismin) { /* temporarily remove base dimensions */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for aspect limits */
		if (c->mina > 0 && c->maxa > 0) {
			if (c->maxa < (float)*w / *h)
				*w = *h * c->maxa + 0.5;
			else if (c->mina < (float)*h / *w)
				*h = *w * c->mina + 0.5;
		}
		if (baseismin) { /* increment calculation requires this */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for increment value */
		if (c->incw)
			*w -= *w % c->incw;
		if (c->inch)
			*h -= *h % c->inch;
		/* restore base dimensions */
		*w = MAX(*w + c->basew, c->minw);
		*h = MAX(*h + c->baseh, c->minh);
		if (c->maxw)
			*w = MIN(*w, c->maxw);
		if (c->maxh)
			*h = MIN(*h, c->maxh);
	}
	return *x != c->x || *y != c->y || *w != c->w || *h != c->h;
}

static void arrangenow(Monitor *m);

/**
 * @brief Request a layout pass, coalesced while an event is being dispatched.
 *
 * During event dispatch and outside a pointer grab the request is recorded in
 * arrange_pending and performed once at the event-loop tail. Low-frequency
 * creation and destruction paths bypass the deferral by calling arrangenow()
 * directly so per-window map/unmap transitions stay visible; calls outside event
 * dispatch (pointer grabs, setup, scan) run immediately.
 *
 * @param[in]  m  Monitor to arrange, or NULL to arrange every monitor.
 *
 * @post Either the layout ran immediately or arrange_pending is set for flusheventtail().
 * @note Reads dispatching and pointergrab; writes arrange_pending.
 * @see arrangenow, flusheventtail, run, arrangemon
 * @anchor spec_arrange
 */
void
arrange(Monitor *m)
{
	if (dispatching && !pointergrab) {
		arrange_pending = 1;
		return;
	}
	arrangenow(m);
}

/**
 * @brief Perform a full layout pass immediately.
 *
 * Shows or hides clients on the requested monitor (or all monitors), invokes
 * each monitor's current layout function and reapplies stacking. This bypasses
 * arrange()'s event-dispatch coalescing.
 *
 * @param[in]  m  Monitor to arrange, or NULL to arrange every monitor.
 *
 * @pre The mons list is valid; if m is non-NULL it must belong to mons and have an allocated gap.
 * @post Client visibility, geometry and stacking reflect the current layout.
 * @note Increments arrange_calls under DWM_TEST; restack() sets bar_draw_pending; issues X11 geometry and stacking calls via the layout and restack.
 * @see arrange, arrangemon, restack, flusheventtail
 * @anchor spec_arrangenow
 */
static void
arrangenow(Monitor *m)
{
#ifdef DWM_TEST
	arrange_calls++; /* test observability: count layout passes */
#endif
	if (m)
		showhide(m->stack);
	else for (m = mons; m; m = m->next)
		showhide(m->stack);
	if (m) {
		arrangemon(m);
		restack(m);
	} else for (m = mons; m; m = m->next)
		arrangemon(m);
}

/**
 * @brief Run deferred layout and bar-redraw work after an event.
 *
 * Performs at most one full layout pass when arrange_pending is set, then
 * issues coalesced bar redraws when bar_draw_pending is set. The layout runs
 * first so bars render against post-layout geometry.
 *
 * @note Clears arrange_pending and bar_draw_pending; calls arrangenow() and drawbars() (Xft/X11 drawing).
 * @see arrange, drawbars, run
 * @anchor spec_flusheventtail
 */
static void
flusheventtail(void)
{
	if (arrange_pending) {
		arrange_pending = 0; /* restack skipped by arrangenow(NULL) */
		bar_draw_pending = 1;
		arrangenow(NULL);
	}
	if (bar_draw_pending) {
		bar_draw_pending = 0;
		drawbars();
	}
}

/**
 * @brief Refresh the layout symbol and run a monitor's layout function.
 *
 * Copies the current layout's symbol into m->ltsymbol and invokes its arrange
 * hook when one exists. Floating layouts have no arrange hook, so placement is
 * left untouched.
 *
 * @param[in]  m  Monitor whose current layout is applied.
 *
 * @note Writes m->ltsymbol; may invoke resize() and other X11 operations through the layout function.
 * @see arrange, setlayout, tile, monocle
 * @anchor spec_arrangemon
 */
void
arrangemon(Monitor *m)
{
	stpncpy_len(m->ltsymbol, sizeof m->ltsymbol, m->lt[m->sellt]->symbol, strlen(m->lt[m->sellt]->symbol));
	if (m->lt[m->sellt]->arrange)
		m->lt[m->sellt]->arrange(m);
}

/**
 * @brief Prepend a client to its monitor's client list.
 *
 * Newest clients become the list head.
 *
 * @param[in,out]  c  Client to link; c->mon must be valid.
 *
 * @note Writes c->next and c->mon->clients.
 * @warning Does not check whether c is already linked.
 * @see detach, attachstack, manage
 * @anchor spec_attach
 */
void
attach(Client *c)
{
	c->next = c->mon->clients;
	c->mon->clients = c;
}

/**
 * @brief Push a client onto its monitor's focus-history stack.
 *
 * The most recently focused client becomes the stack head.
 *
 * @param[in,out]  c  Client to link; c->mon must be valid.
 *
 * @note Writes c->snext and c->mon->stack.
 * @warning Does not check whether c is already on the stack.
 * @see detachstack, focus
 * @anchor spec_attachstack
 */
void
attachstack(Client *c)
{
	c->snext = c->mon->stack;
	c->mon->stack = c;
}

/**
 * @brief Make terminal p adopt the newly spawned client c's window.
 *
 * The two window ids are exchanged so p displays c's content while p's own
 * terminal window is unmapped and parked on c (WithdrawnState) until
 * unswallow() reverses the exchange. Both hash entries are re-indexed after the
 * swap and the parent's title is refreshed.
 *
 * @param[in,out]  p  Terminal client that adopts c's window.
 * @param[in,out]  c  Spawned child client whose window is transferred.
 *
 * @pre c is managed and distinct from p; c->mon is valid.
 * @post p->swallowing == c; window ids and hash entries are swapped; p's bar title segment is dirty.
 * @note Reads enableswallow and swallowfloating; detaches c from the client and focus stacks; mutates winhash; calls XUnmapWindow(), XMoveResizeWindow(), arrangenow(), configure(), updatetitle() and updateclientlist().
 * @warning Removes c from its monitor lists before returning; callers must not reuse the old list links.
 * @see unswallow, swallowingclient, termforwin
 * @anchor spec_swallow
 */
void
swallow(Client *p, Client *c)
{

	if (!enableswallow || c->noswallow || c->isterminal)
		return;
	if (!swallowfloating && c->isfloating)
		return;

	detach(c);
	detachstack(c);

	setclientstate(c, WithdrawnState);
	XUnmapWindow(dpy, p->win);

	p->swallowing = c;
	p->state = CST_HOLDER;
	c->state = CST_PLACEHOLDER;
	c->holder = p;
	c->mon = p->mon;

	/* window identities changed hands; re-index both lookup entries */
	winclient_remove(p);
	winclient_remove(c);
	Window w = p->win;
	p->win = c->win;
	c->win = w;
	winclient_put(p);
	winclient_put(c);
	updatetitle(p);
	/* parent adopted child's name; client list unchanged, tags untouched */
	bar_mark_dirty(p->mon, DIRTY_TITLE);
	XMoveResizeWindow(dpy, p->win, p->x, p->y, p->w, p->h);
	arrangenow(p->mon); /* creation path: keep the swap transition visible */
	configure(p);
	updateclientlist();
}

/**
 * @brief Reverse a swallow exchange and restore the terminal window.
 *
 * Drops both stale hash entries, transfers the parked placeholder's window id
 * back to c, frees the placeholder, leaves fullscreen, remaps the window and
 * restores NormalState before refocusing.
 *
 * @param[in,out]  c  Holder client whose placeholder is being dissolved.
 *
 * @pre c->swallowing != NULL.
 * @post c->swallowing == NULL; c owns its original window id and is mapped in NormalState.
 * @note Calls winclient_remove()/winclient_put(), setfullscreen(), updatetitle(), arrange(), XMapWindow(), XMoveResizeWindow(), setclientstate() and focus().
 * @warning Frees the placeholder client; it must not be referenced afterwards.
 * @see swallow, unmanage
 * @anchor spec_unswallow
 */
void
unswallow(Client *c)
{
	/* the swallowed terminal dies and its window id moves to c;
	 * drop both stale entries first, then re-insert under the new id */
	winclient_remove(c);
	winclient_remove(c->swallowing);
	c->win = c->swallowing->win;
	winclient_put(c);

	{
		Client *ph = c->swallowing;

		ph->state = CST_FREED;
		ph->holder = NULL;
		free(ph);
	}
	c->swallowing = NULL;
	c->state = CST_LISTED;

	/* unfullscreen the client */
	setfullscreen(c, 0);
	updatetitle(c);
	arrange(c->mon);
	XMapWindow(dpy, c->win);
	XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
	setclientstate(c, NormalState);
	focus(NULL);
	arrange(c->mon);
}

/**
 * @brief Route a button press to focus, click classification and bindings.
 *
 * Focuses the clicked monitor and client. If no mouse binding could match, it
 * only focuses and replays the pointer so the application receives the press;
 * otherwise it classifies the click as tag label, layout symbol, status text,
 * window title, client window or root and invokes every matching buttons[] entry.
 *
 * @param[in]  e  X ButtonPress event.
 *
 * @post The pointer button is either replayed to the client or consumed by a binding; selmon and selmon->sel may have changed.
 * @note May set selmon; calls unfocus(), focus(), restack(), XAllowEvents() and TEXTW (DRW/Xft); reads buttons[] and invokes bound functions.
 * @see mousebuttonmatch, cachebuttons, keypress
 * @anchor spec_buttonpress
 */
void
buttonpress(XEvent *e)
{
	unsigned int i, x, click, mask;
	Arg arg = {0};
	Client *c = NULL;
	Monitor *m;
	XButtonPressedEvent *ev = &e->xbutton;

	mask = CLEANMASK(ev->state);
	click = ClkRootWin;
	/* focus monitor if necessary */
	if ((m = wintomon(ev->window)) && m != selmon) {
		unfocus(selmon->sel, 1);
		selmon = m;
		focus(NULL);
	}
	/* binding didn't match: just focus the clicked client (if any) and
	 * replay the synced pointer grab so the application receives the press */
	if (!mousebuttonmatch(ev->button, mask)) {
		if ((c = wintoclient(ev->window))) {
			focus(c);
			restack(selmon);
			XAllowEvents(dpy, ReplayPointer, CurrentTime);
		}
		return;
	}
	if (ev->window == selmon->barwin) {
		i = x = 0;
		/* locate the clicked tag by walking cumulative label widths */
		do
			x += TEXTW(tags[i]);
		while (ev->x >= x && ++i < LENGTH(tags));
		if (i < LENGTH(tags)) {
			click = ClkTagBar;
			arg.ui = 1 << i;
		} else if (ev->x < x + TEXTW(selmon->ltsymbol))
			click = ClkLtSymbol;
		else if (ev->x > selmon->ww - TEXTW(stext))
			click = ClkStatusText;
		else
			click = ClkWinTitle;
	} else if ((c = wintoclient(ev->window))) {
		focus(c);
		restack(selmon);
		XAllowEvents(dpy, ReplayPointer, CurrentTime);
		click = ClkClientWin;
	}
	for (i = 0; i < LENGTH(buttons); i++)
		if (click == buttons[i].click && buttons[i].func && buttons[i].button == ev->button
		&& CLEANMASK(buttons[i].mask) == mask)
			buttons[i].func(click == ClkTagBar && buttons[i].arg.i == 0 ? &arg : &buttons[i].arg);
}

/**
 * @brief Test whether a button and cleaned mask could match any binding.
 *
 * Uses the precomputed button and modifier bitmaps for a constant-time
 * rejection. Buttons beyond the bitmap width always pass the button test; masks
 * outside the union of configured modifiers are rejected immediately.
 *
 * @param[in]  button  Button number from the event.
 * @param[in]  mask  Modifier state already passed through CLEANMASK().
 *
 * @return Non-zero when a configured binding could match, zero otherwise.
 *
 * @note Reads button_button_used and button_mask_used built by cachebuttons().
 * @see cachebuttons, buttonpress
 * @anchor spec_mousebuttonmatch
 */
int
mousebuttonmatch(unsigned int button, unsigned int mask)
{
	return (button >= 8 * sizeof button_button_used || (button_button_used & (1UL << button)))
		&& !(mask & ~button_mask_used);
}

/**
 * @brief Rebuild mousebuttonmatch() fast-path bitmaps from buttons[].
 *
 * ORs together every bound button number and every cleaned modifier mask,
 * ignoring entries without a function.
 *
 * @note Writes button_button_used and button_mask_used; reads buttons[].
 * @see mousebuttonmatch, setup
 * @anchor spec_cachebuttons
 */
void
cachebuttons(void)
{
	unsigned int i;

	button_button_used = 0;
	button_mask_used = 0;
	for (i = 0; i < LENGTH(buttons); i++) {
		if (!buttons[i].func)
			continue;
		if (buttons[i].button < 8 * sizeof button_button_used)
			button_button_used |= 1UL << buttons[i].button;
		button_mask_used |= CLEANMASK(buttons[i].mask);
	}
}

/**
 * @brief Pack a keysym and cleaned modifier into a single 64-bit key.
 *
 * @param[in]  keysym  X keysym.
 * @param[in]  mod  Modifier state, normally after CLEANMASK().
 *
 * @return The keysym shifted into the high 32 bits ORed with the low 32 bits of mod.
 *
 * @note Pure computation; reads no globals.
 * @see keyset_home, keyset_put, iskeybound
 * @anchor spec_keypack
 */
static unsigned long long
keypack(KeySym keysym, unsigned int mod)
{
	return ((unsigned long long)keysym << 32) | (mod & 0xffffffffULL);
}

/**
 * @brief Compute the hash home slot for a packed key.
 *
 * Applies a 64-bit Fibonacci hash and folds the result with
 * (KEYSET_SIZE - 1) so the slot is always in bounds.
 *
 * @param[in]  k  Packed (keysym, modifier) key from keypack().
 *
 * @return Home slot index in [0, KEYSET_SIZE).
 *
 * @pre KEYSET_SIZE is a power of two.
 * @note Pure computation; reads KEYSET_SIZE.
 * @see keyset_put, iskeybound, cachekeys
 * @anchor spec_keyset_home
 */
static unsigned int
keyset_home(unsigned long long k)
{
	/* Fibonacci hash: spreads structured KeySym|mod bit patterns evenly */
	return (unsigned int)(((k * 0x9E3779B97F4A7C15ULL) >> 56) & (KEYSET_SIZE - 1));
}

/**
 * @brief Insert a packed key into the exact-match keyset.
 *
 * Uses linear probing and tolerates duplicates. When the table is nearly full
 * it suppresses the insert and sets keyset_saturated so iskeybound() falls back
 * to the lossy OR-mask test.
 *
 * @param[in]  k  Packed key from keypack().
 *
 * @post The key may be stored and keyset_count incremented, or keyset_saturated may be set.
 * @note Writes keyset[], keyset_count and keyset_saturated.
 * @see keyset_home, iskeybound, cachekeys
 * @anchor spec_keyset_put
 */
static void
keyset_put(unsigned long long k)
{
	unsigned int i = keyset_home(k), probe = 0;

	if (keyset_count >= KEYSET_SIZE - 1) {
		keyset_saturated = 1; /* keep masks as the guard instead */
		return;
	}
	while (keyset[i] && keyset[i] != k) {
		i = (i + 1) & (KEYSET_SIZE - 1);
		if (++probe >= KEYSET_SIZE)
			return;
	}
	if (!keyset[i]) {
		keyset[i] = k;
		keyset_count++;
	}
}

/**
 * @brief Exact fast-path test for a keysym and modifier state.
 *
 * Looks up the packed key in the exact keyset. If the set is empty or
 * saturated it falls back to the lossy union-of-masks test.
 *
 * @param[in]  keysym  X keysym to test.
 * @param[in]  state  Raw event modifier state.
 *
 * @return Non-zero when any configured binding could match, zero otherwise.
 *
 * @note Reads keyset[], keyset_count, keyset_saturated, key_mod_used and key_keysym_used; cleans state with CLEANMASK().
 * @see cachekeys, keypress, keyset_put
 * @anchor spec_iskeybound
 */
static int
iskeybound(KeySym keysym, unsigned int state)
{
	unsigned long long k;
	unsigned int i, probe = 0;

	if (!keyset_count || keyset_saturated)
		return (CLEANMASK(state) & key_mod_used)
		    && (keysym & key_keysym_used);

	k = keypack(keysym, CLEANMASK(state));
	for (i = keyset_home(k); keyset[i]; i = (i + 1) & (KEYSET_SIZE - 1)) {
		if (keyset[i] == k)
			return 1;
		if (++probe >= KEYSET_SIZE)
			break;
	}
	return 0;
}

/**
 * @brief Rebuild keyboard fast-path masks and the exact keyset from keys[].
 *
 * Accumulates the lossy OR-masks of used keysyms and modifiers, clears the
 * exact keyset, and inserts each binding's packed (keysym, cleaned modifier) key.
 * Entries without a function are ignored.
 *
 * @note Writes key_keysym_used, key_mod_used, keyset[], keyset_count and keyset_saturated; calls keyset_put().
 * @see iskeybound, grabkeys, keypress
 * @anchor spec_cachekeys
 */
void
cachekeys(void)
{
	unsigned int i;

	key_keysym_used = 0;
	key_mod_used = 0;
	memset(keyset, 0, sizeof keyset);
	keyset_count = 0;
	keyset_saturated = 0;
	for (i = 0; i < LENGTH(keys); i++) {
		if (!keys[i].func)
			continue;
		key_keysym_used |= keys[i].keysym;
		key_mod_used |= keys[i].mod;
		/* store with the same cleaning keypress() applies at lookup */
		keyset_put(keypack(keys[i].keysym, CLEANMASK(keys[i].mod)));
	}
}

/**
 * @brief Abort if another window manager already owns the root selection.
 *
 * Selecting SubstructureRedirectMask on the root raises BadAccess only when a
 * window manager is already running. A temporary startup error handler turns
 * that error into a fatal message; the regular handler is restored afterwards.
 *
 * @note X11 calls XSetErrorHandler(), XSelectInput() and XSync(); mutates the process X error handler state.
 * @warning Calls DIE (via xerrorstart) when another window manager is present.
 * @see xerrorstart, xerror, setup
 * @anchor spec_checkotherwm
 */
void
checkotherwm(void)
{
	xerrorxlib = XSetErrorHandler(xerrorstart);
	/* this causes an error if some other window manager is running */
	XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureRedirectMask);
	XSync(dpy, False);
	XSetErrorHandler(xerror);
	XSync(dpy, False);
}

/**
 * @brief Tear down dwm state for exit or restart.
 *
 * Clears the window hash, views all tags so every client is reachable,
 * installs a no-op layout to suppress relayouts, unmaps and frees every client
 * and monitor, releases grabs, cursors, colors, fonts and drawing state, then
 * resets X input focus and EWMH state.
 *
 * @post mons == NULL, the window hash is empty, and X resources owned by dwm are released.
 * @note Mutates winhash state, selmon/mons, scheme, drw and root properties; issues many X11 calls and frees clients through unmanage().
 * @warning Must not run while the event loop still uses drw or dpy.
 * @see cleanupmon, unmanage, setup, run
 * @anchor spec_cleanup
 */
void
cleanup(void)
{
	Arg a = {.ui = ~0};
	Layout foo = { "", NULL };
	Monitor *m;
	size_t i;

	memset(winhash, 0, sizeof winhash); /* entries are freed with their clients below */
	winhash_count = 0;
	winhash_overflowed = 0;
	/* view all tags so every client is reachable when unmanaged */
	view(&a);
	/* dummy layout without an arrange fn: prevent relayouts during teardown */
	selmon->lt[selmon->sellt] = &foo;
	for (m = mons; m; m = m->next)
		while (m->stack)
			unmanage(m->stack, 0);
	XUngrabKey(dpy, AnyKey, AnyModifier, root);
	while (mons)
		cleanupmon(mons);
	for (i = 0; i < CurLast; i++)
		drw_cur_free(drw, cursor[i]);
	for (i = 0; i < LENGTH(colors); i++)
		free(scheme[i]);
	free(scheme);
	XDestroyWindow(dpy, wmcheckwin);
	drw_free(drw);
	XSync(dpy, False);
	XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
	XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
}

/**
 * @brief Unlink a monitor, destroy its bar and free it.
 *
 * @param[in]  mon  Monitor to remove from the global list.
 *
 * @pre mon != NULL and mon is linked into mons.
 * @post mon is unlinked, its bar window is unmapped and destroyed, and its memory is freed.
 * @note Writes mons (or the preceding next link); calls XUnmapWindow() and XDestroyWindow().
 * @warning Does not migrate or free the monitor's clients; the caller must handle them first.
 * @see cleanup, updategeom, createmon
 * @anchor spec_cleanupmon
 */
void
cleanupmon(Monitor *mon)
{
	Monitor *m;

	if (mon == mons)
		mons = mons->next;
	else {
		for (m = mons; m && m->next != mon; m = m->next);
		m->next = mon->next;
	}
	XUnmapWindow(dpy, mon->barwin);
	XDestroyWindow(dpy, mon->barwin);
	free(mon);
}

/**
 * @brief Handle EWMH client messages for fullscreen and activation.
 *
 * Processes _NET_WM_STATE fullscreen add/remove/toggle requests and
 * _NET_ACTIVE_WINDOW requests, marking unselected non-urgent clients urgent.
 *
 * @param[in]  e  X ClientMessage event.
 *
 * @post The requested fullscreen or urgency state is applied when the window is managed.
 * @note Calls wintoclient(), setfullscreen() and seturgent(); reads netatom[] and selmon.
 * @see setfullscreen, seturgent, propertynotify
 * @anchor spec_clientmessage
 */
void
clientmessage(XEvent *e)
{
	XClientMessageEvent *cme = &e->xclient;
	Client *c = wintoclient(cme->window);

	if (!c)
		return;
	if (cme->message_type == netatom[NetWMState]) {
		if (cme->data.l[1] == netatom[NetWMFullscreen]
		|| cme->data.l[2] == netatom[NetWMFullscreen])
			setfullscreen(c, (cme->data.l[0] == 1 /* _NET_WM_STATE_ADD    */
				|| (cme->data.l[0] == 2 /* _NET_WM_STATE_TOGGLE */ && !c->isfullscreen)));
	} else if (cme->message_type == netatom[NetActiveWindow]) {
		if (c != selmon->sel && !c->isurgent)
			seturgent(c, 1);
	}
}

/**
 * @brief Send a client a synthetic ConfigureNotify with dwm's geometry.
 *
 * dwm does not reparent clients, so clients cannot infer their frame from real
 * ConfigureNotify events alone; this reports the geometry dwm assigned.
 *
 * @param[in]  c  Client to notify; c->win must be valid.
 *
 * @note Calls XSendEvent() with StructureNotifyMask; reads c->x/y/w/h/bw.
 * @see resizeclient, configurerequest, manage
 * @anchor spec_configure
 */
void
configure(Client *c)
{
	XConfigureEvent ce;

	ce.type = ConfigureNotify;
	ce.display = dpy;
	ce.event = c->win;
	ce.window = c->win;
	ce.x = c->x;
	ce.y = c->y;
	ce.width = c->w;
	ce.height = c->h;
	ce.border_width = c->bw;
	ce.above = None;
	ce.override_redirect = False;
	XSendEvent(dpy, c->win, False, StructureNotifyMask, (XEvent *)&ce);
}

/**
 * @brief React to a root resize by refreshing geometry, bars and layout.
 *
 * When the root window's size changes it records the new screen dimensions,
 * re-probes monitor geometry, resizes the drawing context and bar windows,
 * refits fullscreen clients and finally refocuses and rearranges.
 *
 * @param[in]  e  X ConfigureNotify event.
 *
 * @pre dpy and drw are initialized.
 * @note Writes sw and sh; calls updategeom(), drw_resize(), updatebars(), resizeclient(), XMoveResizeWindow(), focus() and arrange().
 * @see updategeom, updatebars, arrange
 * @anchor spec_configurenotify
 */
void
configurenotify(XEvent *e)
{
	Monitor *m;
	Client *c;
	XConfigureEvent *ev = &e->xconfigure;
	int dirty;

	/* TODO: updategeom handling sucks, needs to be simplified */
	if (ev->window == root) {
		dirty = (sw != ev->width || sh != ev->height);
		sw = ev->width;
		sh = ev->height;
		if (updategeom() || dirty) {
			drw_resize(drw, sw, bh);
			updatebars();
			for (m = mons; m; m = m->next) {
				bar_mark_dirty(m, DIRTY_ALL); /* resize invalidated the pixmap */
				for (c = m->clients; c; c = c->next)
					if (c->isfullscreen)
						resizeclient(c, m->mx, m->my, m->mw, m->mh);
				XMoveResizeWindow(dpy, m->barwin, m->wx, m->by, m->ww, bh);
			}
			focus(NULL);
			arrange(NULL);
		}
	}
}

/**
 * @brief Answer a client's ConfigureRequest.
 *
 * Managed clients may change their border width; floating clients or clients
 * on a monitor without an arrange hook may choose geometry clamped to the
 * monitor and are centered when they exceed it; tiled clients are refused with a
 * synthetic configure reporting dwm's answer. Unmanaged windows are configured
 * as requested.
 *
 * @param[in]  e  X ConfigureRequest event.
 *
 * @note Reads selmon; writes c->bw, c->x/y/w/h and their old values; calls configure(), XMoveResizeWindow(), XConfigureWindow() and XSync().
 * @see configure, resize, manage
 * @anchor spec_configurerequest
 */
void
configurerequest(XEvent *e)
{
	Client *c;
	Monitor *m;
	XConfigureRequestEvent *ev = &e->xconfigurerequest;
	XWindowChanges wc;

	if ((c = wintoclient(ev->window))) {
		if (ev->value_mask & CWBorderWidth)
			c->bw = ev->border_width;
		else if (c->isfloating || !selmon->lt[selmon->sellt]->arrange) {
			m = c->mon;
			if (ev->value_mask & CWX) {
				c->oldx = c->x;
				c->x = m->mx + ev->x;
			}
			if (ev->value_mask & CWY) {
				c->oldy = c->y;
				c->y = m->my + ev->y;
			}
			if (ev->value_mask & CWWidth) {
				c->oldw = c->w;
				c->w = ev->width;
			}
			if (ev->value_mask & CWHeight) {
				c->oldh = c->h;
				c->h = ev->height;
			}
			if ((c->x + c->w) > m->mx + m->mw && c->isfloating)
				c->x = m->mx + (m->mw / 2 - WIDTH(c) / 2); /* center in x direction */
			if ((c->y + c->h) > m->my + m->mh && c->isfloating)
				c->y = m->my + (m->mh / 2 - HEIGHT(c) / 2); /* center in y direction */
			if ((ev->value_mask & (CWX|CWY)) && !(ev->value_mask & (CWWidth|CWHeight)))
				configure(c);
			if (ISVISIBLE(c))
				XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
		} else
			configure(c);
	} else {
		wc.x = ev->x;
		wc.y = ev->y;
		wc.width = ev->width;
		wc.height = ev->height;
		wc.border_width = ev->border_width;
		wc.sibling = ev->above;
		wc.stack_mode = ev->detail;
		XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
	}
	XSync(dpy, False);
}

/**
 * @brief Allocate a monitor initialized from compile-time defaults.
 *
 * Initializes both tagsets to tag 1, copies mfact/nmaster/showbar/topbar,
 * clones default_gap, installs the first two layouts (guarding single-layout
 * configs) and marks every bar segment dirty so the first draw renders
 * everything.
 *
 * @return Newly allocated monitor with an embedded initialized gap.
 *
 * @post The caller owns the monitor; it is not yet linked into mons and has no bar window.
 * @note Uses ecalloc(); reads mfact, nmaster, showbar, topbar, default_gap and layouts[]; sets bar_dirty_segments and bar_exposed.
 * @see cleanupmon, updategeom, setup
 * @anchor spec_createmon
 */
Monitor *
createmon(void)
{
	Monitor *m;

	m = ecalloc(1, sizeof(Monitor));
	m->tagset[0] = m->tagset[1] = 1;
	m->mfact = mfact;
	m->nmaster = nmaster;
	m->showbar = showbar;
	m->topbar = topbar;
	gap_copy(&m->gap, &default_gap);
	m->lt[0] = &layouts[0];
	/* 1 % LENGTH guards single-layout configs */
	m->lt[1] = &layouts[1 % LENGTH(layouts)];
	stpncpy_len(m->ltsymbol, sizeof m->ltsymbol, layouts[0].symbol, strlen(layouts[0].symbol));
	bar_mark_dirty(m, DIRTY_ALL);
	m->bar_exposed = 1;
	return m;
}

/**
 * @brief Handle a destroyed window.
 *
 * Unmanages the client that owned the window. If the destroyed window is a
 * terminal placeholder parked by swallow(), the holder client is unmanaged
 * instead.
 *
 * @param[in]  e  X DestroyWindow event.
 *
 * @note Calls wintoclient(), swallowingclient() and unmanage().
 * @see unmanage, unmapnotify, swallow
 * @anchor spec_destroynotify
 */
void
destroynotify(XEvent *e)
{
	Client *c;
	XDestroyWindowEvent *ev = &e->xdestroywindow;

	if ((c = wintoclient(ev->window)))
		unmanage(c, 1);

	else if ((c = swallowingclient(ev->window)))
		unmanage(c->swallowing, 1);
}

/**
 * @brief Remove a client from its monitor's client list.
 *
 * @param[in]  c  Client to unlink.
 *
 * @pre c->mon is a valid monitor.
 * @post c is no longer linked from c->mon->clients; c->next is left unchanged.
 * @note No-op when c is not in the list; writes the list links.
 * @warning Does not clear c->next and does not update selection or the stack.
 * @see attach, detachstack, unmanage
 * @anchor spec_detach
 */
void
detach(Client *c)
{
	Client **tc;

	for (tc = &c->mon->clients; *tc && *tc != c; tc = &(*tc)->next);
	*tc = c->next;
}

/**
 * @brief Remove a client from the focus-history stack.
 *
 * If the removed client was the monitor's selection, the most recent visible
 * entry remaining on the stack is promoted to sel.
 *
 * @param[in]  c  Client to unlink from the stack.
 *
 * @post c->mon->sel may point at a different client; c->snext is unchanged.
 * @note Writes the monitor stack and sel; reads ISVISIBLE().
 * @see attachstack, focus, unmanage
 * @anchor spec_detachstack
 */
void
detachstack(Client *c)
{
	Client **tc, *t;

	for (tc = &c->mon->stack; *tc && *tc != c; tc = &(*tc)->snext);
	*tc = c->snext;

	/* if c held the selection, promote the most recent visible stack entry */
	if (c == c->mon->sel) {
		for (t = c->mon->stack; t && !ISVISIBLE(t); t = t->snext);
		c->mon->sel = t;
	}
}

/**
 * @brief Return the next or previous monitor relative to selmon.
 *
 * Positive directions advance and wrap to the list head; non-positive
 * directions walk to the predecessor and wrap from the head to the tail.
 *
 * @param[in]  dir  Positive for next, otherwise previous.
 *
 * @return Adjacent monitor in the mons list.
 *
 * @pre selmon belongs to mons and mons is non-NULL.
 * @note Reads selmon and mons.
 * @see focusmon, tagmon, recttomon
 * @anchor spec_dirtomon
 */
Monitor *
dirtomon(int dir)
{
	Monitor *m = NULL;

	if (dir > 0) {
		if (!(m = selmon->next))
			m = mons;
	} else if (selmon == mons)
		for (m = mons; m->next; m = m->next);
	else
		for (m = mons; m->next != selmon; m = m->next);
	return m;
}

/**
 * @brief Render dirty bar segments and copy the pixmap to the bar window.
 *
 * Work is gated three ways: a hidden bar or an optimized fullscreen window
 * skips everything, clean segments skip text rendering and only re-copy the
 * retained pixmap when the window was exposed, and per-segment dirty bits limit
 * drawing to the status, tag and title areas that changed. Status is drawn first
 * so tags can overdraw it.
 *
 * @param[in]  m  Monitor whose bar is drawn.
 *
 * @pre m->barwin exists, drw and drw->fonts are valid, and bh is initialized.
 * @post The bar pixmap matches the window; dirty segments are cleared except under DWM_TEST, and bar_exposed is cleared.
 * @note Reads stext, tags[], scheme[], selmon and m->clients; calls drw_setscheme(), drw_text(), drw_rect() and drw_map() (Xft/X11); mutates bar_dirty_segments and bar_exposed.
 * @warning Accesses drw->fonts->h before the showbar/fullscreen early returns, so drw->fonts must be valid even for callers that end up drawing nothing.
 * @see drawbars, updatestatus, focus, setlayout
 * @anchor spec_drawbar
 */
void
drawbar(Monitor *m)
{
	int x, w, tw = 0;
	int boxs = drw->fonts->h / 9;
	int boxw = drw->fonts->h / 6 + 2;
	unsigned int i, occ = 0, urg = 0;
	Client *c;

	if (!m->showbar)
		return;
	/* skip all bar work when a fullscreen window is focused — content invisible to user */
	if (optimizefullscreen && m->sel && m->sel->isfullscreen)
		return;
	/* skip full re-render when content hasn't changed (expose/restack trivially copy pixmap).
	 * Each monitor tracks its own dirty segments independently. */
	/* skip drw_map when window wasn't exposed since last draw — pixmap still matches screen */
	if (!m->bar_dirty_segments) {
		if (m->bar_exposed)
			drw_map(drw, m->barwin, 0, 0, m->ww, bh);
		m->bar_exposed = 0;
		return;
	}

	/* draw status first so it can be overdrawn by tags later */
	if (m == selmon) { /* status is only drawn on selected monitor */
		if (m->bar_dirty_segments & (DIRTY_STATUS | DIRTY_TITLE))
			tw = TEXTW(stext) - lrpad + 2; /* needed for title width calculation */
		if (m->bar_dirty_segments & DIRTY_STATUS) {
			drw_setscheme(drw, scheme[SchemeNorm]);
			drw_text(drw, m->ww - tw, 0, tw, bh, 0, stext, 0);
		}
	}

	for (c = m->clients; c; c = c->next) {
		occ |= c->tags;
		if (c->isurgent)
			urg |= c->tags;
	}
	x = 0;
	for (i = 0; i < LENGTH(tags); i++) {
		w = TEXTW(tags[i]);
		if (m->bar_dirty_segments & DIRTY_TAGS) {
			drw_setscheme(drw, scheme[m->tagset[m->seltags] & 1 << i ? SchemeSel : SchemeNorm]);
			drw_text(drw, x, 0, w, bh, lrpad / 2, tags[i], urg & 1 << i);
			if (occ & 1 << i)
				drw_rect(drw, x + boxs, boxs, boxw, boxw,
					m == selmon && selmon->sel && selmon->sel->tags & 1 << i,
					urg & 1 << i);
		}
		x += w;
	}
	/* w = TEXTW(m->ltsymbol); */
	drw_setscheme(drw, scheme[SchemeNorm]);
	/* x = drw_text(drw, x, 0, w, bh, lrpad / 2, m->ltsymbol, 0); */

	if ((w = m->ww - tw - x) > bh) {
		if (m->bar_dirty_segments & DIRTY_TITLE) {
			if (m->sel) {
				drw_setscheme(drw, scheme[m == selmon ? SchemeSel : SchemeNorm]);
				drw_text(drw, x, 0, w, bh, lrpad / 2, m->sel->name, 0);
				if (m->sel->isfloating)
					drw_rect(drw, x + boxs, boxs, boxw, boxw, m->sel->isfixed, 0);
			} else {
				drw_setscheme(drw, scheme[SchemeNorm]);
				drw_rect(drw, x, 0, w, bh, 1, 1);
			}
		}
	}
	drw_map(drw, m->barwin, 0, 0, m->ww, bh);
	m->bar_exposed = 0;             /* pixmap now matches screen; no copy needed until next expose */
	/* content now matches pixmap; skip next expose/restack */
	if (bar_clear_on_draw)
		m->bar_dirty_segments = 0; /* tests keep the mask observable */
}

/**
 * @brief Draw every monitor's bar.
 *
 * @note Iterates mons and calls drawbar(), which performs Xft/X11 rendering.
 * @see drawbar, flusheventtail
 * @anchor spec_drawbars
 */
void
drawbars(void)
{
	Monitor *m;

	for (m = mons; m; m = m->next)
		drawbar(m);
}

/**
 * @brief Implement focus-follows-pointer on enter events.
 *
 * Ignores grab-initiated crossings and inferior moves unless the crossing is
 * on the root window. Resolves the client and its monitor, switches the selected
 * monitor when needed, and focuses the entered client.
 *
 * @param[in]  e  X EnterNotify event.
 *
 * @note May mutate selmon via unfocus()/focus(); reads root and mons; calls wintoclient() and wintomon().
 * @see focus, motionnotify, wintomon
 * @anchor spec_enternotify
 */
void
enternotify(XEvent *e)
{
	Client *c;
	Monitor *m;
	XCrossingEvent *ev = &e->xcrossing;

	/* ignore grab-initiated crossings and moves within our own hierarchy,
	 * except root crossings which drive cross-monitor tracking */
	if ((ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root)
		return;
	c = wintoclient(ev->window);
	m = c ? c->mon : wintomon(ev->window);
	if (m != selmon) {
		unfocus(selmon->sel, 1);
		selmon = m;
	} else if (!c || c == selmon->sel)
		return;
	focus(c);
}

/**
 * @brief Redraw a bar after its window contents were lost.
 *
 * On the last Expose of a batch it marks the monitor's bar as exposed and
 * redraws immediately rather than deferring.
 *
 * @param[in]  e  X Expose event.
 *
 * @note Writes m->bar_exposed; calls drawbar() (Xft/X11).
 * @see drawbar, drawbars
 * @anchor spec_expose
 */
void
expose(XEvent *e)
{
	Monitor *m;
	XExposeEvent *ev = &e->xexpose;

	if (ev->count == 0 && (m = wintomon(ev->window))) {
		m->bar_exposed = 1;         /* window content lost; pixmap copy required */
		drawbar(m);              /* always draw immediately on expose — no deferral */
	}
}

/**
 * @brief Make a client the selection, resolving NULL or invisible picks.
 *
 * Falls back to the newest visible client on the focus stack when the
 * requested client is NULL or hidden. Unfocuses the previous selection, switches
 * selmon to the client's monitor, clears urgency, refreshes stack position,
 * button grabs, border and X input focus. A NULL result clears root input focus
 * and _NET_ACTIVE_WINDOW. The bar is dirtied only when the selection actually
 * changed.
 *
 * @param[in]  c  Desired client, or NULL to clear focus.
 *
 * @post selmon may change; selmon->sel is the resolved client (possibly NULL); bar_draw_pending is set.
 * @note Mutates selmon, selmon->sel, the focus stack, bar_dirty_segments and bar_draw_pending; calls seturgent(), grabbuttons(), XSetWindowBorder(), setfocus() and setfocus(NULL) X11 calls.
 * @warning selmon must point to a valid monitor.
 * @see unfocus, setfocus, seturgent, restack
 * @anchor spec_focus
 */
void
focus(Client *c)
{
	Monitor *oldmon = selmon;

	if (!c || !ISVISIBLE(c))
		for (c = selmon->stack; c && !ISVISIBLE(c); c = c->snext);
	if (selmon->sel && selmon->sel != c)
		unfocus(selmon->sel, 0);
	if (c) {
		if (c->mon != selmon)
			selmon = c->mon;
		if (c->isurgent)
			seturgent(c, 0);
		detachstack(c);
		attachstack(c);
		grabbuttons(c, 1);
		XSetWindowBorder(dpy, c->win, scheme[SchemeSel][ColBorder].pixel);
		setfocus(c);
	} else {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
	/* A monitor switch must repaint status on the newly selected monitor. */
	if (selmon != oldmon)
		bar_mark_dirty(selmon, DIRTY_STATUS | DIRTY_TITLE);
	/* only dirty bar if focus actually changed — skip redundant redraws */
	if (c != selmon->sel)
		bar_mark_dirty(selmon, DIRTY_TITLE | DIRTY_TAGS);
	selmon->sel = c;
	bar_draw_pending = 1;
}

/**
 * @brief Reassert dwm's focus when a broken client steals it.
 *
 * @param[in]  e  X FocusIn event.
 *
 * @note Calls setfocus(); reads selmon.
 * @see focus, setfocus
 * @anchor spec_focusin
 */
void
focusin(XEvent *e)
{
	XFocusChangeEvent *ev = &e->xfocus;

	if (selmon->sel && ev->window != selmon->sel->win)
		setfocus(selmon->sel);
}

/**
 * @brief Select the monitor adjacent to selmon and carry focus along.
 *
 * @param[in]  arg  arg->i selects the direction.
 *
 * @note Mutates selmon; calls unfocus() and focus(); no-op for a single monitor or when the target is already selected.
 * @see dirtomon, focus
 * @anchor spec_focusmon
 */
void
focusmon(const Arg *arg)
{
	Monitor *m;

	if (!mons->next)
		return;
	if ((m = dirtomon(arg->i)) == selmon)
		return;
	unfocus(selmon->sel, 0);
	selmon = m;
	focus(NULL);
}

/**
 * @brief Focus the next or previous visible client, wrapping around.
 *
 * @param[in]  arg  arg->i > 0 selects next, otherwise previous.
 *
 * @note Calls focus() and restack(); skips work when nothing is selected or a locked fullscreen client blocks focus changes.
 * @see focus, nexttiled, pop
 * @anchor spec_focusstack
 */
void
focusstack(const Arg *arg)
{
	Client *c = NULL, *i;

	if (!selmon->sel || (selmon->sel->isfullscreen && lockfullscreen))
		return;
	if (arg->i > 0) {
		for (c = selmon->sel->next; c && !ISVISIBLE(c); c = c->next);
		if (!c)
			for (c = selmon->clients; c && !ISVISIBLE(c); c = c->next);
	} else {
		for (i = selmon->clients; i != selmon->sel; i = i->next)
			if (ISVISIBLE(i))
				c = i;
		if (!c)
			for (; i; i = i->next)
				if (ISVISIBLE(i))
					c = i;
	}
	if (c) {
		focus(c);
		restack(selmon);
	}
}

/**
 * @brief Read an Atom-typed property from a client window.
 *
 * @param[in]  c  Client whose window is queried; c->win must be valid.
 * @param[in]  prop  Property atom.
 *
 * @return The Atom value, or None when the property is absent or not an Atom.
 *
 * @note Calls XGetWindowProperty() and XFree().
 * @see getstate, gettextprop, updatewindowtype
 * @anchor spec_getatomprop
 */
Atom
getatomprop(Client *c, Atom prop)
{
	int di;
	unsigned long dl;
	unsigned char *p = NULL;
	Atom da, atom = None;

	if (XGetWindowProperty(dpy, c->win, prop, 0L, sizeof atom, False, XA_ATOM,
		&da, &di, &dl, &dl, &p) == Success && p) {
		atom = *(Atom *)p;
		XFree(p);
	}
	return atom;
}

/**
 * @brief Query the root-relative pointer position.
 *
 * @param[out]  x  Receives the pointer X coordinate on success.
 * @param[out]  y  Receives the pointer Y coordinate on success.
 *
 * @return True on success, False on failure.
 *
 * @note Calls XQueryPointer().
 * @see wintomon, motionnotify, movemouse
 * @anchor spec_getrootptr
 */
int
getrootptr(int *x, int *y)
{
	int di;
	unsigned int dui;
	Window dummy;

	return XQueryPointer(dpy, root, &dummy, &dummy, x, y, &di, &di, &dui);
}

/**
 * @brief Read a window's WM_STATE property.
 *
 * @param[in]  w  Window to query.
 *
 * @return The state value (for example NormalState or IconicState), or -1 on error or absent property.
 *
 * @note Calls XGetWindowProperty() and XFree().
 * @see setclientstate, scan, unmapnotify
 * @anchor spec_getstate
 */
long
getstate(Window w)
{
	int format;
	long result = -1;
	unsigned char *p = NULL;
	unsigned long n, extra;
	Atom real;

	if (unlikely(XGetWindowProperty(dpy, w, wmatom[WMState], 0L, 2L, False, wmatom[WMState],
		&real, &format, &n, &extra, (unsigned char **)&p) != Success))
		return -1;
	if (n != 0)
		result = *p;
	XFree(p);
	return result;
}

/**
 * @brief Fetch a text property into a caller-owned buffer.
 *
 * Handles XA_STRING directly and compound-text through XmbTextPropertyToTextList,
 * always NUL-terminating and truncating to the supplied capacity.
 *
 * @param[in]  w  Window to query.
 * @param[in]  atom  Text property atom.
 * @param[out]  text  Destination buffer.
 * @param[in]  size  Capacity of text in bytes.
 *
 * @return Number of bytes written excluding the NUL terminator, or 0 on failure or empty property.
 *
 * @pre text != NULL and size > 0; otherwise the function returns 0 immediately.
 * @post text is NUL-terminated; on failure text[0] is set to NUL.
 * @note Calls XGetTextProperty(), XmbTextPropertyToTextList(), XFree() and XFreeStringList(); uses stpncpy_len().
 * @warning The returned length excludes the terminator; the buffer remains owned by the caller.
 * @see updatetitle, updatestatus
 * @anchor spec_gettextprop
 */
size_t
gettextprop(Window w, Atom atom, char *text, unsigned int size)
{
	char **list = NULL;
	int n;
	XTextProperty name;
	size_t len;

	name.value = NULL; /* lets the empty-result path free safely */
	if (!text || size == 0)
		return 0;
	text[0] = '\0';
	if (!XGetTextProperty(dpy, w, &name, atom) || !name.nitems) {
		/* nitems == 0 still comes with an allocated (empty) value */
		if (name.value)
			XFree(name.value);
		return 0;
	}
	if (name.encoding == XA_STRING) {
		len = stpncpy_len(text, size, (char *)name.value, name.nitems) - text;
	} else if (XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success && n > 0 && *list) {
		len = stpncpy_len(text, size, *list, strlen(*list)) - text;
		XFreeStringList(list);
	} else {
		XFree(name.value);
		return 0;
	}
	XFree(name.value);
	return len;
}

/**
 * @brief Grab or release mouse buttons on a client window.
 *
 * With focused=1 it grabs the configured ClkClientWin bindings across the
 * normal lock-modifier combinations. With focused=0 it grabs AnyButton in
 * synchronous mode so buttonpress() can classify and replay the click.
 *
 * @param[in]  c  Client whose window is grabbed.
 * @param[in]  focused  Non-zero for normal bindings, zero for replay grabs.
 *
 * @note Calls updatenumlockmask(), XUngrabButton() and XGrabButton(); reads buttons[], numlockmask and LockMask.
 * @see buttonpress, manage, unfocus, updatenumlockmask
 * @anchor spec_grabbuttons
 */
void
grabbuttons(Client *c, int focused)
{
	updatenumlockmask();
	{
		unsigned int i, j;
		unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		if (!focused)
			XGrabButton(dpy, AnyButton, AnyModifier, c->win, False,
				BUTTONMASK, GrabModeSync, GrabModeSync, None, None);
		for (i = 0; i < LENGTH(buttons); i++)
			if (buttons[i].click == ClkClientWin)
				for (j = 0; j < LENGTH(modifiers); j++)
					XGrabButton(dpy, buttons[i].button,
						buttons[i].mask | modifiers[j],
						c->win, False, BUTTONMASK,
						GrabModeAsync, GrabModeSync, None, None);
	}
}

/**
 * @brief Grab all configured key combinations on the root window.
 *
 * Refreshes NumLock state, rebuilds the exact keypress index, ungrabs all
 * keys, obtains the keycode-to-keysym map and grabs each matching keycode for
 * every lock-modifier variant.
 *
 * @pre dpy and root are initialized.
 * @note Calls updatenumlockmask(), cachekeys(), XUngrabKey(), XDisplayKeycodes(), XGetKeyboardMapping(), XGrabKey() and XFree(); reads keys[].
 * @see keypress, mappingnotify, updatenumlockmask
 * @anchor spec_grabkeys
 */
void
grabkeys(void)
{
	updatenumlockmask();
	/* CLEANMASK results changed with numlockmask; rebuild the exact
	 * keypress() index so it stays in sync with lock-key state */
	cachekeys();
	{
		unsigned int i, j, k;
		unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
		int start, end, skip;
		KeySym *syms;

		XUngrabKey(dpy, AnyKey, AnyModifier, root);
		XDisplayKeycodes(dpy, &start, &end);
		syms = XGetKeyboardMapping(dpy, start, end - start + 1, &skip);
		if (unlikely(!syms))
			return;
		for (k = start; k <= end; k++)
			for (i = 0; i < LENGTH(keys); i++) {
				/* skip modifier codes, we do that ourselves */
				if (keys[i].keysym == syms[(k - start) * skip])
					for (j = 0; j < LENGTH(modifiers); j++)
						XGrabKey(dpy, k,
							 keys[i].mod | modifiers[j],
							 root, True,
							 GrabModeAsync, GrabModeAsync);
			}
		XFree(syms);
	}
}

/**
 * @brief Adjust the selected monitor's master-window count.
 *
 * @param[in]  arg  arg->i is the signed delta.
 *
 * @note Writes selmon->nmaster clamped to at least zero; calls arrange().
 * @see tile, setmfact
 * @anchor spec_incnmaster
 */
void
incnmaster(const Arg *arg)
{
	selmon->nmaster = MAX(selmon->nmaster + arg->i, 0);
	arrange(selmon);
}

#ifdef XINERAMA
/**
 * @brief Test whether an Xinerama geometry has not been seen yet.
 *
 * @param[in]  unique  Array of already-accepted geometries.
 * @param[in]  n  Number of entries in unique.
 * @param[in]  info  Candidate geometry.
 *
 * @return 1 when the geometry is new, 0 when it duplicates an existing entry.
 *
 * @note Compiled only when XINERAMA is defined; reads no globals.
 * @see updategeom
 * @anchor spec_isuniquegeom
 */
static int
isuniquegeom(XineramaScreenInfo *unique, size_t n, XineramaScreenInfo *info)
{
	while (n) {
		n--;
		if (unique[n].x_org == info->x_org && unique[n].y_org == info->y_org
		&& unique[n].width == info->width && unique[n].height == info->height)
			return 0;
	}
	return 1;
}
#endif /* XINERAMA */

/**
 * @brief Dispatch a key event to a matching keys[] binding.
 *
 * The iskeybound() fast path rejects events that cannot match any configured
 * keysym and modifier before walking the binding table; a full exact match then
 * invokes the binding function.
 *
 * @param[in]  e  X KeyPress event.
 *
 * @note Calls XKeycodeToKeysym() and iskeybound(); reads keys[] and invokes the bound function.
 * @see iskeybound, grabkeys, cachekeys
 * @anchor spec_keypress
 */
void
keypress(XEvent *e)
{
	unsigned int i;
	KeySym keysym;
	XKeyEvent *ev;

	ev = &e->xkey;
	keysym = XKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0);
	if (!iskeybound(keysym, ev->state))
		return;
	for (i = 0; i < LENGTH(keys); i++)
		if (keysym == keys[i].keysym
		&& CLEANMASK(keys[i].mod) == CLEANMASK(ev->state)
		&& keys[i].func)
			keys[i].func(&(keys[i].arg));
}

/**
 * @brief Ask the selected client to close.
 *
 * Tries the WM_DELETE_WINDOW protocol first. If the client does not support it,
 * the X connection is grabbed, errors are suppressed, and the client is killed
 * with XKillClient().
 *
 * @param[in]  arg  Unused.
 *
 * @note Reads selmon->sel; calls sendevent(), XGrabServer(), XSetErrorHandler(), XSetCloseDownMode(), XKillClient(), XSync() and XUngrabServer().
 * @see sendevent, unmanage
 * @anchor spec_killclient
 */
void
killclient(const Arg *arg)
{
	if (!selmon->sel)
		return;
	if (unlikely(!sendevent(selmon->sel, wmatom[WMDelete]))) {
		XGrabServer(dpy);
		error_guard_begin();
		XSetCloseDownMode(dpy, DestroyAll);
		XKillClient(dpy, selmon->sel->win);
		error_guard_end();
		XUngrabServer(dpy);
	}
}

/**
 * @brief Adopt a new window as a managed client.
 *
 * Allocates a Client, copies geometry and title, inherits monitor and tags
 * from a transient parent or applies rules, clamps the initial position to the
 * workarea, sets the border, refreshes type/size/WM hints, selects input events,
 * grabs buttons, applies floating defaults, indexes the window id, links the
 * client into the monitor lists, appends to _NET_CLIENT_LIST, maps it, swallows
 * it into a matching terminal when possible, and focuses it.
 *
 * @param[in]  w  X window id to adopt.
 * @param[in]  wa  Attributes from XGetWindowAttributes() for w.
 *
 * @pre w is not already managed and wa describes a valid window.
 * @post The client is in its monitor's lists and the window hash, mapped, selected and focused; _NET_CLIENT_LIST includes w.
 * @note Allocates memory; mutates winhash, selmon and monitor lists/tags/bar state; almost every X11 setup call; may call swallow().
 * @warning The client is owned by dwm and must be released through unmanage(); wa must remain valid for the duration of the call.
 * @see unmanage, applyrules, swallow, winclient_put
 * @anchor spec_manage
 */
void
manage(Window w, XWindowAttributes *wa)
{
	Client *c, *t = NULL, *term = NULL;
	Window trans = None;
	XWindowChanges wc;

	c = ecalloc(1, sizeof(Client));
	c->state = CST_LISTED;
	c->win = w;
	c->pid = winpid(w);
	/* geometry */
	c->x = c->oldx = wa->x;
	c->y = c->oldy = wa->y;
	c->w = c->oldw = wa->width;
	c->h = c->oldh = wa->height;
	c->oldbw = wa->border_width;

	updatetitle(c);
	if (XGetTransientForHint(dpy, w, &trans) && (t = wintoclient(trans))) {
		c->mon = t->mon;
		c->tags = t->tags;
	} else {
		c->mon = selmon;
		applyrules(c);
		term = termforwin(c);
	}

	if (c->x + WIDTH(c) > c->mon->wx + c->mon->ww)
		c->x = c->mon->wx + c->mon->ww - WIDTH(c);
	if (c->y + HEIGHT(c) > c->mon->wy + c->mon->wh)
		c->y = c->mon->wy + c->mon->wh - HEIGHT(c);
	c->x = MAX(c->x, c->mon->wx);
	c->y = MAX(c->y, c->mon->wy);
	c->bw = borderpx;

	wc.border_width = c->bw;
	XConfigureWindow(dpy, w, CWBorderWidth, &wc);
	XSetWindowBorder(dpy, w, scheme[SchemeNorm][ColBorder].pixel);
	configure(c); /* propagates border_width, if size doesn't change */
	updatewindowtype(c);
	updatesizehints(c);
	updatewmhints(c);
	XSelectInput(dpy, w, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask);
	grabbuttons(c, 0);
	if (!c->isfloating) {
		c->transient_floating = trans != None || c->isfixed;
		c->isfloating = c->oldstate = c->transient_floating;
	}
	if (c->isfloating)
		XRaiseWindow(dpy, c->win);
	winclient_put(c); /* index for O(1) wintoclient lookups */
	attach(c);
	attachstack(c);
	XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32, PropModeAppend,
		(unsigned char *) &(c->win), 1);
	XMoveResizeWindow(dpy, c->win, c->x + 2 * sw, c->y, c->w, c->h); /* some windows require this */
	setclientstate(c, NormalState);
	if (c->mon == selmon)
		unfocus(selmon->sel, 0);
	c->mon->sel = c;
	/* creation path: lay out immediately (bypasses coalescing) so the
	 * visible spawn->tile transition keeps its animation frames */
	arrangenow(c->mon);
	XMapWindow(dpy, c->win);
	if (term)
		swallow(term, c);
	focus(NULL);
}

/**
 * @brief Refresh keyboard mapping and re-grab keys after a layout change.
 *
 * @param[in]  e  X MappingNotify event.
 *
 * @note Calls XRefreshKeyboardMapping(); calls grabkeys() when a keyboard mapping change is reported.
 * @see grabkeys, keypress
 * @anchor spec_mappingnotify
 */
void
mappingnotify(XEvent *e)
{
	XMappingEvent *ev = &e->xmapping;

	XRefreshKeyboardMapping(ev);
	if (ev->request == MappingKeyboard)
		grabkeys();
}

/**
 * @brief Adopt a newly mapped window.
 *
 * Windows with override_redirect set and windows already managed are ignored;
 * everything else is passed to manage().
 *
 * @param[in]  e  X MapRequest event.
 *
 * @note Calls XGetWindowAttributes(), wintoclient() and manage().
 * @see manage, scan
 * @anchor spec_maprequest
 */
void
maprequest(XEvent *e)
{
	static XWindowAttributes wa;
	XMapRequestEvent *ev = &e->xmaprequest;

	if (unlikely(!XGetWindowAttributes(dpy, ev->window, &wa)))
		return;
	if (wa.override_redirect)
		return;
	if (!wintoclient(ev->window))
		manage(ev->window, &wa);
}

/**
 * @brief Monocle layout: every visible tiled client fills the monitor.
 *
 * Counts visible clients, updates the layout symbol with that count, and
 * resizes each tiled client to the full workarea.
 *
 * @param[in]  m  Monitor to lay out.
 *
 * @note Writes m->ltsymbol; calls resize() (X11); skips floating and invisible clients.
 * @see tile, arrangemon
 * @anchor spec_monocle
 */
void
monocle(Monitor *m)
{
	unsigned int n = 0;
	Client *c;

	for (c = m->clients; c; c = c->next)
		if (ISVISIBLE(c))
			n++;
	if (n > 0) /* override layout symbol */
		snprintf(m->ltsymbol, sizeof m->ltsymbol, "[%d]", n);
	for (c = nexttiled(m->clients); c; c = nexttiled(c->next))
		resize(c, m->wx, m->wy, m->ww - 2 * c->bw, m->wh - 2 * c->bw, 0);
}

/**
 * @brief Switch the selected monitor when the pointer crosses monitors.
 *
 * Tracks root-window motion, maps the pointer position to a monitor and
 * focuses that monitor when it changes. Single-monitor setups return
 * immediately.
 *
 * @param[in]  e  X MotionNotify event.
 *
 * @note Mutates selmon; calls unfocus(), focus() and recttomon(); reads mons.
 * @see enternotify, recttomon
 * @anchor spec_motionnotify
 */
void
motionnotify(XEvent *e)
{
	static Monitor *mon = NULL;
	Monitor *m;
	XMotionEvent *ev = &e->xmotion;

	if (!mons || !mons->next) /* single monitor: no need to track cross-monitor moves */
		return;
	if (ev->window != root)
		return;
	if ((m = recttomon(ev->x_root, ev->y_root, 1, 1)) != mon && mon) {
		unfocus(selmon->sel, 1);
		selmon = m;
		focus(NULL);
	}
	mon = m;
}

/**
 * @brief Acquire the pointer grab and enter the drag state machine.
 *
 * On failure the state remains DRAG_IDLE, so every later path can call
 * drag_end() unconditionally without releasing someone else's grab.
 *
 * @param[in] c      Client to drag; must be non-NULL and live.
 * @param[in] shape  Cursor shape to display during the drag.
 * @return 1 on success, 0 when the grab failed or shutdown was requested.
 *
 * @pre  drag.phase == DRAG_IDLE.
 * @post On success drag.phase == DRAG_GRABBED and pointergrab == 1.
 * @note Calls XGrabPointer; writes the drag state and pointergrab.
 * @anchor spec_drag_begin
 */
static int
drag_begin(Client *c, Cursor shape)
{
	if (!c || lifecycle != LIFE_LIVE || drag.phase != DRAG_IDLE)
		return 0;
	if (unlikely(XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync,
		GrabModeAsync, None, shape, CurrentTime) != GrabSuccess))
		return 0;
	drag.c = c;
	drag.origin = selmon;
	drag.lasttime = 0;
	drag.phase = DRAG_GRABBED;
	pointergrab = 1; /* reflow immediately inside the grab */
	return 1;
}

/**
 * @brief Leave the drag state machine and always release the pointer grab.
 *
 * Idempotent: calling it with no active drag is a no-op, so all early-return
 * and failure paths share the same cleanup.
 *
 * @post drag.phase == DRAG_IDLE and pointergrab == 0.
 * @note Calls XUngrabPointer; writes the drag state and pointergrab.
 * @anchor spec_drag_end
 */
static void
drag_end(void)
{
	if (drag.phase == DRAG_IDLE)
		return;
	drag.phase = DRAG_RELEASING;
	pointergrab = 0;
	XUngrabPointer(dpy, CurrentTime);
	drag.c = NULL;
	drag.origin = NULL;
	drag.phase = DRAG_IDLE;
}

/**
 * @brief Interactively move the selected client with the mouse.
 *
 * Grabs the pointer, warps relative to the grab point, then processes motion
 * events (throttled to about 60 per second) while snapping to workarea edges.
 * Dragging a tiled client past the snap threshold floats it. On release the
 * client is handed off to the monitor under it.
 *
 * @param[in]  arg  Unused.
 *
 * @note Calls XGrabPointer(), XMaskEvent(), XUngrabPointer(), getrootptr(), togglefloating(), resize() and sendmon(); sets pointergrab; may mutate selmon and the client's geometry/floating state.
 * @warning Returns early when nothing is selected, the client is fullscreen, or the pointer grab fails.
 * @see resizemouse, togglefloating, sendmon, recttomon
 * @anchor spec_movemouse
 */
void
movemouse(const Arg *arg)
{
	int x, y, ocx, ocy, nx, ny;
	Client *c;
	Monitor *m;
	XEvent ev;
	Time lasttime = 0;

	if (!(c = selmon->sel))
		return;
	if (c->isfullscreen) /* no support moving fullscreen windows by mouse */
		return;
	restack(selmon);
	ocx = c->x;
	ocy = c->y;
	if (!drag_begin(c, cursor[CurMove]->cursor))
		return;
	if (!getrootptr(&x, &y)) { /* grab is already owned: funnel through cleanup */
		drag_end();
		return;
	}
	drag.phase = DRAG_ACTIVE;
	while (drag.phase == DRAG_ACTIVE && lifecycle == LIFE_LIVE) {
		if (XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask|StructureNotifyMask, &ev) < 0)
			break;
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			/* throttle motion handling to ~60 events/s */
			if ((ev.xmotion.time - lasttime) <= (1000 / 60))
				continue;
			lasttime = ev.xmotion.time;

			nx = ocx + (ev.xmotion.x - x);
			ny = ocy + (ev.xmotion.y - y);
			if (abs(selmon->wx - nx) < snap)
				nx = selmon->wx;
			else if (abs((selmon->wx + selmon->ww) - (nx + WIDTH(c))) < snap)
				nx = selmon->wx + selmon->ww - WIDTH(c);
			if (abs(selmon->wy - ny) < snap)
				ny = selmon->wy;
			else if (abs((selmon->wy + selmon->wh) - (ny + HEIGHT(c))) < snap)
				ny = selmon->wy + selmon->wh - HEIGHT(c);
			if (!c->isfloating && selmon->lt[selmon->sellt]->arrange
			&& (abs(nx - c->x) > snap || abs(ny - c->y) > snap))
				togglefloating(NULL);
			if (!selmon->lt[selmon->sellt]->arrange || c->isfloating)
				resize(c, nx, ny, c->w, c->h, 1);
			break;
		}
		if (ev.type == ButtonRelease || ev.type == DestroyNotify || ev.type == UnmapNotify)
			drag.phase = DRAG_RELEASING;
	}
	drag_end();
	if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
		sendmon(c, m);
		selmon = m;
		focus(NULL);
	}
}

/**
 * @brief Return the next tiled client starting from c.
 *
 * Skips floating and invisible clients in the client list.
 *
 * @param[in]  c  Starting point, possibly NULL.
 *
 * @return First non-floating visible client at or after c, or NULL.
 *
 * @note Reads ISVISIBLE().
 * @see tile, monocle, zoom
 * @anchor spec_nexttiled
 */
Client *
nexttiled(Client *c)
{
	for (; c && (c->isfloating || !ISVISIBLE(c)); c = c->next);
	return c;
}

/**
 * @brief Move a client to the front of its monitor's client list and focus it.
 *
 * @param[in]  c  Client to promote.
 *
 * @note Calls detach(), attach(), focus() and arrange(); mutates list order and selection.
 * @see zoom, focus, attach
 * @anchor spec_pop
 */
void
pop(Client *c)
{
	detach(c);
	attach(c);
	focus(c);
	arrange(c->mon);
}

/**
 * @brief Handle X property changes relevant to dwm.
 *
 * Applies a cheap atom filter before any list walk. Root WM_NAME updates the
 * status text; per-client transient-for, normal-hints, WM_HINTS, name and window
 * type changes update client state and mark the relevant bar segments dirty,
 * deferring bar redraws to the event-loop tail.
 *
 * @param[in]  e  X PropertyNotify event.
 *
 * @note Reads selmon and stext; writes client fields plus bar_dirty_segments and bar_draw_pending; may call arrange(); uses updatestatus(), updatetitle(), updatewmhints() and updatewindowtype().
 * @see updatestatus, updatetitle, updatewmhints, updatewindowtype
 * @anchor spec_propertynotify
 */
void
propertynotify(XEvent *e)
{
	Client *c;
	Window trans;
	XPropertyEvent *ev = &e->xproperty;

	/* cheap atom filter: bail out before any list walk or X11 call for
	 * properties no handler below consumes.  Status scripts and apps
	 * churn exotic atoms at high frequency; this keeps them O(1). */
	if (ev->atom != XA_WM_NAME &&
	    ev->atom != XA_WM_TRANSIENT_FOR &&
	    ev->atom != XA_WM_NORMAL_HINTS &&
	    ev->atom != XA_WM_HINTS &&
	    ev->atom != netatom[NetWMName] &&
	    ev->atom != netatom[NetWMWindowType])
		return;

	if ((ev->window == root) && (ev->atom == XA_WM_NAME)) {
		if (!(optimizefullscreen && selmon->sel && selmon->sel->isfullscreen))
			updatestatus();
	} else if (ev->state == PropertyDelete)
		return; /* ignore */
	else if ((c = wintoclient(ev->window))) {
		switch(ev->atom) {
		default: break;
		case XA_WM_TRANSIENT_FOR:
			if (!c->isfloating && (XGetTransientForHint(dpy, c->win, &trans)) &&
				(c->isfloating = (wintoclient(trans)) != NULL))
				arrange(c->mon);
			break;
		case XA_WM_NORMAL_HINTS:
			c->hintsvalid = 0;
			break;
		case XA_WM_HINTS:
			updatewmhints(c);
		/* urgency indicators in tag rects changed */
		bar_mark_dirty(c->mon, DIRTY_TAGS);
		bar_draw_pending = 1;    /* defer to event-loop tail for coalescing */
			break;
		}
		if (ev->atom == XA_WM_NAME || ev->atom == netatom[NetWMName]) {
			updatetitle(c);
			if (c == c->mon->sel) {
				/* selected client's window title changed */
				bar_mark_dirty(c->mon, DIRTY_TITLE);
				bar_draw_pending = 1;    /* defer to event-loop tail */
			}
		}
		if (ev->atom == netatom[NetWMWindowType])
			updatewindowtype(c);
	}
}

/**
 * @brief Move the process lifecycle from LIVE to QUIT or RESTART.
 *
 * The first request wins: a later SIGTERM cannot cancel a pending restart,
 * and a SIGTERM after QUIT is ignored. This keeps the state machine free of
 * contradictory (running=0, restart=1-then-cleared) combinations.
 *
 * @param[in]  arg  arg->i non-zero requests an exec restart.
 *
 * @pre  Called from the main loop or from a signal handler.
 * @post `lifecycle` is LIFE_QUIT or LIFE_RESTART.
 * @note Writes the single `volatile sig_atomic_t lifecycle` global.
 * @see run, cleanup, sighup, sigterm
 * @anchor spec_quit
 */
void
quit(const Arg *arg)
{
	if (lifecycle != LIFE_LIVE)
		return;
	lifecycle = arg->i ? LIFE_RESTART : LIFE_QUIT;
}

/**
 * @brief Return the monitor with the largest overlap with a rectangle.
 *
 * @param[in]  x  Rectangle X coordinate.
 * @param[in]  y  Rectangle Y coordinate.
 * @param[in]  w  Rectangle width.
 * @param[in]  h  Rectangle height.
 *
 * @return Best-overlapping monitor, or selmon when no monitor overlaps.
 *
 * @note Reads mons and selmon; uses the INTERSECT macro.
 * @see wintomon, motionnotify, movemouse
 * @anchor spec_recttomon
 */
Monitor *
recttomon(int x, int y, int w, int h)
{
	Monitor *m, *r = selmon;
	int a, area = 0;

	for (m = mons; m; m = m->next)
		if ((a = INTERSECT(x, y, w, h, m)) > area) {
			area = a;
			r = m;
		}
	return r;
}

/**
 * @brief Move or resize a client after clamping to its size hints.
 *
 * @param[in]  c  Client to resize.
 * @param[in]  x  Requested X coordinate.
 * @param[in]  y  Requested Y coordinate.
 * @param[in]  w  Requested width.
 * @param[in]  h  Requested height.
 * @param[in]  interact  Non-zero for mouse-driven geometry.
 *
 * @note Calls applysizehints() and, only when the geometry changed, resizeclient() (XConfigureWindow/XSendEvent/XSync).
 * @see applysizehints, resizeclient
 * @anchor spec_resize
 */
void
resize(Client *c, int x, int y, int w, int h, int interact)
{
	if (applysizehints(c, &x, &y, &w, &h, interact))
		resizeclient(c, x, y, w, h);
}

/**
 * @brief Apply geometry unconditionally and notify the client.
 *
 * @param[in]  c  Client to configure.
 * @param[in]  x  New X coordinate.
 * @param[in]  y  New Y coordinate.
 * @param[in]  w  New width.
 * @param[in]  h  New height.
 *
 * @note Writes c->oldx/oldy/oldw/oldh and c->x/y/w/h; calls XConfigureWindow(), configure() and XSync().
 * @warning Bypasses size hints; use resize() when hints must be honored.
 * @see resize, configure, setfullscreen
 * @anchor spec_resizeclient
 */
void
resizeclient(Client *c, int x, int y, int w, int h)
{
	XWindowChanges wc;

	c->oldx = c->x; c->x = wc.x = x;
	c->oldy = c->y; c->y = wc.y = y;
	c->oldw = c->w; c->w = wc.width = w;
	c->oldh = c->h; c->h = wc.height = h;
	wc.border_width = c->bw;
	XConfigureWindow(dpy, c->win, CWX|CWY|CWWidth|CWHeight|CWBorderWidth, &wc);
	configure(c);
	XSync(dpy, False);
}

/**
 * @brief Interactively resize the selected client from the bottom-right.
 *
 * Grabs the pointer, warps to the client's bottom-right corner, then processes
 * throttled motion events while applying size hints. Dragging a tiled client
 * past the snap threshold floats it. On release the client is handed off to the
 * monitor under it and pending enter events are drained.
 *
 * @param[in]  arg  Unused.
 *
 * @note Calls XGrabPointer(), XWarpPointer(), XMaskEvent(), XCheckMaskEvent(), XUngrabPointer(), togglefloating(), resize() and sendmon(); sets pointergrab; may mutate selmon.
 * @warning Returns early when nothing is selected, the client is fullscreen, or the pointer grab fails.
 * @see movemouse, resize, togglefloating
 * @anchor spec_resizemouse
 */
void
resizemouse(const Arg *arg)
{
	int ocx, ocy, nw, nh;
	Client *c;
	Monitor *m;
	XEvent ev;
	Time lasttime = 0;

	if (!(c = selmon->sel))
		return;
	if (c->isfullscreen) /* no support resizing fullscreen windows by mouse */
		return;
	restack(selmon);
	ocx = c->x;
	ocy = c->y;
	if (!drag_begin(c, cursor[CurResize]->cursor))
		return;
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1, c->h + c->bw - 1);
	drag.phase = DRAG_ACTIVE;
	while (drag.phase == DRAG_ACTIVE && lifecycle == LIFE_LIVE) {
		if (XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask|StructureNotifyMask, &ev) < 0)
			break;
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			/* throttle motion handling to ~60 events/s */
			if ((ev.xmotion.time - lasttime) <= (1000 / 60))
				continue;
			lasttime = ev.xmotion.time;

			nw = MAX(ev.xmotion.x - ocx - 2 * c->bw + 1, 1);
			nh = MAX(ev.xmotion.y - ocy - 2 * c->bw + 1, 1);
			if (c->mon->wx + nw >= selmon->wx && c->mon->wx + nw <= selmon->wx + selmon->ww
			&& c->mon->wy + nh >= selmon->wy && c->mon->wy + nh <= selmon->wy + selmon->wh)
			{
				if (!c->isfloating && selmon->lt[selmon->sellt]->arrange
				&& (abs(nw - c->w) > snap || abs(nh - c->h) > snap))
					togglefloating(NULL);
			}
			if (!selmon->lt[selmon->sellt]->arrange || c->isfloating)
				resize(c, c->x, c->y, nw, nh, 1);
			break;
		}
		if (ev.type == ButtonRelease || ev.type == DestroyNotify || ev.type == UnmapNotify)
			drag.phase = DRAG_RELEASING;
	}
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1, c->h + c->bw - 1);
	drag_end();
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
	if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
		sendmon(c, m);
		selmon = m;
		focus(NULL);
	}
}

/**
 * @brief Fix the stacking order on a monitor.
 *
 * Raises the selected floating or non-tiled client, then chains visible tiled
 * clients directly below one another with the bar beneath them. Reordering
 * generates EnterNotify storms, which are drained before returning.
 *
 * @param[in]  m  Monitor whose stack is fixed.
 *
 * @note Writes bar_draw_pending; calls XRaiseWindow(), XConfigureWindow(), XSync() and XCheckMaskEvent().
 * @see drawbar, focus, arrange
 * @anchor spec_restack
 */
void
restack(Monitor *m)
{
	Client *c;
	XEvent ev;
	XWindowChanges wc;

	bar_draw_pending = 1;            /* bar content rendered at event-loop tail */
	if (!m->sel)
		return;
	if (m->sel->isfloating || !m->lt[m->sellt]->arrange)
		XRaiseWindow(dpy, m->sel->win);
	if (m->lt[m->sellt]->arrange) {
		wc.stack_mode = Below;
		wc.sibling = m->barwin;
		for (c = m->stack; c; c = c->snext)
			if (!c->isfloating && ISVISIBLE(c)) {
				XConfigureWindow(dpy, c->win, CWSibling|CWStackMode, &wc);
				wc.sibling = c->win;
			}
	}
	XSync(dpy, False);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
}

/**
 * @brief Run the main event loop.
 *
 * Synchronizes with the X server, then repeatedly waits for events while
 * running. Each handler runs with dispatching set so arrange() requests coalesce;
 * after each dispatch, flusheventtail() performs at most one layout pass and one
 * bar redraw.
 *
 * @note Writes dispatching; reads running; flusheventtail() clears arrange_pending/bar_draw_pending and redraws bars.
 * @see flusheventtail, handler, quit
 * @anchor spec_run
 */
void
run(void)
{
	XEvent ev;
	/* main event loop */
	XSync(dpy, False);
	/* sigaction() without SA_RESTART makes XNextEvent return -1/EINTR when
	 * a handler changes lifecycle, so the loop observes the transition. */
	while (lifecycle == LIFE_LIVE && !XNextEvent(dpy, &ev)) {
		dispatching = 1; /* handlers' arrange() calls coalesce to the tail */
		if (handler[ev.type])
			handler[ev.type](&ev); /* call handler */
		dispatching = 0;
		flusheventtail();
	}
}

/**
 * @brief Adopt windows that already exist at startup.
 *
 * Queries the root tree and manages normal visible or iconic windows first,
 * then transients, so transient parents are already managed.
 *
 * @note Calls XQueryTree(), XGetWindowAttributes(), XGetTransientForHint(), getstate(), manage() and XFree().
 * @see manage, setup
 * @anchor spec_scan
 */
void
scan(void)
{
	unsigned int i, num;
	Window d1, d2, *wins = NULL;
	XWindowAttributes wa;

	if (XQueryTree(dpy, root, &d1, &d2, &wins, &num)) {
		/* skip transients here so parents get managed first */
		for (i = 0; i < num; i++) {
			if (!XGetWindowAttributes(dpy, wins[i], &wa)
			|| wa.override_redirect || XGetTransientForHint(dpy, wins[i], &d1))
				continue;
			if (wa.map_state == IsViewable || getstate(wins[i]) == IconicState)
				manage(wins[i], &wa);
		}
		for (i = 0; i < num; i++) { /* now the transients */
			if (!XGetWindowAttributes(dpy, wins[i], &wa))
				continue;
			if (XGetTransientForHint(dpy, wins[i], &d1)
			&& (wa.map_state == IsViewable || getstate(wins[i]) == IconicState))
				manage(wins[i], &wa);
		}
		if (wins)
			XFree(wins);
	}
}

/**
 * @brief Move a client to another monitor.
 *
 * Refocuses away, unlinks the client from its old lists, assigns it the
 * target monitor's current tags, relinks it there, then refocuses and
 * rearranges.
 *
 * @param[in,out]  c  Client being moved.
 * @param[in]  m  Destination monitor.
 *
 * @pre c and m are non-NULL; a no-op when c is already on m.
 * @post c belongs to m's client and focus stacks with m's current tags; focus and layout are refreshed.
 * @note Calls unfocus(), detach(), detachstack(), attach(), attachstack(), focus() and arrange(); X11 work happens through the layout.
 * @see tagmon, movemouse, resizemouse
 * @anchor spec_sendmon
 */
void
sendmon(Client *c, Monitor *m)
{
	if (c->mon == m)
		return;
	unfocus(c, 1);
	detach(c);
	detachstack(c);
	c->mon = m;
	/* The swallow relationship is a 4-way invariant: move both endpoints. */
	if (c->swallowing)
		c->swallowing->mon = m;
	if (c->holder)
		c->holder->mon = m;
	c->tags = m->tagset[m->seltags]; /* assign tags of target monitor */
	attach(c);
	attachstack(c);
	focus(NULL);
	arrange(NULL);
}

/**
 * @brief Write a client's ICCCM WM_STATE property.
 *
 * @param[in]  c  Client whose window is updated.
 * @param[in]  state  NormalState, IconicState or WithdrawnState.
 *
 * @note Calls XChangeProperty() with the WM_STATE atom.
 * @see getstate, unmanage, swallow
 * @anchor spec_setclientstate
 */
void
setclientstate(Client *c, long state)
{
	long data[] = { state, None };

	XChangeProperty(dpy, c->win, wmatom[WMState], wmatom[WMState], 32,
		PropModeReplace, (unsigned char *)data, 2);
}

/**
 * @brief Send a WM protocol ClientMessage when the client supports it.
 *
 * @param[in]  c  Client to notify.
 * @param[in]  proto  Protocol atom to send.
 *
 * @return Non-zero when the protocol is supported and the message was sent, zero otherwise.
 *
 * @note Calls XGetWMProtocols(), XFree() and XSendEvent(); uses CurrentTime.
 * @see killclient, setfocus
 * @anchor spec_sendevent
 */
int
sendevent(Client *c, Atom proto)
{
	int n;
	Atom *protocols;
	int exists = 0;
	XEvent ev;

	if (XGetWMProtocols(dpy, c->win, &protocols, &n)) {
		while (!exists && n--)
			exists = protocols[n] == proto;
		XFree(protocols);
	}
	if (exists) {
		ev.type = ClientMessage;
		ev.xclient.window = c->win;
		ev.xclient.message_type = wmatom[WMProtocols];
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = proto;
		ev.xclient.data.l[1] = CurrentTime;
		XSendEvent(dpy, c->win, False, NoEventMask, &ev);
	}
	return exists;
}

/**
 * @brief Assert X input focus and announce the active window.
 *
 * Unless the client requested neverfocus, it receives XSetInputFocus() and
 * becomes _NET_ACTIVE_WINDOW. WM_TAKE_FOCUS is always delivered so input-less
 * clients still learn about the focus change.
 *
 * @param[in]  c  Client to focus.
 *
 * @note Calls XSetInputFocus(), XChangeProperty() and sendevent(); reads c->neverfocus, netatom[] and wmatom[].
 * @see focus, focusin, sendevent
 * @anchor spec_setfocus
 */
void
setfocus(Client *c)
{
	if (!c->neverfocus) {
		XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
		XChangeProperty(dpy, root, netatom[NetActiveWindow],
			XA_WINDOW, 32, PropModeReplace,
			(unsigned char *) &(c->win), 1);
	}
	sendevent(c, wmatom[WMTakeFocus]);
}

/**
 * @brief Run killall(1) for a process name and signal with ordered delivery.
 *
 * Forks a child that closes the X connection, starts a new session and execs
 * killall. The parent waits for pipe EOF, which proves the child exited and the
 * signal was delivered; this orders a STOP before a later CONT.
 *
 * @param[in]  name  Process name passed to killall.
 * @param[in]  signal  Signal argument passed to killall, e.g. -STOP.
 *
 * @note Calls pipe(), fork(), close(), setsid(), execlp() and read(); may close the X connection file descriptor in the child; ignores EINTR while waiting.
 * @warning Fork/pipe failure silently returns; name and signal must be valid strings.
 * @see killatfullscreen_stop, killatfullscreen_start_body
 * @anchor spec_killall
 */
void
killall(const char *name, const char *signal)
{
	int pfd[2];
	pid_t pid;
	ssize_t n;
	char b;

	if (pipe(pfd) == -1)
		return;
	if ((pid = fork()) == -1) {
		close(pfd[0]);
		close(pfd[1]);
		return;
	}
	if (pid == 0) {
		close(pfd[0]);
		if (dpy)
			close(ConnectionNumber(dpy));
		setsid();
		execlp("killall", "killall", signal, name, NULL);
		_exit(EXIT_FAILURE);
	}
	close(pfd[1]);
	/* pipe EOF means the killall child has exited, so the signal has been
	 * delivered; ignore EINTR from unrelated signal handlers */
	do
		n = read(pfd[0], &b, 1);
	while (n > 0 || (n < 0 && errno == EINTR));
	close(pfd[0]);
}

/* indirection seam: tests point this at a recorder so killall invocations
 * can be asserted without fork/exec'ing the real binary */
static void (*killall_impl)(const char *, const char *) = killall;

/**
 * @brief Return whether any managed client is fullscreen.
 *
 * @return Non-zero when at least one client has isfullscreen set, zero otherwise.
 *
 * @note Walks every monitor's client list.
 * @see setfullscreen, unmanage, main
 * @anchor spec_isanyfullscreen
 */
static int
isanyfullscreen(void)
{
	for (Monitor *m = mons; m; m = m->next)
		for (Client *c = m->clients; c; c = c->next)
			if (c->isfullscreen)
				return 1;
	return 0;
}

/**
 * @brief Suspend every configured fullscreen-blocked process.
 *
 * @note Calls killall_impl() with -STOP for each entry in killatfullscreen[].
 * @see killatfullscreen_resume, killatfullscreen_start_body
 * @anchor spec_killatfullscreen_stop
 */
void
killatfullscreen_stop(void)
{
	procstate = PROC_STOPPED;
	fs_epoch++; /* invalidate any HUP still pending from a previous session */
	for (unsigned int i = 0; i < LENGTH(killatfullscreen); i++)
		killall_impl(killatfullscreen[i], "-STOP");
}

/**
 * @brief Send CONT, wait, then HUP to configured processes.
 *
 * Sends SIGCONT to let targets resume, sleeps for a one-second grace period,
 * then sends SIGHUP to ask them to terminate. Split from the fork wrapper so
 * tests can execute it synchronously.
 *
 * @note Calls killall_impl() in order and sleep(1).
 * @see killatfullscreen_start, killatfullscreen_stop
 * @anchor spec_killatfullscreen_start_body
 */
static void
killatfullscreen_start_body(void)
{
	for (unsigned int i = 0; i < LENGTH(killatfullscreen); i++)
		killall_impl(killatfullscreen[i], "-CONT");
	sleep(1); /* let targets resume before asking them to terminate */
	/* A new fullscreen session may have begun during the grace period. */
	if (start_epoch != fs_epoch)
		return;
	procstate = PROC_RUNNING;
	for (unsigned int i = 0; i < LENGTH(killatfullscreen); i++)
		killall_impl(killatfullscreen[i], "-HUP");
}

/**
 * @brief Resume and then terminate fullscreen-suspended processes.
 *
 * Under DWM_TEST the body runs synchronously so invocations are observable;
 * in production it is forked so the grace-period sleep does not stall the event
 * loop.
 *
 * @note Depends on DWM_TEST behavior; calls fork() in the non-test build.
 * @see killatfullscreen_start_body, setfullscreen
 * @anchor spec_killatfullscreen_start
 */
void
killatfullscreen_start(void)
{
	procstate = PROC_RESTARTING;
	start_epoch = fs_epoch; /* HUP is cancelled if a new session bumps fs_epoch */
#ifdef DWM_TEST
	/* tests run the body synchronously so invocations are observable;
	 * mirrors the DWM_TEST drawbar suppression convention */
	killatfullscreen_start_body();
#else
	if (fork() == 0) {
		killatfullscreen_start_body();
		exit(EXIT_SUCCESS);
	}
#endif
}

/**
 * @brief Send only CONT to repair stale fullscreen STOPs.
 *
 * Used after a crash or restart when no fullscreen client remains. CONT on an
 * already-running process is a no-op, so the repair is unconditional.
 *
 * @note Calls killall_impl() with -CONT for each configured process.
 * @see killatfullscreen_stop, main
 * @anchor spec_killatfullscreen_resume
 */
void
killatfullscreen_resume(void)
{
	procstate = PROC_RUNNING;
	start_epoch = fs_epoch; /* any pending HUP is now obsolete */
	for (unsigned int i = 0; i < LENGTH(killatfullscreen); i++)
		killall_impl(killatfullscreen[i], "-CONT");
}

/**
 * @brief Enter or leave EWMH fullscreen on a client.
 *
 * Updates _NET_WM_STATE, saves or restores floating/border/geometry state,
 * resizes the client to its monitor, raises it, suspends or resumes the
 * killatfullscreen[] processes, forces a full bar redraw on exit, and rearranges.
 * STOP is paired with the first enter and START with the last exit so leaked
 * STOPs (destroyed while fullscreen, multiple fullscreen clients) stay
 * balanced.
 *
 * @param[in,out]  c  Managed client whose fullscreen state changes.
 * @param[in]  fullscreen  Non-zero to enter, zero to leave fullscreen.
 *
 * @post c->isfullscreen reflects the requested state; on exit the saved geometry, border and floating state are restored.
 * @note Calls isanyfullscreen(), XChangeProperty(), resizeclient(), XRaiseWindow(), killatfullscreen_stop/start() and arrange(); writes c->oldstate/oldbw/isfullscreen/isfloating/bw, bar_dirty_segments and bar_draw_pending.
 * @warning Enter and exit are idempotent; c must remain managed while the call runs.
 * @see clientmessage, updatewindowtype, togglefullscr, isanyfullscreen
 * @anchor spec_setfullscreen
 */
void
setfullscreen(Client *c, int fullscreen)
{
	if (fullscreen && !c->isfullscreen) {
		int was_any = isanyfullscreen();
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
			PropModeReplace, (unsigned char*)&netatom[NetWMFullscreen], 1);
		c->isfullscreen = 1;
		c->oldstate = c->isfloating;
		c->fs_oldfloating = c->isfloating;
		c->oldbw = c->bw;
		c->bw = 0;
		c->isfloating = 1;
		resizeclient(c, c->mon->mx, c->mon->my, c->mon->mw, c->mon->mh);
		XRaiseWindow(dpy, c->win);
		if (!was_any)
			killatfullscreen_stop();
	} else if (!fullscreen && c->isfullscreen){
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
			PropModeReplace, (unsigned char*)0, 0);
		c->isfullscreen = 0;
		/* Preserve dialog/fixed state set after a pre-map fullscreen request. */
		c->isfloating = c->fs_oldfloating || c->transient_floating;
		c->bw = c->oldbw;
		c->x = c->oldx;
		c->y = c->oldy;
		c->w = c->oldw;
		c->h = c->oldh;
		resizeclient(c, c->x, c->y, c->w, c->h);
		/* force full redraw of the bar owning this client */
		bar_mark_dirty(c->mon, DIRTY_ALL);
		bar_draw_pending = 1;
		arrange(c->mon);
		if (!isanyfullscreen())
			killatfullscreen_start();
	}
}

/**
 * @brief Copy a Gap struct field by field.
 *
 * @param[out]  to  Destination gap.
 * @param[in]  from  Source gap.
 *
 * @note Pure copy; reads no globals. Both pointers must be non-NULL.
 * @see setgaps, createmon
 * @anchor spec_gap_copy
 */
void
gap_copy(Gap *to, const Gap *from)
{
	to->isgap   = from->isgap;
	to->realgap = from->realgap;
	to->gappx   = from->gappx;
}

/**
 * @brief Toggle, reset or adjust the selected monitor's gaps.
 *
 * GAP_TOGGLE flips the enable flag, GAP_RESET restores default_gap, and any
 * other value adds arg->i to the real gap (clamped to zero) and enables gaps.
 * The effective gappx is realgap multiplied by isgap so layouts need no extra
 * checks.
 *
 * @param[in]  arg  arg->i is GAP_TOGGLE, GAP_RESET or a signed delta.
 *
 * @note Writes selmon->gap; calls arrange().
 * @see gap_copy, tile, setmfact
 * @anchor spec_setgaps
 */
void
setgaps(const Arg *arg)
{
	Gap *p = &selmon->gap;
	switch(arg->i)
	{
		case GAP_TOGGLE:
			p->isgap = 1 - p->isgap;
			break;
		case GAP_RESET:
			gap_copy(p, &default_gap);
			break;
	default:
		{
			int newgap = (int)p->realgap + arg->i;
			p->realgap = newgap > 0 ? newgap : 0;
			p->isgap = 1;
		}
	}
	p->gappx = p->realgap * p->isgap;
	arrange(selmon);
}

/**
 * @brief Select or toggle the selected monitor's layout.
 *
 * Without an argument, or when the requested layout is already active, it
 * flips to the other layout slot. The layout symbol is refreshed and the tag
 * segment is dirtied only when the effective layout changed. Clients are
 * rearranged when one is selected; otherwise only a bar redraw is scheduled.
 *
 * @param[in]  arg  Optional layout pointer in arg->v; NULL toggles slots.
 *
 * @note Writes selmon->sellt, selmon->lt and selmon->ltsymbol, plus bar_dirty_segments; calls arrange() or sets bar_draw_pending.
 * @see arrangemon, setmfact, tile
 * @anchor spec_setlayout
 */
void
setlayout(const Arg *arg)
{
	const Layout *old = selmon->lt[selmon->sellt];
	/* requesting the already-active layout flips back to the previous one */
	if (!arg || !arg->v || arg->v != selmon->lt[selmon->sellt])
		selmon->sellt ^= 1;
	if (arg && arg->v)
		selmon->lt[selmon->sellt] = (Layout *)arg->v;
	stpncpy_len(selmon->ltsymbol, sizeof selmon->ltsymbol, selmon->lt[selmon->sellt]->symbol, strlen(selmon->lt[selmon->sellt]->symbol));
	/* layout symbol in tag area changed — only dirty if layout actually changed */
	if (selmon->lt[selmon->sellt] != old)
		bar_mark_dirty(selmon, DIRTY_TAGS);
	if (selmon->sel)
		arrange(selmon);
	else
		bar_draw_pending = 1;    /* no selected client; just update bar content */
}

/**
 * @brief Set or adjust the master-area fraction.
 *
 * Values below 1.0 are relative additions to the current mfact; values of 1.0
 * or more set the fraction absolutely as arg->f - 1.0, matching the historical
 * binding convention.
 *
 * @param[in]  arg  Fraction delta or absolute value.
 *
 * @note Writes selmon->mfact; calls arrange(); requires an active layout with an arrange hook.
 * @warning Results outside [0.05, 0.95] are rejected, so some absolute values silently do nothing.
 * @see incnmaster, tile
 * @anchor spec_setmfact
 */
void
setmfact(const Arg *arg)
{
	float f;

	if (!arg || !selmon->lt[selmon->sellt]->arrange)
		return;
	f = arg->f < 1.0 ? arg->f + selmon->mfact : arg->f - 1.0;
	if (f < 0.05 || f > 0.95)
		return;
	selmon->mfact = f;
	arrange(selmon);
}

/**
 * @brief Perform one-time initialization of dwm.
 *
 * Installs child-reaping and signal handlers, initializes the screen, drawing
 * context and fonts, interns atoms, creates cursors and color schemes, builds
 * button and key caches, creates bars, publishes EWMH support/check windows,
 * selects root events, grabs keys/buttons and performs the first draw and
 * focus.
 *
 * @pre main() has already opened dpy and xcon.
 * @post Event-handling state is ready; callers proceed to scan() and run().
 * @note Allocates and initializes dpy-derived globals including root, sw, sh, drw, scheme, atoms, cursors, wmcheckwin, lrpad and bh; issues many X11 calls.
 * @warning Calls DIE when no font can be loaded.
 * @see main, run, cleanup, updategeom
 * @anchor spec_setup
 */
void
setup(void)
{
	int i;
	XSetWindowAttributes wa;
	Atom utf8string;
	struct sigaction sa;

	/* do not transform children into zombies when they terminate */
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_NOCLDSTOP | SA_NOCLDWAIT | SA_RESTART;
	sa.sa_handler = SIG_IGN;
	sigaction(SIGCHLD, &sa, NULL);

	/* clean up any zombies (inherited from .xinitrc etc) immediately */
	while (waitpid(-1, NULL, WNOHANG) > 0);

	/* No SA_RESTART: SIGHUP/SIGTERM must interrupt a blocked XNextEvent(). */
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = sighup;
	sigaction(SIGHUP, &sa, NULL);
	sa.sa_handler = sigterm;
	sigaction(SIGTERM, &sa, NULL);

	/* init screen */
	screen = DefaultScreen(dpy);
	sw = DisplayWidth(dpy, screen);
	sh = DisplayHeight(dpy, screen);
	root = RootWindow(dpy, screen);
	drw = drw_create(dpy, screen, root, sw, sh);
	if (unlikely(!drw_fontset_create(drw, fonts, LENGTH(fonts))))
		DIE("drw_fontset_create():no fonts could be loaded.");
	lrpad = drw->fonts->h * lrpad_modifier;
	bh = drw->fonts->h + 2;
	updategeom();
	/* init atoms */
	utf8string = XInternAtom(dpy, "UTF8_STRING", False);
	wmatom[WMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
	wmatom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	wmatom[WMState] = XInternAtom(dpy, "WM_STATE", False);
	wmatom[WMTakeFocus] = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
	netatom[NetActiveWindow] = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
	netatom[NetSupported] = XInternAtom(dpy, "_NET_SUPPORTED", False);
	netatom[NetWMName] = XInternAtom(dpy, "_NET_WM_NAME", False);
	netatom[NetWMState] = XInternAtom(dpy, "_NET_WM_STATE", False);
	netatom[NetWMCheck] = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
	netatom[NetWMFullscreen] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
	netatom[NetWMWindowType] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
	netatom[NetWMWindowTypeDialog] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
	netatom[NetClientList] = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
	/* init cursors */
	cursor[CurNormal] = drw_cur_create(drw, XC_left_ptr);
	cursor[CurResize] = drw_cur_create(drw, XC_sizing);
	cursor[CurMove] = drw_cur_create(drw, XC_fleur);
	/* init appearance */
	scheme = ecalloc(LENGTH(colors), sizeof(Clr *));
	for (i = 0; i < LENGTH(colors); i++)
		scheme[i] = drw_scm_create(drw, colors[i], 3);
	cachebuttons();
	cachekeys();
	/* init bars */
	updatebars();
	updatestatus();
	/* supporting window for NetWMCheck */
	wmcheckwin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMCheck], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMName], utf8string, 8,
		PropModeReplace, (unsigned char *) "dwm", 3);
	XChangeProperty(dpy, root, netatom[NetWMCheck], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);
	/* EWMH support per view */
	XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
		PropModeReplace, (unsigned char *) netatom, NetLast);
	XDeleteProperty(dpy, root, netatom[NetClientList]);
	/* select events */
	wa.cursor = cursor[CurNormal]->cursor;
	wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask
		|ButtonPressMask|PointerMotionMask|EnterWindowMask
		|LeaveWindowMask|StructureNotifyMask|PropertyChangeMask;
	XChangeWindowAttributes(dpy, root, CWEventMask|CWCursor, &wa);
	XSelectInput(dpy, root, wa.event_mask);
	grabkeys();
	drawbars();
	focus(NULL);
}

/**
 * @brief Set or clear a client's urgency flag and mirror it in WM_HINTS.
 *
 * @param[in,out]  c  Client to update.
 * @param[in]  urg  Non-zero to mark urgent, zero to clear.
 *
 * @note Writes c->isurgent; calls XGetWMHints(), XSetWMHints() and XFree(). Does not itself dirty the bar.
 * @see updatewmhints, clientmessage, focus
 * @anchor spec_seturgent
 */
void
seturgent(Client *c, int urg)
{
	XWMHints *wmh;

	c->isurgent = urg;
	if (unlikely(!(wmh = XGetWMHints(dpy, c->win))))
		return;
	wmh->flags = urg ? (wmh->flags | XUrgencyHint) : (wmh->flags & ~XUrgencyHint);
	XSetWMHints(dpy, c->win, wmh);
	XFree(wmh);
}

/**
 * @brief Place visible clients and park invisible ones off-screen.
 *
 * Visible clients are processed top-down at their current coordinates;
 * invisible clients are processed bottom-up and moved two widths to the left so
 * even their borders stay off-screen. Unmapping is avoided to prevent
 * UnmapNotify/withdraw churn on tag switches.
 *
 * @param[in]  c  Start of the monitor's focus stack, possibly NULL.
 *
 * @note Calls XMoveWindow() and resize() (X11); reads ISVISIBLE/monitor layout; does not modify list membership.
 * @see arrange, arrangenow
 * @anchor spec_showhide
 */
void
showhide(Client *c)
{
	if (!c)
		return;
	if (ISVISIBLE(c)) {
		/* show clients top down */
		XMoveWindow(dpy, c->win, c->x, c->y);
		if ((!c->mon->lt[c->mon->sellt]->arrange || c->isfloating) && !c->isfullscreen)
			resize(c, c->x, c->y, c->w, c->h, 0);
		showhide(c->snext);
	} else {
		/* hide clients bottom up */
		showhide(c->snext);
		/* park twice the width left: even borders stay off-screen */
		XMoveWindow(dpy, c->win, WIDTH(c) * -2, c->y);
	}
}

/**
 * @brief SIGHUP handler requesting a restart.
 *
 * @param[in]  unused  Signal number, ignored.
 *
 * @note Calls quit() with the restart flag; runs in signal-handler context.
 * @see sigterm, quit, setup
 * @anchor spec_sighup
 */
void
sighup(int unused)
{
	Arg a = {.i = 1};
	quit(&a);
}

/**
 * @brief SIGTERM handler requesting a clean shutdown.
 *
 * @param[in]  unused  Signal number, ignored.
 *
 * @note Calls quit() without requesting a restart; runs in signal-handler context.
 * @see sighup, quit
 * @anchor spec_sigterm
 */
void
sigterm(int unused)
{
	Arg a = {.i = 0};
	quit(&a);
}

/**
 * @brief Fork and exec a configured command.
 *
 * When spawning dmenu the monitor argument is retargeted to selmon. The child
 * starts a new session and closes the X connection so the program outlives
 * dwm.
 *
 * @param[in]  arg  arg->v is the NULL-terminated command vector.
 *
 * @note May write dmenumon; calls fork(), setsid(), close() and execvp().
 * @warning The child calls DIE if execvp fails; arg->v must be NULL-terminated.
 * @see main, setup
 * @anchor spec_spawn
 */
void
spawn(const Arg *arg)
{
	if (arg->v == dmenucmd)
		dmenumon[0] = '0' + selmon->num;
	if (fork() == 0) {
		if (dpy)
			close(ConnectionNumber(dpy));
		setsid();
		execvp(((char **)arg->v)[0], (char **)arg->v);
		DIE("execvp():dwm: execvp '%s' failed:", ((char **)arg->v)[0]);
	}
}

/**
 * @brief Replace the selected client's tags.
 *
 * @param[in]  arg  arg->ui is the new tag mask.
 *
 * @note Writes c->tags; calls focus() and arrange(); no-op without a selection or when the mask has no valid tags.
 * @see toggletag, tagmon, toggleview
 * @anchor spec_tag
 */
void
tag(const Arg *arg)
{
	if (selmon->sel && arg->ui & TAGMASK) {
		selmon->sel->tags = arg->ui & TAGMASK;
		focus(NULL);
		arrange(selmon);
	}
}

/**
 * @brief Send the selected client to the adjacent monitor.
 *
 * @param[in]  arg  arg->i selects the direction.
 *
 * @note Calls dirtomon() and sendmon(); no-op for a single monitor or without a selection.
 * @see sendmon, focusmon
 * @anchor spec_tagmon
 */
void
tagmon(const Arg *arg)
{
	if (!selmon->sel || !mons->next)
		return;
	sendmon(selmon->sel, dirtomon(arg->i));
}

/**
 * @brief Apply the default master/stack tiling layout.
 *
 * Snapshots the visible non-floating clients once so placement is linear
 * rather than restarting a filtered scan per window. The master column takes
 * mfact of the workarea when it holds clients; remaining clients fill the slave
 * column. Gaps are carved around and between windows.
 *
 * @param[in]  m  Monitor to lay out.
 *
 * @note Allocates and frees a temporary client array; calls resize() (X11); reads m->nmaster, m->mfact and m->gap.
 * @warning The monitor's gap must be initialized; ecalloc() may terminate on allocation failure.
 * @see monocle, arrangemon, setmfact, incnmaster
 * @anchor spec_tile
 */
void
tile(Monitor *m)
{
	unsigned int i, n, h, mw, my, ty;
	Client *c;

	/* Single pass over the client list: the previous implementation
	 * restarted a nexttiled() scan for every placement step, making the
	 * whole layout O(n^2).  Collect the tiled clients once (same filter
	 * and order nexttiled used: skip floating and invisible), then place
	 * from that snapshot — geometry math is byte-for-byte unchanged. */
	for (n = 0, c = m->clients; c; c = c->next)
		if (!c->isfloating && ISVISIBLE(c))
			n++;
	if (n == 0)
		return;

	/* heap snapshot: avoids a large stack VLA at high client counts */
	Client **tiled = ecalloc(n, sizeof *tiled);
	for (i = 0, c = m->clients; c; c = c->next)
		if (!c->isfloating && ISVISIBLE(c))
			tiled[i++] = c;

	if (n > m->nmaster)
		mw = m->nmaster ? m->ww * m->mfact : 0;
	else
		mw = m->ww - m->gap.gappx;
	for (i = 0, my = ty = m->gap.gappx; i < n; i++) {
		c = tiled[i];
		if (i < m->nmaster) {
			h = (m->wh - my) / (MIN(n, m->nmaster) - i) - m->gap.gappx;
			resize(c, m->wx + m->gap.gappx, m->wy + my, mw - (2*c->bw) - m->gap.gappx, h - (2*c->bw), 0);
			if (my + HEIGHT(c) + m->gap.gappx < m->wh)
				my += HEIGHT(c) + m->gap.gappx;
		} else {
			h = (m->wh - ty) / (n - i) - m->gap.gappx;
			resize(c, m->wx + mw + m->gap.gappx, m->wy + ty, m->ww - mw - (2*c->bw) - 2*m->gap.gappx, h - (2*c->bw), 0);
			if (ty + HEIGHT(c) + m->gap.gappx < m->wh)
				ty += HEIGHT(c) + m->gap.gappx;
		}
	}
	free(tiled);
}

/**
 * @brief Show or hide the selected monitor's bar.
 *
 * @param[in]  arg  Unused.
 *
 * @note Writes selmon->showbar; calls updatebarpos(), XMoveResizeWindow(); marks all bar segments dirty and arranges.
 * @see updatebarpos, drawbar
 * @anchor spec_togglebar
 */
void
togglebar(const Arg *arg)
{
	selmon->showbar = !selmon->showbar;
	updatebarpos(selmon);
	XMoveResizeWindow(dpy, selmon->barwin, selmon->wx, selmon->by, selmon->ww, bh);
	/* bar shown/hidden — all segments stale */
	bar_mark_dirty(selmon, DIRTY_ALL);
	arrange(selmon);
}

/**
 * @brief Toggle the selected client between floating and tiled.
 *
 * Fixed-size clients always remain floating. Switching to floating preserves
 * the current geometry.
 *
 * @param[in]  arg  Unused.
 *
 * @note Writes c->isfloating; calls resize() and arrange(); no-op without a selection or for fullscreen clients.
 * @see movemouse, resizemouse, tile
 * @anchor spec_togglefloating
 */
void
togglefloating(const Arg *arg)
{
	if (!selmon->sel)
		return;
	if (selmon->sel->isfullscreen) /* no support for fullscreen windows */
		return;
	/* fixed-size (isfixed) windows can never untile */
	selmon->sel->isfloating = !selmon->sel->isfloating || selmon->sel->isfixed;
	if (selmon->sel->isfloating)
		resize(selmon->sel, selmon->sel->x, selmon->sel->y,
			selmon->sel->w, selmon->sel->h, 0);
	arrange(selmon);
}

/**
 * @brief Toggle the selected client's EWMH fullscreen state.
 *
 * @param[in]  arg  Unused.
 *
 * @note Calls setfullscreen(); no-op without a selected client.
 * @see setfullscreen, clientmessage
 * @anchor spec_togglefullscr
 */
void
togglefullscr(const Arg *arg)
{
  if(selmon->sel)
    setfullscreen(selmon->sel, !selmon->sel->isfullscreen);
}

/**
 * @brief XOR tags on the selected client, refusing an empty result.
 *
 * @param[in]  arg  arg->ui is the tag mask to toggle.
 *
 * @note Writes c->tags; calls focus() and arrange(); no-op when the result would tag the client nowhere.
 * @see tag, toggleview
 * @anchor spec_toggletag
 */
void
toggletag(const Arg *arg)
{
	unsigned int newtags;

	if (!selmon->sel)
		return;
	newtags = selmon->sel->tags ^ (arg->ui & TAGMASK);
	if (newtags) {
		selmon->sel->tags = newtags;
		focus(NULL);
		arrange(selmon);
	}
}

/**
 * @brief XOR tags in the selected monitor's current view.
 *
 * @param[in]  arg  arg->ui is the tag mask to toggle.
 *
 * @note Writes selmon->tagset and bar_dirty_segments; calls focus() and arrange(); no-op when the result would be an empty view.
 * @see view, tag
 * @anchor spec_toggleview
 */
void
toggleview(const Arg *arg)
{
	unsigned int newtagset = selmon->tagset[selmon->seltags] ^ (arg->ui & TAGMASK);

	if (newtagset) {
		selmon->tagset[selmon->seltags] = newtagset;
		bar_mark_dirty(selmon, DIRTY_TAGS);
		focus(NULL);
		arrange(selmon);
	}
}

/**
 * @brief Deselect a client visually and optionally clear X input focus.
 *
 * @param[in]  c  Client to unfocus, possibly NULL.
 * @param[in]  setfocus  Non-zero to return input focus to the root window.
 *
 * @note Calls grabbuttons() and XSetWindowBorder(); with setfocus also calls XSetInputFocus() and deletes _NET_ACTIVE_WINDOW. Does not modify selmon->sel.
 * @see focus, grabbuttons
 * @anchor spec_unfocus
 */
void
unfocus(Client *c, int setfocus)
{
	if (!c)
		return;
	grabbuttons(c, 0);
	XSetWindowBorder(dpy, c->win, scheme[SchemeNorm][ColBorder].pixel);
	if (setfocus) {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
}

/**
 * @brief Stop managing a client and release its resources.
 *
 * Unwinds a swallow relationship first. Otherwise removes the client from the
 * client and focus stacks and the window hash, restores its border and withdraws
 * it unless X already destroyed it, frees the client, reflows the monitor,
 * rebuilds _NET_CLIENT_LIST, and resumes fullscreen-blocked processes when the
 * last fullscreen client disappeared.
 *
 * @param[in]  c  Managed client to release.
 * @param[in]  destroyed  Non-zero when X already destroyed the window.
 *
 * @pre c is managed by dwm.
 * @post c is freed, lists and hash are updated, the monitor is rearranged and focused.
 * @note Frees c or its parked placeholder; mutates winhash and monitor lists; calls detach/detachstack/arrangenow/focus/updateclientlist and X11 border/withdraw calls.
 * @warning Do not use c after this returns; swallow-related paths return early after freeing only the placeholder.
 * @see manage, destroynotify, unmapnotify, unswallow
 * @anchor spec_unmanage
 */
void
unmanage(Client *c, int destroyed)
{
	Monitor *m = c->mon;
	XWindowChanges wc;
	int was_fullscreen = c->isfullscreen;

	if (c->swallowing) {
		unswallow(c);
		return;
	}

	Client *s = swallowingclient(c->win);
	if (s) {
		winclient_remove(s->swallowing); /* child's sole ref besides p->swallowing: drop it before free */
		free(s->swallowing);
		s->swallowing = NULL;
		arrangenow(m); /* destruction path: immediate like manage() */
		focus(NULL);
		return;
	}

	detach(c);
	detachstack(c);
	winclient_remove(c); /* drop lookup index before the client is freed */
	if (!destroyed) {
		wc.border_width = c->oldbw;
		XGrabServer(dpy); /* avoid race conditions */
		error_guard_begin();
		XSelectInput(dpy, c->win, NoEventMask);
		XConfigureWindow(dpy, c->win, CWBorderWidth, &wc); /* restore border */
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		setclientstate(c, WithdrawnState);
		error_guard_end();
		XUngrabServer(dpy);
	}
	c->state = CST_FREED;
	free(c);

	if (!s) {
		arrangenow(m); /* destruction path: immediate like manage() */
		focus(NULL);
		updateclientlist();
		if (was_fullscreen && !isanyfullscreen())
			killatfullscreen_start();
	}
}

/**
 * @brief Handle an UnmapNotify event.
 *
 * A synthetic UnmapNotify means the client withdrew by ICCCM state change, so
 * only WM_STATE is updated. An unsynthesized event means the window vanished and
 * the client is unmanaged.
 *
 * @param[in]  e  X UnmapNotify event.
 *
 * @note Calls wintoclient(), setclientstate() or unmanage().
 * @see manage, unmanage, destroynotify
 * @anchor spec_unmapnotify
 */
void
unmapnotify(XEvent *e)
{
	Client *c;
	XUnmapEvent *ev = &e->xunmap;

	if ((c = wintoclient(ev->window))) {
		if (ev->send_event) {
			setclientstate(c, WithdrawnState);
			c->state = CST_WITHDRAWN;
		}
		else
			unmanage(c, 0);
	}
}

/**
 * @brief Create a bar window for every monitor that lacks one.
 *
 * @note Allocates m->barwin and calls XCreateWindow(), XDefineCursor(), XMapRaised() and XSetClassHint(); reads cursor[], bh and screen. Safe to call repeatedly.
 * @see updatebarpos, drawbar, setup
 * @anchor spec_updatebars
 */
void
updatebars(void)
{
	Monitor *m;
	XSetWindowAttributes wa = {
		.override_redirect = True,
		.background_pixmap = ParentRelative,
		.event_mask = ButtonPressMask|ExposureMask
	};
	XClassHint ch = {"dwm", "dwm"};
	for (m = mons; m; m = m->next) {
		if (m->barwin)
			continue;
		m->barwin = XCreateWindow(dpy, root, m->wx, m->by, m->ww, bh, 0, DefaultDepth(dpy, screen),
				CopyFromParent, DefaultVisual(dpy, screen),
				CWOverrideRedirect|CWBackPixmap|CWEventMask, &wa);
		XDefineCursor(dpy, m->barwin, cursor[CurNormal]->cursor);
		XMapRaised(dpy, m->barwin);
		XSetClassHint(dpy, m->barwin, &ch);
	}
}

/**
 * @brief Recompute a monitor's workarea and bar position.
 *
 * Starts from the full monitor rectangle. With the bar visible it subtracts bh
 * from the workarea and places the bar at the top or bottom according to
 * topbar; a hidden bar is parked at -bh, off-screen.
 *
 * @param[in,out]  m  Monitor whose wy/wh/by are recomputed.
 *
 * @note Writes m->wy, m->wh and m->by; reads m->my/m->mh/m->showbar/m->topbar and bh.
 * @see togglebar, updategeom, createmon
 * @anchor spec_updatebarpos
 */
void
updatebarpos(Monitor *m)
{
	m->wy = m->my;
	m->wh = m->mh;
	if (m->showbar) {
		m->wh -= bh;
		m->by = m->topbar ? m->wy : m->wy + m->wh;
		m->wy = m->topbar ? m->wy + bh : m->wy;
	} else
		m->by = -bh;
}

/**
 * @brief Rebuild the EWMH _NET_CLIENT_LIST root property.
 *
 * @note Calls XDeleteProperty() and XChangeProperty(); walks every monitor's client list.
 * @see manage, unmanage
 * @anchor spec_updateclientlist
 */
void
updateclientlist()
{
	Client *c;
	Monitor *m;

	XDeleteProperty(dpy, root, netatom[NetClientList]);
	for (m = mons; m; m = m->next)
		for (c = m->clients; c; c = c->next)
			XChangeProperty(dpy, root, netatom[NetClientList],
				XA_WINDOW, 32, PropModeAppend,
				(unsigned char *) &(c->win), 1);
}

/**
 * @brief Synchronize dwm's monitor list with the X server.
 *
 * When Xinerama is active it deduplicates screen geometries, adds and removes
 * monitors, and migrates clients off removed monitors to the head of mons. With
 * no Xinerama it maintains a single full-screen monitor. The return value tells
 * the caller whether geometry changed or clients moved.
 *
 * @return Non-zero when monitor geometry changed or clients were migrated, zero otherwise.
 *
 * @note Allocates monitors and a temporary unique-geometry array; calls XineramaIsActive/XineramaQueryScreens or uses sw/sh; may reassign selmon; calls createmon(), cleanupmon(), updatebarpos() and XFree().
 * @warning Removed monitors' clients are migrated to the mons head; callers must refresh bars and the drawing context.
 * @see configurenotify, setup, createmon, cleanupmon
 * @anchor spec_updategeom
 */
int
updategeom(void)
{
	int dirty = 0;

	lastgeom = (GeomChange){0};

#ifdef XINERAMA
	if (XineramaIsActive(dpy)) {
		int i, j, n, nn;
		Client *c;
		Monitor *m;
		XineramaScreenInfo *info = XineramaQueryScreens(dpy, &nn);
		XineramaScreenInfo *unique = NULL;

		for (n = 0, m = mons; m; m = m->next, n++);
		/* only consider unique geometries as separate screens */
		unique = ecalloc(nn, sizeof(XineramaScreenInfo));
		for (i = 0, j = 0; i < nn; i++)
			if (isuniquegeom(unique, j, &info[i]))
				memcpy(&unique[j++], &info[i], sizeof(XineramaScreenInfo));
		XFree(info);
		nn = j;

		/* new monitors if nn > n */
		for (i = n; i < nn; i++) {
			for (m = mons; m && m->next; m = m->next);
			if (m)
				m->next = createmon();
				else
				mons = createmon();
			lastgeom.added++;
		}
		for (i = 0, m = mons; i < nn && m; m = m->next, i++)
			if (i >= n
			|| unique[i].x_org != m->mx || unique[i].y_org != m->my
			|| unique[i].width != m->mw || unique[i].height != m->mh)
			{
				dirty = 1;
				lastgeom.resized++;
				m->num = i;
				m->mx = m->wx = unique[i].x_org;
				m->my = m->wy = unique[i].y_org;
				m->mw = m->ww = unique[i].width;
				m->mh = m->wh = unique[i].height;
				updatebarpos(m);
			}
		/* removed monitors if n > nn */
		for (i = nn; i < n; i++) {
			for (m = mons; m && m->next; m = m->next);
			lastgeom.removed++;
			lastgeom.removed_mon = m;
			while ((c = m->clients)) {
				dirty = 1;
				m->clients = c->next;
				detachstack(c);
				c->mon = mons;
				attach(c);
				attachstack(c);
			}
			/* Swallowed placeholders are off-list: migrate them via the
			 * hash so no client keeps a dangling monitor pointer. */
			for (unsigned int hi = 0; hi < WINHASH_SIZE; hi++)
				if (winhash[hi] && winhash[hi]->mon == m)
					winhash[hi]->mon = mons;
			if (m == selmon)
				selmon = mons;
			cleanupmon(m);
		}
		free(unique);
	} else
#endif /* XINERAMA */
	{ /* default monitor setup */
		if (!mons)
			mons = createmon();
		if (mons->mw != sw || mons->mh != sh) {
			dirty = 1;
			lastgeom.resized++;
			mons->mw = mons->ww = sw;
			mons->mh = mons->wh = sh;
			updatebarpos(mons);
		}
	}
	if (dirty) {
		selmon = mons;
		selmon = wintomon(root);
	}
	return dirty;
}

/**
 * @brief Determine the modifier bit occupied by NumLock.
 *
 * Scans the X modifier map for the keycode bound to XK_Num_Lock and records
 * the corresponding modifier mask so lock variants of grabs can be
 * generated.
 *
 * @note Writes numlockmask; calls XGetModifierMapping(), XKeysymToKeycode() and XFreeModifiermap().
 * @see grabbuttons, grabkeys
 * @anchor spec_updatenumlockmask
 */
void
updatenumlockmask(void)
{
	unsigned int i, j;
	XModifierKeymap *modmap;

	numlockmask = 0;
	modmap = XGetModifierMapping(dpy);
	for (i = 0; i < 8; i++)
		for (j = 0; j < modmap->max_keypermod; j++)
			if (modmap->modifiermap[i * modmap->max_keypermod + j]
				== XKeysymToKeycode(dpy, XK_Num_Lock))
				numlockmask = (1 << i);
	XFreeModifiermap(modmap);
}

/**
 * @brief Cache a client's ICCCM WM_NORMAL_HINTS.
 *
 * Reads base/min/max size, increment and aspect fields, treating absent hints
 * as zero. An equal non-zero min and max size sets isfixed. The cache is marked
 * valid until the property changes.
 *
 * @param[in,out]  c  Client whose hint fields are refreshed.
 *
 * @note Calls XGetWMNormalHints(); writes c->basew/baseh, minw/minh, maxw/maxh, incw/inch, mina/maxa, isfixed and hintsvalid.
 * @see applysizehints, propertynotify
 * @anchor spec_updatesizehints
 */
void
updatesizehints(Client *c)
{
	long msize;
	XSizeHints size;

	if (!XGetWMNormalHints(dpy, c->win, &size, &msize))
		/* size is uninitialized, ensure that size.flags aren't used */
		size.flags = PSize;
	if (size.flags & PBaseSize) {
		c->basew = size.base_width;
		c->baseh = size.base_height;
	} else if (size.flags & PMinSize) {
		c->basew = size.min_width;
		c->baseh = size.min_height;
	} else
		c->basew = c->baseh = 0;
	if (size.flags & PResizeInc) {
		c->incw = size.width_inc;
		c->inch = size.height_inc;
	} else
		c->incw = c->inch = 0;
	if (size.flags & PMaxSize) {
		c->maxw = size.max_width;
		c->maxh = size.max_height;
	} else
		c->maxw = c->maxh = 0;
	if (size.flags & PMinSize) {
		c->minw = size.min_width;
		c->minh = size.min_height;
	} else if (size.flags & PBaseSize) {
		c->minw = size.base_width;
		c->minh = size.base_height;
	} else
		c->minw = c->minh = 0;
	if (size.flags & PAspect) {
		c->mina = (float)size.min_aspect.y / size.min_aspect.x;
		c->maxa = (float)size.max_aspect.x / size.max_aspect.y;
	} else
		c->maxa = c->mina = 0.0;
	c->isfixed = (c->maxw && c->maxh && c->maxw == c->minw && c->maxh == c->minh);
	c->hintsvalid = 1;
}

/**
 * @brief Refresh the status text from the root window's WM_NAME.
 *
 * Skips work while the bar is frozen by fullscreen optimization. Falls back to
 * the version string when the property is absent, and skips the redraw when the
 * text is byte-for-byte unchanged. A changed status dirties the status and title
 * segments because the title boundary may shift.
 *
 * @note Writes stext, stext_len, selmon->bar_dirty_segments and bar_draw_pending; calls gettextprop() (X11).
 * @see drawbar, propertynotify, updatetitle
 * @anchor spec_updatestatus
 */
void
updatestatus(void)
{
	size_t len;
	char buf[sizeof stext];

	/* skip when fullscreen - bar is frozen, status would never be drawn */
	if (optimizefullscreen && selmon->sel && selmon->sel->isfullscreen)
		return;
	len = gettextprop(root, XA_WM_NAME, buf, sizeof buf);
	if (!len) {
		/* stock dwm fallback: version string when the root has no WM_NAME */
		len = sizeof("dwm-" VERSION) - 1;
		strcpy_len(buf, "dwm-" VERSION, len);
	}
	/* status unchanged byte-for-byte: no redraw needed */
	if (len == stext_len && !memcmp(buf, stext, len + 1))
		return;
	memcpy(stext, buf, len + 1);
	stext_len = len;
	/* status changed; title boundary may shift if width differs */
	bar_mark_dirty(selmon, DIRTY_STATUS | DIRTY_TITLE);
	bar_draw_pending = 1;            /* coalesce with other pending draws */
}

/**
 * @brief Refresh a client's cached title.
 *
 * Prefers the UTF-8 _NET_WM_NAME property, falling back to WM_NAME. An empty
 * result is replaced with the "broken" marker.
 *
 * @param[in,out]  c  Client whose name buffer is refreshed.
 *
 * @note Writes c->name; calls gettextprop() (X11) and stpcpy_len().
 * @see gettextprop, propertynotify, manage
 * @anchor spec_updatetitle
 */
void
updatetitle(Client *c)
{
	if (!gettextprop(c->win, netatom[NetWMName], c->name, sizeof c->name))
		gettextprop(c->win, XA_WM_NAME, c->name, sizeof c->name);
	if (unlikely(c->name[0] == '\0')) /* hack to mark broken clients */
		stpcpy_len(c->name, broken, sizeof(broken));
}

/**
 * @brief Apply EWMH window state and type hints.
 *
 * A pre-map fullscreen state request enters fullscreen; a dialog window type
 * makes the client floating.
 *
 * @param[in,out]  c  Client whose EWMH properties are applied.
 *
 * @note Calls getatomprop() and setfullscreen(); writes c->isfloating.
 * @see getatomprop, setfullscreen, manage
 * @anchor spec_updatewindowtype
 */
void
updatewindowtype(Client *c)
{
	Atom state = getatomprop(c, netatom[NetWMState]);
	Atom wtype = getatomprop(c, netatom[NetWMWindowType]);

	if (state == netatom[NetWMFullscreen])
		setfullscreen(c, 1);
	if (wtype == netatom[NetWMWindowTypeDialog])
		c->isfloating = 1;
}

/**
 * @brief Track WM_HINTS urgency and input focus mode.
 *
 * Clears urgency at the source for the selected client; for others it mirrors
 * the XUrgencyHint into c->isurgent. An InputHint with input unset makes the
 * client neverfocus.
 *
 * @param[in,out]  c  Client whose WM_HINTS are read.
 *
 * @note Calls XGetWMHints(), XSetWMHints() and XFree(); reads selmon; writes c->isurgent and c->neverfocus.
 * @see seturgent, manage, propertynotify
 * @anchor spec_updatewmhints
 */
void
updatewmhints(Client *c)
{
	XWMHints *wmh;

	/* focused clients must not appear urgent: clear the hint at the source */
	if ((wmh = XGetWMHints(dpy, c->win))) {
		if (c == selmon->sel && wmh->flags & XUrgencyHint) {
			wmh->flags &= ~XUrgencyHint;
			XSetWMHints(dpy, c->win, wmh);
		} else
			c->isurgent = (wmh->flags & XUrgencyHint) ? 1 : 0;
		if (wmh->flags & InputHint)
			c->neverfocus = !wmh->input;
		else
			c->neverfocus = 0;
		XFree(wmh);
	}
}

/**
 * @brief View the tags in arg->ui, or revert to the alternate tagset.
 *
 * An empty mask leaves the alternate tagset's contents in place, effectively
 * reverting to the previously viewed tags. Requests for the already-current view
 * are ignored.
 *
 * @param[in]  arg  arg->ui is the desired tag mask.
 *
 * @note Writes selmon->seltags/tagset and bar_dirty_segments; calls focus() and arrange().
 * @see toggleview, tag, focus
 * @anchor spec_view
 */
void
view(const Arg *arg)
{
	if ((arg->ui & TAGMASK) == selmon->tagset[selmon->seltags])
		return;
	selmon->seltags ^= 1; /* toggle sel tagset */
	if (arg->ui & TAGMASK)
		selmon->tagset[selmon->seltags] = arg->ui & TAGMASK;
	bar_mark_dirty(selmon, DIRTY_TAGS);
	focus(NULL);
	arrange(selmon);
}

/**
 * @brief Determine the PID owning a client window.
 *
 * On Linux this queries the X-Res extension; on OpenBSD it reads the
 * _NET_WM_PID property. Zero is returned when the owner cannot be
 * determined.
 *
 * @param[in]  w  Client window id.
 *
 * @return Owning process id, or 0 when unavailable.
 *
 * @note Linux uses xcb_res_query_client_ids() and frees the reply/error; OpenBSD uses XGetWindowProperty() and XFree().
 * @see getparentprocess, manage, termforwin
 * @anchor spec_winpid
 */
pid_t
winpid(Window w)
{

	pid_t result = 0;

#ifdef __linux__
	xcb_res_client_id_spec_t spec = {0};
	spec.client = w;
	spec.mask = XCB_RES_CLIENT_ID_MASK_LOCAL_CLIENT_PID;

	xcb_generic_error_t *e = NULL;
	xcb_res_query_client_ids_cookie_t c = xcb_res_query_client_ids(xcon, 1, &spec);
	xcb_res_query_client_ids_reply_t *r = xcb_res_query_client_ids_reply(xcon, c, &e);

	if (unlikely(!r)) {
		free(e);
		return (pid_t)0;
	}

	xcb_res_client_id_value_iterator_t i = xcb_res_query_client_ids_ids_iterator(r);
	for (; i.rem; xcb_res_client_id_value_next(&i)) {
		spec = i.data->spec;
		if (spec.mask & XCB_RES_CLIENT_ID_MASK_LOCAL_CLIENT_PID) {
			uint32_t *t = xcb_res_client_id_value_value(i.data);
			result = (pid_t)*t; /* 0xFFFFFFFF is the -1 "no pid" sentinel */
			break;
		}
	}

	free(r);

	if (result == (pid_t)-1)
		result = 0;

#endif /* __linux__ */

#ifdef __OpenBSD__
        Atom type;
        int format;
        unsigned long len, bytes;
        unsigned char *prop;
        pid_t ret;

        if (XGetWindowProperty(dpy, w, XInternAtom(dpy, "_NET_WM_PID", 0), 0, 1, False, AnyPropertyType, &type, &format, &len, &bytes, &prop) != Success || !prop)
               return 0;

        ret = *(pid_t*)prop;
        XFree(prop);
        result = ret;

#endif /* __OpenBSD__ */
	return result;
}

/**
 * @brief Return the parent PID of a process.
 *
 * On Linux it parses /proc/&lt;pid&gt;/stat after the last ')' so process names
 * containing spaces or parentheses are handled. On OpenBSD it uses kvm. Returns
 * zero for invalid PIDs or unreadable data.
 *
 * @param[in]  p  Process id to inspect.
 *
 * @return Parent process id, or 0 when it cannot be determined or p <= 0.
 *
 * @note Opens/reads /proc or kvm structures; performs no X11 calls.
 * @see isdescprocess, termforwin, winpid
 * @anchor spec_getparentprocess
 */
pid_t
getparentprocess(pid_t p)
{
	unsigned int v = 0;

	if (p <= 0)
		return 0;

#ifdef __linux__
	int fd;
	char buf[4096 + 1];
	ssize_t n;
	char *path = buf;
	/* snprintf(buf, sizeof(buf) - 1, "/proc/%u/stat", (unsigned)p); */
	path = stpcpy_len(path, S_LITERAL("/proc/"));
	path = numpcpy_unsafe(path, (unsigned)p);
	path = stpcpy_len(path, S_LITERAL("/stat"));
	if ((fd = open(buf, O_RDONLY)) < 0)
		return 0;
	n = read(fd, buf, sizeof(buf) - 1);
	if (n > 0) {
		buf[n] = '\0';
		/* Process name in /proc/[pid]/stat is enclosed in parentheses '(...)'
		 * and may contain spaces or parentheses. Find the last ')' to safely
		 * parse state and ppid after it. */
		/* 1. pid: %d */
		/* 2. comm: (%s) */
		char *s = strrchr_len(buf, ')', (size_t)n);
		if (s) {
			++s;
			/* 3. state: %c */
			++s;
			++s;
			/* 4. ppid %d */
			v = (unsigned int)atoi(s);
		}
	}
	close(fd);
#endif /* __linux__*/

#ifdef __OpenBSD__
	int n;
	kvm_t *kd;
	struct kinfo_proc *kp;

	kd = kvm_openfiles(NULL, NULL, NULL, KVM_NO_FILES, NULL);
	if (unlikely(!kd))
		return 0;

	kp = kvm_getprocs(kd, KERN_PROC_PID, p, sizeof(*kp), &n);
	if (!kp || n <= 0) {
		kvm_close(kd);
		return 0;
	}
	v = kp->p_ppid;
	kvm_close(kd);
#endif /* __OpenBSD__ */

	return (pid_t)v;
}

/**
 * @brief Test whether one process is an ancestor of another.
 *
 * @param[in]  p  Candidate ancestor PID.
 * @param[in]  c  Candidate descendant PID; the walk uses a local copy.
 *
 * @return Non-zero when p is an ancestor of c, zero otherwise or when the relationship cannot be determined.
 *
 * @note Repeatedly calls getparentprocess(); rejects non-positive PIDs.
 * @see getparentprocess, termforwin
 * @anchor spec_isdescprocess
 */
int
isdescprocess(pid_t p, pid_t c)
{
	if (p <= 0 || c <= 0)
		return 0;

	while (p != c && c != 0)
		c = getparentprocess(c);

	return (p == c && c != 0);
}

/**
 * @brief Find the terminal client that should swallow a new window.
 *
 * Windows without a PID and terminal windows themselves never swallow. Scans
 * every monitor for a terminal whose process is an ancestor of w's process and
 * which is not already hosting a swallowed window.
 *
 * @param[in]  w  Newly managed client to match.
 *
 * @return Candidate terminal client, or NULL when none qualifies.
 *
 * @note Reads mons and client lists; calls isdescprocess().
 * @see swallow, isdescprocess, winpid
 * @anchor spec_termforwin
 */
Client *
termforwin(const Client *w)
{
	Client *c;
	Monitor *m;

	if (!w->pid || w->isterminal)
		return NULL;

	for (m = mons; m; m = m->next) {
		for (c = m->clients; c; c = c->next) {
			if (c->isterminal && !c->swallowing && c->pid && isdescprocess(c->pid, w->pid))
				return c;
		}
	}

	return NULL;
}

/**
 * @brief Find the client holding a swallowed placeholder for a window.
 *
 * @param[in]  w  Placeholder window id.
 *
 * @return Holder client whose c->swallowing->win == w, or NULL.
 *
 * @note Walks every monitor's client list; reads no X properties.
 * @see swallow, unswallow, destroynotify
 * @anchor spec_swallowingclient
 */
Client *
swallowingclient(Window w)
{
	Client *c;
	Monitor *m;

	for (m = mons; m; m = m->next) {
		for (c = m->clients; c; c = c->next) {
			if (c->swallowing && c->swallowing->win == w)
				return c;
		}
	}

	return NULL;
}

/**
 * @brief Compute the hash home slot for a window id.
 *
 * Uses a multiplicative Fibonacci-style hash to spread structured XID bit patterns evenly.
 *
 * @param[in]  w  Window id to hash.
 *
 * @return Home slot index in [0, WINHASH_SIZE).
 *
 * @note Pure computation; relies on WINHASH_SIZE being a power of two.
 * @see winclient_put, winclient_remove, wintoclient
 * @anchor spec_winhash_home
 */
static unsigned int
winhash_home(Window w)
{
	return ((unsigned int)w * 2654435761u) >> 24;
}

/**
 * @brief Insert a client into the window-id hash.
 *
 * Uses linear probing. When the table is full the insert is suppressed and
 * winhash_overflowed is set permanently, because the suppressed client lives
 * only in the authoritative monitor lists.
 *
 * @param[in]  c  Client to index under its current c->win.
 *
 * @note Writes winhash[], winhash_count and winhash_overflowed.
 * @warning Suppressed clients are list-only; lookups must keep the overflow fallback enabled.
 * @see winclient_remove, wintoclient
 * @anchor spec_winclient_put
 */
static void
winclient_put(Client *c)
{
	unsigned int i = winhash_home(c->win);

	if (winhash_count >= WINHASH_SIZE) {
		/* remember the overflow permanently: after any later removal the
		 * count drops below capacity, but the suppressed client is still
		 * list-only, so the fallback must stay enabled. */
		winhash_overflowed = 1;
		return;
	}
	while (winhash[i] && winhash[i] != c)
		i = (i + 1) & (WINHASH_SIZE - 1);
	if (!winhash[i]) {
		winhash[i] = c;
		winhash_count++;
	}
}

/**
 * @brief Remove a client from the window-id hash.
 *
 * Clears the probed slot, then rehashes the following collision cluster until
 * the next empty slot so probe chains stay valid.
 *
 * @param[in]  c  Client to remove, keyed by its current c->win.
 *
 * @note Writes winhash[] and winhash_count; calls winclient_put() while rehashing.
 * @warning Absent or previously suppressed entries are silently ignored.
 * @see winclient_put, wintoclient
 * @anchor spec_winclient_remove
 */
static void
winclient_remove(Client *c)
{
	unsigned int i = winhash_home(c->win);
	unsigned int probe = 0;

	while (winhash[i] && winhash[i] != c) {
		i = (i + 1) & (WINHASH_SIZE - 1);
		if (++probe >= WINHASH_SIZE)
			return; /* not present (insert was suppressed) */
	}
	if (!winhash[i])
		return;
	winhash[i] = NULL;
	winhash_count--;
	for (probe = 0; winhash[(i + 1 + probe) & (WINHASH_SIZE - 1)]; probe++) {
		Client *t = winhash[(i + 1 + probe) & (WINHASH_SIZE - 1)];
		winhash[(i + 1 + probe) & (WINHASH_SIZE - 1)] = NULL;
		winhash_count--;
		winclient_put(t);
	}
}

/**
 * @brief Map a window id to its Client through the hash index.
 *
 * Probes the open-addressing table. An empty-slot miss is authoritative unless
 * an insert was ever suppressed; then the monitor client lists are walked as the
 * fallback.
 *
 * @param[in]  w  Window id to resolve.
 *
 * @return Matching client, or NULL when the window is not managed.
 *
 * @note Reads winhash[], winhash_overflowed, mons and client lists.
 * @see wintomon, winclient_put, winclient_remove
 * @anchor spec_wintoclient
 */
Client *
wintoclient(Window w)
{
	Client *c;
	Monitor *m;
	unsigned int i = winhash_home(w), probe = 0;

	while (winhash[i]) {
		if (winhash[i]->win == w)
			return winhash[i];
		i = (i + 1) & (WINHASH_SIZE - 1);
		if (++probe >= WINHASH_SIZE)
			break;
	}
	/* An empty-slot miss is authoritative unless an insert was ever
	 * suppressed (suppressed inserts live only in the lists). */
	if (!winhash_overflowed)
		return NULL;
	for (m = mons; m; m = m->next)
		for (c = m->clients; c; c = c->next)
			if (c->win == w)
				return c;
	return NULL;
}

/**
 * @brief Resolve a window id to its monitor.
 *
 * The root window follows the pointer through getrootptr()/recttomon(); bar
 * windows map to their owner monitor; client windows map to c->mon; anything
 * else defaults to selmon.
 *
 * @param[in]  w  Window id to resolve.
 *
 * @return Owning or best-matching monitor; never NULL while selmon is valid.
 *
 * @note Calls getrootptr() (XQueryPointer), wintoclient() and recttomon(); reads mons, root and selmon.
 * @see wintoclient, recttomon
 * @anchor spec_wintomon
 */
Monitor *
wintomon(Window w)
{
	int x, y;
	Client *c;
	Monitor *m;

	if (w == root && getrootptr(&x, &y))
		return recttomon(x, y, 1, 1);
	for (m = mons; m; m = m->next)
		if (w == m->barwin)
			return m;
	if ((c = wintoclient(w)))
		return c->mon;
	return selmon;
}

static int (*saved_error_handler)(Display *, XErrorEvent *);
static int error_guard_depth;

/**
 * @brief Enter a scoped window in which all X errors are ignored.
 *
 * Saves the previous handler so nested guards restore correctly; XSync before
 * the swap ensures earlier errors are delivered to the old handler.
 *
 * @post error_guard_depth is incremented and xerrordummy is installed.
 * @note Calls XSync and XSetErrorHandler.
 * @anchor spec_error_guard_begin
 */
static void
error_guard_begin(void)
{
	if (error_guard_depth++ == 0) {
		XSync(dpy, False);
		saved_error_handler = XSetErrorHandler(xerrordummy);
	}
}

/**
 * @brief Leave the innermost scoped error-ignore window.
 *
 * @pre error_guard_depth > 0.
 * @post The handler saved by error_guard_begin() is restored.
 * @note Calls XSync and XSetErrorHandler.
 * @anchor spec_error_guard_end
 */
static void
error_guard_end(void)
{
	if (--error_guard_depth == 0) {
		XSync(dpy, False);
		XSetErrorHandler(saved_error_handler);
	}
}

/**
 * @brief Non-fatal X error handler for race-prone requests.
 *
 * Ignores known benign failures on destroyed or already-configured windows
 * (BadWindow, certain BadMatch/BadDrawable cases, and duplicate grabs). Other
 * errors are printed and delegated to the previously installed handler.
 *
 * @param[in]  dpy  Display connection (unused).
 * @param[in]  ee  X error event.
 *
 * @return 0 for ignored errors, otherwise the delegate handler's result, which may exit the process.
 *
 * @note Reads xerrorxlib; delegates through it, possibly calling exit().
 * @see xerrordummy, xerrorstart
 * @anchor spec_xerror
 */
int
xerror(Display *dpy, XErrorEvent *ee)
{
	if (ee->error_code == BadWindow
	|| (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch)
	|| (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolyFillRectangle && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolySegment && ee->error_code == BadDrawable)
	|| (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch)
	|| (ee->request_code == X_GrabButton && ee->error_code == BadAccess)
	|| (ee->request_code == X_GrabKey && ee->error_code == BadAccess)
	|| (ee->request_code == X_CopyArea && ee->error_code == BadDrawable))
		return 0;
	fprintf(stderr, "dwm: fatal error: request code=%d, error code=%d\n",
		ee->request_code, ee->error_code);
	return xerrorxlib(dpy, ee); /* may call exit */
}

/**
 * @brief Ignore-all X error handler for deliberate race sections.
 *
 * @param[in]  dpy  Display connection (unused).
 * @param[in]  ee  X error event (unused).
 *
 * @return Always 0.
 *
 * @see xerror, killclient, unmanage
 * @anchor spec_xerrordummy
 */
int
xerrordummy(Display *dpy, XErrorEvent *ee)
{
	return 0;
}

/**
 * @brief Startup X error handler that aborts on a conflicting window manager.
 *
 * @param[in]  dpy  Display connection (unused).
 * @param[in]  ee  X error event (unused).
 *
 * @return -1; unreachable because DIE terminates the process.
 *
 * @note Calls DIE() with a fatal message.
 * @see checkotherwm, xerror
 * @anchor spec_xerrorstart
 */
int
xerrorstart(Display *dpy, XErrorEvent *ee)
{
	DIE("XOpenDisplay():dwm: another window manager is already running");
	return -1;
}

/**
 * @brief Promote the selected tiled client to master.
 *
 * When the selected client is already master, the next tiled client is
 * promoted instead.
 *
 * @param[in]  arg  Unused.
 *
 * @note Calls nexttiled() and pop(); X11 work happens through the resulting arrange.
 * @see pop, nexttiled, tile
 * @anchor spec_zoom
 */
void
zoom(const Arg *arg)
{
	Client *c = selmon->sel;

	if (!selmon->lt[selmon->sellt]->arrange || !c || c->isfloating)
		return;
	/* c already master: take the next tiled client instead */
	if (c == nexttiled(selmon->clients) && !(c = nexttiled(c->next)))
		return;
	pop(c);
}

#ifndef DWM_TEST
/**
 * @brief Program entry point: open the display, initialize, scan, run and clean up.
 *
 * Handles -v/usage, sets the locale, opens the X display and XCB connection,
 * checks for a conflicting window manager, initializes dwm, adopts existing
 * windows, repairs a stale fullscreen STOP, runs the event loop, optionally
 * re-execs itself for a restart, then cleans up.
 *
 * @param[in]  argc  Argument count.
 * @param[in]  argv  Argument vector; argv[0] is reused for a restart.
 *
 * @return EXIT_SUCCESS after a clean shutdown.
 *
 * @note Compiled only when DWM_TEST is not defined; writes dpy, xcon and restart; calls checkotherwm(), setup(), scan(), run(), cleanup(), XCloseDisplay() and execvp() on restart.
 * @warning Calls DIE on invalid usage or when the display cannot be opened.
 * @see setup, scan, run, cleanup
 * @anchor spec_main
 */
int
main(int argc, char *argv[])
{
	if (unlikely(argc == 2 && !strcmp("-v", argv[1])))
		DIE("dwm-" VERSION);
	else if (unlikely(argc != 1))
		DIE("usage: dwm [-v]");
	if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		fputs("warning: no locale support\n", stderr);
	if (unlikely(!(dpy = XOpenDisplay(NULL))))
		DIE("XOpenDisplay():dwm: cannot open display");
	if (unlikely(!(xcon = XGetXCBConnection(dpy))))
		DIE("XGetXCBConnection():dwm: cannot get xcb connection");
	checkotherwm();
	setup();
#ifdef __OpenBSD__
	if (unlikely(pledge("stdio rpath proc exec ps", NULL) == -1))
		DIE("pledge():pledge:");
#endif /* __OpenBSD__ */
	scan();
	/* Repair a stale STOP from a crash while fullscreen was active: if no
	 * client is fullscreen now, a previous dwm left dwmblocks stopped. CONT
	 * is a no-op when already running, so unconditional repair is safe. */
	if (!isanyfullscreen())
		killatfullscreen_resume();
	run();
	{
		int restarting = (lifecycle == LIFE_RESTART);

		cleanup();
		XCloseDisplay(dpy);
		if (restarting) {
			execvp(argv[0], argv);
			DIE("execvp():dwm: restart failed");
		}
	}
	return EXIT_SUCCESS;
}
#endif /* ndef DWM_TEST */
