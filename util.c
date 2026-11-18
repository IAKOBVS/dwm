/* See LICENSE file for copyright and license details. */

/**
 * @file util.c
 * @brief Shared low-level helpers used across dwm and drw.
 *
 * Provides:
 *   die()   — fatal-error reporter that prints file/line/func/format + errno,
 *             then exits with EXIT_FAILURE. Every crash site passes __FILE__,
 *             __LINE__, __func__ so you can locate the failure without a
 *             debugger.
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

/**
 * @brief Print a fatal diagnostic with source location and errno, then exit.
 *
 * Called through the `DIE()` macro, which injects `__FILE__`, `__LINE__`, and
 * `__func__`. Output format:
 * `file:line: func(): <formatted message> errno (N): <strerror>`.
 *
 * @param[in] file  Source file name.
 * @param[in] line  Source line number.
 * @param[in] func  Enclosing function name.
 * @param[in] fmt   printf-style format string.
 * @param[in] ...   Arguments for @p fmt.
 *
 * @note Reads the global `errno` and writes to stderr.
 * @warning Never returns; terminates with `EXIT_FAILURE`.
 * @see ecalloc, DIE
 * @anchor spec_die
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

	exit(EXIT_FAILURE);
}

/**
 * @brief Allocate zeroed memory for an array and die if allocation fails.
 *
 * Checked `calloc` wrapper: the return value is always non-NULL, so callers
 * never need a NULL check.
 *
 * @param[in] nmemb  Number of elements.
 * @param[in] size   Size of each element in bytes.
 * @return Zeroed pointer to `nmemb * size` bytes.
 *
 * @note On failure calls `DIE("calloc():calloc:")`, which terminates.
 * @warning The multiplication is not overflow-checked by this wrapper.
 * @see die
 * @anchor spec_ecalloc
 */
void *
ecalloc(size_t nmemb, size_t size)
{
	void *p;

	if (unlikely(!(p = calloc(nmemb, size))))
		DIE("calloc():calloc:");
	return p;
}
