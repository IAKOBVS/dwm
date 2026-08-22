/* dwm.h — central header for the dynamic window manager.
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
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <X11/Xft/Xft.h>

#include "drw.h"

#define GAP_TOGGLE 100
#define GAP_RESET  0

/* safe string helpers — memcpy + NUL-terminate with known source length */
static inline void
strcpy_len(char *dst, const char *src, size_t src_len)
{
	memcpy(dst, src, src_len);
	dst[src_len] = '\0';
}

/* like strcpy_len but returns pointer to the NUL terminator */
static inline char *
stpcpy_len(char *dst, const char *src, size_t src_len)
{
	memcpy(dst, src, src_len);
	dst[src_len] = '\0';
	return dst + src_len;
}

/* bounded copy — like strncpy but with explicit source length, no padding */
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

typedef union {
	int i;
	unsigned int ui;
	float f;
	const void *v;
} Arg;

typedef struct {
	unsigned int click;
	unsigned int mask;
	unsigned int button;
	void (*func)(const Arg *arg);
	const Arg arg;
} Button;

typedef struct Monitor Monitor;
typedef struct Client Client;
struct Client {
	char name[256];
	float mina, maxa;
	int x, y, w, h;
	int oldx, oldy, oldw, oldh;
	int basew, baseh, incw, inch, maxw, maxh, minw, minh, hintsvalid;
	int bw, oldbw;
	unsigned int tags;
	unsigned char isfixed, isfloating, isurgent, neverfocus, oldstate, isfullscreen, isterminal, noswallow;
	pid_t pid;
	Client *next;
	Client *snext;
	Client *swallowing;
	Monitor *mon;
	Window win;
};

typedef struct {
	unsigned int mod;
	KeySym keysym;
	void (*func)(const Arg *);
	const Arg arg;
} Key;

static KeySym key_keysym_used;
static unsigned int key_mod_used;
static unsigned long button_button_used;
static unsigned int button_mask_used;

typedef struct {
	const char *symbol;
	void (*arrange)(Monitor *);
} Layout;

typedef struct {
	short realgap;
	short gappx;
	unsigned char isgap;
} Gap;

struct Monitor {
	char ltsymbol[16];
	float mfact;
	int nmaster;
	int num;
	int by;               /* bar geometry */
	int mx, my, mw, mh;   /* screen size */
	int wx, wy, ww, wh;   /* window area  */
	int bar_dirty_segments;
	unsigned int seltags;
	unsigned int sellt;
	unsigned int tagset[2];
	unsigned char showbar;
	unsigned char topbar;
	unsigned char bar_exposed;
	Gap gap;
	Client *clients;
	Client *sel;
	Client *stack;
	Monitor *next;
	Window barwin;
	const Layout *lt[2];
};

typedef struct {
	const char *class;
	const char *instance;
	const char *title;
	unsigned int tags;
	unsigned char isfloating;
	unsigned char isterminal;
	unsigned char noswallow;
	int monitor;
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
static int restart = 0;
static int running = 1;
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

#endif /* DWM_H */
