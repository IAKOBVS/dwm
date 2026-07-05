/* Micro-benchmark: measure cold vs warm drw_fontset_getwidth with cache invalidation.
 * Requires Xvfb on :99.
 */
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
	if (!dpy) DIE("XOpenDisplay():no :99");
	screen = DefaultScreen(dpy);
	drw = drw_create(dpy, screen, RootWindow(dpy, screen),
	                 DisplayWidth(dpy, screen), DisplayHeight(dpy, screen));
	if (!drw) DIE("drw_create():drw_create failed");
	const char *fonts[] = {"monospace:size=10:antialias=true:autohint=true"};
	if (!drw_fontset_create(drw, fonts, 1)) DIE("drw_fontset_create():fontset creation failed");
}

/* Run N iterations, return average ns/call */
static long run_n(const char *text, int N)
{
	int i;
	volatile unsigned int r = 0;
	long t0 = nanos_mono();
	for (i = 0; i < N; i++)
		r += drw_fontset_getwidth(drw, text);
	long t1 = nanos_mono();
	(void)r;
	return (t1 - t0) / N;
}

static void bench(const char *label, const char *text)
{
	int W = 50000;

	/* Cold: invalidate cache, then measure bulk but first call is cold */
	drw_fontset_invalidate_cache();
	long cold_bulk = run_n(text, W);

	/* Warm: cache already populated from cold bulk */
	long warm_bulk = run_n(text, W);

	/* Relative improvement */
	double pct = 100.0 * (cold_bulk - warm_bulk) / (double)cold_bulk;

	printf("  %-50s cold=%6ld ns  warm=%6ld ns  (%.1f%% faster)\n",
	       label, cold_bulk, warm_bulk, pct);

	if (pct < 5.0 && text) /* flag negligible gains */
		printf("    → negligible (ASCII or already cached)\n");
}

int main(void)
{
	setup();
	puts("=== drw_fontset_getwidth: cache miss vs cache hit ===\n");

	bench("ASCII \"hello\" (5 chars)", "hello");
	bench("ASCII long (30 chars)", "test abc def 12345 test abc");

	bench("emoji \"⚡\"", "⚡");
	bench("emoji \"🔥\"", "🔥");
	bench("emoji \"💯\"", "💯");

	bench("mixed \"status 🔥⚡💯 abc\"", "status 🔥⚡💯 abc");
	bench("mixed long emoji heavy", "🔴 live | 🔥 cpu 45°C | ⚡ bat 85%");

	/* Multi-emoji: all distinct, then repeat */
	puts("\n--- Multi-emoji sequences ---");
	bench("5 distinct emoji \"🔥⚡💯✨🚀\"", "🔥⚡💯✨🚀");
	bench("same 5 emoji repeat (all cached)", "🔥⚡💯✨🚀");

	/* Full bar: simulate what drawbar does */
	puts("\n--- Simulated bar segments ---");
	bench("status text only", "🔴 rec | 🔥 58°C | ⚡ 92% | 📶 wifi | 💾 45G");
	bench("same status (cached)", "🔴 rec | 🔥 58°C | ⚡ 92% | 📶 wifi | 💾 45G");

	drw_free(drw);
	XCloseDisplay(dpy);
	puts("\nDone.");
	return 0;
}
