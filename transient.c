/* cc transient.c -o transient -lX11
 *
 * transient.c — standalone test program for dwm transient-window handling.
 *
 * Purpose:
 *   Creates a fixed-size 400×400 "floating" window, then after 5 seconds
 *   creates a 100×100 "transient" window with WM_TRANSIENT_FOR set to the
 *   first window.  This lets you verify that dwm correctly recognises
 *   transients and handles them as floating by default.
 *
 * Build:
 *   cc transient.c -o transient -lX11
 *
 * Expected dwm behavior:
 *   - The floating window should appear (dwm marks it floating).
 *   - After 5 s the transient appears, floating, on top of its parent.
 */

#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

int main(void) {
	Display *d;
	Window r, f, t = None;
	XSizeHints h;
	XEvent e;

	d = XOpenDisplay(NULL);
	if (!d)
		exit(1);
	r = DefaultRootWindow(d);

	/* Create a fixed-size 400×400 window — dwm should honour the size hints
	 * and treat it as floating (PMinSize == PMaxSize → isfixed). */
	f = XCreateSimpleWindow(d, r, 100, 100, 400, 400, 0, 0, 0);
	h.min_width = h.max_width = h.min_height = h.max_height = 400;
	h.flags = PMinSize | PMaxSize;
	XSetWMNormalHints(d, f, &h);
	XStoreName(d, f, "floating");
	XMapWindow(d, f);

	XSelectInput(d, f, ExposureMask);
	while (1) {
		XNextEvent(d, &e);

		if (t == None) {
			sleep(5);
			/* Create a transient child — dwm should keep it floating
			 * and stacked above the parent. */
			t = XCreateSimpleWindow(d, r, 50, 50, 100, 100, 0, 0, 0);
			XSetTransientForHint(d, t, f);
			XStoreName(d, t, "transient");
			XMapWindow(d, t);
			XSelectInput(d, t, ExposureMask);
		}
	}

	XCloseDisplay(d);
	exit(0);
}
