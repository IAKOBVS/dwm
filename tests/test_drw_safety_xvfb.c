#include <stdio.h>
#include <assert.h>
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include "../drw.h"

int main(void) {
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "Failed to open display (is Xvfb running?)\n");
        return 1;
    }
    
    // drw_create with 0 width/height
    Drw *drw = drw_create(dpy, DefaultScreen(dpy), DefaultRootWindow(dpy), 0, 0);
    assert(drw != NULL);
    assert(drw->w >= 1); // Verify clamping
    assert(drw->h >= 1); // Verify clamping
    
    // drw_resize with 0 width/height
    drw_resize(drw, 0, 0);
    assert(drw->w >= 1);
    assert(drw->h >= 1);
    
    // Create scheme for drw_rect/drw_text
    const char *colors[] = { "#000000", "#ffffff", "#aaaaaa" };
    drw->scheme = drw_scm_create(drw, (const char **)colors, 3);
    
    // drw_rect with 0 width/height (should return early, no crash)
    drw_rect(drw, 0, 0, 0, 0, 0, 0);
    
    // drw_text with extreme left padding (should not crash/underflow)
    // Needs a basic font to not fail early.
    drw->fonts = drw_fontset_create(drw, (const char *[]){"monospace:size=10"}, 1);
    if (drw->fonts) {
        drw_text(drw, 0, 0, 10, 10, 100, "test", 0);
    }
    
    printf("Safety checks passed successfully!\n");
    return 0;
}
