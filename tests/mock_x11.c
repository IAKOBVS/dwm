#include "mock_x11.h"
#include <errno.h>
#include <stdarg.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include <xcb/res.h>
#include <X11/Xft/Xft.h>

#include "../util.h"

static Window mock_next_win = 1000;
static Atom mock_next_atom = 100;
int _mock_x11_last_error_code = 0;

/* strdup is not C99; implement locally for portability */
static char *
mock_strdup(const char *s)
{
	size_t len;
	char *p;
	if (!s) return NULL;
	len = strlen(s);
	p = ecalloc(1, len + 1);
	memcpy(p, s, len);
	return p;
}

/* Mock control variable definitions (defaults match old behavior) */
const char *mock_class_res_class = NULL;
const char *mock_class_res_name  = NULL;
int  mock_normal_hints_return = 1;
int  mock_normal_hints_flags = PSize;
int  mock_normal_hints_base_width = 0;
int  mock_normal_hints_base_height = 0;
int  mock_normal_hints_min_width = 0;
int  mock_normal_hints_min_height = 0;
int  mock_normal_hints_max_width = 0;
int  mock_normal_hints_max_height = 0;
int  mock_normal_hints_width_inc = 0;
int  mock_normal_hints_height_inc = 0;
int  mock_normal_hints_min_aspect_x = 0;
int  mock_normal_hints_min_aspect_y = 0;
int  mock_normal_hints_max_aspect_x = 0;
int  mock_normal_hints_max_aspect_y = 0;
int  mock_gettextprop_return = 0;
const char *mock_gettextprop_value = NULL;
Atom mock_gettextprop_encoding = XA_STRING;
int  mock_getwindowproperty_return = 0;
Atom mock_getwindowproperty_atom = 0;
int  mock_gettransient_return = 0;
Window mock_gettransient_win = None;
long mock_wmhints_flags = InputHint;
Bool  mock_wmhints_input = True;
int   mock_wmhints_return_null = 0;
int  mock_override_redirect = 0;
int  mock_map_state = IsViewable;
const char *mock_textlist_text = NULL;
int  mock_textlist_count = 0;
uint32_t mock_winpid_value = 0;
int  mock_winpid_set =       0;
int  mock_keyboardmapping_return_null = 0;
KeySym mock_keyboardmapping_first_keysym = 0;
int  mock_modmap_has_numlock = 0;
int  mock_grabkey_calls = 0;
int  mock_ungrabkey_calls = 0;
int  mock_die_abort = 0;

int   mock_wmprotocols_return = 0;
Atom *mock_wmprotocols_list = NULL;
int   mock_wmprotocols_count = 0;

int   mock_event_queue_count = 0;
XEvent mock_event_queue[8];

int mock_querypointer_return = 1;
int mock_querypointer_root_x = 0;
int mock_querypointer_root_y = 0;

int    mock_querytree_return = 0;
Window mock_querytree_root = 0;
Window *mock_querytree_children = NULL;
unsigned int mock_querytree_nchildren = 0;

int mock_fork_return = -1;

int mock_grabpointer_return = 0;  /* 0=GrabSuccess, non-zero=failure */
int mock_fontset_fail = 0;        /* 0=normal, 1=drw_fontset_create returns NULL */

int mock_getwindowattr_call_count = 0;
int mock_getwindowattr_fail_at = 0;  /* 0=never fail */

void
mock_x11_reset(void)
{
	mock_class_res_class = NULL;
	mock_class_res_name = NULL;
	mock_normal_hints_return = 1;
	mock_normal_hints_flags = PSize;
	mock_normal_hints_base_width = 0;
	mock_normal_hints_base_height = 0;
	mock_normal_hints_min_width = 0;
	mock_normal_hints_min_height = 0;
	mock_normal_hints_max_width = 0;
	mock_normal_hints_max_height = 0;
	mock_normal_hints_width_inc = 0;
	mock_normal_hints_height_inc = 0;
	mock_normal_hints_min_aspect_x = 0;
	mock_normal_hints_min_aspect_y = 0;
	mock_normal_hints_max_aspect_x = 0;
	mock_normal_hints_max_aspect_y = 0;
	mock_gettextprop_return = 0;
	mock_gettextprop_value = NULL;
	mock_gettextprop_encoding = XA_STRING;
	mock_getwindowproperty_return = 0;
	mock_getwindowproperty_atom = 0;
	mock_gettransient_return = 0;
	mock_gettransient_win = None;
	mock_wmhints_flags = InputHint;
	mock_wmhints_input = True;
	mock_wmhints_return_null = 0;
	mock_override_redirect = 0;
	mock_map_state = IsViewable;
	mock_textlist_text = NULL;
	mock_textlist_count = 0;
	mock_winpid_value = 0;
	mock_winpid_set = 0;
	mock_keyboardmapping_return_null = 0;
	mock_keyboardmapping_first_keysym = 0;
	mock_modmap_has_numlock = 0;
	mock_grabkey_calls = 0;
	mock_ungrabkey_calls = 0;
	mock_die_abort = 0;
	mock_wmprotocols_return = 0;
	mock_wmprotocols_list = NULL;
	mock_wmprotocols_count = 0;
	mock_event_queue_count = 0;
	mock_querypointer_return = 1;
	mock_querypointer_root_x = 0;
	mock_querypointer_root_y = 0;
	mock_querytree_return = 0;
	mock_querytree_root = 0;
	mock_querytree_children = NULL;
	mock_querytree_nchildren = 0;
	mock_fork_return = -1;
	mock_grabpointer_return = 0;
	mock_fontset_fail = 0;
	mock_getwindowattr_call_count = 0;
	mock_getwindowattr_fail_at = 0;
}

Display *
XOpenDisplay(const char *name)
{
	Display *dpy = calloc(1, sizeof(Display));
	dpy->fd = -1;
	return dpy;
}

void
XCloseDisplay(Display *dpy)
{
	free(dpy);
}

int (*XSetErrorHandler(int (*handler)(Display *, XErrorEvent *)))(Display *, XErrorEvent *)
{
	(void)handler;
	return NULL;
}

int (*XSetErrorHandler_ret(int (*handler)(Display *, XErrorEvent *)))(Display *, XErrorEvent *)
{
	(void)handler;
	return NULL;
}

int
XSupportsLocale(void)
{
	return 1;
}

Window
XCreateSimpleWindow(Display *dpy, Window parent, int x, int y,
		    unsigned int w, unsigned int h, unsigned int bw,
		    unsigned long border, unsigned long background)
{
	(void)dpy; (void)parent; (void)x; (void)y;
	(void)w; (void)h; (void)bw; (void)border; (void)background;
	return mock_next_win++;
}

Window
XCreateWindow(Display *dpy, Window parent, int x, int y,
	      unsigned int w, unsigned int h, unsigned int bw,
	      int depth, unsigned int cls, Visual *visual,
	      unsigned long mask, XSetWindowAttributes *attr)
{
	(void)dpy; (void)parent; (void)x; (void)y;
	(void)w; (void)h; (void)bw; (void)depth; (void)cls;
	(void)visual; (void)mask; (void)attr;
	return mock_next_win++;
}

void
XDestroyWindow(Display *dpy, Window w)
{
	(void)dpy; (void)w;
}

void
XMapWindow(Display *dpy, Window w)
{
	(void)dpy; (void)w;
}

void
XMapRaised(Display *dpy, Window w)
{
	(void)dpy; (void)w;
}

void
XUnmapWindow(Display *dpy, Window w)
{
	(void)dpy; (void)w;
}

void
XMoveWindow(Display *dpy, Window w, int x, int y)
{
	(void)dpy; (void)w; (void)x; (void)y;
}

void
XMoveResizeWindow(Display *dpy, Window w, int x, int y,
		  unsigned int width, unsigned int height)
{
	(void)dpy; (void)w; (void)x; (void)y;
	(void)width; (void)height;
}

void
XResizeWindow(Display *dpy, Window w, unsigned int width, unsigned int height)
{
	(void)dpy; (void)w; (void)width; (void)height;
}

void
XRaiseWindow(Display *dpy, Window w)
{
	(void)dpy; (void)w;
}

void
XLowerWindow(Display *dpy, Window w)
{
	(void)dpy; (void)w;
}

void
XConfigureWindow(Display *dpy, Window w, unsigned long mask,
		 XWindowChanges *wc)
{
	(void)dpy; (void)w; (void)mask; (void)wc;
}

void
XSetWindowBorder(Display *dpy, Window w, unsigned long border)
{
	(void)dpy; (void)w; (void)border;
}

void
XSetWindowBorderWidth(Display *dpy, Window w, unsigned int width)
{
	(void)dpy; (void)w; (void)width;
}

void
XChangeWindowAttributes(Display *dpy, Window w, unsigned long mask,
			XSetWindowAttributes *attr)
{
	(void)dpy; (void)w; (void)mask; (void)attr;
}

void
XSelectInput(Display *dpy, Window w, long mask)
{
	(void)dpy; (void)w; (void)mask;
}

void
XSetInputFocus(Display *dpy, Window w, int revert, Time time)
{
	(void)dpy; (void)w; (void)revert; (void)time;
}

void
XSync(Display *dpy, Bool discard)
{
	(void)dpy; (void)discard;
}

void
XDefineCursor(Display *dpy, Window w, Cursor cursor)
{
	(void)dpy; (void)w; (void)cursor;
}

void
XChangeProperty(Display *dpy, Window w, Atom property, Atom type,
		int format, int mode, unsigned char *data, int nelements)
{
	(void)dpy; (void)w; (void)property; (void)type;
	(void)format; (void)mode; (void)data; (void)nelements;
}

void
XDeleteProperty(Display *dpy, Window w, Atom property)
{
	(void)dpy; (void)w; (void)property;
}

void
XSetClassHint(Display *dpy, Window w, XClassHint *classhint)
{
	(void)dpy; (void)w; (void)classhint;
}

Atom
XInternAtom(Display *dpy, const char *name, Bool only_if_exists)
{
	(void)dpy; (void)name; (void)only_if_exists;
	return mock_next_atom++;
}

int
XGetWindowProperty(Display *dpy, Window w, Atom property, long offset,
		   long length, Bool delete, Atom req_type, Atom *actual_type,
		   int *actual_format, unsigned long *nitems,
		   unsigned long *bytes_after, unsigned char **prop)
{
	(void)dpy; (void)w; (void)property; (void)offset; (void)length;
	(void)delete; (void)req_type;
	if (!mock_getwindowproperty_return) {
		*actual_type = None;
		*actual_format = 0;
		*nitems = 0;
		*bytes_after = 0;
		*prop = NULL;
		return 1; /* technically the mock always returns Success, but nitems=0 means fail */
	}
	*actual_type = XA_CARDINAL;
	*actual_format = 32;
	*nitems = 1;
	*bytes_after = 0;
	*prop = malloc(sizeof(Atom));
	if (*prop) *(Atom *)*prop = mock_getwindowproperty_atom;
	return Success;
}

int
XGetWMProtocols(Display *dpy, Window w, Atom **protocols, int *count)
{
	(void)dpy; (void)w;
	if (!mock_wmprotocols_return) {
		*protocols = NULL;
		*count = 0;
		return 0;
	}
	if (mock_wmprotocols_count > 0 && mock_wmprotocols_list) {
		*protocols = malloc(mock_wmprotocols_count * sizeof(Atom));
		memcpy(*protocols, mock_wmprotocols_list, mock_wmprotocols_count * sizeof(Atom));
	} else {
		*protocols = NULL;
	}
	*count = mock_wmprotocols_count;
	return 1;
}

int
XGetWMNormalHints(Display *dpy, Window w, XSizeHints *hints, long *returned)
{
	(void)dpy; (void)w;
	if (!mock_normal_hints_return) {
		*returned = 0;
		return 0;
	}
	memset(hints, 0, sizeof(*hints));
	hints->flags = mock_normal_hints_flags;
	hints->base_width  = mock_normal_hints_base_width;
	hints->base_height = mock_normal_hints_base_height;
	hints->min_width   = mock_normal_hints_min_width;
	hints->min_height  = mock_normal_hints_min_height;
	hints->max_width   = mock_normal_hints_max_width;
	hints->max_height  = mock_normal_hints_max_height;
	hints->width_inc   = mock_normal_hints_width_inc;
	hints->height_inc  = mock_normal_hints_height_inc;
	hints->min_aspect.x = mock_normal_hints_min_aspect_x;
	hints->min_aspect.y = mock_normal_hints_min_aspect_y;
	hints->max_aspect.x = mock_normal_hints_max_aspect_x;
	hints->max_aspect.y = mock_normal_hints_max_aspect_y;
	*returned = 0;
	return 1;
}

int
XGetClassHint(Display *dpy, Window w, XClassHint *hint)
{
	(void)dpy; (void)w;
	hint->res_class = mock_strdup(mock_class_res_class);
	hint->res_name  = mock_strdup(mock_class_res_name);
	return 1;
}

XWMHints *
XGetWMHints(Display *dpy, Window w)
{
	(void)dpy; (void)w;
	if (mock_wmhints_return_null)
		return NULL;
	XWMHints *wmh = calloc(1, sizeof(XWMHints));
	wmh->flags = mock_wmhints_flags;
	wmh->input = mock_wmhints_input;
	return wmh;
}

void
XSetWMHints(Display *dpy, Window w, XWMHints *wmh)
{
	(void)dpy; (void)w; (void)wmh;
}

int
XGetTransientForHint(Display *dpy, Window w, Window *prop)
{
	(void)dpy; (void)w;
	*prop = mock_gettransient_win;
	return mock_gettransient_return;
}

int
XGetWindowAttributes(Display *dpy, Window w, XWindowAttributes *attr)
{
	(void)dpy; (void)w;
	mock_getwindowattr_call_count++;
	if (mock_getwindowattr_fail_at > 0 && mock_getwindowattr_call_count >= mock_getwindowattr_fail_at)
		return 0;
	memset(attr, 0, sizeof(*attr));
	attr->map_state = mock_map_state;
	attr->override_redirect = mock_override_redirect;
	attr->width = 800;
	attr->height = 600;
	attr->x = 0;
	attr->y = 0;
	attr->border_width = 0;
	return 1;
}

int
XGetTextProperty(Display *dpy, Window w, XTextProperty *tp, Atom atom)
{
	(void)dpy; (void)w; (void)atom;
	if (!mock_gettextprop_return) return 0;
	if (tp) {
		tp->value = (unsigned char *)mock_strdup(mock_gettextprop_value);
		tp->encoding = mock_gettextprop_encoding;
		tp->format = 8;
		tp->nitems = mock_gettextprop_value ? strlen(mock_gettextprop_value) : 0;
	}
	return 1;
}

int
XmbTextPropertyToTextList(Display *dpy, const XTextProperty *tp,
			  char ***list, int *count)
{
	(void)dpy; (void)tp;
	if (!mock_textlist_text) {
		if (list) *list = NULL;
		if (count) *count = 0;
		return 0; /* Success but empty */
	}
	if (list) {
		*list = calloc(2, sizeof(char *));
		(*list)[0] = mock_strdup(mock_textlist_text);
		(*list)[1] = NULL;
	}
	if (count) *count = mock_textlist_count > 0 ? mock_textlist_count : 1;
	return 0; /* Success */
}

void
XFreeStringList(char **list)
{
	if (list) {
		int i;
		for (i = 0; list[i]; i++)
			free(list[i]);
		free(list);
	}
}

int
XQueryTree(Display *dpy, Window w, Window *root_return,
	   Window *parent_return, Window **children, unsigned int *nchildren)
{
	(void)dpy; (void)w;
	if (mock_querytree_return && mock_querytree_children) {
		*root_return = mock_querytree_root;
		*parent_return = 42;
		/* Return a heap-allocated copy so XFree (which calls free) is safe */
		*children = ecalloc(mock_querytree_nchildren, sizeof(Window));
		memcpy(*children, mock_querytree_children,
		       mock_querytree_nchildren * sizeof(Window));
		*nchildren = mock_querytree_nchildren;
		return 1;
	}
	*root_return = 42;
	*parent_return = 42;
	*children = NULL;
	*nchildren = 0;
	return 1;
}

int
XQueryPointer(Display *dpy, Window w, Window *root_return,
	      Window *child_return, int *root_x, int *root_y,
	      int *win_x, int *win_y, unsigned int *mask)
{
	(void)dpy; (void)w;
	*root_return = 42;
	*child_return = None;
	if (mock_querypointer_return) {
		*root_x = mock_querypointer_root_x;
		*root_y = mock_querypointer_root_y;
	} else {
		*root_x = 0; *root_y = 0;
	}
	*win_x = 0; *win_y = 0;
	*mask = 0;
	return mock_querypointer_return ? True : False;
}

void
XFree(void *data)
{
	/* Every pointer returned by these mocks is heap-allocated and dwm
	 * releases it here per Xlib contract; nothing else is ever passed. */
	free(data);
}

void
XWarpPointer(Display *dpy, Window src, Window dest,
	     int src_x, int src_y, unsigned int src_w, unsigned int src_h,
	     int dest_x, int dest_y)
{
	(void)dpy; (void)src; (void)dest;
	(void)src_x; (void)src_y; (void)src_w; (void)src_h;
	(void)dest_x; (void)dest_y;
}

void
XAllowEvents(Display *dpy, int mode, Time time)
{
	(void)dpy; (void)mode; (void)time;
}

int
XGrabPointer(Display *dpy, Window w, Bool owner_events, unsigned int mask,
	     int pointer_mode, int keyboard_mode, Window confine_to,
	     Cursor cursor, Time time)
{
	(void)dpy; (void)w; (void)owner_events; (void)mask;
	(void)pointer_mode; (void)keyboard_mode; (void)confine_to;
	(void)cursor; (void)time;
	return mock_grabpointer_return;
}

void
XUngrabPointer(Display *dpy, Time time)
{
	(void)dpy; (void)time;
}

int
XGrabServer(Display *dpy)
{
	(void)dpy;
	return 0;
}

void
XUngrabServer(Display *dpy)
{
	(void)dpy;
}

void
XGrabButton(Display *dpy, unsigned int button, unsigned int modifiers,
	    Window grab_window, Bool owner_events, unsigned int mask,
	    int pointer_mode, int keyboard_mode, Window confine_to,
	    Cursor cursor)
{
	(void)dpy; (void)button; (void)modifiers; (void)grab_window;
	(void)owner_events; (void)mask; (void)pointer_mode;
	(void)keyboard_mode; (void)confine_to; (void)cursor;
}

void
XUngrabButton(Display *dpy, unsigned int button, unsigned int modifiers,
	      Window w)
{
	(void)dpy; (void)button; (void)modifiers; (void)w;
}

void
XGrabKey(Display *dpy, int keycode, unsigned int modifiers, Window w,
	 Bool owner_events, int pointer_mode, int keyboard_mode)
{
	(void)dpy; (void)keycode; (void)modifiers; (void)w;
	(void)owner_events; (void)pointer_mode; (void)keyboard_mode;
	mock_grabkey_calls++;
}

void
XUngrabKey(Display *dpy, int keycode, unsigned int modifiers, Window w)
{
	(void)dpy; (void)keycode; (void)modifiers; (void)w;
	mock_ungrabkey_calls++;
}

int
XDisplayKeycodes(Display *dpy, int *min_keycodes, int *max_keycodes)
{
	(void)dpy;
	*min_keycodes = 8;
	*max_keycodes = 255;
	return 0;
}

KeySym *
XGetKeyboardMapping(Display *dpy, KeyCode first, int count, int *keysyms_per_keycode)
{
	(void)dpy; (void)first; (void)count;
	if (mock_keyboardmapping_return_null)
		return NULL;
	*keysyms_per_keycode = 2;
	KeySym *syms = calloc((size_t)count * 2, sizeof(KeySym));
	if (syms && mock_keyboardmapping_first_keysym)
		syms[0] = mock_keyboardmapping_first_keysym;
	return syms;
}

void
XRefreshKeyboardMapping(XMappingEvent *ev)
{
	(void)ev;
}

KeySym
XKeycodeToKeysym(Display *dpy, KeyCode keycode, int index)
{
	(void)dpy; (void)index;
	return (KeySym)keycode;
}

XModifierKeymap *
XGetModifierMapping(Display *dpy)
{
	(void)dpy;
	XModifierKeymap *m = calloc(1, sizeof(*m));
	m->max_keypermod = 1;
	m->modifiermap = calloc(8, sizeof(KeyCode));
	if (mock_modmap_has_numlock) {
		/* Put Num_Lock keycode (XKeysymToKeycode returns 0x7F=127)
		 * in the Mod3 (index 2) slot so updatenumlockmask() finds it */
		m->modifiermap[2] = 127;
	}
	return m;
}

void
XFreeModifiermap(XModifierKeymap *m)
{
	if (m) {
		free(m->modifiermap);
		free(m);
	}
}

void
XKillClient(Display *dpy, XID resource)
{
	(void)dpy; (void)resource;
}

void
XSetCloseDownMode(Display *dpy, int mode)
{
	(void)dpy; (void)mode;
}

Cursor
XCreateFontCursor(Display *dpy, unsigned int shape)
{
	(void)dpy; (void)shape;
	return (Cursor)1;
}

void
XFreeCursor(Display *dpy, Cursor cursor)
{
	(void)dpy; (void)cursor;
}

Pixmap
XCreatePixmap(Display *dpy, Drawable d, unsigned int w, unsigned int h,
	      unsigned int depth)
{
	assert(w > 0 && h > 0);
	(void)dpy; (void)d; (void)w; (void)h; (void)depth;
	return (Pixmap)mock_next_win++;
}

void
XFreePixmap(Display *dpy, Pixmap p)
{
	(void)dpy; (void)p;
}

GC
XCreateGC(Display *dpy, Drawable d, unsigned long mask, void *values)
{
	(void)dpy; (void)d; (void)mask; (void)values;
	GC gc = calloc(1, sizeof(void *));
	return gc;
}

void
XFreeGC(Display *dpy, GC gc)
{
	(void)dpy;
	free(gc);
}

void
XSetLineAttributes(Display *dpy, GC gc, unsigned int width,
		   int line_style, int cap_style, int join_style)
{
	(void)dpy; (void)gc; (void)width;
	(void)line_style; (void)cap_style; (void)join_style;
}

void
XSetForeground(Display *dpy, GC gc, unsigned long color)
{
	(void)dpy; (void)gc; (void)color;
}

void
XSetBackground(Display *dpy, GC gc, unsigned long color)
{
	(void)dpy; (void)gc; (void)color;
}

void
XDrawRectangle(Display *dpy, Drawable d, GC gc,
	       int x, int y, unsigned int w, unsigned int h)
{
	assert(w < 100000 && h < 100000);
	(void)dpy; (void)d; (void)gc; (void)x; (void)y; (void)w; (void)h;
}

void
XFillRectangle(Display *dpy, Drawable d, GC gc,
	       int x, int y, unsigned int w, unsigned int h)
{
	assert(w < 100000 && h < 100000);
	(void)dpy; (void)d; (void)gc; (void)x; (void)y; (void)w; (void)h;
}

void
XCopyArea(Display *dpy, Drawable src, Drawable dest, GC gc,
	  int src_x, int src_y, unsigned int w, unsigned int h,
	  int dest_x, int dest_y)
{
	(void)dpy; (void)src; (void)dest; (void)gc;
	(void)src_x; (void)src_y; (void)w; (void)h;
	(void)dest_x; (void)dest_y;
}

int
XSendEvent(Display *dpy, Window w, Bool propagate, long mask, XEvent *ev)
{
	(void)dpy; (void)w; (void)propagate; (void)mask; (void)ev;
	return 1;
}

/* Map X11 event type to its corresponding event-mask bit(s). */
static long
event_type_to_mask(int type)
{
	switch (type) {
	case KeyPress:      return KeyPressMask;
	case KeyRelease:    return KeyReleaseMask;
	case ButtonPress:   return ButtonPressMask;
	case ButtonRelease: return ButtonReleaseMask;
	case MotionNotify:  return PointerMotionMask;
	case EnterNotify:   return EnterWindowMask;
	case LeaveNotify:   return LeaveWindowMask;
	case FocusIn:
	case FocusOut:      return FocusChangeMask;
	case Expose:        return ExposureMask;
	case DestroyNotify: return StructureNotifyMask;
	case UnmapNotify:   return StructureNotifyMask;
	case MapRequest:    return SubstructureRedirectMask;
	case ConfigureNotify:  return StructureNotifyMask;
	case ConfigureRequest: return SubstructureRedirectMask;
	case PropertyNotify:   return PropertyChangeMask;
	case ClientMessage:    return SubstructureNotifyMask;
	default:              return 0;
	}
}

int
XCheckMaskEvent(Display *dpy, long mask, XEvent *ev)
{
	(void)dpy;
	for (int i = 0; i < mock_event_queue_count; i++) {
		if (event_type_to_mask(mock_event_queue[i].type) & mask) {
			*ev = mock_event_queue[i];
			for (int j = i; j < mock_event_queue_count - 1; j++)
				mock_event_queue[j] = mock_event_queue[j+1];
			mock_event_queue_count--;
			return True;
		}
	}
	return False;
}

int
XNextEvent(Display *dpy, XEvent *ev)
{
	(void)dpy;
	if (mock_event_queue_count > 0) {
		*ev = mock_event_queue[0];
		for (int i = 0; i < mock_event_queue_count - 1; i++)
			mock_event_queue[i] = mock_event_queue[i+1];
		mock_event_queue_count--;
		return 0;
	}
	/* No more events: signal end by returning error */
	return -1;
}

int
XMaskEvent(Display *dpy, long mask, XEvent *ev)
{
	(void)dpy;
	for (int i = 0; i < mock_event_queue_count; i++) {
		if (event_type_to_mask(mock_event_queue[i].type) & mask) {
			*ev = mock_event_queue[i];
			for (int j = i; j < mock_event_queue_count - 1; j++)
				mock_event_queue[j] = mock_event_queue[j+1];
			mock_event_queue_count--;
			return 0;
		}
	}
	/* No matching event in queue — real Xlib would block */
	ev->type = 0;
	return -1;
}

int
XPending(Display *dpy)
{
	(void)dpy;
	return 0;
}

int
XEventsQueued(Display *dpy, int mode)
{
	(void)dpy; (void)mode;
	return 0;
}

KeySym
XLookupKeysym(XKeyEvent *ev, int index)
{
	(void)ev; (void)index;
	return 0;
}

int
XLookupString(XKeyEvent *ev, char *buf, int len, KeySym *ks, void *comp)
{
	(void)ev; (void)buf; (void)len; (void)comp;
	*ks = 0;
	return 0;
}

int
XBell(Display *dpy, int percent)
{
	(void)dpy; (void)percent;
	return 0;
}

int
XSetTransientForHint(Display *dpy, Window w, Window prop)
{
	(void)dpy; (void)w; (void)prop;
	return 0;
}

Status
XGetIconSizes(Display *dpy, Window w, XSizeHints **size, int *count)
{
	(void)dpy; (void)w;
	*size = NULL;
	*count = 0;
	return 0;
}

/* ecalloc and die from util.c */
void *
ecalloc(size_t nmemb, size_t size)
{
	void *p;
	if (!(p = calloc(nmemb, size)))
		DIE("calloc():calloc:");
	return p;
}

/* fork() mock for spawn() testing.
 * When mock_fork_return == -1 (default), returns -1 to indicate error.
 * Tests that need real fork() should set mock_fork_return = 0 and handle
 * child/parent logic themselves, or use mock_die_abort. */
#undef fork
pid_t
fork(void)
{
	return (pid_t)mock_fork_return;
}

void
die(const char *file, int line, const char *func, const char *fmt, ...)
{
	va_list ap;
	if (mock_die_abort) {
		mock_die_abort = 2;
		return;
	}
	fprintf(stderr, "%s:%d: %s(): ", file, line, func);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, " errno (%d): %s\n", errno, strerror(errno));
	abort();
}

/* XKeysymToKeycode stub */
KeyCode
XKeysymToKeycode(Display *dpy, KeySym ks)
{
	(void)dpy;
	return (KeyCode)(ks & 0xFF);
}

/* xcb stubs for winpid */
static uint32_t mock_winpid_pid;
static xcb_res_client_id_value_t *mock_cid_value;

xcb_res_query_client_ids_cookie_t
xcb_res_query_client_ids(xcb_connection_t *c, uint32_t spec_len, const void *spec)
{
	(void)c; (void)spec_len; (void)spec;
	xcb_res_query_client_ids_cookie_t cookie = {0};
	return cookie;
}

xcb_res_query_client_ids_reply_t *
xcb_res_query_client_ids_reply(xcb_connection_t *c, xcb_res_query_client_ids_cookie_t cookie, xcb_generic_error_t **e)
{
	(void)c; (void)cookie; (void)e;
	if (!mock_winpid_set)
		return NULL;
	/* Return a non-NULL pointer; the iterator stub ignores it anyway */
	return calloc(1, 256);
}

xcb_res_client_id_value_iterator_t
xcb_res_query_client_ids_ids_iterator(void *r)
{
	(void)r;
	xcb_res_client_id_value_iterator_t i = {0};
	if (mock_winpid_set) {
		mock_winpid_pid = mock_winpid_value;
		/* Allocate struct + space for 1 uint32_t value inline */
		free(mock_cid_value);
		mock_cid_value = calloc(1, sizeof(xcb_res_client_id_value_t) + sizeof(uint32_t));
		mock_cid_value->spec.mask = XCB_RES_CLIENT_ID_MASK_LOCAL_CLIENT_PID;
		mock_cid_value->spec.client = 0;
		mock_cid_value->length = 1;
		/* value data is stored inline after the struct */
		memcpy((char *)mock_cid_value + sizeof(xcb_res_client_id_value_t),
		       &mock_winpid_pid, sizeof(uint32_t));
		i.data = mock_cid_value;
		i.rem = 1;
	}
	return i;
}

void
xcb_res_client_id_value_next(xcb_res_client_id_value_iterator_t *i)
{
	(void)i;
}

uint32_t *
xcb_res_client_id_value_value(void *data)
{
	(void)data;
	if (!mock_winpid_set)
		return NULL;
	/* value data is inline after the struct */
	return (uint32_t *)((char *)mock_cid_value + sizeof(xcb_res_client_id_value_t));
}

/* ------------------------------------------------------------------ */
/* Xft / Fontconfig stubs  (for drw.c testing)                         */
/* ------------------------------------------------------------------ */

struct _XftDraw { void *dummy; };  /* complete the forward decl from mock_x11.h */

XftDraw *
XftDrawCreate(void *dpy, unsigned long drawable, void *visual, unsigned long colormap)
{
	static struct _XftDraw draw;
	(void)dpy; (void)drawable; (void)visual; (void)colormap;
	return &draw;
}

void
XftDrawDestroy(XftDraw *draw)
{
	(void)draw;
}

void
XftDrawStringUtf8(XftDraw *draw, void *color, void *font,
		  int x, int y, const FcChar8 *string, int len)
{
	(void)draw; (void)color; (void)font;
	(void)x; (void)y; (void)string; (void)len;
}

int
XftCharExists(void *dpy, void *xfont, unsigned long codepoint)
{
	(void)dpy; (void)xfont; (void)codepoint;
	return 1;
}

void
XftTextExtentsUtf8(void *dpy, void *xfont,
		   const FcChar8 *text, int len, void *ext)
{
	(void)dpy; (void)xfont; (void)text; (void)len;
	(void)ext;
}

int
XftColorAllocName(void *dpy, void *visual, unsigned long colormap,
		  const char *name, void *dest)
{
	(void)dpy; (void)visual; (void)colormap; (void)name; (void)dest;
	return 1;
}

void
XftColorFree(void *dpy, void *visual, unsigned long colormap, void *color)
{
	(void)dpy; (void)visual; (void)colormap; (void)color;
}

void *
XftFontOpenName(void *dpy, int screen, const char *name)
{
	(void)dpy; (void)screen; (void)name;
	return NULL;
}

void *
XftFontOpenPattern(void *dpy, void *pattern)
{
	(void)dpy; (void)pattern;
	return NULL;
}

void
XftFontClose(void *dpy, void *font)
{
	(void)dpy; (void)font;
}

void *
XftFontMatch(void *dpy, int screen, void *pattern, void *result)
{
	(void)dpy; (void)screen; (void)pattern; (void)result;
	return NULL;
}

void *
FcNameParse(const FcChar8 *name)
{
	(void)name;
	return NULL;
}

FcBool
FcConfigSubstitute(void *config, void *pattern, FcMatchKind kind)
{
	(void)config; (void)pattern; (void)kind;
	return 0;
}

void
FcDefaultSubstitute(void *pattern)
{
	(void)pattern;
}

void *
FcFontMatch(void *config, void *pattern, FcResult *result)
{
	(void)config; (void)pattern;
	if (result) *result = 0;
	return NULL;
}

void
FcPatternDestroy(void *pattern)
{
	(void)pattern;
}

void *
FcPatternDuplicate(const void *pattern)
{
	(void)pattern;
	return NULL;
}

FcBool
FcPatternGetBool(const void *p, const char *object, int n, FcBool *b)
{
	(void)p; (void)object; (void)n;
	if (b) *b = FcFalse;
	return 0;
}

FcBool
FcPatternAddBool(void *p, const char *object, FcBool b)
{
	(void)p; (void)object; (void)b;
	return 0;
}

FcBool
FcPatternAddCharSet(void *p, const char *object, FcCharSet *c)
{
	(void)p; (void)object; (void)c;
	return 0;
}

FcBool
FcPatternGetCharSet(const void *p, const char *object, int n, FcCharSet **c)
{
	(void)p; (void)object; (void)n;
	if (c) *c = NULL;
	return 0;
}

FcBool
FcPatternGetString(const void *p, const char *object, int n, FcChar8 **s)
{
	(void)p; (void)object; (void)n;
	if (s) *s = NULL;
	return 0;
}

FcBool
FcPatternGetInteger(const void *p, const char *object, int n, int *i)
{
	(void)p; (void)object; (void)n;
	if (i) *i = 0;
	return 0;
}

FcCharSet *
FcCharSetCreate(void)
{
	return calloc(1, 4);
}

void
FcCharSetDestroy(FcCharSet *cs)
{
	free(cs);
}

FcBool
FcCharSetAddChar(FcCharSet *cs, FcChar32 ucs4)
{
	(void)cs; (void)ucs4;
	return 0;
}
