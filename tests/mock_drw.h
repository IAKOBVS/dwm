#ifndef MOCK_DRW_H
#define MOCK_DRW_H

#include "mock_x11.h"

/* Stub types matching drw.h */
typedef struct { Cursor cursor; } Cur;
typedef struct Fnt {
	Display *dpy;
	unsigned int h;
	XftFont *xfont;
	FcPattern *pattern;
	struct Fnt *next;
} Fnt;
enum { ColFg, ColBg, ColBorder };
typedef XftColor Clr;

typedef struct {
	unsigned int w, h;
	Display *dpy;
	int screen;
	Window root;
	Drawable drawable;
	GC gc;
	Clr *scheme;
	Fnt *fonts;
} Drw;

/* No-op stubs — these never actually render */
static inline Drw *
drw_create(Display *dpy, int screen, Window root,
	   unsigned int w, unsigned int h)
{
	Drw *drw = calloc(1, sizeof(Drw));
	drw->dpy = dpy;
	drw->screen = screen;
	drw->root = root;
	drw->w = w;
	drw->h = h;
	return drw;
}

static inline void
drw_resize(Drw *drw, unsigned int w, unsigned int h)
{
	if (!drw) return;
	drw->w = w;
	drw->h = h;
}

static inline void
drw_free(Drw *drw)
{
	free(drw);
}

static inline Fnt *
drw_fontset_create(Drw *drw, const char *fonts[], size_t fontcount)
{
	(void)fonts; (void)fontcount;
	Fnt *fnt = calloc(1, sizeof(Fnt));
	fnt->h = 15;
	drw->fonts = fnt;
	return fnt;
}

static inline void
drw_fontset_free(Fnt *font)
{
	(void)font;
}

static inline unsigned int
drw_fontset_getwidth(Drw *drw, const char *text)
{
	(void)drw;
	if (!text) return 0;
	return (unsigned int)strlen(text) * 10;
}

static inline unsigned int
drw_fontset_getwidth_clamp(Drw *drw, const char *text, unsigned int n)
{
	unsigned int w = drw_fontset_getwidth(drw, text);
	return n < w ? n : w;
}

static inline void
drw_fontset_invalidate_cache(void)
{
}

static inline void
drw_font_getexts(Fnt *font, const char *text, unsigned int len,
		 unsigned int *w, unsigned int *h)
{
	if (w) *w = len * 10;
	if (h) *h = font ? font->h : 15;
}

static inline void
drw_clr_create(Drw *drw, Clr *dest, const char *clrname)
{
	(void)drw; (void)dest; (void)clrname;
}

static inline Clr *
drw_scm_create(Drw *drw, const char *clrnames[], size_t clrcount)
{
	(void)drw; (void)clrnames;
	return calloc(clrcount, sizeof(Clr));
}

static inline Cur *
drw_cur_create(Drw *drw, int shape)
{
	(void)drw; (void)shape;
	Cur *cur = calloc(1, sizeof(Cur));
	return cur;
}

static inline void
drw_cur_free(Drw *drw, Cur *cursor)
{
	(void)drw;
	free(cursor);
}

static inline void
drw_setfontset(Drw *drw, Fnt *set)
{
	if (drw) drw->fonts = set;
}

static inline void
drw_setscheme(Drw *drw, Clr *scm)
{
	if (drw) drw->scheme = scm;
}

static inline void
drw_rect(Drw *drw, int x, int y, unsigned int w, unsigned int h,
	 int filled, int invert)
{
	(void)drw; (void)x; (void)y; (void)w; (void)h;
	(void)filled; (void)invert;
}

static inline int
drw_text(Drw *drw, int x, int y, unsigned int w, unsigned int h,
	 unsigned int lpad, const char *text, int invert)
{
	(void)drw; (void)x; (void)y; (void)w; (void)h;
	(void)lpad; (void)text; (void)invert;
	return 0;
}

static inline void
drw_map(Drw *drw, Window win, int x, int y,
	unsigned int w, unsigned int h)
{
	(void)drw; (void)win; (void)x; (void)y; (void)w; (void)h;
}

#endif /* MOCK_DRW_H */
