/* See LICENSE file for copyright and license details. */

#ifndef UTIL_H
#define UTIL_H 1

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

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

#	define ISALPHA(c)  (((unsigned)c | 32) - 'a' < 26)
#	define ISDIGIT(c)  ((unsigned)c - '0' < 10)
#	define ISGRAPH(c)  ((unsigned)c - 0x21 < 0x5e)
#	define ISASCII(c)  (!(c & ~0x7f))
#	define ISBLANK(c)  ((c == ' ') | (c == '\t'))
#	define ISCNTRL(c)  ((unsigned)c < 0x20 || c == 0x7f)
#	define ISLOWER(c)  ((unsigned)c - 'a' < 26)
#	define ISUPPER(c)  ((unsigned)c - 'A' < 26)
#	define ISPRINT(c)  ((unsigned)c - 0x20 < 0x5f)
#	define ISALNUM(c)  (ISALPHA(c) | ISDIGIT(c))
#	define ISPUNCT(c)  (ISGRAPH(c) ^ ISALNUM(c))
#	define ISSPACE(c)  ((c == ' ') | ((unsigned)c - '\t' < 5))
#	define TOLOWER(c)  (c | (ISUPPER(c) ? 32 : 0))
#	define TOUPPER(c)  (c & (ISLOWER(c) ? 0x5f : (int)-1))
#	define ISXDIGIT(c) (ISDIGIT(c) | (((unsigned)c | 32) - 'a' < 6))

static inline int
xisalpha(int c)
{
	return ISALPHA((unsigned char)c);
}

static inline int
xisdigit(int c)
{
	return ISDIGIT((unsigned char)c);
}

static inline int
xisalnum(int c)
{
	return ISALNUM((unsigned char)c);
}

static inline int
xisascii(int c)
{
	return ISASCII((unsigned char)c);
}

static inline int
xisblank(int c)
{
	return ISBLANK((unsigned char)c);
}

static inline int
xiscntrl(int c)
{
	return ISCNTRL((unsigned char)c);
}

static inline int
xisgraph(int c)
{
	return ISGRAPH((unsigned char)c);
}

static inline int
xislower(int c)
{
	return ISLOWER((unsigned char)c);
}

static inline int
xisupper(int c)
{
	return ISUPPER((unsigned char)c);
}

static inline int
xisprint(int c)
{
	return ISPRINT((unsigned char)c);
}

static inline int
xispunct(int c)
{
	return ISPUNCT((unsigned char)c);
}

static inline int
xisspace(int c)
{
	return ISSPACE((unsigned char)c);
}

static inline int
xisxdigit(int c)
{
	return ISXDIGIT((unsigned char)c);
}

static inline int
xtolower(int c)
{
	return TOLOWER((unsigned char)c);
}

static inline int
xtoupper(int c)
{
	return TOUPPER((unsigned char)c);
}

static char *
numpcpy_unsafe(char *buf, uint64_t number)
{
	if (number <= 9) {
		*buf = (char)(number + '0');
		*(buf + 1) = '\0';
		return buf + 1;
	}
	char *start = buf;
	do {
		*buf++ = (char)(number % 10 + '0');
	} while ((number /= 10) != 0);
	char *ret = buf;
	*buf = '\0';
	char *end = buf - 1;
	while (start < end) {
		char tmp = *start;
		*start++ = *end;
		*end-- = tmp;
	}
	return ret;
}

static inline char *
numpcpy_unsafe_signed(char *buf, int64_t number)
{
	if (number < 0) {
		*buf++ = '-';
		number = (int64_t)(-(uint64_t)number);
	}
	return numpcpy_unsafe(buf, (uint64_t)number);
}

static inline char *
strrchr_len(const char *s, int c, size_t n)
{
#ifdef _GNU_SOURCE
	return memrchr(s, c, n);
#else
	return strchr(s, c);
#endif		
}

/* DIE — call die(), injecting __FILE__/__LINE__/__func__ automatically. */
#define DIE(...) die(__FILE__, __LINE__, __func__, __VA_ARGS__)
/* die — print "file:line: func(): fmt ... errno" to stderr, then abort. */
void die(const char *file, int line, const char *func, const char *fmt, ...);
/* ecalloc — zeroed nmemb*size allocation; terminates the process via
 * die() on failure instead of returning NULL. */
void *ecalloc(size_t nmemb, size_t size);

#endif /* UTIL_H */
