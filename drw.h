/* See LICENSE file for copyright and license details. */

/**
 * @file drw.h
 * @brief Type definitions and public API for the drw drawing library.
 *
 * Provides an abstraction over X11/Xft drawing primitives used by dwm's bar.
 * Key types:
 *   Drw    — drawing context (display, drawable, GC, fonts, color schemes,
 *            emoji cache)
 *   Fnt    — linked-list font chain (fallback fonts for missing glyphs)
 *   Clr    — XftColor wrapper (ColFg/ColBg/ColBorder)
 *   Cur    — cursor wrapper
 *
 * The Drw struct owns a persistent XftDraw (→ Xft glyph cache) and an
 * emoji render cache that avoids repeat PNG-decompress for color emoji.
 *
 * Function contracts live at their definitions in `drw.c` (each carries an
 * `@anchor spec_<function>`); this header is the type/API surface.
 */

#ifndef DRW_H
#define DRW_H 1

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

/**
 * @brief An X cursor resource.
 */
typedef struct {
	Cursor cursor; /* X resource ID of the created cursor */
} Cur;

/**
 * @brief One loaded font in a fallback chain.
 *
 * The chain is tried in order: the first font that has a glyph wins. Every
 * node owns its `XftFont` and `FcPattern`.
 */
typedef struct Fnt {
	Display *dpy;       /* display the XftFont was opened on (needed to close it) */
	unsigned int h;     /* font line height (ascent + descent) */
	XftFont *xfont;     /* loaded Xft font */
	FcPattern *pattern; /* parsed name pattern; base for fallback matches */
	struct Fnt *next;   /* next font tried when a glyph is missing here */
} Fnt;

/** @brief Color scheme indices used by `Clr` arrays. */
enum { ColFg, ColBg, ColBorder }; /* Clr scheme index */

/** @brief An allocated Xft color (foreground, background, or border). */
typedef XftColor Clr;

/* Emoji render cache — caches rendered emoji pixmaps keyed by codepoint.
 * Avoids repeat FT_Load_Glyph → png_read_image → inflate for color emoji
 * that appear in multiple bar redraws. */
/** @brief Number of slots in the per-`Drw` emoji render cache. */
#define EMOJI_CACHE_SIZE 32

/**
 * @brief One rendered-emoji pixmap cache entry.
 *
 * Key is `(codepoint, fg, bg)`: the background is baked into the pixmap, so a
 * scheme change must miss and re-render.
 */
typedef struct {
	long codepoint;     /* -1 = empty */
	Pixmap pixmap;      /* cached rendered emoji pixmap (same depth as screen) */
	int w;              /* glyph width in pixels */
	unsigned long fg;   /* foreground pixel the pixmap was rendered with */
	unsigned long bg;   /* background pixel baked into the cached pixmap  */
	unsigned int generation; /* font generation the pixmap was rendered with */
} EmojiCacheSlot;

/**
 * @brief Drawing context: drawable, GC, fonts, schemes, and caches.
 *
 * All `drw_*` operations draw into `drawable`; `drw_map` copies the result to
 * a window. `xftd` is kept alive across draws so Xft's internal glyph cache
 * stays warm.
 */
typedef struct {
	unsigned int w, h;           /* drawable dimensions in pixels */
	Display *dpy;                /* X display */
	int screen;                  /* X screen number */
	Window root;                 /* root window (parent for pixmap/GC creation) */
	Drawable drawable;           /* off-screen pixmap drawn into, then copied to the bar */
	XftDraw *xftd;               /* persistent XftDraw — keeps Xft's internal glyph cache alive */
	GC gc;                       /* graphics context for rect fills / XCopyArea */
	Clr *scheme;                 /* active color scheme (array indexed by Col*) */
	Fnt *fonts;                  /* active font chain (first match wins) */
	EmojiCacheSlot emoji_cache[EMOJI_CACHE_SIZE]; /* rendered-emoji pixmap cache */
} Drw;

/* Drawable abstraction — contracts: spec_drw_create, spec_drw_resize,
 * spec_drw_free in drw.c. */
Drw *drw_create(Display *dpy, int screen, Window win, unsigned int w, unsigned int h);
void drw_resize(Drw *drw, unsigned int w, unsigned int h);
void drw_free(Drw *drw);

/* Fnt abstraction — contracts: spec_drw_fontset_create, spec_drw_fontset_free,
 * spec_drw_fontset_getwidth, spec_drw_fontset_getwidth_clamp,
 * spec_drw_fontset_invalidate_cache, spec_drw_font_getexts. */
Fnt *drw_fontset_create(Drw* drw, const char *fonts[], size_t fontcount);
void drw_fontset_free(Fnt* set);
unsigned int drw_fontset_getwidth(Drw *drw, const char *text);
unsigned int drw_fontset_getwidth_clamp(Drw *drw, const char *text, unsigned int n);
void drw_fontset_invalidate_cache(void);
void drw_font_getexts(Fnt *font, const char *text, unsigned int len, unsigned int *w, unsigned int *h);

/* Colorscheme abstraction — contracts: spec_drw_clr_create,
 * spec_drw_scm_create. */
void drw_clr_create(Drw *drw, Clr *dest, const char *clrname);
Clr *drw_scm_create(Drw *drw, const char *clrnames[], size_t clrcount);

/* Cursor abstraction — contracts: spec_drw_cur_create, spec_drw_cur_free. */
Cur *drw_cur_create(Drw *drw, int shape);
void drw_cur_free(Drw *drw, Cur *cursor);

/* Drawing context manipulation — contracts: spec_drw_setfontset,
 * spec_drw_setscheme. */
void drw_setfontset(Drw *drw, Fnt *set);
void drw_setscheme(Drw *drw, Clr *scm);

/* Drawing functions — contracts: spec_drw_rect, spec_drw_text. */
void drw_rect(Drw *drw, int x, int y, unsigned int w, unsigned int h, int filled, int invert);
int drw_text(Drw *drw, int x, int y, unsigned int w, unsigned int h, unsigned int lpad, const char *text, int invert);

/* Map functions — contract: spec_drw_map. */
void drw_map(Drw *drw, Window win, int x, int y, unsigned int w, unsigned int h);

#endif /* DRW_H */
