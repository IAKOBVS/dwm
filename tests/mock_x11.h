#ifndef MOCK_X11_H
#define MOCK_X11_H
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned long XID;
typedef XID Window;
typedef XID Drawable;
typedef XID Pixmap;
typedef XID Cursor;
typedef XID Colormap;
typedef XID Atom;
typedef unsigned long Time;
typedef unsigned long Mask;
typedef int Bool;
#define True 1
#define False 0
#define None 0L
#define CurrentTime 0L

typedef struct _XDisplay { int fd; } Display;
typedef struct { Window root; int width, height; int depth; } Screen;
typedef struct { void *data; } *GC;
typedef struct { unsigned long pixel; unsigned short red, green, blue; char alpha; } XftColor;
typedef struct _FcPattern FcPattern;
typedef struct { void *xfont; int ascent, descent; } XftFont;
typedef void *Visual;
typedef struct _XftDraw XftDraw;
typedef unsigned long KeySym;
typedef unsigned char KeyCode;

typedef struct {
	int type;
	unsigned long serial;
	Bool send_event;
	Display *display;
	Window window;
	Window root;
	Window subwindow;
	Time time;
	int x, y;
	int x_root, y_root;
	unsigned int state;
	unsigned int keycode;
	Bool same_screen;
} XKeyEvent;

typedef struct {
	int type;
	unsigned long serial;
	Bool send_event;
	Display *display;
	Window window;
	Window root;
	Window subwindow;
	Time time;
	int x, y;
	int x_root, y_root;
	unsigned int state;
	unsigned int button;
	Bool same_screen;
} XButtonEvent;
typedef XButtonEvent XButtonPressedEvent;

typedef struct {
	int type;
	unsigned long serial;
	Bool send_event;
	Display *display;
	Window window;
	Window root;
	Window subwindow;
	Time time;
	int x, y;
	int x_root, y_root;
	unsigned int state;
	char is_hint;
	Bool same_screen;
} XMotionEvent;

typedef struct {
	int type;
	unsigned long serial;
	Bool send_event;
	Display *display;
	Window window;
	Window root;
	Window subwindow;
	Time time;
	int x, y;
	int x_root, y_root;
	int mode;
	int detail;
	Bool same_screen;
	Bool focus;
	unsigned int state;
} XCrossingEvent;

typedef struct {
	int type;
	unsigned long serial;
	Bool send_event;
	Display *display;
	Window window;
	int x, y;
	int width, height;
	int count;
} XExposeEvent;

typedef struct {
	int type;
	unsigned long serial;
	Bool send_event;
	Display *display;
	Window event;
	Window window;
	int x, y;
	int width, height;
	int border_width;
	Window above;
	Bool override_redirect;
} XConfigureEvent;

typedef struct {
	int type;
	unsigned long serial;
	Bool send_event;
	Display *display;
	Window parent;
	Window window;
} XMapRequestEvent;

typedef struct {
	int type;
	unsigned long serial;
	Bool send_event;
	Display *display;
	Window event;
	Window window;
} XDestroyWindowEvent;

typedef struct {
	int type;
	unsigned long serial;
	Bool send_event;
	Display *display;
	Window window;
	int x, y;
	int width, height;
	int border_width;
	Window above;
	int detail;
	unsigned long value_mask;
} XConfigureRequestEvent;

typedef struct {
	int type;
	unsigned long serial;
	Bool send_event;
	Display *display;
	Window window;
	int mode, detail;
} XFocusChangeEvent;

typedef struct {
	int type;
	unsigned long serial;
	Bool send_event;
	Display *display;
	Window event;
	Window window;
	int request;
	int first_keycode;
	int count;
} XMappingEvent;

typedef struct {
	int type;
	unsigned long serial;
	Bool send_event;
	Display *display;
	Window window;
	Window event;
	Window parent;
	Bool override_redirect;
} XUnmapEvent;

typedef struct {
	int type;
	unsigned long serial;
	Bool send_event;
	Display *display;
	Window window;
	Atom atom;
	Time time;
	int state;
} XPropertyEvent;

typedef struct {
	int type;
	unsigned long serial;
	Bool send_event;
	Display *display;
	Window window;
	Atom message_type;
	int format;
	union { char b[20]; short s[10]; long l[5]; } data;
} XClientMessageEvent;

typedef struct {
	int type;
	unsigned long serial;
	Bool send_event;
	Display *display;
	Window window;
} XAnyEvent;

typedef struct {
	unsigned char *value;
	Atom encoding;
	int format;
	unsigned long nitems;
	unsigned long bytes_after;
} XTextProperty;
#define NXTextProperty XTextProperty

typedef struct {
	int type;
	unsigned long serial;
	unsigned char error_code;
	unsigned char request_code;
	unsigned char minor_code;
	XID resourceid;
} XErrorEvent;

typedef union {
	int type;
	XAnyEvent xany;
	XKeyEvent xkey;
	XButtonEvent xbutton;
	XMotionEvent xmotion;
	XCrossingEvent xcrossing;
	XFocusChangeEvent xfocus;
	XExposeEvent xexpose;
	XConfigureEvent xconfigure;
	XPropertyEvent xproperty;
	XClientMessageEvent xclient;
	XConfigureRequestEvent xconfigurerequest;
	XDestroyWindowEvent xdestroywindow;
	XMappingEvent xmapping;
	XMapRequestEvent xmaprequest;
	XUnmapEvent xunmap;
} XEvent;

typedef struct {
	int x, y;
	int width, height;
	int border_width;
	Window sibling;
	int stack_mode;
} XWindowChanges;

typedef struct {
	char *res_name;
	char *res_class;
} XClassHint;

typedef struct {
	long flags;
	int x, y;
	int width, height;
	int min_width, min_height;
	int max_width, max_height;
	int width_inc, height_inc;
	struct { int x, y; } min_aspect, max_aspect;
	int base_width, base_height;
	int win_gravity;
} XSizeHints;

#define PSize      1L
#define PResizeInc 2L
#define PMinSize   4L
#define PMaxSize   8L
#define PBaseSize  16L
#define PAspect    64L

typedef struct {
	long flags;
	Bool input;
	int initial_state;
	Pixmap icon_pixmap;
	Window icon_window;
	int icon_x, icon_y;
	Pixmap icon_mask;
	XID window_group;
} XWMHints;

#define InputHint      (1L << 0)
#define StateHint      (1L << 1)
#define XUrgencyHint   (1L << 8)

typedef struct {
	int max_keypermod;
	KeyCode *modifiermap;
} XModifierKeymap;

typedef struct {
	Pixmap background_pixmap;
	unsigned long background_pixel;
	Pixmap border_pixmap;
	unsigned long border_pixel;
	int bit_gravity;
	int win_gravity;
	int backing_store;
	unsigned long backing_planes;
	unsigned long backing_pixel;
	Bool save_under;
	long event_mask;
	long do_not_propagate_mask;
	Bool override_redirect;
	Colormap colormap;
	Cursor cursor;
} XSetWindowAttributes;

typedef struct {
	int x, y;
	int width, height;
	int border_width;
	int depth;
	Visual *visual;
	Window root;
	int xclass;
	int bit_gravity;
	int win_gravity;
	int backing_store;
	unsigned long backing_planes;
	unsigned long backing_pixel;
	Bool save_under;
	Colormap colormap;
	Bool map_installed;
	int map_state;
	long all_event_masks;
	long your_event_mask;
	long do_not_propagate_mask;
	Bool override_redirect;
	Screen *screen;
} XWindowAttributes;

#define IsUnmapped   0
#define IsUnviewable 0
#define IsViewable   2
#define LineSolid   0
#define CapButt     0
#define JoinMiter   0
#define IconicState  3
#define NormalState  1
#define WithdrawnState 0

#define NoEventMask              0L
#define KeyPressMask             (1L<<0)
#define KeyReleaseMask           (1L<<1)
#define ButtonPressMask          (1L<<2)
#define ButtonReleaseMask        (1L<<3)
#define EnterWindowMask          (1L<<4)
#define LeaveWindowMask          (1L<<5)
#define PointerMotionMask        (1L<<6)
#define ExposureMask             (1L<<15)
#define StructureNotifyMask      (1L<<17)
#define SubstructureRedirectMask (1L<<20)
#define SubstructureNotifyMask   (1L<<19)
#define FocusChangeMask          (1L<<21)
#define PropertyChangeMask       (1L<<22)

#define Button1         1
#define Button2         2
#define Button3         3
#define Button4         4
#define Button5         5

#define PropModeReplace 0
#define PropModeAppend  1
#define PropModePrepend 2

#define GrabModeSync    0
#define GrabModeAsync   1
#define GrabSuccess     0

#define RevertToParent  0
#define RevertToPointerRoot 1
#define RevertToNone    2

#define CWBorderWidth   (1L<<4)
#define CWX             (1L<<0)
#define CWY             (1L<<1)
#define CWWidth         (1L<<2)
#define CWHeight        (1L<<3)
#define CWSibling       (1L<<5)
#define CWStackMode     (1L<<6)
#define CWEventMask     (1L<<11)
#define CWCursor        (1L<<12)
#define CWBackPixmap    (1L<<13)
#define CWOverrideRedirect (1L<<18)

#define Below           2
#define Above           0
#define CopyFromParent  0

#define Success         0
#define BadMatch         8L
#define BadWindow        3L
#define BadDrawable      9L
#define BadAccess       10L

#define AnyModifier     0
#define AnyKey          0
#define AnyButton       0

#define XA_WM_NAME              ((Atom) 39)
#define XA_WM_TRANSIENT_FOR     ((Atom) 44)
#define XA_WM_NORMAL_HINTS      ((Atom) 40)
#define XA_WM_HINTS             ((Atom) 35)
#define XA_ATOM                 ((Atom) 4)
#define XA_WINDOW               ((Atom) 5)
#define XA_STRING               ((Atom) 31)
#define XA_CARDINAL             ((Atom) 6)
#define AnyPropertyType         ((Atom) 0L)

#define KeyPress        2
#define KeyRelease      3
#define ButtonPress     4
#define ButtonRelease   5
#define MotionNotify    6
#define EnterNotify     7
#define LeaveNotify     8
#define FocusIn         9
#define FocusOut        10
#define Expose          12
#define DestroyNotify   17
#define UnmapNotify     18
#define MapRequest      20
#define ConfigureNotify 22
#define ConfigureRequest 23
#define PropertyNotify  28
#define ClientMessage   33
#define MappingNotify   34
#define LASTEvent       36

#define NotifyNormal   0
#define NotifyInferior 2

#define X_ConfigureWindow    12
#define X_SetInputFocus      42
#define X_ChangeWindowAttributes  2
#define X_PolyText8          44
#define X_PolyFillRectangle  68
#define X_PolySegment        70
#define X_CopyArea           62
#define X_GrabButton         28
#define X_GrabKey            29

#define XC_left_ptr   68
#define XC_sizing     120
#define XC_fleur      52

#define DefaultScreen(dpy)        0
#define DisplayWidth(dpy, scr)    1920
#define DisplayHeight(dpy, scr)   1080
#define DisplayString(dpy)        ""
#define RootWindow(dpy, scr)      ((Window) 42L)
#define DefaultRootWindow(dpy)    ((Window) 42L)
#define DefaultDepth(dpy, scr)    24
#define DefaultVisual(dpy, scr)   ((Visual*)(void*)0)
#define DefaultColormap(dpy, scr) ((Colormap) 0L)
#define ConnectionNumber(dpy)     3
#define ScreenCount(dpy)          1

#define NotGrab             0
#define GrabSuccess         0
#define AlreadyGrabbed      1
#define GrabNotViewable     2
#define GrabFrozen          3
#define GrabModeSync        0
#define GrabModeAsync       1

#define NotifyNormal        0
#define NotifyGrab          1
#define NotifyUngrab        2
#define NotifyWhileGrabbed  3
#define NotifyNonlinear     4
#define NotifyNonlinearVirtual 5
#define NotifyPointer       6
#define NotifyPointerRoot   7
#define NotifyDetailNone    8

#define ParentRelative  1L

/* error & event constants */
#define PropertyDelete  2
#define ReplayPointer   1
#define PointerRoot     1
#define DestroyAll      0
#define MappingKeyboard 1

/* modifier masks */
#define ShiftMask       0x00000001
#define LockMask        0x00000002
#define ControlMask     0x00000004
#define Mod1Mask        0x00000008
#define Mod2Mask        0x00000008
#define Mod3Mask        0x00000020
#define Mod4Mask        0x00000080
#define Mod5Mask        0x00000100

/* keysyms */
#define XK_0            0x30
#define XK_1            0x31
#define XK_2            0x32
#define XK_3            0x33
#define XK_4            0x34
#define XK_5            0x35
#define XK_6            0x36
#define XK_7            0x37
#define XK_8            0x38
#define XK_9            0x39
#define XK_F1           0xFFBE
#define XK_F2           0xFFBF
#define XK_F3           0xFFC0
#define XK_F4           0xFFC1
#define XK_F5           0xFFC2
#define XK_F6           0xFFC3
#define XK_F7           0xFFC4
#define XK_F8           0xFFC5
#define XK_F9           0xFFC6
#define XK_F10          0xFFC7
#define XK_F11          0xFFC8
#define XK_F12          0xFFC9
#define XK_Return       0xFF0D
#define XK_Tab          0xFF09
#define XK_space        0x0200
#define XK_BackSpace    0xFF08
#define XK_Escape       0xFF1B
#define XK_Num_Lock     0xFF7F
#define XK_comma        0x2C
#define XK_period       0x2E
#define XK_minus        0x2D
#define XK_equal        0x3D
#define XK_grave        0x60
#define XK_a            0x61
#define XK_b            0x62
#define XK_c            0x63
#define XK_d            0x64
#define XK_e            0x65
#define XK_f            0x66
#define XK_g            0x67
#define XK_h            0x68
#define XK_i            0x69
#define XK_j            0x6A
#define XK_k            0x6B
#define XK_l            0x6C
#define XK_m            0x6D
#define XK_n            0x6E
#define XK_o            0x6F
#define XK_p            0x70
#define XK_q            0x71
#define XK_r            0x72
#define XK_s            0x73
#define XK_t            0x74
#define XK_u            0x75
#define XK_v            0x76
#define XK_w            0x77
#define XK_x            0x78
#define XK_y            0x79
#define XK_z            0x7A

typedef int Status;

/* Mock X11 function declarations */
Display *XOpenDisplay(const char *name);
void XCloseDisplay(Display *dpy);
int (*XSetErrorHandler(int (*handler)(Display *, XErrorEvent *)))(Display *, XErrorEvent *);
int (*XSetErrorHandler_ret(int (*handler)(Display *, XErrorEvent *)))(Display *, XErrorEvent *);
int XSupportsLocale(void);
Window XCreateSimpleWindow(Display *dpy, Window parent, int x, int y,
                           unsigned int w, unsigned int h, unsigned int bw,
                           unsigned long border, unsigned long background);
Window XCreateWindow(Display *dpy, Window parent, int x, int y,
                     unsigned int w, unsigned int h, unsigned int bw,
                     int depth, unsigned int cls, Visual *visual,
                     unsigned long mask, XSetWindowAttributes *attr);
void XDestroyWindow(Display *dpy, Window w);
void XMapWindow(Display *dpy, Window w);
void XMapRaised(Display *dpy, Window w);
void XUnmapWindow(Display *dpy, Window w);
void XMoveWindow(Display *dpy, Window w, int x, int y);
void XMoveResizeWindow(Display *dpy, Window w, int x, int y,
                       unsigned int width, unsigned int height);
void XResizeWindow(Display *dpy, Window w, unsigned int width, unsigned int height);
void XRaiseWindow(Display *dpy, Window w);
void XLowerWindow(Display *dpy, Window w);
void XConfigureWindow(Display *dpy, Window w, unsigned long mask,
                      XWindowChanges *wc);
void XSetWindowBorder(Display *dpy, Window w, unsigned long border);
void XSetWindowBorderWidth(Display *dpy, Window w, unsigned int width);
void XChangeWindowAttributes(Display *dpy, Window w, unsigned long mask,
                             XSetWindowAttributes *attr);
void XSelectInput(Display *dpy, Window w, long mask);
void XSetInputFocus(Display *dpy, Window w, int revert, Time time);
void XSync(Display *dpy, Bool discard);
void XDefineCursor(Display *dpy, Window w, Cursor cursor);
void XChangeProperty(Display *dpy, Window w, Atom property, Atom type,
                     int format, int mode, unsigned char *data, int nelements);
void XDeleteProperty(Display *dpy, Window w, Atom property);
/* Mock control variables — tests can set these to control mock return values */
extern const char *mock_class_res_class;
extern const char *mock_class_res_name;
extern int  mock_normal_hints_return; /* 0=fail, 1=success */
extern int  mock_normal_hints_flags;
extern int  mock_normal_hints_base_width;
extern int  mock_normal_hints_base_height;
extern int  mock_normal_hints_min_width;
extern int  mock_normal_hints_min_height;
extern int  mock_normal_hints_max_width;
extern int  mock_normal_hints_max_height;
extern int  mock_normal_hints_width_inc;
extern int  mock_normal_hints_height_inc;
extern int  mock_normal_hints_min_aspect_x;
extern int  mock_normal_hints_min_aspect_y;
extern int  mock_normal_hints_max_aspect_x;
extern int  mock_normal_hints_max_aspect_y;
extern int  mock_gettextprop_return; /* 0=fail, 1=success */
extern const char *mock_gettextprop_value;
extern Atom mock_gettextprop_encoding;
extern int  mock_getwindowproperty_return; /* 0=fail, 1=success */
extern Atom mock_getwindowproperty_atom;
extern int  mock_gettransient_return; /* 0=fail, 1=success */
extern Window mock_gettransient_win;
extern long mock_wmhints_flags;   /* XGetWMHints returns this as wmh->flags */
extern Bool  mock_wmhints_input;  /* XGetWMHints returns this as wmh->input */
extern int   mock_wmhints_return_null; /* 1 = XGetWMHints returns NULL */
extern int  mock_override_redirect; /* XGetWindowAttributes override_redirect */
extern int  mock_map_state;         /* XGetWindowAttributes map_state (IsViewable default) */
extern const char *mock_textlist_text; /* XmbTextPropertyToTextList: text to return in list */
extern int  mock_textlist_count;   /* XmbTextPropertyToTextList: count to return */
extern uint32_t mock_winpid_value; /* winpid return value (via xcb stubs) */
extern int  mock_winpid_set;       /* 1=use mock_winpid_value, 0=return NULL */

extern int  mock_keyboardmapping_return_null; /* 1 = XGetKeyboardMapping returns NULL */
extern KeySym mock_keyboardmapping_first_keysym; /* if non-zero, syms[0] = this value */

extern int  mock_modmap_has_numlock; /* 1 = XGetModifierMapping puts Num_Lock keycode in modmap */
extern int  mock_grabkey_calls;      /* counts XGrabKey invocations (grabkeys fan-out) */
extern int  mock_ungrabkey_calls;    /* counts XUngrabKey invocations */
extern int  mock_die_abort; /* 0=normal abort, 1=mock: set to 2 and return instead of aborting */

extern int   mock_wmprotocols_return; /* 0=fail, 1=success */
extern Atom *mock_wmprotocols_list;   /* array of atoms to return */
extern int   mock_wmprotocols_count;  /* number of atoms in list */

/* Event queue for XMaskEvent / XNextEvent / XCheckMaskEvent */
extern int   mock_event_queue_count;
extern XEvent mock_event_queue[8];

/* XQueryPointer control */
extern int mock_querypointer_return;   /* 0=False, 1=True */
extern int mock_querypointer_root_x;
extern int mock_querypointer_root_y;

/* XQueryTree control */
extern int    mock_querytree_return;   /* 0=False, 1=True */
extern Window mock_querytree_root;
extern Window *mock_querytree_children;
extern unsigned int mock_querytree_nchildren;

/* fork() control for spawn */
extern int mock_fork_return;  /* 0=child, >0=parent, -1=error (default) */

/* XGrabPointer return control */
extern int mock_grabpointer_return;  /* 0=GrabSuccess (default), non-zero=failure */
extern int mock_fontset_fail;        /* 0=normal (default), 1=drw_fontset_create returns NULL */

/* XGetWindowAttributes call-count control */
extern int mock_getwindowattr_call_count;  /* incremented on each call */
extern int mock_getwindowattr_fail_at;     /* fail (return 0) on call N; 0=never fail */

void mock_x11_reset(void);

void XSetClassHint(Display *dpy, Window w, XClassHint *classhint);
Atom XInternAtom(Display *dpy, const char *name, Bool only_if_exists);
int XGetWindowProperty(Display *dpy, Window w, Atom property, long offset,
                       long length, Bool del, Atom req_type, Atom *actual_type,
                       int *actual_format, unsigned long *nitems,
                       unsigned long *bytes_after, unsigned char **prop);
int XGetWMProtocols(Display *dpy, Window w, Atom **protocols, int *count);
int XGetWMNormalHints(Display *dpy, Window w, XSizeHints *hints, long *returned);
int XGetClassHint(Display *dpy, Window w, XClassHint *hint);
XWMHints *XGetWMHints(Display *dpy, Window w);
void XSetWMHints(Display *dpy, Window w, XWMHints *wmh);
int XGetTransientForHint(Display *dpy, Window w, Window *prop);
int XGetWindowAttributes(Display *dpy, Window w, XWindowAttributes *attr);
int XGetTextProperty(Display *dpy, Window w, XTextProperty *tp, Atom atom);
int XmbTextPropertyToTextList(Display *dpy, const XTextProperty *tp,
                              char ***list, int *count);
void XFreeStringList(char **list);
int XQueryTree(Display *dpy, Window w, Window *root_return,
               Window *parent_return, Window **children, unsigned int *nchildren);
int XQueryPointer(Display *dpy, Window w, Window *root_return,
                  Window *child_return, int *root_x, int *root_y,
                  int *win_x, int *win_y, unsigned int *mask);
void XFree(void *data);
void XWarpPointer(Display *dpy, Window src, Window dest,
                  int src_x, int src_y, unsigned int src_w, unsigned int src_h,
                  int dest_x, int dest_y);
void XAllowEvents(Display *dpy, int mode, Time time);
int XGrabPointer(Display *dpy, Window w, Bool owner_events, unsigned int mask,
                 int pointer_mode, int keyboard_mode, Window confine_to,
                 Cursor cursor, Time time);
void XUngrabPointer(Display *dpy, Time time);
int XGrabServer(Display *dpy);
void XUngrabServer(Display *dpy);
void XGrabButton(Display *dpy, unsigned int button, unsigned int modifiers,
                 Window grab_window, Bool owner_events, unsigned int mask,
                 int pointer_mode, int keyboard_mode, Window confine_to,
                 Cursor cursor);
void XUngrabButton(Display *dpy, unsigned int button, unsigned int modifiers,
                   Window w);
void XGrabKey(Display *dpy, int keycode, unsigned int modifiers, Window w,
              Bool owner_events, int pointer_mode, int keyboard_mode);
void XUngrabKey(Display *dpy, int keycode, unsigned int modifiers, Window w);
int XDisplayKeycodes(Display *dpy, int *min_keycodes, int *max_keycodes);
KeySym *XGetKeyboardMapping(Display *dpy, KeyCode first, int count, int *keysyms_per_keycode);
void XRefreshKeyboardMapping(XMappingEvent *ev);
KeySym XKeycodeToKeysym(Display *dpy, KeyCode keycode, int index);
KeyCode XKeysymToKeycode(Display *dpy, KeySym ks);
XModifierKeymap *XGetModifierMapping(Display *dpy);
void XFreeModifiermap(XModifierKeymap *m);
void XKillClient(Display *dpy, XID resource);
void XSetCloseDownMode(Display *dpy, int mode);
Cursor XCreateFontCursor(Display *dpy, unsigned int shape);
void XFreeCursor(Display *dpy, Cursor cursor);
Pixmap XCreatePixmap(Display *dpy, Drawable d, unsigned int w, unsigned int h,
                     unsigned int depth);
void XFreePixmap(Display *dpy, Pixmap p);
GC XCreateGC(Display *dpy, Drawable d, unsigned long mask, void *values);
void XFreeGC(Display *dpy, GC gc);
void XSetLineAttributes(Display *dpy, GC gc, unsigned int width,
                        int line_style, int cap_style, int join_style);
void XSetForeground(Display *dpy, GC gc, unsigned long color);
void XSetBackground(Display *dpy, GC gc, unsigned long color);
void XDrawRectangle(Display *dpy, Drawable d, GC gc,
                    int x, int y, unsigned int w, unsigned int h);
void XFillRectangle(Display *dpy, Drawable d, GC gc,
                    int x, int y, unsigned int w, unsigned int h);
void XCopyArea(Display *dpy, Drawable src, Drawable dest, GC gc,
               int src_x, int src_y, unsigned int w, unsigned int h,
               int dest_x, int dest_y);
int XSendEvent(Display *dpy, Window w, Bool propagate, long mask, XEvent *ev);
int XCheckMaskEvent(Display *dpy, long mask, XEvent *ev);
int XNextEvent(Display *dpy, XEvent *ev);
int XMaskEvent(Display *dpy, long mask, XEvent *ev);
int XPending(Display *dpy);
int XEventsQueued(Display *dpy, int mode);
KeySym XLookupKeysym(XKeyEvent *ev, int index);
int XLookupString(XKeyEvent *ev, char *buf, int len, KeySym *ks, void *comp);
int XBell(Display *dpy, int percent);
int XSetTransientForHint(Display *dpy, Window w, Window prop);
Status XGetIconSizes(Display *dpy, Window w, XSizeHints **size, int *count);

#endif /* MOCK_X11_H */
