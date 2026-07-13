#ifndef __X11_XFT_XFT_H
#define __X11_XFT_XFT_H

/* Minimal Xft/Fontconfig type and function declarations for compiling
 * drw.c in test mode.  Signatures match the real Xft API 1:1, but
 * opaque types are simplified to void* / int / unsigned long.
 *
 * Implementations live in mock_x11.c.
 */

typedef unsigned char FcChar8;
typedef unsigned long FcChar32;
typedef int FcBool;
typedef int FcResult;
typedef int FcMatchKind;
typedef int XftResult;
typedef unsigned char XftChar8;
#define FcTrue  1
#define FcFalse 0
#define FcMatchPattern 0
#define FcMatchFont    1

typedef struct _FcCharSet FcCharSet;
typedef struct _XftDraw XftDraw;
typedef struct _FcPattern FcPattern;

typedef struct {
	int xOff;
	int yOff;
	int width;
	int height;
} XGlyphInfo;

/* Real Xft signatures — opaque types mapped to void* / int / unsigned long */
XftDraw *XftDrawCreate(void *dpy, unsigned long drawable, void *visual,
                        unsigned long colormap);
void XftDrawDestroy(XftDraw *draw);
void XftDrawStringUtf8(XftDraw *draw, void *color, void *font,
                        int x, int y, const FcChar8 *string, int len);
int XftCharExists(void *dpy, void *xfont, unsigned long codepoint);
void XftTextExtentsUtf8(void *dpy, void *xfont,
                         const FcChar8 *text, int len, void *ext);
int XftColorAllocName(void *dpy, void *visual, unsigned long colormap,
                       const char *name, void *dest);
void XftColorFree(void *dpy, void *visual, unsigned long colormap,
                   void *color);
void *XftFontOpenName(void *dpy, int screen, const char *name);
void *XftFontOpenPattern(void *dpy, void *pattern);
void XftFontClose(void *dpy, void *font);
void *XftFontMatch(void *dpy, int screen, void *pattern, void *result);

/* Fontconfig functions */
void *FcNameParse(const FcChar8 *name);
FcBool FcConfigSubstitute(void *config, void *pattern, FcMatchKind kind);
void FcDefaultSubstitute(void *pattern);
void *FcFontMatch(void *config, void *pattern, FcResult *result);
void FcPatternDestroy(void *pattern);
void *FcPatternDuplicate(const void *pattern);
FcBool FcPatternGetBool(const void *p, const char *object, int n, FcBool *b);
FcBool FcPatternAddBool(void *p, const char *object, FcBool b);
FcBool FcPatternAddCharSet(void *p, const char *object, FcCharSet *c);
FcBool FcPatternGetCharSet(const void *p, const char *object, int n,
                            FcCharSet **c);
FcBool FcPatternGetString(const void *p, const char *object, int n,
                           FcChar8 **s);
FcBool FcPatternGetInteger(const void *p, const char *object, int n, int *i);
FcCharSet *FcCharSetCreate(void);
void FcCharSetDestroy(FcCharSet *cs);
FcBool FcCharSetAddChar(FcCharSet *cs, FcChar32 ucs4);

#define FC_CHARSET  "charset"
#define FC_SCALABLE "scalable"
#define FC_FAMILY   "family"
#define FC_STYLE    "style"
#define FC_SLANT    "slant"
#define FC_WEIGHT   "weight"
#define FC_SIZE     "size"
#define FC_FONTVERSION "fontversion"
#define FC_LANG     "lang"

#endif /* __X11_XFT_XFT_H */
