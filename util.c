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
 * @details Formats `file:line: func(): <message> errno (N): <strerror>` to stderr
 *          and terminates the process; see @ref arch_module_util (util.c/.h).
 *          Invoked via the DIE() macro, which injects `__FILE__`, `__LINE__` and
 *          `__func__`. Variadic arguments are consumed with va_start(),
 *          vfprintf() and va_end(). Failure is never reported as a typed error
 *          code: the only outcome is termination via exit(EXIT_FAILURE).
 * @param[in] file  Source file name.
 * @param[in] line  Source line number.
 * @param[in] func  Enclosing function name.
 * @param[in] fmt   printf-style format string.
 * @param[in] ...   Arguments for @p fmt.
 *
 * @note Reads the global `errno` and writes to stderr.
 * @par Side Effects:
 *      Writes a diagnostic to stderr and terminates the entire process with
 *      exit(EXIT_FAILURE); atexit() handlers and stdio flushing run, but no
 *      typed error code is ever returned to the caller.
 * @par Threading and Reentrancy:
 *      Single-threaded; no locks. Not async-signal-safe: fprintf(), vfprintf(),
 *      strerror(), exit() and stdio locking are undefined in a signal handler.
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
 * @details Checked calloc() wrapper for the util module; see @ref arch_module_util
 *          (util.c/.h). The return value is always non-NULL, so callers never need
 *          a NULL check. Allocation failure is not reported as a typed error code:
 *          DIE() invokes die(), which terminates via exit(EXIT_FAILURE). The block
 *          is zero-filled, owned by the caller, and released with free().
 * @param[in] nmemb  Number of elements.
 * @param[in] size   Size of each element in bytes.
 * @return Zeroed pointer to `nmemb * size` bytes.
 *
 * @note On failure calls `DIE("calloc():calloc:")`, which terminates.
 * @par Side Effects:
 *      On success, allocates heap state owned by the caller; on failure, writes
 *      a diagnostic to stderr and terminates the process with exit(EXIT_FAILURE).
 *      No X11/Xft calls on any path.
 * @par Threading and Reentrancy:
 *      Single-threaded; no locks. Not async-signal-safe: calloc() and the
 *      die()/exit() failure path are undefined from a signal handler.
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
