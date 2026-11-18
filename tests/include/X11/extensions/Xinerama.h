/*
 * Xinerama.h — minimal mock of the Xinerama extension interface.
 *
 * Only the symbols dwm.c references under -DXINERAMA are declared here.
 * The isuniquegeom test provides its own stub definitions; this header
 * exists so the test can compile dwm.c with XINERAMA enabled without a
 * real Xinerama installation.
 */

#ifndef DWM_MOCK_XINERAMA_H
#define DWM_MOCK_XINERAMA_H 1

#include <X11/Xlib.h>

typedef struct {
	int screen_number;
	short x_org;
	short y_org;
	unsigned short width;
	unsigned short height;
} XineramaScreenInfo;

Bool XineramaIsActive(Display *dpy);
XineramaScreenInfo *XineramaQueryScreens(Display *dpy, int *number);

#endif /* DWM_MOCK_XINERAMA_H */
