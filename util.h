/* See LICENSE file for copyright and license details. */

#ifndef UTIL_H
#define UTIL_H 1

#include <stdlib.h>

#define MAX(A, B)               ((A) > (B) ? (A) : (B))
#define MIN(A, B)               ((A) < (B) ? (A) : (B))
#define BETWEEN(X, A, B)        ((A) <= (X) && (X) <= (B))
#ifdef __GNUC__
#define likely(x)               __builtin_expect(!!(x), 1)
#define unlikely(x)             __builtin_expect(!!(x), 0)
#else
#define likely(x)               (x)
#define unlikely(x)             (x)
#endif

/* DIE — call die(), injecting __FILE__/__LINE__/__func__ automatically. */
#define DIE(...) die(__FILE__, __LINE__, __func__, __VA_ARGS__)
/* die — print "file:line: func(): fmt ... errno" to stderr, then abort. */
void die(const char *file, int line, const char *func, const char *fmt, ...);
/* ecalloc — zeroed nmemb*size allocation; terminates the process via
 * die() on failure instead of returning NULL. */
void *ecalloc(size_t nmemb, size_t size);

#endif /* UTIL_H */
