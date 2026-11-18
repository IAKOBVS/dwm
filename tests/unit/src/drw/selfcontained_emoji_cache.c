#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Minimal type stubs                                                  */
/* ------------------------------------------------------------------ */
typedef int Display;
typedef int Pixmap;
typedef int XftFont;
typedef int FcPattern;

typedef struct Fnt {
	Display *dpy;
	unsigned int h;
	XftFont *xfont;
	FcPattern *pattern;
	struct Fnt *next;
} Fnt;

typedef int Window;
typedef int Drawable;
typedef int GC;
typedef int Clr;

/* Emoji render cache — copied from drw.h and drw.c */
#define EMOJI_CACHE_SIZE 32

struct EmojiCacheSlot {
	long codepoint;     /* -1 = empty */
	Pixmap pixmap;      /* cached rendered emoji pixmap */
	int w;              /* glyph width in pixels */
	unsigned long fg;   /* foreground pixel the pixmap was rendered with */
	unsigned long bg;   /* background pixel baked into the cached pixmap  */
};

typedef struct {
	unsigned int w, h;
	Display *dpy;
	int screen;
	Window root;
	Drawable drawable;
	GC gc;
	Clr *scheme;
	Fnt *fonts;
	struct EmojiCacheSlot emoji_cache[EMOJI_CACHE_SIZE];
} Drw;

/* ------------------------------------------------------------------ */
/* Stubs for X11 calls                                                 */
/* ------------------------------------------------------------------ */
static int _xfree_pixmap_count = 0;

void
XFreePixmap(Display *dpy, Pixmap p)
{
	(void)dpy;
	if (p != 0)
		_xfree_pixmap_count++;
}

/* ------------------------------------------------------------------ */
/* Emoji render cache — copied from drw.c                              */
/* ------------------------------------------------------------------ */

/* Look up a codepoint in the emoji render cache.
 * A hit requires the scheme pixels to match as well: the cached pixmap
 * contains the background it was rendered over, so reusing it under a
 * different fg/bg would draw stale colors.
 * Returns the cache index on hit, -1 on miss. */
static int
emoji_cache_lookup(Drw *drw, long codepoint, unsigned long fg, unsigned long bg)
{
	unsigned int i = (unsigned int)codepoint & (EMOJI_CACHE_SIZE - 1);
	unsigned int probe = 0;

	while (drw->emoji_cache[i].codepoint != -1) {
		if (drw->emoji_cache[i].codepoint == codepoint
		&& drw->emoji_cache[i].fg == fg
		&& drw->emoji_cache[i].bg == bg)
			return (int)i;
		i = (i + 1) & (EMOJI_CACHE_SIZE - 1);
		if (++probe >= EMOJI_CACHE_SIZE)
			break;
	}
	return -1;
}

/* Insert a codepoint + rendered pixmap into the emoji render cache.
 * If the slot is occupied, the old pixmap is freed. */
static void
emoji_cache_insert(Drw *drw, long codepoint, Pixmap pixmap, int w,
                   unsigned long fg, unsigned long bg)
{
	unsigned int i = (unsigned int)codepoint & (EMOJI_CACHE_SIZE - 1);
	unsigned int probe = 0;

	while (drw->emoji_cache[i].codepoint != -1 &&
	       drw->emoji_cache[i].codepoint != codepoint) {
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
}

/* Invalidate the entire emoji render cache, freeing all pixmaps. */
static void
emoji_cache_invalidate(Drw *drw)
{
	for (int i = 0; i < EMOJI_CACHE_SIZE; i++) {
		if (drw->emoji_cache[i].codepoint != -1 && drw->emoji_cache[i].pixmap)
			XFreePixmap(drw->dpy, drw->emoji_cache[i].pixmap);
		drw->emoji_cache[i].codepoint = -1;
		drw->emoji_cache[i].pixmap = 0;
		drw->emoji_cache[i].w = 0;
		drw->emoji_cache[i].fg = 0;
		drw->emoji_cache[i].bg = 0;
	}
}

/* ------------------------------------------------------------------ */
/* Test framework                                                       */
/* ------------------------------------------------------------------ */
static int total = 0, failed = 0;

#define ASSERT(cond, msg) do { \
	total++; \
	if (!(cond)) { \
		fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, msg); \
		failed++; \
	} \
} while(0)

#define ASSERT_EQ(a, b, msg) do { \
	total++; \
	if ((a) != (b)) { \
		fprintf(stderr, "  FAIL %s:%d: %s (%d != %d)\n", __FILE__, __LINE__, msg, (int)(a), (int)(b)); \
		failed++; \
	} \
} while(0)

/* ------------------------------------------------------------------ */
/* Shared test state                                                    */
/* ------------------------------------------------------------------ */
static Display stub_dpy;
static Drw drw;

/* Scheme pixels used by most tests */
#define FG1 0xff0000UL
#define BG1 0x000000UL

static void
setup(void)
{
	memset(&drw, 0, sizeof(drw));
	drw.dpy = &stub_dpy;
	_xfree_pixmap_count = 0;
	for (int i = 0; i < EMOJI_CACHE_SIZE; i++)
		drw.emoji_cache[i].codepoint = -1;
}

/* ------------------------------------------------------------------ */
/* Tests                                                                */
/* ------------------------------------------------------------------ */

/* Empty cache returns -1 for any lookup */
static void
test_empty_cache_returns_miss(void)
{
	setup();
	ASSERT_EQ(emoji_cache_lookup(&drw, 0x26A1, FG1, BG1), -1, "empty cache miss for ⚡");
	ASSERT_EQ(emoji_cache_lookup(&drw, 0x1F525, FG1, BG1), -1, "empty cache miss for 🔥");
	ASSERT_EQ(emoji_cache_lookup(&drw, 0x41, FG1, BG1), -1, "empty cache miss for 'A'");
}

/* Insert then lookup returns the stored pixmap */
static void
test_insert_then_lookup_hits(void)
{
	int idx;
	setup();
	emoji_cache_insert(&drw, 0x26A1, (Pixmap)42, 16, FG1, BG1);
	idx = emoji_cache_lookup(&drw, 0x26A1, FG1, BG1);
	ASSERT(idx >= 0, "lookup of inserted ⚡ returns index >= 0");
	ASSERT_EQ(drw.emoji_cache[idx].codepoint, 0x26A1, "stored codepoint matches");
	ASSERT_EQ(drw.emoji_cache[idx].pixmap, (Pixmap)42, "stored pixmap matches");
	ASSERT_EQ(drw.emoji_cache[idx].w, 16, "stored width matches");
	ASSERT_EQ(drw.emoji_cache[idx].fg, FG1, "stored fg matches");
	ASSERT_EQ(drw.emoji_cache[idx].bg, BG1, "stored bg matches");
}

/* Different codepoints coexist in cache */
static void
test_multiple_inserts_work(void)
{
	int idx1, idx2;
	setup();
	emoji_cache_insert(&drw, 0x26A1, (Pixmap)42, 16, FG1, BG1);
	emoji_cache_insert(&drw, 0x1F525, (Pixmap)99, 20, FG1, BG1);
	idx1 = emoji_cache_lookup(&drw, 0x26A1, FG1, BG1);
	idx2 = emoji_cache_lookup(&drw, 0x1F525, FG1, BG1);
	ASSERT(idx1 >= 0, "⚡ found");
	ASSERT(idx2 >= 0, "🔥 found");
	ASSERT(drw.emoji_cache[idx1].pixmap != drw.emoji_cache[idx2].pixmap,
	       "different emoji have different cache slots");
}

/* Invalidation clears all entries and frees pixmaps */
static void
test_invalidation_frees_pixmaps(void)
{
	setup();
	emoji_cache_insert(&drw, 0x26A1, (Pixmap)42, 16, FG1, BG1);
	emoji_cache_insert(&drw, 0x1F525, (Pixmap)99, 20, FG1, BG1);
	ASSERT_EQ(_xfree_pixmap_count, 0, "no pixmaps freed before invalidation");
	emoji_cache_invalidate(&drw);
	ASSERT_EQ(_xfree_pixmap_count, 2, "both pixmaps freed on invalidation");
	ASSERT_EQ(emoji_cache_lookup(&drw, 0x26A1, FG1, BG1), -1, "⚡ gone after invalidation");
	ASSERT_EQ(emoji_cache_lookup(&drw, 0x1F525, FG1, BG1), -1, "🔥 gone after invalidation");
}

/* Re-inserting the same codepoint frees the old pixmap */
static void
test_reinsert_same_codepoint_frees_old(void)
{
	setup();
	emoji_cache_insert(&drw, 0x26A1, (Pixmap)42, 16, FG1, BG1);
	ASSERT_EQ(_xfree_pixmap_count, 0, "no frees after first insert");
	emoji_cache_insert(&drw, 0x26A1, (Pixmap)99, 20, FG1, BG1);
	ASSERT_EQ(_xfree_pixmap_count, 1, "old pixmap freed on re-insert");
	ASSERT_EQ(emoji_cache_lookup(&drw, 0x26A1, FG1, BG1),
	          emoji_cache_lookup(&drw, 0x26A1, FG1, BG1),
	          "same slot reused");
}

/* Cache handles sequential insertion up to full capacity */
static void
test_fill_to_capacity(void)
{
	int i;
	setup();
	/* Insert 32 unique codepoints */
	for (i = 0; i < EMOJI_CACHE_SIZE; i++)
		emoji_cache_insert(&drw, 0x1000 + i, (Pixmap)(100 + i), 16 + i, FG1, BG1);

	/* All should be findable */
	for (i = 0; i < EMOJI_CACHE_SIZE; i++) {
		int idx = emoji_cache_lookup(&drw, 0x1000 + i, FG1, BG1);
		ASSERT(idx >= 0, "all 32 entries findable");
		if (idx >= 0) {
			ASSERT_EQ(drw.emoji_cache[idx].pixmap, (Pixmap)(100 + i),
			          "pixmap value preserved");
		}
	}
}

/* Inserting when cache is full overwrites a slot (no infinite loop) */
static void
test_overflow_does_not_loop(void)
{
	int i;
	setup();
	/* Fill cache */
	for (i = 0; i < EMOJI_CACHE_SIZE; i++)
		emoji_cache_insert(&drw, 0x1000 + i, (Pixmap)(100 + i), 16, FG1, BG1);
	/* Insert one more — should overwrite, not loop */
	emoji_cache_insert(&drw, 0x2000, (Pixmap)999, 20, FG1, BG1);
	ASSERT(1, "overflow does not cause infinite loop");
	/* The newest entry should be findable */
	ASSERT(emoji_cache_lookup(&drw, 0x2000, FG1, BG1) >= 0,
	       "overflow entry is findable");
}

/* Invalidation on already-empty cache is a no-op (no crash) */
static void
test_invalidate_empty_cache(void)
{
	setup();
	emoji_cache_invalidate(&drw);
	ASSERT_EQ(_xfree_pixmap_count, 0, "no pixmaps freed on empty cache");
	ASSERT(1, "invalidate on empty cache does not crash");
}

/* Lookup after invalidate and re-insert works */
static void
test_reuse_after_invalidate(void)
{
	int idx;
	setup();
	emoji_cache_insert(&drw, 0x26A1, (Pixmap)42, 16, FG1, BG1);
	emoji_cache_invalidate(&drw);
	emoji_cache_insert(&drw, 0x26A1, (Pixmap)99, 20, FG1, BG1);
	idx = emoji_cache_lookup(&drw, 0x26A1, FG1, BG1);
	ASSERT(idx >= 0, "re-inserted ⚡ is findable");
	ASSERT_EQ(drw.emoji_cache[idx].pixmap, (Pixmap)99, "new pixmap stored after re-insert");
}

/* Same codepoint under a different scheme must MISS: the cached pixmap
 * has the old background baked in, so reuse would draw stale colors. */
static void
test_scheme_change_misses(void)
{
	setup();
	emoji_cache_insert(&drw, 0x26A1, (Pixmap)42, 16, FG1, BG1);
	/* different fg/bg pair → no hit */
	ASSERT_EQ(emoji_cache_lookup(&drw, 0x26A1, 0x00ff00UL, BG1), -1,
	          "different fg misses");
	ASSERT_EQ(emoji_cache_lookup(&drw, 0x26A1, FG1, 0xffffffUL), -1,
	          "different bg misses");
	ASSERT_EQ(emoji_cache_lookup(&drw, 0x26A1, 0x00ff00UL, 0xffffffUL), -1,
	          "both different miss");
	/* original colors still hit */
	ASSERT(emoji_cache_lookup(&drw, 0x26A1, FG1, BG1) >= 0,
	       "original colors still hit");
}

/* Re-render after a scheme change overwrites the entry and swaps colors:
 * new colors hit, old colors now miss. */
static void
test_rerender_after_scheme_change_swaps_colors(void)
{
	setup();
	emoji_cache_insert(&drw, 0x1F525, (Pixmap)42, 16, FG1, BG1);
	ASSERT_EQ(_xfree_pixmap_count, 0, "first insert frees nothing");

	/* draw under SchemeSel: miss → render → insert replaces entry */
	ASSERT_EQ(emoji_cache_lookup(&drw, 0x1F525, 0x00ff00UL, 0xffffffUL), -1,
	          "scheme change misses first");
	emoji_cache_insert(&drw, 0x1F525, (Pixmap)77, 18, 0x00ff00UL, 0xffffffUL);
	ASSERT_EQ(_xfree_pixmap_count, 1, "old-scheme pixmap freed on replace");

	ASSERT_EQ(emoji_cache_lookup(&drw, 0x1F525, FG1, BG1), -1,
	          "old scheme now misses");
	ASSERT(emoji_cache_lookup(&drw, 0x1F525, 0x00ff00UL, 0xffffffUL) >= 0,
	       "new scheme hits after re-render");
}

/* Invert-flip equivalence: (fg=ColBg, bg=ColFg) is just another color
 * pair — swapping the two pixels is a distinct key from the unflipped one. */
static void
test_inverted_colors_are_distinct_key(void)
{
	setup();
	emoji_cache_insert(&drw, 0x270C, (Pixmap)7, 12, BG1, FG1);
	ASSERT(emoji_cache_lookup(&drw, 0x270C, BG1, FG1) >= 0,
	       "inverted key hits");
	ASSERT_EQ(emoji_cache_lookup(&drw, 0x270C, FG1, BG1), -1,
	       "uninverted key misses after inverted draw");
}

/* ------------------------------------------------------------------ */
/* Main                                                                  */
/* ------------------------------------------------------------------ */
int
main(void)
{
	test_empty_cache_returns_miss();
	test_insert_then_lookup_hits();
	test_multiple_inserts_work();
	test_invalidation_frees_pixmaps();
	test_reinsert_same_codepoint_frees_old();
	test_fill_to_capacity();
	test_overflow_does_not_loop();
	test_invalidate_empty_cache();
	test_reuse_after_invalidate();
	test_scheme_change_misses();
	test_rerender_after_scheme_change_swaps_colors();
	test_inverted_colors_are_distinct_key();

	printf("=== RESULTS ===\n");
	printf("Total: %d | Passed: %d | Failed: %d\n", total, total - failed, failed);
	return failed ? 1 : 0;
}
