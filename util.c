/* See LICENSE file for copyright and license details.
 *
 * util.c — shared low-level helpers used across dwm and drw.
 *
 * Provides:
 *   die()   — fatal-error reporter that prints file/line/func/format + errno,
 *             then calls abort(). Every crash site passes __FILE__, __LINE__,
 *             __func__ so you can locate the failure without a debugger.
 *   ecalloc() — checked calloc: never returns NULL; calls die() on OOM.
 *               Use everywhere instead of raw calloc/malloc.
 */
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "util.h"

/* die — print a formatted error message with source location and errno,
 * then abort. Called via the DIE() macro which injects __FILE__, __LINE__,
 * __func__. Example output:
 *   dwm.c:1234: setup(): XOpenDisplay():dwm: cannot open display errno (1): ...
 */
void
die(const char *file, int line, const char *func, const char *fmt, ...)
{
	va_list ap;

	fprintf(stderr, "%s:%d: %s(): ", file, line, func);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fprintf(stderr, " errno (%d): %s\n", errno, strerror(errno));

	abort();
}

/* ecalloc — checked calloc wrapper. Guaranteed non-NULL return.
 * If calloc fails, prints diagnostic via DIE() and terminates. */
void *
ecalloc(size_t nmemb, size_t size)
{
	void *p;

	if (unlikely(!(p = calloc(nmemb, size))))
		DIE("calloc():calloc:");
	return p;
}
