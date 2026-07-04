#include "mock_x11.h"
#include <stdarg.h>
#include <assert.h>
#include <stdint.h>
#include <xcb/xcb.h>
#include <xcb/res.h>

/* die forward declaration for ecalloc */
void die(const char *fmt, ...);

static Window mock_next_win = 1000;
static Atom mock_next_atom = 100;
int _mock_x11_last_error_code = 0;

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
	*actual_type = XA_CARDINAL;
	*actual_format = 32;
	*nitems = 0;
	*bytes_after = 0;
	*prop = NULL;
	return Success;
}

int
XGetWMProtocols(Display *dpy, Window w, Atom **protocols, int *count)
{
	(void)dpy; (void)w;
	*protocols = NULL;
	*count = 0;
	return 0;
}

int
XGetWMNormalHints(Display *dpy, Window w, XSizeHints *hints, long *returned)
{
	(void)dpy; (void)w;
	memset(hints, 0, sizeof(*hints));
	hints->flags = PSize;
	*returned = 0;
	return 1;
}

int
XGetClassHint(Display *dpy, Window w, XClassHint *hint)
{
	(void)dpy; (void)w;
	hint->res_name = NULL;
	hint->res_class = NULL;
	return 1;
}

XWMHints *
XGetWMHints(Display *dpy, Window w)
{
	(void)dpy; (void)w;
	XWMHints *wmh = calloc(1, sizeof(XWMHints));
	wmh->flags = InputHint;
	wmh->input = True;
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
	*prop = None;
	return 0;
}

int
XGetWindowAttributes(Display *dpy, Window w, XWindowAttributes *attr)
{
	(void)dpy; (void)w;
	memset(attr, 0, sizeof(*attr));
	attr->map_state = IsViewable;
	attr->override_redirect = False;
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
	(void)dpy; (void)w; (void)tp; (void)atom;
	return 0;
}

int
XmbTextPropertyToTextList(Display *dpy, const XTextProperty *tp,
			  char ***list, int *count)
{
	(void)dpy; (void)tp; (void)list; (void)count;
	return 0;
}

void
XFreeStringList(char **list)
{
	(void)list;
}

int
XQueryTree(Display *dpy, Window w, Window *root_return,
	   Window *parent_return, Window **children, unsigned int *nchildren)
{
	(void)dpy; (void)w;
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
	*root_x = 0; *root_y = 0;
	*win_x = 0; *win_y = 0;
	*mask = 0;
	return 1;
}

void
XFree(void *data)
{
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
	return GrabSuccess;
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
}

void
XUngrabKey(Display *dpy, int keycode, unsigned int modifiers, Window w)
{
	(void)dpy; (void)keycode; (void)modifiers; (void)w;
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
	*keysyms_per_keycode = 2;
	return calloc((size_t)count * 2, sizeof(KeySym));
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

int
XCheckMaskEvent(Display *dpy, long mask, XEvent *ev)
{
	(void)dpy; (void)mask;
	ev->type = 0;
	return 0;
}

int
XNextEvent(Display *dpy, XEvent *ev)
{
	(void)dpy;
	ev->type = 0;
	return 0;
}

int
XMaskEvent(Display *dpy, long mask, XEvent *ev)
{
	(void)dpy; (void)mask;
	ev->type = 0;
	return 0;
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
		die("calloc failed");
	return p;
}

void
die(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
	exit(1);
}

/* XKeysymToKeycode stub */
KeyCode
XKeysymToKeycode(Display *dpy, KeySym ks)
{
	(void)dpy;
	return (KeyCode)(ks & 0xFF);
}

/* xcb stubs for winpid */
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
	return NULL;
}

xcb_res_client_id_value_iterator_t
xcb_res_query_client_ids_ids_iterator(void *r)
{
	(void)r;
	xcb_res_client_id_value_iterator_t i = {0};
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
	return NULL;
}
