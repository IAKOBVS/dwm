/*
 * e2e_client.c — minimal X client for the Xvfb E2E smoke test.
 *
 * Creates one mapped 400x300 top-level window with WM_NAME/WM_CLASS set to
 * the given name, writes the window id to the optional file, then sleeps
 * forever so dwm has something to manage. It intentionally does NOT handle
 * WM_DELETE_WINDOW: killclient() must fall back to XKillClient.
 */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int
main(int argc, char **argv)
{
	Display *dpy;
	int screen;
	Window win;
	XClassHint ch;
	const char *name = argc > 1 ? argv[1] : "e2e";
	const char *widfile = argc > 2 ? argv[2] : NULL;

	if (!(dpy = XOpenDisplay(NULL)))
		return 1;

	screen = DefaultScreen(dpy);
	win = XCreateSimpleWindow(dpy, RootWindow(dpy, screen), 0, 0, 400, 300, 1,
		BlackPixel(dpy, screen), WhitePixel(dpy, screen));
	XStoreName(dpy, win, name);
	memset(&ch, 0, sizeof ch);
	ch.res_name = "e2e";
	ch.res_class = "E2e";
	XSetClassHint(dpy, win, &ch);
	XMapWindow(dpy, win);
	XFlush(dpy);

	if (widfile) {
		FILE *f = fopen(widfile, "w");

		if (f) {
			fprintf(f, "%lu\n", (unsigned long)win);
			fclose(f);
		}
	}

	for (;;)
		pause();
	return 0;
}
