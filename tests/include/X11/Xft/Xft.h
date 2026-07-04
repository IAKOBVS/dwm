#ifndef __X11_XFT_XFT_H
#define __X11_XFT_XFT_H

/* Minimal Xft types for compiling drw.c in test mode.
 * XftColor is in mock_x11.h; these are the remaining types
 * needed by drw_fontset_getwidth / glyph_getwidth / drw_text. */

typedef unsigned char FcChar8;
typedef int FcBool;
typedef struct _FcCharSet FcCharSet;
typedef struct _XftDraw XftDraw;

typedef struct {
	int xOff;
	int yOff;
	int width;
	int height;
} XGlyphInfo;

/* XftFont is in mock_x11.h; it's forward-declared as a
 * struct with { void *xfont; int ascent, descent; } */

/* Function declarations — implemented as test stubs */
int XftCharExists(void *dpy, void *xfont, unsigned long codepoint);
void XftTextExtentsUtf8(void *dpy, void *xfont, const FcChar8 *text,
                         int len, XGlyphInfo *ext);
XftDraw *XftDrawCreate(void *dpy, void *drawable, int screen,
                        unsigned long colormap);
void XftDrawDestroy(void *dpy, XftDraw *draw);
void XftDrawStringUtf8(void *dpy, XftDraw *draw, void *color,
                        void *font, int x, int y,
                        const FcChar8 *string, int len);
void *XftFontOpenName(void *dpy, int screen, const char *name);
void *XftFontOpenPattern(void *dpy, void *pattern);
void XftFontClose(void *dpy, void *font);
void FcPatternDestroy(void *pattern);
void FcPatternDel(void *p, const char *object);
FcBool FcPatternGetCharSet(void *p, const char *object, int n,
                            FcCharSet **c);

#endif /* __X11_XFT_XFT_H */
