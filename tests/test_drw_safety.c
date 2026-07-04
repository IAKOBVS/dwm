#define DWM_TEST 1
#define _GNU_SOURCE

#include "mock_x11.h"

typedef struct { int x; } XftDraw;
typedef unsigned char XftChar8;
typedef int XftResult;

XftDraw *XftDrawCreate(Display *dpy, Drawable drawable, Visual *visual, Colormap colormap) { return (XftDraw*)1; }
void XftDrawDestroy(XftDraw *draw) {}
void XftDrawStringUtf8(XftDraw *d, XftColor *color, XftFont *font, int x, int y, const XftChar8 *string, int len) {}
int XftCharExists(Display *dpy, XftFont *xfont, long codepoint) { return 1; }
void XftColorAllocName(Display *dpy, Visual *visual, Colormap cmap, const char *name, XftColor *dest) {}
void XftColorFree(Display *dpy, Visual *visual, Colormap cmap, XftColor *color) {}
XftFont *XftFontOpenName(Display *dpy, int screen, const char *name) { return (XftFont*)1; }
XftFont *XftFontOpenPattern(Display *dpy, FcPattern *pattern) { return (XftFont*)1; }
void XftFontClose(Display *dpy, XftFont *xfont) {}

FcPattern *FcNameParse(const FcChar8 *name) { return (FcPattern*)1; }
FcBool FcConfigSubstitute(FcConfig *config, FcPattern *p, FcMatchKind kind) { return 1; }
void FcDefaultSubstitute(FcPattern *pattern) {}
FcPattern *FcFontMatch(FcConfig *config, FcPattern *p, FcResult *result) { return (FcPattern*)1; }
void FcPatternDestroy(FcPattern *p) {}
FcCharSet *FcCharSetCreate(void) { return (FcCharSet*)1; }
void FcCharSetDestroy(FcCharSet *c) {}
FcBool FcCharSetAddChar(FcCharSet *c, FcChar32 ucs4) { return 1; }
FcResult FcPatternGetBool(const FcPattern *p, const char *object, int n, FcBool *b) { return 0; }
FcResult FcPatternAddCharSet(FcPattern *p, const char *object, const FcCharSet *c) { return 0; }
FcResult FcPatternGetInteger(const FcPattern *p, const char *object, int n, int *i) { return 0; }
FcResult FcPatternGetString(const FcPattern *p, const char *object, int n, FcChar8 **s) { return 0; }

#include "../util.c"
#include "../drw.c"

int main(void) {
    Display *dpy = XOpenDisplay(NULL);
    
    // drw_create with 0 width/height
    Drw *drw = drw_create(dpy, 0, 100, 0, 0);
    
    // drw_resize with 0 width/height
    drw_resize(drw, 0, 0);
    
    // Create scheme for drw_rect/drw_text
    const char *colors[] = { "#000000", "#ffffff", "#aaaaaa" };
    drw->scheme = drw_scm_create(drw, (const char **)colors, 3);
    
    // drw_rect with 0 width
    drw_rect(drw, 0, 0, 0, 0, 0, 0);
    
    // drw_text with extreme left padding
    drw_text(drw, 0, 0, 10, 10, 100, "test", 0);
    
    printf("Safety checks passed!\n");
    return 0;
}
