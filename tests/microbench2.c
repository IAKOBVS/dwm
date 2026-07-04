/* Test: compare cache hit vs miss for drw_fontset_getwidth */
#define _DEFAULT_SOURCE
#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

#include "drw.h"
#include "util.h"

Display *dpy;
int screen;
static Drw *drw;

static long nanos_mono(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (long)ts.tv_sec * 1000000000L + ts.tv_nsec;
}

static void setup(void)
{
	dpy = XOpenDisplay(":99");
	if (!dpy) die("no :99");
	screen = DefaultScreen(dpy);
	drw = drw_create(dpy, screen, RootWindow(dpy, screen),
	                 DisplayWidth(dpy, screen), DisplayHeight(dpy, screen));
	if (!drw) die("drw_create");
	const char *fonts[] = {"monospace:size=10:antialias=true:autohint=true"};
	if (!drw_fontset_create(drw, fonts, 1)) die("fontset");
}

int main(void)
{
	setup();

	/* First, measure direct Xft calls to establish baseline */
	XftFont *xfont = drw->fonts->xfont;
	const FcChar8 *utf8 = (const FcChar8 *)"\xe2\x9a\xa1"; /* ⚡ */
	int len = 3;
	long t, cold, warm;
	int N = 100000;

	printf("=== Direct Xft measurements (N=%d) ===\n\n", N);

	/* XftCharExists */
	t = nanos_mono();
	int ex = 0;
	for (int i = 0; i < N; i++)
		ex += XftCharExists(dpy, xfont, 0x26A1);
	cold = nanos_mono() - t;
	printf("XftCharExists(⚡): %ld ns total, %ld ns/call (exists=%d)\n", cold, cold/N, ex);

	/* XftTextExtentsUtf8 */
	XGlyphInfo ext;
	t = nanos_mono();
	for (int i = 0; i < N; i++)
		XftTextExtentsUtf8(dpy, xfont, utf8, len, &ext);
	warm = nanos_mono() - t;
	printf("XftTextExtentsUtf8(⚡): %ld ns total, %ld ns/call\n", warm, warm/N);

	printf("\n=== drw_fontset_getwidth (N=%d, fresh cache each) ===\n\n", N);

	/* Cold: invalidate cache, then measure */
	drw_fontset_invalidate_cache();
	t = nanos_mono();
	unsigned int r1 = 0;
	for (int i = 0; i < N; i++)
		r1 += drw_fontset_getwidth(drw, "⚡");
	long cold_time = nanos_mono() - t;

	/* Warm: cache already populated */
	t = nanos_mono();
	unsigned int r2 = 0;
	for (int i = 0; i < N; i++)
		r2 += drw_fontset_getwidth(drw, "⚡");
	long warm_time = nanos_mono() - t;

	printf("cold ⚡: %ld ns (%ld ns/call)\n", cold_time, cold_time / N);
	printf("warm ⚡: %ld ns (%ld ns/call)\n", warm_time, warm_time / N);
	printf("cache saves: %.1f%% (%.1fx)\n\n",
	       100.0 * (cold_time - warm_time) / cold_time,
	       (double)cold_time / (warm_time ? warm_time : 1));

	/* Same for 🔥 */
	drw_fontset_invalidate_cache();
	t = nanos_mono();
	r1 = 0;
	for (int i = 0; i < N; i++)
		r1 += drw_fontset_getwidth(drw, "🔥");
	cold_time = nanos_mono() - t;

	t = nanos_mono();
	r2 = 0;
	for (int i = 0; i < N; i++)
		r2 += drw_fontset_getwidth(drw, "🔥");
	warm_time = nanos_mono() - t;

	printf("cold 🔥: %ld ns (%ld ns/call)\n", cold_time, cold_time / N);
	printf("warm 🔥: %ld ns (%ld ns/call)\n", warm_time, warm_time / N);
	printf("cache saves: %.1f%% (%.1fx)\n\n",
	       100.0 * (cold_time - warm_time) / cold_time,
	       (double)cold_time / (warm_time ? warm_time : 1));

	/* Multiple distinct emoji */
	drw_fontset_invalidate_cache();
	t = nanos_mono();
	r1 = 0;
	for (int i = 0; i < N; i++)
		r1 += drw_fontset_getwidth(drw, "🔥⚡💯✨🚀");
	cold_time = nanos_mono() - t;

	t = nanos_mono();
	r2 = 0;
	for (int i = 0; i < N; i++)
		r2 += drw_fontset_getwidth(drw, "🔥⚡💯✨🚀");
	warm_time = nanos_mono() - t;

	printf("cold 5 emoji: %ld ns (%ld ns/call)\n", cold_time, cold_time / N);
	printf("warm 5 emoji: %ld ns (%ld ns/call)\n", warm_time, warm_time / N);
	printf("cache saves: %.1f%% (%.1fx)\n", 100.0 * (cold_time - warm_time) / cold_time, (double)cold_time / (warm_time ? warm_time : 1));

	printf("\n=== Real status text ===\n\n");
	const char *status = "🔴 rec | 🔥 58°C | ⚡ 92% | 📶 wifi | 💾 45G";

	drw_fontset_invalidate_cache();
	t = nanos_mono();
	r1 = 0;
	for (int i = 0; i < N/10; i++)
		r1 += drw_fontset_getwidth(drw, status);
	cold_time = nanos_mono() - t;

	t = nanos_mono();
	r2 = 0;
	for (int i = 0; i < N/10; i++)
		r2 += drw_fontset_getwidth(drw, status);
	warm_time = nanos_mono() - t;

	printf("cold \"%s\": %ld (%ld ns/call)\n", status, cold_time, cold_time / (N/10));
	printf("warm \"%s\": %ld (%ld ns/call)\n", status, warm_time, warm_time / (N/10));
	printf("cache saves: %.1f%% (%.1fx)\n",
	       100.0 * (cold_time - warm_time) / cold_time,
	       (double)cold_time / (warm_time ? warm_time : 1));

	(void)r1; (void)r2;

	drw_free(drw);
	XCloseDisplay(dpy);
	return 0;
}
