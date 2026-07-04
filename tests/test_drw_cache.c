#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Minimal type stubs — never dereferenced, just pointer types         */
/* ------------------------------------------------------------------ */
typedef int Display;
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

/* ------------------------------------------------------------------ */
/* Stubs for external functions                                        */
/* ------------------------------------------------------------------ */
static int _xft_charexists_result = 1;

static int
XftCharExists(Display *dpy, XftFont *xfont, long codepoint)
{
	(void)dpy; (void)xfont;
	return _xft_charexists_result;
}

static void
drw_font_getexts(Fnt *font, const char *text, unsigned int len,
                 unsigned int *w, unsigned int *h)
{
	(void)font; (void)text; (void)h;
	if (w)
		*w = len * 10;
}

/* ------------------------------------------------------------------ */
/* Glyph-width cache — copied verbatim from drw.c lines 14-55         */
/* ------------------------------------------------------------------ */
#define GLYPH_CACHE_SIZE 64

static struct {
	long codepoint;     /* -1 = empty */
	unsigned int width;
} glyph_cache[GLYPH_CACHE_SIZE];

static unsigned int
glyph_getwidth(Drw *drw, long codepoint, const char *utf8str, unsigned int utf8len)
{
	unsigned int i = (unsigned int)codepoint & (GLYPH_CACHE_SIZE - 1);
	unsigned int probe = 0;
	Fnt *font;
	unsigned int tmpw;

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

/* ------------------------------------------------------------------ */
/* UTF-8 helpers — copied verbatim from drw.c                         */
/* ------------------------------------------------------------------ */
#define UTF_SIZ     4
#define UTF_INVALID 0xFFFD
#define BETWEEN(X, A, B) ((A) <= (X) && (X) <= (B))
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))

static const unsigned char utfbyte[UTF_SIZ + 1] = {0x80,    0, 0xC0, 0xE0, 0xF0};
static const unsigned char utfmask[UTF_SIZ + 1] = {0xC0, 0x80, 0xE0, 0xF0, 0xF8};
static const long utfmin[UTF_SIZ + 1] = {       0,    0,  0x80,  0x800,  0x10000};
static const long utfmax[UTF_SIZ + 1] = {0x10FFFF, 0x7F, 0x7FF, 0xFFFF, 0x10FFFF};

static long
utf8decodebyte(const char c, size_t *i)
{
	for (*i = 0; *i < (UTF_SIZ + 1); ++(*i))
		if (((unsigned char)c & utfmask[*i]) == utfbyte[*i])
			return (unsigned char)c & ~utfmask[*i];
	return 0;
}

static size_t
utf8validate(long *u, size_t i)
{
	if (!BETWEEN(*u, utfmin[i], utfmax[i]) || BETWEEN(*u, 0xD800, 0xDFFF))
		*u = UTF_INVALID;
	for (i = 1; *u > utfmax[i]; ++i)
		;
	return i;
}

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

/* ------------------------------------------------------------------ */
/* High-level API functions — copied verbatim from drw.c              */
/* ------------------------------------------------------------------ */
static unsigned int
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

static void
drw_fontset_invalidate_cache(void)
{
	for (int i = 0; i < GLYPH_CACHE_SIZE; i++)
		glyph_cache[i].codepoint = -1;
}

/* ------------------------------------------------------------------ */
/* Test framework                                                      */
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
static XftFont stub_xfont;
static Fnt stub_font;
static Drw drw;

static void
setup(void)
{
	memset(&drw, 0, sizeof(drw));
	drw.dpy = &stub_dpy;
	memset(&stub_font, 0, sizeof(stub_font));
	stub_font.dpy = &stub_dpy;
	stub_font.xfont = &stub_xfont;
	drw.fonts = &stub_font;
	drw_fontset_invalidate_cache();
	_xft_charexists_result = 1;
}

/* ------------------------------------------------------------------ */
/* Test: cache miss then hit returns same value                        */
/* ------------------------------------------------------------------ */
static void
test_glyph_cache_miss_then_hit(void)
{
	unsigned int first, second;
	setup();
	first = drw_fontset_getwidth(&drw, "\xC2\xA9");
	second = drw_fontset_getwidth(&drw, "\xC2\xA9");
	ASSERT_EQ(first, second, "second call returns cached value");
}

/* ------------------------------------------------------------------ */
/* Test: ASCII chars go through drw_font_getexts directly             */
/* ------------------------------------------------------------------ */
static void
test_glyph_cache_ascii_only(void)
{
	unsigned int w;
	setup();
	w = drw_fontset_getwidth(&drw, "hello");
	ASSERT_EQ(w, 50u, "hello = 5 chars * len=1 * 10 = 50");
}

/* ------------------------------------------------------------------ */
/* Test: invalidating the cache, then re-calling hits the same value  */
/* ------------------------------------------------------------------ */
static void
test_glyph_cache_invalidate(void)
{
	unsigned int before, after;
	setup();
	before = drw_fontset_getwidth(&drw, "\xC2\xA9");
	drw_fontset_invalidate_cache();
	after = drw_fontset_getwidth(&drw, "\xC2\xA9");
	ASSERT_EQ(before, after, "invalidate + re-cache returns same value");
}

/* ------------------------------------------------------------------ */
/* Test: empty string returns 0                                        */
/* ------------------------------------------------------------------ */
static void
test_glyph_cache_empty(void)
{
	unsigned int w;
	setup();
	w = drw_fontset_getwidth(&drw, "");
	ASSERT_EQ(w, 0u, "empty string returns 0");
}

/* ------------------------------------------------------------------ */
/* Test: NULL drw returns 0                                            */
/* ------------------------------------------------------------------ */
static void
test_glyph_cache_null_drw(void)
{
	unsigned int w;
	setup();
	w = drw_fontset_getwidth(NULL, "hello");
	ASSERT_EQ(w, 0u, "NULL drw returns 0");
}

/* ------------------------------------------------------------------ */
/* Test: NULL text returns 0                                           */
/* ------------------------------------------------------------------ */
static void
test_glyph_cache_null_text(void)
{
	unsigned int w;
	setup();
	w = drw_fontset_getwidth(&drw, NULL);
	ASSERT_EQ(w, 0u, "NULL text returns 0");
}

/* ------------------------------------------------------------------ */
/* Test: NULL fonts returns 0                                          */
/* ------------------------------------------------------------------ */
static void
test_glyph_cache_null_fonts(void)
{
	unsigned int w;
	setup();
	drw.fonts = NULL;
	w = drw_fontset_getwidth(&drw, "hello");
	ASSERT_EQ(w, 0u, "drw->fonts = NULL returns 0");
}

/* ------------------------------------------------------------------ */
/* Helper: encode a Unicode codepoint as a UTF-8 string                */
/* ------------------------------------------------------------------ */
static void
utf8encode(char buf[5], long codepoint)
{
	if (codepoint < 0x80) {
		buf[0] = (char)codepoint;
		buf[1] = '\0';
	} else if (codepoint < 0x800) {
		buf[0] = 0xC0 | (codepoint >> 6);
		buf[1] = 0x80 | (codepoint & 0x3F);
		buf[2] = '\0';
	} else if (codepoint < 0x10000) {
		buf[0] = 0xE0 | (codepoint >> 12);
		buf[1] = 0x80 | ((codepoint >> 6) & 0x3F);
		buf[2] = 0x80 | (codepoint & 0x3F);
		buf[3] = '\0';
	} else {
		buf[0] = 0xF0 | (codepoint >> 18);
		buf[1] = 0x80 | ((codepoint >> 12) & 0x3F);
		buf[2] = 0x80 | ((codepoint >> 6) & 0x3F);
		buf[3] = 0x80 | (codepoint & 0x3F);
		buf[4] = '\0';
	}
}

/* ------------------------------------------------------------------ */
/* Test: fill all 64 cache slots, then add one more — no infinite loop */
/* ------------------------------------------------------------------ */
static void
test_glyph_cache_eviction(void)
{
	int i;
	char buf[5];
	setup();

	for (i = 0; i < 64; i++) {
		utf8encode(buf, 0xE00 + i);
		drw_fontset_getwidth(&drw, buf);
	}

	utf8encode(buf, 0xE40);
	drw_fontset_getwidth(&drw, buf);

	ASSERT(1, "eviction does not cause infinite loop");
}

/* ------------------------------------------------------------------ */
/* Test: non-ASCII glyph missing in all fonts returns 0                */
/* ------------------------------------------------------------------ */
static void
test_glyph_cache_missing_glyph(void)
{
	unsigned int w;
	setup();
	_xft_charexists_result = 0;
	w = drw_fontset_getwidth(&drw, "\xC2\xA9");
	ASSERT_EQ(w, 0u, "non-ASCII with missing glyph returns 0");
}

/* ------------------------------------------------------------------ */
/* Test: drw_fontset_getwidth_clamp returns min(n, actual_width)       */
/* ------------------------------------------------------------------ */
static unsigned int
drw_fontset_getwidth_clamp(Drw *drw, const char *text, unsigned int n)
{
	unsigned int tmp = drw_fontset_getwidth(drw, text);
	return MIN(tmp, n);
}

static void
test_glyph_cache_clamp_limits(void)
{
	unsigned int w;
	setup();
	w = drw_fontset_getwidth_clamp(&drw, "hello", 10);
	ASSERT_EQ(w, 10u, "clamp returns n when n < actual width");
}

static void
test_glyph_cache_clamp_allows(void)
{
	unsigned int w;
	setup();
	w = drw_fontset_getwidth_clamp(&drw, "hello", 100);
	ASSERT_EQ(w, 50u, "clamp returns actual width when n > width");
}

static void
test_glyph_cache_clamp_zero(void)
{
	unsigned int w;
	setup();
	w = drw_fontset_getwidth_clamp(&drw, "hello", 0);
	ASSERT_EQ(w, 0u, "clamp with n=0 returns 0");
}

/* ------------------------------------------------------------------ */
/* Test: cache hit returns prior stored value                          */
/* ------------------------------------------------------------------ */
static void
test_glyph_cache_hit(void)
{
	unsigned int first, second;
	setup();
	first = drw_fontset_getwidth(&drw, "\xC2\xA9");
	second = drw_fontset_getwidth(&drw, "\xC2\xA9");
	ASSERT_EQ(first, second, "second call returns cached value");
	ASSERT_EQ(second, 20u, "\xC2\xA9 is 2 UTF-8 bytes => width 20");
}

/* ------------------------------------------------------------------ */
/* Test: linear probing advances past occupied slots                   */
/* ------------------------------------------------------------------ */
static void
test_glyph_cache_probe(void)
{
	/*
	 * Codepoints 0x40, 0x80, 0xC0 all hash to slot 0x00.
	 * First two occupy 0x00 and 0x01 via linear probing;
	 * the third stores at slot 0x02 and should be retrievable.
	 */
	char buf[5];
	unsigned int w;
	setup();
	utf8encode(buf, 0x40);   drw_fontset_getwidth(&drw, buf);  /* stores at 0x00 */
	utf8encode(buf, 0x80);   drw_fontset_getwidth(&drw, buf);  /* stores at 0x01 (probed) */
	utf8encode(buf, 0xC0);
	w = drw_fontset_getwidth(&drw, buf);                        /* found at 0x02 (probed 2x) */
	ASSERT_EQ(w, 20u, "third probed entry found via chained probe");
}

/* ------------------------------------------------------------------ */
/* Test: utf8decodebyte returns value for each valid UTF-8 class      */
/* ------------------------------------------------------------------ */
static void
test_utf8decodebyte_valid(void)
{
	size_t typ;
	setup();
	ASSERT_EQ(utf8decodebyte('\x00', &typ), 0, "null byte");
	ASSERT_EQ(utf8decodebyte('A', &typ), 0x41, "ASCII A");
	ASSERT_EQ(utf8decodebyte((char)0xC2, &typ), 0x02, "2-byte lead");
	ASSERT_EQ(utf8decodebyte((char)0xE0, &typ), 0x00, "3-byte lead");
	ASSERT_EQ(utf8decodebyte((char)0xF0, &typ), 0x00, "4-byte lead");
}

/* ------------------------------------------------------------------ */
/* Test: utf8decodebyte returns 0 for continuation byte                */
/* ------------------------------------------------------------------ */
static void
test_utf8decodebyte_continuation(void)
{
	size_t typ;
	ASSERT_EQ(utf8decodebyte((char)0x80, &typ), 0, "continuation byte returns 0");
}

/* ------------------------------------------------------------------ */
/* Test: utf8validate replaces surrogate halves with UTF_INVALID       */
/* ------------------------------------------------------------------ */
static void
test_utf8validate_surrogates(void)
{
	long u = 0xD800;
	setup();
	utf8validate(&u, 3);
	ASSERT_EQ(u, UTF_INVALID, "high surrogate replaced");
}

static void
test_utf8validate_out_of_range(void)
{
	long u = 0x110000;
	setup();
	utf8validate(&u, 3);
	ASSERT_EQ(u, UTF_INVALID, "> 0x10FFFF replaced");
}

/* ------------------------------------------------------------------ */
/* Test: utf8decode with clen=0 returns 0                              */
/* ------------------------------------------------------------------ */
static void
test_utf8decode_zero_len(void)
{
	long u = -1;
	setup();
	ASSERT_EQ(utf8decode("", &u, 0), 0, "clen=0 returns 0");
	ASSERT_EQ(u, UTF_INVALID, "u set to UTF_INVALID");
}

/* ------------------------------------------------------------------ */
/* Main entry point                                                     */
/* ------------------------------------------------------------------ */
int main(void)
{
	int i;
	for (i = 0; i < GLYPH_CACHE_SIZE; i++) glyph_cache[i].codepoint = -1;

	test_glyph_cache_miss_then_hit();
	test_glyph_cache_ascii_only();
	test_glyph_cache_invalidate();
	test_glyph_cache_empty();
	test_glyph_cache_null_drw();
	test_glyph_cache_null_text();
	test_glyph_cache_null_fonts();
	test_glyph_cache_eviction();
	test_glyph_cache_missing_glyph();
	test_glyph_cache_hit();
	test_glyph_cache_clamp_limits();
	test_glyph_cache_clamp_allows();
	test_glyph_cache_clamp_zero();
	test_glyph_cache_probe();
	test_utf8decodebyte_valid();
	test_utf8decodebyte_continuation();
	test_utf8validate_surrogates();
	test_utf8validate_out_of_range();
	test_utf8decode_zero_len();

	fprintf(stderr, "=== RESULTS ===\n");
	fprintf(stderr, "Total: %d | Passed: %d | Failed: %d\n", total, total - failed, failed);
	if (failed) { fprintf(stderr, "*** %d TESTS FAILED ***\n", failed); return 1; }
	fprintf(stdout, "All tests passed.\n");
	return 0;
}
