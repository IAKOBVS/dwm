/* See LICENSE file for copyright and license details. */

/**
 * @file dwm.h
 * @brief Central header for the dynamic window manager.
 *
 * Defines all shared types, enums, forward declarations, and global
 * variables used across dwm.c.  The core data structures are:
 *
 *   Client  — an X window managed by dwm (geometry, tags, swallowing, etc.)
 *   Monitor — a physical or virtual screen (linked list, tag sets, layout)
 *   Key     — hotkey binding (mod + keysym → function + arg)
 *   Button  — mouse binding (click area + button → function + arg)
 *   Layout  — arrange function + display symbol
 *   Rule    — auto-properties matched against class/instance/title
 *   Gap     — inner/outer gap state (isgap, realgap, gappx)
 *
 * Bar dirty-tracking (Phase 2-4 optimizations, per-monitor):
 *   m->bar_dirty_segments — bitmask (DIRTY_STATUS|DIRTY_TAGS|DIRTY_TITLE)
 *   m->bar_exposed        — flags when an Expose event invalidated the pixmap
 *   bar_draw_pending      — deferred draw coalesced at the event-loop tail
 */

#ifndef DWM_H
#define DWM_H 1

#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <X11/Xft/Xft.h>

#include "drw.h"

#define GAP_TOGGLE 100
#define GAP_RESET  0

/**
 * @brief Copy a known-length string and NUL-terminate it.
 *
 * Avoids `strlen` by taking the source length explicitly. The caller must
 * guarantee `dst` has room for `src_len + 1` bytes.
 *
 * @details Copies exactly @p src_len bytes with memcpy() and writes a trailing
 *          NUL, avoiding a strlen() pass; see @ref arch_module_dwm (dwm.h).
 * @param[out] dst      Destination buffer.
 * @param[in]  src      Source bytes (need not be NUL-terminated).
 * @param[in]  src_len  Number of bytes to copy.
 * @post `dst[src_len] == '\0'`.
 * @par Side Effects:
 *      Direct writes to the caller-supplied destination buffer.
 * @par Threading and Reentrancy:
 *      Single-threaded; touches only its arguments and no globals, so safe from
 *      a signal handler when the buffers are not shared.
 * @warning Unbounded: no destination size is checked.
 * @anchor spec_strcpy_len
 */
static inline void
strcpy_len(char *dst, const char *src, size_t src_len)
{
	memcpy(dst, src, src_len);
	dst[src_len] = '\0';
}

/**
 * @brief Copy a known-length string and return the new terminator position.
 *
 * Like @ref spec_strcpy_len but returns a pointer to the NUL byte, enabling
 * callers to append further data without re-scanning.
 *
 * @details Like @ref spec_strcpy_len, copies @p src_len bytes plus a NUL via
 *          memcpy(), but returns the address of the written terminator so
 *          callers can append without rescanning; see @ref arch_module_dwm (dwm.h).
 * @param[out] dst      Destination buffer.
 * @param[in]  src      Source bytes.
 * @param[in]  src_len  Number of bytes to copy.
 * @return Pointer to the written NUL terminator (`dst + src_len`).
 * @par Side Effects:
 *      Direct writes to the caller-supplied destination buffer.
 * @par Threading and Reentrancy:
 *      Single-threaded; no locks or globals, so safe from a signal handler when
 *      the buffers are not shared.
 * @warning Unbounded: no destination size is checked.
 * @anchor spec_stpcpy_len
 */
static inline char *
stpcpy_len(char *dst, const char *src, size_t src_len)
{
	memcpy(dst, src, src_len);
	dst[src_len] = '\0';
	return dst + src_len;
}

/**
 * @brief Bounded copy of a known-length string.
 *
 * Copies at most `dstsize - 1` bytes and always NUL-terminates, unlike
 * `strncpy` it never pads the remainder of the destination.
 *
 * @details Copies `min(src_len, dstsize - 1)` bytes into @p dst and always
 *          appends a NUL; unlike strncpy() it never pads the remainder of the
 *          destination; see @ref arch_module_dwm (dwm.h). The exact return value
 *          is the address of the written terminator: `dst + src_len` when
 *          `src_len < dstsize`, otherwise `dst + dstsize - 1` on truncation.
 * @param[out] dst      Destination buffer.
 * @param[in]  dstsize  Capacity of @p dst in bytes.
 * @param[in]  src      Source bytes.
 * @param[in]  src_len  Available source length.
 * @return Pointer to the written NUL terminator.
 * @pre  dstsize > 0.
 * @post The result is truncated to `dstsize - 1` bytes when necessary.
 * @par Side Effects:
 *      Direct writes to the caller-supplied destination buffer.
 * @par Threading and Reentrancy:
 *      Single-threaded; no locks or globals, so safe from a signal handler when
 *      the buffers are not shared.
 * @anchor spec_stpncpy_len
 */
static inline char *
stpncpy_len(char *dst, size_t dstsize, const char *src, size_t src_len)
{
	size_t len = src_len;
	if (len >= dstsize)
		len = dstsize - 1;
	memcpy(dst, src, len);
	dst[len] = '\0';
	return dst + len;
}

/* declarations */
enum { CurNormal, CurResize, CurMove, CurLast }; /* cursor */
enum { SchemeNorm, SchemeSel }; /* color schemes */
enum { NetSupported, NetWMName, NetWMState, NetWMCheck,
       NetWMFullscreen, NetActiveWindow, NetWMWindowType,
       NetWMWindowTypeDialog, NetClientList, NetLast }; /* EWMH atoms */
enum { WMProtocols, WMDelete, WMState, WMTakeFocus, WMLast }; /* default atoms */
enum { ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle,
       ClkClientWin, ClkRootWin, ClkLast }; /* clicks */

/**
 * @brief Generic binding argument; one field is valid per consumer.
 * @details Tagged-by-convention union passed by value to Key and Button
 *          handlers; see @ref arch_module_dwm (dwm.h). The handler decides which
 *          single member is meaningful. Holds no dynamic storage and does not
 *          own the pointee of `v`, whose lifetime stays with the caller.
 * @par Ownership:
 *      Values are copied into static binding tables at setup; `v` is borrowed.
 * @par Threading and Reentrancy:
 *      Read-only after setup; single-threaded, with no locks or globals.
 */
typedef union {
	int i;             /**< Signed integer argument. */
	unsigned int ui;   /**< Unsigned integer argument. */
	float f;           /**< Floating-point argument (e.g. mfact). */
	const void *v;     /**< Pointer argument (e.g. command vector). */
} Arg;

/**
 * @brief Mouse binding: click region + button + modifier mask → action.
 * @details One entry of the static buttons[] table; a press matches when its
 *          click region, button and modifier mask all agree, then func is
 *          invoked with a pointer to arg; see @ref arch_module_dwm (dwm.h).
 * @par Ownership:
 *      The Arg is embedded by value; func is a plain code address with no
 *      dynamic storage or ownership.
 * @par Threading and Reentrancy:
 *      Read-only after cachebuttons(); dispatched on the single X event loop.
 */
typedef struct {
	unsigned int click;              /**< Clk* region that must be clicked. */
	unsigned int mask;               /**< Required modifier state. */
	unsigned int button;             /**< X button number. */
	void (*func)(const Arg *arg);    /**< Action to run on match. */
	const Arg arg;                   /**< Argument passed to @ref func. */
} Button;

typedef struct Monitor Monitor;
typedef struct Client Client;

/**
 * @brief A managed top-level X window.
 * @details Owned by exactly one monitor while managed; linked into `mon->clients`
 *          via `next`, `mon->stack` via `snext`, and the global window hash.
 *          Lifecycle is tracked explicitly by `state`; see @ref arch_module_dwm
 *          (dwm.h).
 * @par Ownership:
 *      Allocated by manage() and freed by unmanage(); `mon` is non-owning, and
 *      `swallowing`/`holder` are non-owning cross-references between clients.
 * @par Threading and Reentrancy:
 *      Mutated only by the single-threaded X event loop; no internal locks.
 */
/**
 * @brief Explicit lifecycle state of a managed client.
 * @details Ordered CST_ABSENT -> CST_LISTED -> ... -> CST_FREED, replacing
 *          implicit boolean pairs so no contradictory state is representable.
 * @par Threading and Reentrancy:
 *          Written only from the single event loop and read as a plain integer.
 */
typedef enum {
	CST_ABSENT = 0,      /**< not allocated */
	CST_LISTED,          /**< linked into monitor lists and winhash */
	CST_WITHDRAWN,       /**< still managed, ICCCM WithdrawnState */
	CST_HOLDER,          /**< listed terminal owning a swallowed placeholder */
	CST_PLACEHOLDER,     /**< off-list child parked under a holder */
	CST_FREED            /**< released */
} ClientState;

struct Client {
	ClientState state;                   /**< Explicit lifecycle state. */
	char name[256];                      /**< WM_NAME/`_NET_WM_NAME` cache. */
	float mina, maxa;                    /**< Aspect ratio hints (min/max). */
	int x, y, w, h;                      /**< Last applied geometry. */
	int oldx, oldy, oldw, oldh;          /**< Pre-fullscreen/floating geometry. */
	int basew, baseh;                    /**< Size-hint base dimensions. */
	int incw, inch;                      /**< Resize increment hints. */
	int maxw, maxh;                      /**< Maximum size hints (0 = unbounded). */
	int minw, minh;                      /**< Minimum size hints. */
	int hintsvalid;                      /**< Non-zero once hints are cached. */
	int bw, oldbw;                       /**< Current/previous border width. */
	unsigned int tags;                   /**< Tag bitmask; never zero, masked to TAGMASK. */
	unsigned char isfixed;               /**< Size hints marked the size fixed. */
	unsigned char isfloating;            /**< Floating instead of tiled. */
	unsigned char isurgent;              /**< Urgency hint requested attention. */
	unsigned char neverfocus;            /**< Input hint forbids focus. */
	unsigned char oldstate;              /**< Previous ICCCM WM_STATE. */
	unsigned char fs_oldfloating;        /**< Floating state captured at fullscreen entry. */
	unsigned char transient_floating;    /**< Dialog/fixed state that must remain floating. */
	unsigned char isfullscreen;          /**< Fullscreen state. */
	unsigned char isterminal;            /**< Matched a terminal rule. */
	unsigned char noswallow;             /**< Must not be swallowed. */
	pid_t pid;                           /**< Owning process PID, or 0. */
	Client *next;                        /**< Next client on `mon->clients`. */
	Client *snext;                       /**< Next client on `mon->stack`. */
	Client *swallowing;                  /**< Child this client swallowed, or NULL. */
	Client *holder;                      /**< Owning client when CST_PLACEHOLDER. */
	Monitor *mon;                        /**< Owning monitor. */
	Window win;                          /**< X window ID. */
};

/**
 * @brief Keyboard binding: modifier + keysym → action.
 * @details One entry of the static keys[] table; matched by exact keysym plus
 *          CLEANMASK(mod) and dispatched with arg; see @ref arch_module_dwm
 *          (dwm.h).
 * @par Ownership:
 *      The Arg is embedded by value; there is no dynamic allocation or ownership.
 * @par Threading and Reentrancy:
 *      Read-only after cachekeys(); dispatched on the single X event loop.
 */
typedef struct {
	unsigned int mod;                /**< Required modifier mask. */
	KeySym keysym;                   /**< Key symbol. */
	void (*func)(const Arg *);       /**< Action to run on match. */
	const Arg arg;                   /**< Argument passed to @ref func. */
} Key;

static KeySym key_keysym_used;       /**< Keysym of the last matched binding. */
static unsigned int key_mod_used;    /**< Modifier mask of the last matched binding. */
/* Exact binding index: open-addressed set of packed (keysym, CLEANMASK(mod))
 * pairs rebuilt by cachekeys(); lets keypress() reject non-bindings in O(1)
 * instead of the lossy OR-masks above (which pass any keysym once enough
 * bindings are merged, and can't reject chords merely sharing a mod bit). */
#define KEYSET_SIZE 256 /* power of two; keyset_home() folds to this width */
static unsigned long long keyset[KEYSET_SIZE]; /**< Packed exact bindings. */
static unsigned int keyset_count;              /**< Occupied keyset slots. */
static unsigned char keyset_saturated;         /**< Set full: masks remain the guard. */
static unsigned long long button_button_used;  /**< OR of all bound buttons. */
static unsigned int button_mask_used;          /**< OR of all bound button masks. */

/**
 * @brief A layout: arrange function plus the symbol shown in the bar.
 * @details The arrange callback mutates a Monitor's client geometry, while
 *          symbol is a short string rendered in the tag bar; see
 *          @ref arch_module_dwm (dwm.h).
 * @par Ownership:
 *      Symbols are static string literals and the layout objects live in
 *      config arrays; neither is owned by a Monitor.
 * @par Threading and Reentrancy:
 *      Read-only after setup; arrange() runs only from the single event loop.
 */
typedef struct {
	const char *symbol;              /**< Short layout symbol. */
	void (*arrange)(Monitor *);      /**< Layout implementation. */
} Layout;

/**
 * @brief Gap state for a monitor (toggled/adjusted by @ref spec_setgaps).
 * @details Holds the configured pixel gap (gappx), the currently effective gap
 *          (realgap) and the enable flag; the struct is copied by value into
 *          each Monitor; see @ref arch_module_dwm (dwm.h).
 * @par Threading and Reentrancy:
 *      Plain value struct with no locks; mutated only by the single event loop.
 */
typedef struct {
	short realgap;            /**< Effective per-tile gap. */
	short gappx;              /**< Configured gap size. */
	unsigned char isgap;      /**< Non-zero when gaps are enabled. */
} Gap;

/**
 * @brief A work area (one per Xinerama screen, or one fallback screen).
 * @details Owned by the global `mons` list. Holds its clients, focus stack, tag
 *          view, layouts, geometry, and bar state; see @ref arch_module_dwm
 *          (dwm.h). Bar redraws are gated by `bar_dirty_segments` and
 *          `bar_exposed`.
 * @par Ownership:
 *      The monitor owns its Client allocations and its `gap`; it is linked by
 *      `next` and released by cleanupmon().
 * @par Threading and Reentrancy:
 *      Mutated only by the single-threaded X event loop; no internal locks.
 */
struct Monitor {
	char ltsymbol[16];                 /**< Current layout symbol. */
	float mfact;                       /**< Master-area fraction (0.05-0.95). */
	int nmaster;                       /**< Number of clients in the master area. */
	int num;                           /**< Monitor index. */
	int by;                            /**< Bar geometry: bar y position. */
	int mx, my, mw, mh;                /**< Full screen geometry. */
	int wx, wy, ww, wh;                /**< Work area (screen minus bar/gaps). */
	unsigned int seltags;              /**< Active tag set index (0 or 1). */
	unsigned int sellt;                /**< Active layout index (0 or 1). */
	unsigned int tagset[2];            /**< Previous/current tag views. */
	unsigned char showbar;             /**< Bar visibility. */
	unsigned char topbar;              /**< Bar position (top/bottom). */
	unsigned char bar_exposed;         /**< Expose invalidated the bar pixmap. */
	unsigned char bar_dirty_segments;  /**< DIRTY_STATUS|DIRTY_TAGS|DIRTY_TITLE mask. */
	Gap gap;                           /**< Gap state. */
	Client *clients;                   /**< Managed clients in management order. */
	Client *sel;                       /**< Selected client, or NULL. */
	Client *stack;                     /**< Focus/stack order, most recent first. */
	Monitor *next;                     /**< Next monitor in `mons`. */
	Window barwin;                     /**< Bar window ID. */
	const Layout *lt[2];               /**< Previous/current layout. */
};

/**
 * @brief A tag/geometry rule matched by @ref spec_applyrules.
 * @details Configured static matchers for WM_CLASS class/instance and title,
 *          plus the tags and flags applied on a match; see @ref arch_module_dwm
 *          (dwm.h).
 * @par Ownership:
 *      String fields are static literals from config; the array is never mutated.
 * @par Threading and Reentrancy:
 *      Read-only after setup and evaluated only from the single event loop.
 */
typedef struct {
	const char *class;               /**< Substring to match WM_CLASS class. */
	const char *instance;            /**< Substring to match WM_CLASS instance. */
	const char *title;               /**< Substring to match the window title. */
	unsigned int tags;               /**< Tags OR-ed onto a matching client. */
	unsigned char isfloating;        /**< Set floating state on match. */
	unsigned char isterminal;        /**< Mark client as a terminal. */
	unsigned char noswallow;         /**< Prevent swallowing on match. */
	int monitor;                     /**< Target monitor number, or -1. */
} Rule;

/* function declarations */
static void applyrules(Client *c);
static int applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact);
static void arrange(Monitor *m);
static void arrangemon(Monitor *m);
static void attach(Client *c);
static void attachstack(Client *c);
static void sighup(int unused);
static void sigterm(int unused);
static void buttonpress(XEvent *e);
static void cachebuttons(void);
static void cachekeys(void);
static void checkotherwm(void);
static void cleanup(void);
static void cleanupmon(Monitor *mon);
static void clientmessage(XEvent *e);
static void configure(Client *c);
static void configurenotify(XEvent *e);
static void configurerequest(XEvent *e);
static Monitor *createmon(void);
static void destroynotify(XEvent *e);
static void detach(Client *c);
static void detachstack(Client *c);
static Monitor *dirtomon(int dir);
static void drawbar(Monitor *m);
static void drawbars(void);
static void enternotify(XEvent *e);
static void expose(XEvent *e);
static void focus(Client *c);
static void focusin(XEvent *e);
static void focusmon(const Arg *arg);
static void focusstack(const Arg *arg);
static void gap_copy(Gap *to, const Gap *from);
static Atom getatomprop(Client *c, Atom prop);
static int getrootptr(int *x, int *y);
static long getstate(Window w);
static size_t gettextprop(Window w, Atom atom, char *text, unsigned int size);
static void grabbuttons(Client *c, int focused);
static void grabkeys(void);
static void incnmaster(const Arg *arg);
static void keypress(XEvent *e);
static void killclient(const Arg *arg);
static void manage(Window w, XWindowAttributes *wa);
static void mappingnotify(XEvent *e);
static void maprequest(XEvent *e);
static void monocle(Monitor *m);
static int mousebuttonmatch(unsigned int button, unsigned int mask);
static void motionnotify(XEvent *e);
static void movemouse(const Arg *arg);
static Client *nexttiled(Client *c);
static void pop(Client *c);
static void propertynotify(XEvent *e);
static void quit(const Arg *arg);
static Monitor *recttomon(int x, int y, int w, int h);
static void resize(Client *c, int x, int y, int w, int h, int interact);
static void resizeclient(Client *c, int x, int y, int w, int h);
static void resizemouse(const Arg *arg);
static void restack(Monitor *m);
static void run(void);
static void scan(void);
static int sendevent(Client *c, Atom proto);
static void sendmon(Client *c, Monitor *m);
static void setclientstate(Client *c, long state);
static void setfocus(Client *c);
static void setfullscreen(Client *c, int fullscreen);
static void killall(const char *name, const char *signal);
static void killatfullscreen_stop(void);
static void killatfullscreen_start(void);
static void killatfullscreen_resume(void);
static void setgaps(const Arg *arg);
static void setlayout(const Arg *arg);
static void setmfact(const Arg *arg);
static void setup(void);
static void seturgent(Client *c, int urg);
static void showhide(Client *c);
static void spawn(const Arg *arg);
static void tag(const Arg *arg);
static void tagmon(const Arg *arg);
static void tile(Monitor *m);
static void togglebar(const Arg *arg);
static void togglefloating(const Arg *arg);
static void togglefullscr(const Arg *arg);
static void toggletag(const Arg *arg);
static void toggleview(const Arg *arg);
static void unfocus(Client *c, int setfocus);
static void unmanage(Client *c, int destroyed);
static void unmapnotify(XEvent *e);
static void updatebarpos(Monitor *m);
static void updatebars(void);
static void updateclientlist(void);
static int updategeom(void);
static void updatenumlockmask(void);
static void updatesizehints(Client *c);
static void updatestatus(void);
static void updatetitle(Client *c);
static void updatewindowtype(Client *c);
static void updatewmhints(Client *c);
static void view(const Arg *arg);
static Client *wintoclient(Window w);
static Monitor *wintomon(Window w);
static void error_guard_begin(void);
static void error_guard_end(void);
static int xerror(Display *dpy, XErrorEvent *ee);
static int xerrordummy(Display *dpy, XErrorEvent *ee);
static int xerrorstart(Display *dpy, XErrorEvent *ee);
static void zoom(const Arg *arg);

static pid_t getparentprocess(pid_t p);
static int isdescprocess(pid_t p, pid_t c);
static Client *swallowingclient(Window w);
static Client *termforwin(const Client *c);
static pid_t winpid(Window w);

/* variables */
static const char broken[] = "broken";
static char stext[256];
static size_t stext_len = 0;
static int screen;
static int sw, sh;           /* X display screen geometry width, height */
static int bh;               /* bar height */
static int lrpad;            /* sum of left and right padding for text */
static int (*xerrorxlib)(Display *, XErrorEvent *);
static unsigned int numlockmask = 0;
static void (*handler[LASTEvent]) (XEvent *) = {
	[ButtonPress] = buttonpress,
	[ClientMessage] = clientmessage,
	[ConfigureRequest] = configurerequest,
	[ConfigureNotify] = configurenotify,
	[DestroyNotify] = destroynotify,
	[EnterNotify] = enternotify,
	[Expose] = expose,
	[FocusIn] = focusin,
	[KeyPress] = keypress,
	[MappingNotify] = mappingnotify,
	[MapRequest] = maprequest,
	[MotionNotify] = motionnotify,
	[PropertyNotify] = propertynotify,
	[UnmapNotify] = unmapnotify
};
static Atom wmatom[WMLast], netatom[NetLast];

/**
 * @brief Explicit process lifecycle: one atomic state, no contradictory pairs.
 * @details LIFE_LIVE -> LIFE_QUIT (clean exit) or LIFE_RESTART (shutdown followed
 *          by execvp()); stored in `volatile sig_atomic_t lifecycle` so signal
 *          handlers may advance it safely.
 * @par Threading and Reentrancy:
 *          The only state written from signal handlers; the event loop polls it.
 */
typedef enum {
	LIFE_LIVE = 0,       /**< event loop running */
	LIFE_QUIT,           /**< clean shutdown requested */
	LIFE_RESTART         /**< clean shutdown followed by execvp() */
} LifecycleState;
static volatile sig_atomic_t lifecycle = LIFE_LIVE;

/**
 * @brief Explicit state of the processes suspended for fullscreen.
 * @details PROC_RUNNING -> PROC_STOPPED (STOP sent) -> PROC_RESTARTING (CONT
 *          sent, HUP pending) -> back to PROC_RUNNING; avoids paired boolean
 *          flags that could disagree.
 * @par Threading and Reentrancy:
 *          Plain enum read and written only by the single-threaded event loop.
 */
typedef enum {
	PROC_RUNNING = 0,  /**< targets are running */
	PROC_STOPPED,      /**< STOP has been sent, no HUP pending */
	PROC_RESTARTING    /**< CONT sent, HUP may be pending */
} ProcessState;
/**
 * @brief Structured result of a monitor geometry reprobe.
 * @details Returned by updategeom() to describe added, removed and resized
 *          monitors plus the last removed monitor; see @ref arch_module_dwm
 *          (dwm.h).
 * @par Ownership:
 *          `removed_mon` may point at a monitor that has been freed, so treat it
 *          as a non-owning observation rather than a live handle.
 * @par Threading and Reentrancy:
 *          Plain value struct with no locks; produced on the single event loop.
 */
typedef struct {
	unsigned int added;      /**< monitors added */
	unsigned int removed;    /**< monitors removed */
	unsigned int resized;    /**< monitors whose geometry changed */
	Monitor *removed_mon;    /**< last removed monitor (may be freed) */
} GeomChange;
static GeomChange lastgeom;

static ProcessState procstate = PROC_RUNNING;
static unsigned int fs_epoch = 0;    /**< incremented on each fullscreen stop session */
static unsigned int start_epoch = 0; /**< epoch captured by the pending start */
static Cur *cursor[CurLast];
static Clr **scheme;
static Display *dpy;
static Drw *drw;
static Monitor *mons, *selmon;
static Window root, wmcheckwin;

static xcb_connection_t *xcon;
/* bar_dirty_segments and bar_exposed are per-monitor fields in struct Monitor.
 * drawbar() skips unchanged segments and early-returns with pixmap copy when all clean. */
#define DIRTY_STATUS 1
#define DIRTY_TAGS   2
#define DIRTY_TITLE  4
#define DIRTY_ALL    (DIRTY_STATUS | DIRTY_TAGS | DIRTY_TITLE)

#endif /* DWM_H */
