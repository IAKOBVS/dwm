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
	unsigned int generation;
} glyph_cache[GLYPH_CACHE_SIZE];
static unsigned int font_generation = 1;

/* Per-font fallback bookkeeping; cleared when the font chain changes. */
#define NOMATCHES_LEN 64
static struct {
	long codepoint[NOMATCHES_LEN];
	unsigned int idx;
} nomatches;
static unsigned int ellipsis_width;

/**
 * @brief Look up or measure the advance width of a non-ASCII codepoint.
 *
 * First checks the open-addressing glyph_cache; on miss walks the font
 * chain, calls XftCharExists + drw_font_getexts, and populates the cache.
 * The table is a fixed 64-entry (GLYPH_CACHE_SIZE) open-addressing hash
 * keyed by codepoint with linear probing and wraps at the table size.
 * When a full-table probe finds no empty slot, the measured width is
 * returned but not stored, so a live entry is never silently evicted.
 *
 * @param[in] drw       Drawing context whose font chain is probed.
 * @param[in] codepoint Unicode codepoint to measure.
 * @param[in] utf8str   Encoded bytes of the codepoint (cache-miss measuring only).
 * @param[in] utf8len   Byte length of utf8str (cache-miss measuring only).
 *
 * @return Advance width in pixels, or 0 if no font in the chain covers it.
 *
 * @pre  drw != NULL and drw->fonts != NULL; utf8str/utf8len valid when the
 *       codepoint is not already cached.
 * @post On a cacheable miss the codepoint/width pair is stored in glyph_cache.
 * @note ASCII codepoints bypass this helper; it is only used for > 0x7F.
 * @warning The cache is global static state and is not thread-safe.
 * @see drw_fontset_getwidth, drw_fontset_invalidate_cache
 * @anchor spec_glyph_getwidth
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
		if (glyph_cache[i].codepoint == codepoint
		&& glyph_cache[i].generation == font_generation)
			return glyph_cache[i].width;
		i = (i + 1) & (GLYPH_CACHE_SIZE - 1);
		if (++probe >= GLYPH_CACHE_SIZE)
			break;
	}

	for (font = drw->fonts; font; font = font->next) {
		if (XftCharExists(drw->dpy, font->xfont, codepoint)) {
			drw_font_getexts(font, utf8str, utf8len, &tmpw, NULL);
			/* probe >= size means the walk found no empty slot — `i`
			 * now aliases a live entry, so skip storing rather than
			 * silently evicting it */
			if (probe < GLYPH_CACHE_SIZE) {
				glyph_cache[i].codepoint = codepoint;
				glyph_cache[i].width = tmpw;
				glyph_cache[i].generation = font_generation;
			}
			return tmpw;
		}
	}
	return 0;
}

/**
 * @brief Look up a codepoint in the emoji render cache.
 *
 * Open-addressing hash with linear probing over drw->emoji_cache; returns
 * the slot index on hit, -1 on miss.  A hit requires the scheme pixels to
 * match as well: the cached pixmap contains the background it was rendered
 * over, so reusing it under a different fg/bg would draw stale colors.
 * The cache key is therefore (codepoint, fg, bg).
 *
 * @param[in] drw       Drawing context owning the cache.
 * @param[in] codepoint Emoji codepoint to find.
 * @param[in] fg        Foreground pixel of the current draw.
 * @param[in] bg        Background pixel baked into the desired pixmap.
 *
 * @return Slot index >= 0 on a matching hit, -1 on miss.
 *
 * @pre  drw != NULL and the cache slots are initialized (codepoint -1 empty).
 * @post No cache state is modified.
 * @note Pure lookup; never renders or frees anything.
 * @see emoji_cache_insert, emoji_cache_invalidate
 * @anchor spec_emoji_cache_lookup
 */
static int
emoji_cache_lookup(Drw *drw, long codepoint, unsigned long fg, unsigned long bg)
{
	unsigned int i = (unsigned int)codepoint & (EMOJI_CACHE_SIZE - 1);
	unsigned int probe = 0;

	while (drw->emoji_cache[i].codepoint != -1) {
		if (drw->emoji_cache[i].codepoint == codepoint
		&& drw->emoji_cache[i].fg == fg
		&& drw->emoji_cache[i].bg == bg
		&& drw->emoji_cache[i].generation == font_generation)
			return (int)i;
		i = (i + 1) & (EMOJI_CACHE_SIZE - 1);
		if (++probe >= EMOJI_CACHE_SIZE)
			break;
	}
	return -1;
}

/**
 * @brief Insert a newly rendered emoji pixmap into the emoji render cache.
 *
 * Uses open-addressing with linear probing; evicts (and frees) any
 * existing entry at the target slot.  The pixmap must have been created
 * from the same-depth drawable and is later blitted with XCopyArea.
 * fg/bg are stored with the entry so lookups can reject entries that
 * were captured under a different color scheme.
 *
 * @param[in] drw       Drawing context owning the cache.
 * @param[in] codepoint Emoji codepoint being cached.
 * @param[in] pixmap    Rendered pixmap to store; ownership transfers to the cache.
 * @param[in] w         Cached glyph width in pixels.
 * @param[in] fg        Foreground pixel used to render pixmap.
 * @param[in] bg        Background pixel baked into pixmap.
 *
 * @pre  drw != NULL; pixmap was created at the drawable's depth and width w.
 * @post The slot holds the new entry; a previously stored pixmap at that
 *       slot, if any, has been freed with XFreePixmap.
 * @note Calls XFreePixmap on eviction.
 * @warning The cache takes ownership of pixmap; callers must not free it.
 * @see emoji_cache_lookup, emoji_cache_invalidate
 * @anchor spec_emoji_cache_insert
 */
static void
emoji_cache_insert(Drw *drw, long codepoint, Pixmap pixmap, int w,
                   unsigned long fg, unsigned long bg)
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
	drw->emoji_cache[i].fg = fg;
	drw->emoji_cache[i].bg = bg;
	drw->emoji_cache[i].generation = font_generation;
}

/**
 * @brief Clear the entire emoji render cache, freeing each cached pixmap.
 *
 * Called from drw_fontset_create (font change invalidates all cached
 * glyphs), drw_resize (pixmap height changed) and drw_free (teardown).
 *
 * @param[in] drw Drawing context whose cache is cleared.
 *
 * @pre  drw != NULL; its cache was initialized in drw_create.
 * @post Every slot is reset to codepoint -1, pixmap 0 and w 0.
 * @note Frees each non-empty cached pixmap via XFreePixmap.
 * @see emoji_cache_lookup, emoji_cache_insert
 * @anchor spec_emoji_cache_invalidate
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
		drw->emoji_cache[i].generation = 0;
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

/**
 * @brief Classify a UTF-8 byte and extract its data bits.
 *
 * Scans the utfbyte/utfmask tables to identify the byte type.  Sets *i to
 * the sequence length (1–4) for a leading byte, or 0 for a continuation
 * byte, and returns the payload bits (low 6/5/4/3 bits depending on type).
 *
 * @param[in]  c Leading or continuation UTF-8 byte.
 * @param[out] i Sequence length for a leading byte; 0 for a continuation byte.
 *
 * @return Payload bits with the leading type bits masked off (0 if no table
 *         entry matched).
 *
 * @pre  i != NULL.
 * @post *i is written on every path.
 * @note Pure helper; reads only the static utfbyte/utfmask tables.
 * @see utf8decode, utf8validate
 * @anchor spec_utf8decodebyte
 */
static long
utf8decodebyte(const char c, size_t *i)
{
	for (*i = 0; *i < (UTF_SIZ + 1); ++(*i))
		if (((unsigned char)c & utfmask[*i]) == utfbyte[*i])
			return (unsigned char)c & ~utfmask[*i];
	return 0;
}

/**
 * @brief Validate a decoded UTF-32 codepoint against its sequence length.
 *
 * Rejects surrogates (U+D800–U+DFFF) and out-of-range values, replacing
 * them with UTF_INVALID.  Then returns the minimal byte-sequence length
 * that can represent the (possibly corrected) codepoint.
 *
 * @param[in,out] u Decoded codepoint; overwritten with UTF_INVALID when invalid.
 * @param[in]     i Nominal sequence length of the decoded codepoint.
 *
 * @return Minimal sequence length (1–4) able to represent *u.
 *
 * @pre  u != NULL; i in [0, UTF_SIZ].
 * @post *u is either unchanged or set to UTF_INVALID.
 * @note Pure helper; reads only the static utfmin/utfmax tables.
 * @see utf8decode, utf8decodebyte
 * @anchor spec_utf8validate
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

/**
 * @brief Decode a UTF-8 byte sequence of at most clen bytes into *u.
 *
 * Decodes the leading byte with utf8decodebyte and consumes continuation
 * bytes until the sequence is complete or a non-continuation byte is seen,
 * then validates the result with utf8validate.
 *
 * @param[in]  c    Pointer to the first byte of the sequence.
 * @param[out] u    Decoded UTF-32 codepoint (UTF_INVALID on failure).
 * @param[in]  clen Number of bytes available at c (at most UTF_SIZ).
 *
 * @return Number of bytes consumed: the sequence length on success, 0 for
 *         an incomplete sequence, or 1 for an invalid leading byte.
 *
 * @pre  c != NULL; u != NULL; clen may be 0.
 * @post *u is always written (UTF_INVALID on entry/failure).
 * @note If clen == 0 it returns 0 immediately without reading c.
 * @see utf8decodebyte, utf8validate
 * @anchor spec_utf8decode
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

/**
 * @brief Create a drawing context with an off-screen pixmap, GC and XftDraw.
 *
 * Allocates a Drw, creates a backing pixmap, a GC, and a persistent XftDraw
 * tied to the pixmap.  The XftDraw is long-lived so that Xft's internal
 * glyph cache (FT_Load_Glyph results) survives across bar redraws.  Zero
 * width/height are clamped to 1 so XCreatePixmap never sees a zero size.
 * All emoji cache slots are initialized to -1 (empty).
 *
 * @param[in] dpy    X display.
 * @param[in] screen X screen number.
 * @param[in] root   Root window used as the pixmap/GC parent.
 * @param[in] w      Initial width; 0 is clamped to 1.
 * @param[in] h      Initial height; 0 is clamped to 1.
 *
 * @return Newly allocated Drw context, or NULL on allocation failure.
 *
 * @pre  dpy != NULL; root is a valid window on screen.
 * @post drw->w/h are at least 1; emoji_cache is empty.
 * @note Calls XCreatePixmap, XCreateGC, XftDrawCreate and XSetLineAttributes.
 * @warning Caller owns the result and must release it with drw_free.
 * @see drw_resize, drw_free
 * @anchor spec_drw_create
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

/**
 * @brief Resize the backing pixmap and re-create the XftDraw.
 *
 * The old XftDraw was tied to the old pixmap and must be destroyed — Xft
 * will internally free any cached glyph images for that drawable.  Zero
 * width/height are clamped to 1.  The emoji render cache is invalidated:
 * its pixmaps were captured at the old bar height, so blitting them at the
 * new height would read past the pixmap (BadMatch).  The glyph-width cache
 * survives (the font chain is unchanged).
 *
 * @param[in] drw Drawing context; NULL is ignored.
 * @param[in] w   New width; 0 is clamped to 1.
 * @param[in] h   New height; 0 is clamped to 1.
 *
 * @pre  drw may be NULL (no-op).
 * @post drawable/xftd are re-created at the new size; emoji cache is empty.
 * @note Calls emoji_cache_invalidate, XFreePixmap, XftDrawDestroy,
 *       XCreatePixmap and XftDrawCreate.
 * @see drw_create, emoji_cache_invalidate
 * @anchor spec_drw_resize
 */
void
drw_resize(Drw *drw, unsigned int w, unsigned int h)
{
	if (!drw)
		return;

	emoji_cache_invalidate(drw);
	drw->w = w = w ? w : 1;
	drw->h = h = h ? h : 1;
	if (drw->drawable)
		XFreePixmap(drw->dpy, drw->drawable);
	XftDrawDestroy(drw->xftd); /* old XftDraw was tied to old pixmap */
	drw->drawable = XCreatePixmap(drw->dpy, drw->root, w, h, DefaultDepth(drw->dpy, drw->screen));
	/* re-create XftDraw tied to the new pixmap drawable */
	drw->xftd = XftDrawCreate(drw->dpy, drw->drawable, DefaultVisual(drw->dpy, drw->screen), DefaultColormap(drw->dpy, drw->screen));
}

/**
 * @brief Free a drawing context and all associated resources.
 *
 * Invalidates the emoji cache and destroys the XftDraw before its pixmap is
 * freed, then frees the backing pixmap, the GC, the font chain and the Drw
 * itself.
 *
 * @param[in] drw Drawing context to destroy.
 *
 * @pre  drw must have been returned by drw_create (non-NULL).
 * @post All owned X resources and heap memory are released.
 * @note Calls emoji_cache_invalidate, XftDrawDestroy, XFreePixmap, XFreeGC
 *       and drw_fontset_free.
 * @warning Passing NULL dereferences; callers must guard.
 * @see drw_create, drw_fontset_free
 * @anchor spec_drw_free
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

/**
 * @brief Load a single XftFont from a font name string or an FcPattern.
 *
 * When loading from a name we also store the parsed FcPattern so that
 * drw_text can use it as a base for FcFontMatch fallback lookups.  When a
 * pattern is supplied (system fallback), no name pattern is stored.  The
 * font's line height is computed as ascent + descent.
 *
 * @param[in] drw         Drawing context supplying dpy and screen.
 * @param[in] fontname    Font name string, or NULL when fontpattern is used.
 * @param[in] fontpattern FcPattern to open directly, or NULL when fontname is used.
 *
 * @return Newly allocated Fnt node, or NULL on load/parse failure.
 *
 * @pre  Exactly one of fontname/fontpattern is non-NULL; drw != NULL.
 * @post On success the returned node owns xfont and, for name loads, pattern.
 * @note Calls XftFontOpenName/FcNameParse or XftFontOpenPattern; dies if
 *       neither argument is supplied.
 * @warning On failure prints to stderr; internal helper, use drw_fontset_create.
 * @see drw_fontset_create, xfont_free
 * @anchor spec_xfont_create
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

/**
 * @brief Free a single Fnt node.
 *
 * Destroys the node's FcPattern (if any), closes its XftFont and frees the
 * node.  Does NOT follow font->next (the caller manages the chain).
 *
 * @param[in] font Font node to release; NULL is ignored.
 *
 * @pre  font is either NULL or a node returned by xfont_create.
 * @post The node and its pattern/font are released.
 * @note Calls FcPatternDestroy and XftFontClose.
 * @warning Does not recurse; use drw_fontset_free for a full chain.
 * @see xfont_create, drw_fontset_free
 * @anchor spec_xfont_free
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

/**
 * @brief Create a font chain from an ordered list of font name strings.
 *
 * Fonts are loaded in reverse and prepended so that the first-named font
 * ends up at the head (drw->fonts).  Both the emoji render cache and the
 * global glyph-width cache are invalidated because the new fonts may have
 * different glyph metrics.  The result is stored in drw->fonts.
 *
 * @param[in] drw       Drawing context; NULL returns NULL.
 * @param[in] fonts     Array of font name strings; NULL returns NULL.
 * @param[in] fontcount Number of entries in fonts.
 *
 * @return Head of the new font chain (also stored in drw->fonts), or NULL.
 *
 * @pre  drw != NULL and fonts != NULL, otherwise NULL is returned immediately.
 * @post drw->fonts points at the new chain; both caches are cleared.
 * @note Calls emoji_cache_invalidate, drw_fontset_invalidate_cache and
 *       xfont_create; failed fonts are silently skipped.
 * @warning Caller must release the chain with drw_fontset_free.
 * @see drw_fontset_free, xfont_create
 * @anchor spec_drw_fontset_create
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

/**
 * @brief Recursively free a font chain.
 *
 * Walks the linked list of Fnt nodes via font->next and frees each with
 * xfont_free.  A NULL head is a no-op.
 *
 * @param[in] font Head of the font chain to free.
 *
 * @pre  font is NULL or a chain created by drw_fontset_create.
 * @post Every node in the chain is released.
 * @note Recursive; calls xfont_free per node.
 * @see drw_fontset_create, xfont_free
 * @anchor spec_drw_fontset_free
 */
void
drw_fontset_free(Fnt *font)
{
	if (font) {
		drw_fontset_free(font->next);
		xfont_free(font);
	}
}

/**
 * @brief Allocate an Xft color by named string.
 *
 * Accepts names such as "#rrggbb" or "blue".  The alpha channel is forced
 * to fully opaque (0xff << 24) so that cairo-like compositing doesn't dim
 * text when drawn onto the backing pixmap.
 *
 * @param[in]  drw     Drawing context supplying dpy, screen, visual and colormap.
 * @param[out] dest    XftColor to populate.
 * @param[in]  clrname Color name string.
 *
 * @pre  drw, dest and clrname are all non-NULL, otherwise this is a no-op.
 * @post dest holds the allocated color with opaque alpha.
 * @note Calls XftColorAllocName; dies on allocation failure.
 * @warning dest is an out-parameter and must be an XftColor owned by the caller.
 * @see drw_scm_create
 * @anchor spec_drw_clr_create
 */
void
drw_clr_create(Drw *drw, Clr *dest, const char *clrname)
{
	if (!drw || !dest || !clrname)
		return;

	if (unlikely(!XftColorAllocName(drw->dpy, DefaultVisual(drw->dpy, drw->screen), DefaultColormap(drw->dpy, drw->screen), clrname, dest)))
		DIE("XftColorAllocName():error, cannot allocate color '%s'", clrname);

	dest->pixel |= 0xffu << 24; /* unsigned shift: 0xff << 24 overflows int */
}

/**
 * @brief Allocate an array of Clr (XftColor) from string names.
 *
 * Each entry is filled by drw_clr_create.  The array is heap-allocated and
 * the caller takes ownership.
 *
 * @param[in] drw      Drawing context; NULL returns NULL.
 * @param[in] clrnames Array of color name strings; NULL returns NULL.
 * @param[in] clrcount Number of colors; must be >= 2 or NULL is returned.
 *
 * @return Heap-allocated Clr array, or NULL on bad args/allocation failure.
 *
 * @pre  drw != NULL, clrnames != NULL and clrcount >= 2.
 * @post Returned array entries are allocated via drw_clr_create.
 * @note Calls ecalloc and drw_clr_create.
 * @warning Caller must free() the returned array.
 * @see drw_clr_create
 * @anchor spec_drw_scm_create
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

/**
 * @brief Set the active font chain for subsequent draws.
 *
 * Does NOT take ownership of the chain (the caller manages its lifetime).
 * A NULL drw is ignored.
 *
 * @param[in] drw Drawing context; NULL is ignored.
 * @param[in] set Font chain to make active (stored verbatim in drw->fonts).
 *
 * @pre  drw may be NULL (no-op).
 * @post drw->fonts points at set when drw is non-NULL.
 * @note Does not invalidate either glyph or emoji cache.
 * @warning No ownership transfer; freeing set while drw still uses it is unsafe.
 * @see drw_fontset_create, drw_fontset_free
 * @anchor spec_drw_setfontset
 */
void
drw_setfontset(Drw *drw, Fnt *set)
{
	if (!drw)
		return;
	drw->fonts = set;
	/* A different font chain changes metrics and glyph images. */
	emoji_cache_invalidate(drw);
	drw_fontset_invalidate_cache();
}

/**
 * @brief Set the active color scheme for subsequent draws.
 *
 * Does NOT take ownership of the scheme (the caller manages its lifetime).
 * A NULL drw is ignored.
 *
 * @param[in] drw Drawing context; NULL is ignored.
 * @param[in] scm Color scheme array to make active.
 *
 * @pre  drw may be NULL (no-op); scm indexes ColFg/ColBg/ColBorder.
 * @post drw->scheme points at scm when drw is non-NULL.
 * @note Does not allocate or free colors.
 * @warning No ownership transfer; the scheme must outlive its use.
 * @see drw_scm_create
 * @anchor spec_drw_setscheme
 */
void
drw_setscheme(Drw *drw, Clr *scm)
{
	if (drw)
		drw->scheme = scm;
}

/**
 * @brief Draw a filled or stroked rectangle using the current scheme.
 *
 * When invert is set, foreground/background colors are swapped.  Stroked
 * rectangles are drawn 1 px narrower/shorter (w-1, h-1) to fit inside the
 * given bounding box.  Zero width or height is a no-op.
 *
 * @param[in] drw    Drawing context; NULL is a no-op.
 * @param[in] x      Left edge in drawable coordinates.
 * @param[in] y      Top edge in drawable coordinates.
 * @param[in] w      Rectangle width; 0 is a no-op.
 * @param[in] h      Rectangle height; 0 is a no-op.
 * @param[in] filled Non-zero fills the rectangle, zero strokes its outline.
 * @param[in] invert Non-zero swaps foreground and background.
 *
 * @pre  drw and drw->scheme are non-NULL and w/h are non-zero.
 * @post Pixels are written into the off-screen pixmap via the GC.
 * @note Calls XSetForeground then XFillRectangle or XDrawRectangle.
 * @see drw_text, drw_map
 * @anchor spec_drw_rect
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

/**
 * @brief Render text (or measure its width) using the font chain.
 *
 * When x=y=w=h=0 the function measures width only (no X drawing).  In that
 * mode w is set from invert (the caller passes a width budget via the invert
 * argument, as drw_fontset_getwidth_clamp does).  Otherwise it draws
 * filled-background text within the given rectangle, inset by lpad on the
 * left.
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
 * Consecutive ASCII bytes are batched into one XftDrawStringUtf8 call.
 * Non-ASCII glyphs use glyph_getwidth and may be blitted from the emoji
 * render cache instead of re-rendered.  Codepoints known to be missing from
 * the system are remembered to avoid repeated XftFontMatch calls, and a
 * match found for a missing glyph is appended to the font chain.
 *
 * @param[in] drw    Drawing context; NULL returns 0.
 * @param[in] x      Left edge (draw mode) or unused (measure mode).
 * @param[in] y      Top edge (draw mode) or unused (measure mode).
 * @param[in] w      Available width; 0 with x=y=h=0 selects measure mode.
 * @param[in] h      Rectangle height (draw mode) or unused (measure mode).
 * @param[in] lpad   Left padding inside the rectangle (draw mode only).
 * @param[in] text   NUL-terminated UTF-8 text; NULL returns 0.
 * @param[in] invert Non-zero swaps colors in draw mode, or width budget in
 *                   measure mode.
 *
 * @return X-coordinate past the last drawn character, or the computed width
 *         when measuring.
 *
 * @pre  drw->fonts != NULL; in draw mode drw->scheme != NULL and w != 0.
 * @post Draw mode writes into the off-screen pixmap and may populate
 *       glyph_cache, emoji_cache and the font chain.
 * @note Calls Xft text APIs, Fc* matching and emoji cache helpers; uses
 *       the persistent drw->xftd drawable.
 * @warning May append fallback fonts to drw->fonts, mutating the chain.
 * @see drw_fontset_getwidth, drw_fontset_getwidth_clamp, glyph_getwidth
 * @anchor spec_drw_text
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
	if (!drw || (render && (!drw->scheme || !w)) || !text || !drw->fonts)
		return 0;

	if (!render) {
		w = invert ? (unsigned int)invert : ~0u; /* unsigned budget, never -1 */
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
							/* emoji render cache: if we have a cached pixmap
							 * blit it with XCopyArea instead of re-rendering
							 * through Xft (which would trigger FT_Load_Glyph
							 * → png_read → inflate).  The cache key includes
							 * the fg/bg pixels because the captured pixmap has
							 * the scheme background baked in — reusing it under
							 * another scheme would draw stale colors. */
							unsigned long efg = drw->scheme[invert ? ColBg : ColFg].pixel;
							unsigned long ebg = drw->scheme[invert ? ColFg : ColBg].pixel;
							eidx = emoji_cache_lookup(drw, cp, efg, ebg);
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
								emoji_cache_insert(drw, cp, cpm, (int)cw, efg, ebg);
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
			for (i = 0; i < NOMATCHES_LEN; ++i) {
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
					goto no_match;
				}
			} else {
				/* No fallback available: skip the codepoint instead of
				 * retrying the same text forever. */
no_match:
				/* remember this codepoint as a no-match so we skip the
				 * expensive XftFontMatch call next time we see it */
				/* ++idx cycles slots round-robin, overwriting the oldest entry */
				nomatches.codepoint[++nomatches.idx % NOMATCHES_LEN] = utf8codepoint;
				text += utf8charlen;
				usedfont = drw->fonts;
			}
		}
	}

	return x + (render ? w : 0);
}

/**
 * @brief Copy a region of the off-screen pixmap onto the target window.
 *
 * Flushes the connection (XSync) so bar content appears immediately.
 *
 * @param[in] drw Drawing context; NULL is a no-op.
 * @param[in] win Destination window.
 * @param[in] x   Source/destination x-coordinate.
 * @param[in] y   Source/destination y-coordinate.
 * @param[in] w   Region width.
 * @param[in] h   Region height.
 *
 * @pre  drw != NULL; win is a valid drawable of matching depth.
 * @post The region is copied and the X connection is synced.
 * @note Calls XCopyArea then XSync.
 * @see drw_text, drw_rect
 * @anchor spec_drw_map
 */
void
drw_map(Drw *drw, Window win, int x, int y, unsigned int w, unsigned int h)
{
	if (!drw)
		return;

	XCopyArea(drw->dpy, drw->drawable, win, drw->gc, x, y, w, h, x, y);
	XSync(drw->dpy, False);
}

/**
 * @brief Measure the total advance width of text without drawing.
 *
 * ASCII characters are measured directly against the first font; non-ASCII
 * goes through glyph_getwidth (cache-aware).  Each character is decoded with
 * utf8decode first so multi-byte sequences advance correctly.
 *
 * @param[in] drw  Drawing context; NULL returns 0.
 * @param[in] text NUL-terminated UTF-8 text; NULL returns 0.
 *
 * @return Total width in pixels, or 0 when drw/drw->fonts/text is NULL.
 *
 * @pre  drw and drw->fonts are non-NULL, otherwise 0 is returned.
 * @post May populate the global glyph_cache for non-ASCII codepoints.
 * @note Calls utf8decode, drw_font_getexts and glyph_getwidth; no X drawing.
 * @see glyph_getwidth, drw_fontset_getwidth_clamp
 * @anchor spec_drw_fontset_getwidth
 */
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

/**
 * @brief Clear the global glyph-width cache.
 *
 * Call when fonts change so stale advance widths are not reused.  Every
 * slot's codepoint is reset to -1 (empty).
 *
 * @pre  None; the cache is global static state.
 * @post All glyph_cache entries are empty.
 * @note Does not touch the per-Dr context emoji cache.
 * @see glyph_getwidth, drw_fontset_create
 * @anchor spec_drw_fontset_invalidate_cache
 */
void
drw_fontset_invalidate_cache(void)
{
	font_generation++;
	memset(&nomatches, 0, sizeof nomatches);
	ellipsis_width = 0;
	for (int i = 0; i < GLYPH_CACHE_SIZE; i++)
		glyph_cache[i].codepoint = -1;
}

/**
 * @brief Measure text width capped at n, returning MIN(n, width).
 *
 * n is passed as drw_text's invert argument with zeroed coordinates, which
 * selects measure-only mode with a width budget of n so the scan stops
 * early once the budget is exceeded.
 *
 * @param[in] drw  Drawing context; NULL yields 0.
 * @param[in] text NUL-terminated UTF-8 text; NULL yields 0.
 * @param[in] n    Width cap/budget; 0 yields 0.
 *
 * @return MIN(n, measured width).
 *
 * @pre  drw, drw->fonts and text are non-NULL and n != 0, else 0.
 * @post May populate the global glyph_cache via drw_text.
 * @note Calls drw_text in measure-only mode (no drawing).
 * @see drw_text, drw_fontset_getwidth
 * @anchor spec_drw_fontset_getwidth_clamp
 */
unsigned int
drw_fontset_getwidth_clamp(Drw *drw, const char *text, unsigned int n)
{
	unsigned int tmp = 0;
	if (drw && drw->fonts && text && n)
		tmp = drw_text(drw, 0, 0, 0, 0, 0, text, n);
	return MIN(n, tmp);
}

/**
 * @brief Query the advance width of a text run in one specific font.
 *
 * Uses XftTextExtentsUtf8.  *w receives the horizontal advance; *h receives
 * the font's overall line height (ascent + descent), not a per-text value.
 * Either output pointer may be NULL to skip it.
 *
 * @param[in]  font Font whose metrics are used.
 * @param[in]  text UTF-8 text bytes; NULL is a no-op.
 * @param[in]  len  Number of bytes to measure.
 * @param[out] w    Receives horizontal advance, or NULL to skip.
 * @param[out] h    Receives font line height, or NULL to skip.
 *
 * @pre  font != NULL and text != NULL, otherwise nothing is written.
 * @post w/h, when non-NULL, are written from the XGlyphInfo result.
 * @note Calls XftTextExtentsUtf8; no drawing occurs.
 * @see drw_fontset_getwidth, glyph_getwidth
 * @anchor spec_drw_font_getexts
 */
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

/**
 * @brief Create a cursor wrapper for a core shape constant.
 *
 * Wraps XCreateFontCursor in a heap-allocated Cur.  Returns NULL if drw is
 * NULL or allocation fails.
 *
 * @param[in] drw   Drawing context supplying dpy.
 * @param[in] shape Core cursor shape constant (e.g. XC_xterm).
 *
 * @return New Cur wrapper, or NULL on NULL drw/allocation failure.
 *
 * @pre  drw != NULL for an actual cursor to be created.
 * @post The returned wrapper owns an X cursor resource.
 * @note Calls ecalloc and XCreateFontCursor.
 * @warning Caller must release with drw_cur_free.
 * @see drw_cur_free
 * @anchor spec_drw_cur_create
 */
Cur *
drw_cur_create(Drw *drw, int shape)
{
	Cur *cur;

	if (!drw || !(cur = ecalloc(1, sizeof(Cur))))
		return NULL;

	cur->cursor = XCreateFontCursor(drw->dpy, shape);

	return cur;
}

/**
 * @brief Release the X cursor resource and free the wrapper.
 *
 * Safe to call with a NULL cursor.  A NULL drw with a non-NULL cursor is not
 * checked and would dereference drw->dpy.
 *
 * @param[in] drw    Drawing context supplying dpy.
 * @param[in] cursor Cursor wrapper to release; NULL is a no-op.
 *
 * @pre  cursor is NULL or was returned by drw_cur_create.
 * @post The X cursor is freed and the wrapper released.
 * @note Calls XFreeCursor then free.
 * @see drw_cur_create
 * @anchor spec_drw_cur_free
 */
void
drw_cur_free(Drw *drw, Cur *cursor)
{
	if (!cursor)
		return;

	XFreeCursor(drw->dpy, cursor->cursor);
	free(cursor);
}
