/* See LICENSE file for copyright and license details. */
/*
 * drw.c — lightweight X11/Xft drawing library for dwm.
 *
 * Architecture overview
 * ---------------------
 * This file provides text and rectangle rendering primitives used
 * primarily for dwm's status bar.  Key subsystems:
 *
 *   Glyph-width cache (GLYPH_CACHE_SIZE=64, open-addressing hash)
 *     caches per-codepoint advance widths for non-ASCII glyphs.
 *     ASCII chars bypass the cache and call XftTextExtentsUtf8 directly.
 *
 *   Emoji render cache (EMOJI_CACHE_SIZE=32, open-addressing hash)
 *     caches rendered emoji pixmaps so that the expensive
 *     FT_Load_Glyph → png_read_image → inflate pipeline runs once per
 *     unique codepoint per font cycle, not once per drawbar redraw.
 *     Cached pixmaps are blitted via XCopyArea on subsequent draws.
 *
 *   UTF-8 decode helpers (utf8decodebyte/utf8validate/utf8decode)
 *     decode multi-byte sequences, reject surrogates and out-of-range
 *     values, and return the UTF-32 codepoint + byte length.
 *
 *   Font-chain fallback (Fnt linked list)
 *     drw_text iterates the user-specified font chain for each codepoint;
 *     if none match it calls XftFontMatch to find a system fallback and
 *     appends it to the chain.  A small static set of "no-match"
 *     codepoints avoids repeating the XftFontMatch call for characters
 *     known to be missing from the system.
 *
 *   Persistent XftDraw (drw->xftd, created in drw_create)
 *     Xft caches rendered glyphs per-drawable.  By keeping the XftDraw
 *     alive across bar redraws we avoid repeat FT_Load_Glyph work even
 *     for uncached emoji.  Re-created in drw_resize when the backing
 *     pixmap changes size.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

#include "drw.h"
#include "util.h"

#define UTF_INVALID 0xFFFD
#define UTF_SIZ     4

/*
 * Glyph-width cache — caches per-emoji widths keyed by Unicode codepoint.
 * Only non-ASCII codepoints (> 0x7F) are cached since ASCII chars have no
 * emoji PNG overhead. Cache is invalidated on font change via
 * drw_fontset_invalidate_cache().
 */
#define GLYPH_CACHE_SIZE 64

static struct {
	long codepoint; /* -1 = empty */
	unsigned int width;
} glyph_cache[GLYPH_CACHE_SIZE];

/*
 * Look up or measure the advance width of a non-ASCII codepoint.
 * First checks the open-addressing glyph_cache; on miss walks the font
 * chain, calls XftCharExists + drw_font_getexts, and populates the cache.
 * Returns 0 if no font in the chain covers the codepoint.
 * drw, utf8str, utf8len: used only when measuring (cache miss).
 */
static unsigned int
glyph_getwidth(Drw *drw, long codepoint, const char *utf8str, unsigned int utf8len)
{
	unsigned int i = (unsigned int)codepoint & (GLYPH_CACHE_SIZE - 1);
	unsigned int probe = 0;
	Fnt *font;
	unsigned int tmpw;

	/* open-addressing probe: search the slot at index (codepoint % GLYPH_CACHE_SIZE)
	 * and linear-probe forward, wrapping at GLYPH_CACHE_SIZE. */
	while (glyph_cache[i].codepoint != -1) {
		if (glyph_cache[i].codepoint == codepoint)
			return glyph_cache[i].width;
		i = (i + 1) & (GLYPH_CACHE_SIZE - 1);
		if (++probe >= GLYPH_CACHE_SIZE)
			break;
	}

	for (font = drw->fonts; font; font = font->next) {
		if (XftCharExists(drw->dpy, font->xfont, codepoint)) {
			drw_font_getexts(font, utf8str, utf8len, &tmpw, NULL);
			if (probe < GLYPH_CACHE_SIZE) {
				glyph_cache[i].codepoint = codepoint;
				glyph_cache[i].width = tmpw;
			}
			return tmpw;
		}
	}
	return 0;
}

/*
 * Look up a codepoint in the emoji render cache.  Open-addressing hash
 * with linear probing; returns the slot index on hit, -1 on miss.
 */
static int
emoji_cache_lookup(Drw *drw, long codepoint)
{
	unsigned int i = (unsigned int)codepoint & (EMOJI_CACHE_SIZE - 1);
	unsigned int probe = 0;

	while (drw->emoji_cache[i].codepoint != -1) {
		if (drw->emoji_cache[i].codepoint == codepoint)
			return (int)i;
		i = (i + 1) & (EMOJI_CACHE_SIZE - 1);
		if (++probe >= EMOJI_CACHE_SIZE)
			break;
	}
	return -1;
}

/*
 * Insert a newly rendered emoji pixmap into the emoji render cache.
 * Uses open-addressing with linear probing; evicts (and frees) any
 * existing entry at the target slot.  The pixmap must have been created
 * from the same-depth drawable and is later blitted with XCopyArea.
 */
static void
emoji_cache_insert(Drw *drw, long codepoint, Pixmap pixmap, int w)
{
	unsigned int i = (unsigned int)codepoint & (EMOJI_CACHE_SIZE - 1);
	unsigned int probe = 0;

	while (drw->emoji_cache[i].codepoint != -1 && drw->emoji_cache[i].codepoint != codepoint) {
		i = (i + 1) & (EMOJI_CACHE_SIZE - 1);
		if (++probe >= EMOJI_CACHE_SIZE)
			break;
	}

	if (drw->emoji_cache[i].codepoint != -1 && drw->emoji_cache[i].pixmap)
		XFreePixmap(drw->dpy, drw->emoji_cache[i].pixmap);

	drw->emoji_cache[i].codepoint = codepoint;
	drw->emoji_cache[i].pixmap = pixmap;
	drw->emoji_cache[i].w = w;
}

/*
 * Clear the entire emoji render cache, freeing each cached pixmap.
 * Called from drw_fontset_create (font change invalidates all cached
 * glyphs) and from drw_free (teardown).
 */
static void
emoji_cache_invalidate(Drw *drw)
{
	for (int i = 0; i < EMOJI_CACHE_SIZE; i++) {
		if (drw->emoji_cache[i].codepoint != -1 && drw->emoji_cache[i].pixmap)
			XFreePixmap(drw->dpy, drw->emoji_cache[i].pixmap);
		drw->emoji_cache[i].codepoint = -1;
		drw->emoji_cache[i].pixmap = 0;
		drw->emoji_cache[i].w = 0;
	}
}

/* UTF-8 decode tables: index 0 = continuation byte, 1 = ASCII,
 * 2 = 2-byte start, 3 = 3-byte start, 4 = 4-byte start.
 * utfbyte[mask]  — the bit pattern that a byte of type `i` must match;
 *                   continuation byte (0x80) is used as the mask check.
 * utfmask[t]     — the mask to isolate the leading bits.
 * utfmin[i]      — smallest codepoint valid for a sequence of length i.
 * utfmax[i]      — largest codepoint valid for a sequence of length i.
 */
static const unsigned char utfbyte[UTF_SIZ + 1] = { 0x80, 0, 0xC0, 0xE0, 0xF0 };
static const unsigned char utfmask[UTF_SIZ + 1] = { 0xC0, 0x80, 0xE0, 0xF0, 0xF8 };
static const long utfmin[UTF_SIZ + 1] = { 0, 0, 0x80, 0x800, 0x10000 };
static const long utfmax[UTF_SIZ + 1] = { 0x10FFFF, 0x7F, 0x7FF, 0xFFFF, 0x10FFFF };

/*
 * Classify a UTF-8 byte and extract its data bits.
 * Returns the payload bits (low 6/5/4/3 bits depending on byte type).
 * Sets *i to the sequence length (1–4) or 0 for continuation bytes.
 */
static long
utf8decodebyte(const char c, size_t *i)
{
	for (*i = 0; *i < (UTF_SIZ + 1); ++(*i))
		if (((unsigned char)c & utfmask[*i]) == utfbyte[*i])
			return (unsigned char)c & ~utfmask[*i];
	return 0;
}

/*
 * Validate a decoded UTF-32 codepoint: reject surrogates
 * (U+D800–U+DFFF) and out-of-range values, replacing with
 * UTF_INVALID.  Returns the minimal byte-sequence length
 * that can represent the (possibly corrected) codepoint.
 */
static size_t
utf8validate(long *u, size_t i)
{
	if (!BETWEEN(*u, utfmin[i], utfmax[i]) || BETWEEN(*u, 0xD800, 0xDFFF))
		*u = UTF_INVALID;
	for (i = 1; *u > utfmax[i]; ++i)
		;
	return i;
}

/*
 * Decode a UTF-8 byte sequence of at most clen bytes into *u.
 * Returns the number of bytes consumed (0 = incomplete sequence).
 * On invalid starting byte, advances 1 position and sets *u = UTF_INVALID.
 */
static size_t
utf8decode(const char *c, long *u, size_t clen)
{
	size_t i, j, len, type;
	long udecoded;

	*u = UTF_INVALID;
	if (!clen)
		return 0;
	udecoded = utf8decodebyte(c[0], &len);
	if (!BETWEEN(len, 1, UTF_SIZ))
		return 1;
	for (i = 1, j = 1; i < clen && j < len; ++i, ++j) {
		udecoded = (udecoded << 6) | utf8decodebyte(c[i], &type);
		if (type)
			return j;
	}
	if (j < len)
		return 0;
	*u = udecoded;
	utf8validate(u, len);

	return len;
}

/*
 * Create a drawing context: allocates an off-screen pixmap, a GC, and a
 * persistent XftDraw tied to the pixmap.  The XftDraw is long-lived so
 * that Xft's internal glyph cache (FT_Load_Glyph results) survives
 * across bar redraws.  Returns NULL on allocation failure.
 */
Drw *
drw_create(Display *dpy, int screen, Window root, unsigned int w, unsigned int h)
{
	Drw *drw = ecalloc(1, sizeof(Drw));

	drw->dpy = dpy;
	drw->screen = screen;
	drw->root = root;
	drw->w = w = w ? w : 1;
	drw->h = h = h ? h : 1;
	drw->drawable = XCreatePixmap(dpy, root, w, h, DefaultDepth(dpy, screen));
	drw->gc = XCreateGC(dpy, root, 0, NULL);
	/* persistent XftDraw: Xft caches rendered glyphs per-drawable;
	 * we keep the XftDraw alive across redraws so the cached glyphs
	 * (FT_Load_Glyph / png_read / inflate results) are not discarded. */
	drw->xftd = XftDrawCreate(dpy, drw->drawable, DefaultVisual(dpy, screen), DefaultColormap(dpy, screen));
	XSetLineAttributes(dpy, drw->gc, 1, LineSolid, CapButt, JoinMiter);

	for (int i = 0; i < EMOJI_CACHE_SIZE; i++)
		drw->emoji_cache[i].codepoint = -1;

	return drw;
}

/*
 * Resize the backing pixmap and re-create the XftDraw.  The old XftDraw
 * was tied to the old pixmap and must be destroyed — Xft will internally
 * free any cached glyph images for that drawable.  The emoji render cache
 * and glyph-width cache survive (font chain is unchanged).
 */
void
drw_resize(Drw *drw, unsigned int w, unsigned int h)
{
	if (!drw)
		return;

	drw->w = w = w ? w : 1;
	drw->h = h = h ? h : 1;
	if (drw->drawable)
		XFreePixmap(drw->dpy, drw->drawable);
	XftDrawDestroy(drw->xftd); /* old XftDraw was tied to old pixmap */
	drw->drawable = XCreatePixmap(drw->dpy, drw->root, w, h, DefaultDepth(drw->dpy, drw->screen));
	/* re-create XftDraw tied to the new pixmap drawable */
	drw->xftd = XftDrawCreate(drw->dpy, drw->drawable, DefaultVisual(drw->dpy, drw->screen), DefaultColormap(drw->dpy, drw->screen));
}

/*
 * Free a drawing context and all associated resources: emoji cache,
 * XftDraw, backing pixmap, GC, and the font chain.
 */
void
drw_free(Drw *drw)
{
	emoji_cache_invalidate(drw);
	XftDrawDestroy(drw->xftd); /* destroy XftDraw before its pixmap is freed */
	XFreePixmap(drw->dpy, drw->drawable);
	XFreeGC(drw->dpy, drw->gc);
	drw_fontset_free(drw->fonts);
	free(drw);
}

/*
 * Load a single XftFont from either a font name string or an FcPattern.
 * When loading from a name we also store the parsed FcPattern so that
 * drw_text can use it as a base for FcFontMatch fallback lookups.
 * Returns NULL on failure.  Internal helper — use drw_fontset_create.
 */
static Fnt *
xfont_create(Drw *drw, const char *fontname, FcPattern *fontpattern)
{
	Fnt *font;
	XftFont *xfont = NULL;
	FcPattern *pattern = NULL;

	if (fontname) {
		/* Using the pattern found at font->xfont->pattern does not yield the
		 * same substitution results as using the pattern returned by
		 * FcNameParse; using the latter results in the desired fallback
		 * behaviour whereas the former just results in missing-character
		 * rectangles being drawn, at least with some fonts. */
		if (!(xfont = XftFontOpenName(drw->dpy, drw->screen, fontname))) {
			fprintf(stderr, "error, cannot load font from name: '%s'\n", fontname);
			return NULL;
		}
		if (!(pattern = FcNameParse((FcChar8 *)fontname))) {
			fprintf(stderr, "error, cannot parse font name to pattern: '%s'\n", fontname);
			XftFontClose(drw->dpy, xfont);
			return NULL;
		}
	} else if (fontpattern) {
		if (!(xfont = XftFontOpenPattern(drw->dpy, fontpattern))) {
			fprintf(stderr, "error, cannot load font from pattern.\n");
			return NULL;
		}
	} else {
		DIE("xfont_create():no font specified.");
	}

	font = ecalloc(1, sizeof(Fnt));
	font->xfont = xfont;
	font->pattern = pattern;
	font->h = xfont->ascent + xfont->descent;
	font->dpy = drw->dpy;

	return font;
}

/*
 * Free a single Fnt node: destroy its FcPattern and close the XftFont.
 * Does NOT follow font->next (caller manages the chain).
 */
static void
xfont_free(Fnt *font)
{
	if (!font)
		return;
	if (font->pattern)
		FcPatternDestroy(font->pattern);
	XftFontClose(font->dpy, font->xfont);
	free(font);
}

/*
 * Create a font chain from an ordered list of font name strings.
 * Fonts are loaded in reverse and prepended so that the first-named
 * font ends up at the head (drw->fonts).  Both caches are invalidated
 * because the new fonts may have different glyph metrics.
 */
Fnt *
drw_fontset_create(Drw *drw, const char *fonts[], size_t fontcount)
{
	Fnt *cur, *ret = NULL;
	size_t i;

	if (!drw || !fonts)
		return NULL;

	emoji_cache_invalidate(drw);
	drw_fontset_invalidate_cache();

	for (i = 1; i <= fontcount; i++) {
		if ((cur = xfont_create(drw, fonts[fontcount - i], NULL))) {
			cur->next = ret;
			ret = cur;
		}
	}
	return (drw->fonts = ret);
}

/*
 * Recursively free a font chain (linked list of Fnt nodes).
 */
void
drw_fontset_free(Fnt *font)
{
	if (font) {
		drw_fontset_free(font->next);
		xfont_free(font);
	}
}

/*
 * Allocate an Xft color by named string (e.g. "#rrggbb" or "blue").
 * Sets the alpha channel to fully opaque (0xff << 24) so that cairo-like
 * compositing doesn't dim text when drawn onto the backing pixmap.
 */
void
drw_clr_create(Drw *drw, Clr *dest, const char *clrname)
{
	if (!drw || !dest || !clrname)
		return;

	if (unlikely(!XftColorAllocName(drw->dpy, DefaultVisual(drw->dpy, drw->screen), DefaultColormap(drw->dpy, drw->screen), clrname, dest)))
		DIE("XftColorAllocName():error, cannot allocate color '%s'", clrname);

	dest->pixel |= 0xff << 24;
}

/*
 * Allocate an array of Clr (XftColor) from string names.  Returns a
 * heap-allocated array that the caller must free() when done.
 */
Clr *
drw_scm_create(Drw *drw, const char *clrnames[], size_t clrcount)
{
	size_t i;
	Clr *ret;

	if (!drw || !clrnames || clrcount < 2 || !(ret = ecalloc(clrcount, sizeof(XftColor))))
		return NULL;

	for (i = 0; i < clrcount; i++)
		drw_clr_create(drw, &ret[i], clrnames[i]);
	return ret;
}

/*
 * Set the active font chain for subsequent draws.  Does NOT take
 * ownership of the chain (caller manages lifetime).
 */
void
drw_setfontset(Drw *drw, Fnt *set)
{
	if (drw)
		drw->fonts = set;
}

/*
 * Set the active color scheme for subsequent draws.  Does NOT take
 * ownership of the scheme (caller manages lifetime).
 */
void
drw_setscheme(Drw *drw, Clr *scm)
{
	if (drw)
		drw->scheme = scm;
}

/*
 * Draw a filled or stroked rectangle using the current scheme.
 * When invert is set, foreground/background colors are swapped.
 * Stroked rectangles are drawn 1 px narrower/shorter (w-1, h-1)
 * to fit inside the given bounding box.
 */
void
drw_rect(Drw *drw, int x, int y, unsigned int w, unsigned int h, int filled, int invert)
{
	if (!drw || !drw->scheme || !w || !h)
		return;
	XSetForeground(drw->dpy, drw->gc, invert ? drw->scheme[ColBg].pixel : drw->scheme[ColFg].pixel);
	if (filled)
		XFillRectangle(drw->dpy, drw->drawable, drw->gc, x, y, w, h);
	else
		XDrawRectangle(drw->dpy, drw->drawable, drw->gc, x, y, w - 1, h - 1);
}

/*
 * Render text (or measure its width) using the font chain.
 *
 * When x=y=w=h=0 the function measures width only (no X drawing).
 * Otherwise it draws filled-background text within the given rectangle.
 *
 * Rendering loop:
 *   1. Scan forward through text, accumulating width in ew.
 *   2. When width exceeds available w, record overflow and compute the
 *      ellipsis insertion point.
 *   3. When the current codepoint needs a different font (nextfont),
 *      break and flush accumulated run in the current font.
 *   4. After flushing, either switch fonts (nextfont), try XftFontMatch
 *      fallback, or continue scanning.
 *
 * Returns the x-coordinate past the last drawn character (or the
 * computed width when measuring).
 */
int
drw_text(Drw *drw, int x, int y, unsigned int w, unsigned int h, unsigned int lpad, const char *text, int invert)
{
	int i, ellipsis_x = 0;
	unsigned int tmpw, ew, ellipsis_w = 0, ellipsis_len;
	XftDraw *d = NULL;
	Fnt *usedfont, *curfont, *nextfont;
	int utf8strlen, utf8charlen, render = x || y || w || h;
	long utf8codepoint = 0;
	const char *utf8str;
	FcCharSet *fccharset;
	FcPattern *fcpattern;
	FcPattern *match;
	XftResult result;
	int charexists = 0, overflow = 0;
	/* keep track of a couple codepoints for which we have no match. */
	enum { nomatches_len = 64 };
	static struct {
		long codepoint[nomatches_len];
		unsigned int idx;
	} nomatches;
	static unsigned int ellipsis_width = 0;

	if (!drw || (render && (!drw->scheme || !w)) || !text || !drw->fonts)
		return 0;

	if (!render) {
		w = invert ? invert : ~invert;
	} else {
		XSetForeground(drw->dpy, drw->gc, drw->scheme[invert ? ColFg : ColBg].pixel);
		XFillRectangle(drw->dpy, drw->drawable, drw->gc, x, y, w, h);
		d = drw->xftd; /* use persistent XftDraw (created in drw_create) */
		x += lpad;
		w = (w >= lpad) ? (w - lpad) : 0;
	}

	usedfont = drw->fonts;
	if (!ellipsis_width && render)
		ellipsis_width = drw_fontset_getwidth(drw, "...");
	while (1) {
		ew = ellipsis_len = utf8strlen = 0;
		utf8str = text;
		nextfont = NULL;
		while (*text) {
			/* ASCII fast-path: single-byte, always exists in the first font,
			 * measure directly (skip XftCharExists). */
			if ((unsigned char)*text <= 0x7F) {
				utf8codepoint = (unsigned char)*text;
				utf8charlen = 1;
				charexists = 1;
				curfont = drw->fonts;
				drw_font_getexts(curfont, text, 1, &tmpw, NULL);
			} else {
				utf8charlen = utf8decode(text, &utf8codepoint, UTF_SIZ);
				for (curfont = drw->fonts; curfont; curfont = curfont->next) {
					charexists = XftCharExists(drw->dpy, curfont->xfont, utf8codepoint);
					if (charexists) {
						tmpw = glyph_getwidth(drw, utf8codepoint, text, utf8charlen);
						break;
					}
				}
			}

			if (charexists) {
				/* track the last safe ellipsis cut point */
				if (ew + ellipsis_width <= w) {
					ellipsis_x = x + ew;
					ellipsis_w = w - ew;
					ellipsis_len = utf8strlen;
				}

				if (ew + tmpw > w) {
					overflow = 1;
					if (!render)
						x += tmpw;
					else
						utf8strlen = ellipsis_len;
				} else if (curfont == usedfont) {
					utf8strlen += utf8charlen;
					text += utf8charlen;
					ew += tmpw;
				} else {
					nextfont = curfont;
				}
			}

			if (overflow || !charexists || nextfont)
				break;
			else
				charexists = 0;
		}

		if (utf8strlen) {
			if (render) {
				const char *rp = utf8str;
				int rrem = utf8strlen, ascii_off = 0, ascii_len = 0, ascii_start_x = 0;
				int cx = x, ty2;
				ty2 = y + (h - usedfont->h) / 2 + usedfont->xfont->ascent;
				while (rrem > 0) {
					long cp;
					size_t clen;
					unsigned int cw;
					/* ASCII batch accumulation: group consecutive ASCII bytes
					 * and draw them with a single XftDrawStringUtf8 call.
					 * This reduces Xft/FreeType call overhead. */
					if ((unsigned char)*rp <= 0x7F) {
						cp = (unsigned char)*rp;
						clen = 1;
						if (!ascii_len) {
							ascii_off = (int)(rp - utf8str);
							ascii_start_x = cx;
						}
						ascii_len += (int)clen;
						drw_font_getexts(usedfont, rp, (unsigned int)clen, &cw, NULL);
						cx += (int)cw;
					} else {
						clen = utf8decode(rp, &cp, (unsigned int)rrem);
						int eidx;
						/* flush pending ASCII batch before a non-ASCII glyph */
						if (ascii_len) {
							XftDrawStringUtf8(d,
							                  &drw->scheme[invert ? ColBg : ColFg],
							                  usedfont->xfont,
							                  ascii_start_x,
							                  ty2,
							                  (XftChar8 *)(utf8str + ascii_off),
							                  ascii_len);
							ascii_len = 0;
						}
						cw = glyph_getwidth(drw, cp, rp, (unsigned int)clen);
						if (cw > 0) {
							/* emoji render cache: if we have a cached pixmap,
						 * blit it with XCopyArea instead of re-rendering
						 * through Xft (which would trigger FT_Load_Glyph
						 * → png_read → inflate).  On cache miss, render
						 * via Xft, capture the result with XCopyArea into
						 * a fresh pixmap, and insert into the cache. */
							eidx = emoji_cache_lookup(drw, cp);
							if (eidx >= 0) {
								EmojiCacheSlot *eslot = &drw->emoji_cache[eidx];
								XCopyArea(drw->dpy, eslot->pixmap, drw->drawable, drw->gc, 0, 0, (unsigned int)eslot->w, h, cx, y);
							} else {
								Pixmap cpm;
								XftDrawStringUtf8(d,
								                  &drw->scheme[invert ? ColBg : ColFg],
								                  usedfont->xfont,
								                  cx,
								                  ty2,
								                  (XftChar8 *)rp,
								                  (int)clen);
								cpm = XCreatePixmap(drw->dpy, drw->drawable, (unsigned int)cw, h, DefaultDepth(drw->dpy, drw->screen));
								XCopyArea(drw->dpy, drw->drawable, cpm, drw->gc, cx, y, (unsigned int)cw, h, 0, 0);
								emoji_cache_insert(drw, cp, cpm, (int)cw);
							}
							cx += (int)cw;
						}
					}
					rp += clen;
					rrem -= (int)clen;
				}
				/* flush any trailing ASCII run */
				if (ascii_len) {
					XftDrawStringUtf8(d,
					                  &drw->scheme[invert ? ColBg : ColFg],
					                  usedfont->xfont,
					                  ascii_start_x,
					                  ty2,
					                  (XftChar8 *)(utf8str + ascii_off),
					                  ascii_len);
				}
			}
			x += ew;
			w -= ew;
		}
		if (render && overflow)
			drw_text(drw, ellipsis_x, y, ellipsis_w, h, 0, "...", invert);

		if (!*text || overflow) {
			break;
		} else if (nextfont) {
			charexists = 0;
			usedfont = nextfont;
		} else {
			/* No font in the chain covers this codepoint.  Try XftFontMatch
			 * to find a system fallback.  If found and the font has the glyph,
			 * append it to the font chain so subsequent occurrences reuse it. */
			charexists = 1;

			/* match-nomatch optimization: skip XftFontMatch for codepoints
			 * we have previously failed to find — XftFontMatch is expensive. */
			for (i = 0; i < nomatches_len; ++i) {
				if (utf8codepoint == nomatches.codepoint[i])
					goto no_match;
			}

			fccharset = FcCharSetCreate();
			FcCharSetAddChar(fccharset, utf8codepoint);

			if (unlikely(!drw->fonts->pattern)) {
				/* Refer to the comment in xfont_create for more information. */
				DIE("the first font in the cache must be loaded from a font string.");
			}

			fcpattern = FcPatternDuplicate(drw->fonts->pattern);
			FcPatternAddCharSet(fcpattern, FC_CHARSET, fccharset);
			FcPatternAddBool(fcpattern, FC_SCALABLE, FcTrue);

			FcConfigSubstitute(NULL, fcpattern, FcMatchPattern);
			FcDefaultSubstitute(fcpattern);
			match = XftFontMatch(drw->dpy, drw->screen, fcpattern, &result);

			FcCharSetDestroy(fccharset);
			FcPatternDestroy(fcpattern);

			if (match) {
				usedfont = xfont_create(drw, NULL, match);
				if (usedfont && XftCharExists(drw->dpy, usedfont->xfont, utf8codepoint)) {
					for (curfont = drw->fonts; curfont->next; curfont = curfont->next)
						; /* NOP */
					curfont->next = usedfont;
				} else {
					xfont_free(usedfont);
					/* remember this codepoint as a no-match so we skip the
					 * expensive XftFontMatch call next time we see it */
					nomatches.codepoint[++nomatches.idx % nomatches_len] = utf8codepoint;
no_match:
					text += utf8charlen;
					usedfont = drw->fonts;
				}
			}
		}
	}

	return x + (render ? w : 0);
}

void
drw_map(Drw *drw, Window win, int x, int y, unsigned int w, unsigned int h)
{
	if (!drw)
		return;

	XCopyArea(drw->dpy, drw->drawable, win, drw->gc, x, y, w, h, x, y);
	XSync(drw->dpy, False);
}

unsigned int
drw_fontset_getwidth(Drw *drw, const char *text)
{
	unsigned int total = 0;
	long codepoint;
	size_t len;

	if (!drw || !drw->fonts || !text)
		return 0;

	while (*text) {
		len = utf8decode(text, &codepoint, UTF_SIZ);
		if (codepoint <= 0x7F) {
			unsigned int tmpw;
			drw_font_getexts(drw->fonts, text, (unsigned int)len, &tmpw, NULL);
			total += tmpw;
		} else {
			total += glyph_getwidth(drw, codepoint, text, (unsigned int)len);
		}
		text += len;
	}
	return total;
}

/* Clear glyph cache; call when fonts change. */
void
drw_fontset_invalidate_cache(void)
{
	for (int i = 0; i < GLYPH_CACHE_SIZE; i++)
		glyph_cache[i].codepoint = -1;
}

unsigned int
drw_fontset_getwidth_clamp(Drw *drw, const char *text, unsigned int n)
{
	unsigned int tmp = 0;
	if (drw && drw->fonts && text && n)
		tmp = drw_text(drw, 0, 0, 0, 0, 0, text, n);
	return MIN(n, tmp);
}

void
drw_font_getexts(Fnt *font, const char *text, unsigned int len, unsigned int *w, unsigned int *h)
{
	XGlyphInfo ext;

	if (!font || !text)
		return;

	XftTextExtentsUtf8(font->dpy, font->xfont, (XftChar8 *)text, len, &ext);
	if (w)
		*w = ext.xOff;
	if (h)
		*h = font->h;
}

Cur *
drw_cur_create(Drw *drw, int shape)
{
	Cur *cur;

	if (!drw || !(cur = ecalloc(1, sizeof(Cur))))
		return NULL;

	cur->cursor = XCreateFontCursor(drw->dpy, shape);

	return cur;
}

void
drw_cur_free(Drw *drw, Cur *cursor)
{
	if (!cursor)
		return;

	XFreeCursor(drw->dpy, cursor->cursor);
	free(cursor);
}
