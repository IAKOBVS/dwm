/* See LICENSE file for copyright and license details. */

/**
 * @file util.h
 * @brief Allocation, fatal-error, character-classification, and string helpers.
 *
 * The `xis*`/`xto*` wrappers are C-locale, cast-safe equivalents of
 * `<ctype.h>` that accept any `int` (including EOF and negative values)
 * without undefined behavior.
 */

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

/**
 * @brief Test for an ASCII alphabetic character.
 * @details Pure C-locale byte classification for the util module; see
 *          @ref arch_module_util (util.c/.h). @p c is cast to unsigned char
 *          before testing, so any int — including EOF and negative values —
 *          is handled without undefined behavior; no globals are read.
 * @param[in] c  Value to test; any int is accepted.
 * @return Non-zero if @p c is `A-Z` or `a-z`, zero otherwise.
 * @par Side Effects:
 *      None; pure classification of a byte value.
 * @par Threading and Reentrancy:
 *      Single-threaded; no locks or globals, so safe from signal handlers.
 * @anchor spec_xisalpha
 */
static inline int
xisalpha(int c)
{
	return ISALPHA((unsigned char)c);
}

/**
 * @brief Test for an ASCII decimal digit.
 * @details Pure C-locale byte classification for the util module; see
 *          @ref arch_module_util (util.c/.h). The cast to unsigned char makes
 *          EOF and negative inputs well-defined; a single range test is used
 *          and no globals are consulted.
 * @param[in] c  Value to test.
 * @return Non-zero if @p c is `0-9`, zero otherwise.
 * @par Side Effects:
 *      None; pure classification of a byte value.
 * @par Threading and Reentrancy:
 *      Single-threaded; no locks or globals, so safe from signal handlers.
 * @anchor spec_xisdigit
 */
static inline int
xisdigit(int c)
{
	return ISDIGIT((unsigned char)c);
}

/**
 * @brief Test for an ASCII alphanumeric character.
 * @details Pure C-locale byte classification for the util module; see
 *          @ref arch_module_util (util.c/.h). Combines the alphabetic and digit
 *          range tests after a single cast to unsigned char, so any int is safe;
 *          no globals are read.
 * @param[in] c  Value to test.
 * @return Non-zero if @p c is alphabetic or a digit, zero otherwise.
 * @par Side Effects:
 *      None; pure classification of a byte value.
 * @par Threading and Reentrancy:
 *      Single-threaded; no locks or globals, so safe from signal handlers.
 * @anchor spec_xisalnum
 */
static inline int
xisalnum(int c)
{
	return ISALNUM((unsigned char)c);
}

/**
 * @brief Test for an ASCII value (0x00-0x7f).
 * @details Pure C-locale byte classification for the util module; see
 *          @ref arch_module_util (util.c/.h). The value is reduced to one byte
 *          with a cast to unsigned char first, so EOF and negative ints cannot
 *          trigger undefined behavior and no globals are read.
 * @param[in] c  Value to test.
 * @return Non-zero if only the low seven bits are set.
 * @par Side Effects:
 *      None; pure classification of a byte value.
 * @par Threading and Reentrancy:
 *      Single-threaded; no locks or globals, so safe from signal handlers.
 * @anchor spec_xisascii
 */
static inline int
xisascii(int c)
{
	return ISASCII((unsigned char)c);
}

/**
 * @brief Test for an ASCII blank (space or tab).
 * @details Pure C-locale byte classification for the util module; see
 *          @ref arch_module_util (util.c/.h). Compares the byte obtained via an
 *          unsigned char cast, so any int including EOF is well-defined; no
 *          globals are consulted.
 * @param[in] c  Value to test.
 * @return Non-zero for `' '` or `'\t'`.
 * @par Side Effects:
 *      None; pure classification of a byte value.
 * @par Threading and Reentrancy:
 *      Single-threaded; no locks or globals, so safe from signal handlers.
 * @anchor spec_xisblank
 */
static inline int
xisblank(int c)
{
	return ISBLANK((unsigned char)c);
}

/**
 * @brief Test for an ASCII control character.
 * @details Pure C-locale byte classification for the util module; see
 *          @ref arch_module_util (util.c/.h). Performs unsigned range compares
 *          on the byte formed by casting @p c, so EOF and negative values are
 *          safe; no globals are read.
 * @param[in] c  Value to test.
 * @return Non-zero for 0x00-0x1f or 0x7f.
 * @par Side Effects:
 *      None; pure classification of a byte value.
 * @par Threading and Reentrancy:
 *      Single-threaded; no locks or globals, so safe from signal handlers.
 * @anchor spec_xiscntrl
 */
static inline int
xiscntrl(int c)
{
	return ISCNTRL((unsigned char)c);
}

/**
 * @brief Test for a printable, non-space ASCII character.
 * @details Pure C-locale byte classification for the util module; see
 *          @ref arch_module_util (util.c/.h). Uses an unsigned range compare on
 *          the byte obtained from @p c, making EOF and negatives safe; no
 *          globals are read.
 * @param[in] c  Value to test.
 * @return Non-zero for 0x21-0x7e.
 * @par Side Effects:
 *      None; pure classification of a byte value.
 * @par Threading and Reentrancy:
 *      Single-threaded; no locks or globals, so safe from signal handlers.
 * @anchor spec_xisgraph
 */
static inline int
xisgraph(int c)
{
	return ISGRAPH((unsigned char)c);
}

/**
 * @brief Test for an ASCII lowercase letter.
 * @details Pure C-locale byte classification for the util module; see
 *          @ref arch_module_util (util.c/.h). A single unsigned range test on
 *          the byte cast from @p c makes any int, including EOF, well-defined;
 *          no globals are read.
 * @param[in] c  Value to test.
 * @return Non-zero for `a-z`.
 * @par Side Effects:
 *      None; pure classification of a byte value.
 * @par Threading and Reentrancy:
 *      Single-threaded; no locks or globals, so safe from signal handlers.
 * @anchor spec_xislower
 */
static inline int
xislower(int c)
{
	return ISLOWER((unsigned char)c);
}

/**
 * @brief Test for an ASCII uppercase letter.
 * @details Pure C-locale byte classification for the util module; see
 *          @ref arch_module_util (util.c/.h). A single unsigned range test on
 *          the byte cast from @p c makes any int, including EOF, well-defined;
 *          no globals are read.
 * @param[in] c  Value to test.
 * @return Non-zero for `A-Z`.
 * @par Side Effects:
 *      None; pure classification of a byte value.
 * @par Threading and Reentrancy:
 *      Single-threaded; no locks or globals, so safe from signal handlers.
 * @anchor spec_xisupper
 */
static inline int
xisupper(int c)
{
	return ISUPPER((unsigned char)c);
}

/**
 * @brief Test for a printable ASCII character including space.
 * @details Pure C-locale byte classification for the util module; see
 *          @ref arch_module_util (util.c/.h). Uses an unsigned range compare on
 *          the byte cast from @p c, so EOF and negative inputs are safe; no
 *          globals are consulted.
 * @param[in] c  Value to test.
 * @return Non-zero for 0x20-0x7e.
 * @par Side Effects:
 *      None; pure classification of a byte value.
 * @par Threading and Reentrancy:
 *      Single-threaded; no locks or globals, so safe from signal handlers.
 * @anchor spec_xisprint
 */
static inline int
xisprint(int c)
{
	return ISPRINT((unsigned char)c);
}

/**
 * @brief Test for an ASCII punctuation character.
 * @details Pure C-locale byte classification for the util module; see
 *          @ref arch_module_util (util.c/.h). Computed as the graph range XOR
 *          the alphanumeric tests on the unsigned byte cast from @p c, so any
 *          int including EOF is well-defined; no globals are read.
 * @param[in] c  Value to test.
 * @return Non-zero for printable characters that are not alphanumeric.
 * @par Side Effects:
 *      None; pure classification of a byte value.
 * @par Threading and Reentrancy:
 *      Single-threaded; no locks or globals, so safe from signal handlers.
 * @anchor spec_xispunct
 */
static inline int
xispunct(int c)
{
	return ISPUNCT((unsigned char)c);
}

/**
 * @brief Test for ASCII whitespace.
 * @details Pure C-locale byte classification for the util module; see
 *          @ref arch_module_util (util.c/.h). Matches space plus the unsigned
 *          byte range 0x09-0x0d after casting @p c, so EOF and negative values
 *          are safe; no globals are read.
 * @param[in] c  Value to test.
 * @return Non-zero for space and 0x09-0x0d.
 * @par Side Effects:
 *      None; pure classification of a byte value.
 * @par Threading and Reentrancy:
 *      Single-threaded; no locks or globals, so safe from signal handlers.
 * @anchor spec_xisspace
 */
static inline int
xisspace(int c)
{
	return ISSPACE((unsigned char)c);
}

/**
 * @brief Test for an ASCII hexadecimal digit.
 * @details Pure C-locale byte classification for the util module; see
 *          @ref arch_module_util (util.c/.h). Combines the decimal digit test
 *          with an A-F range test after a single cast to unsigned char, so any
 *          int including EOF is well-defined; no globals are read.
 * @param[in] c  Value to test.
 * @return Non-zero for `0-9`, `A-F`, or `a-f`.
 * @par Side Effects:
 *      None; pure classification of a byte value.
 * @par Threading and Reentrancy:
 *      Single-threaded; no locks or globals, so safe from signal handlers.
 * @anchor spec_xisxdigit
 */
static inline int
xisxdigit(int c)
{
	return ISXDIGIT((unsigned char)c);
}

/**
 * @brief Map an ASCII uppercase letter to lowercase.
 * @details Locale-independent ASCII case conversion for the util module; see
 *          @ref arch_module_util (util.c/.h). @p c is cast to unsigned char
 *          before the uppercase test and bit operation, so negative ints and
 *          EOF are safe and the result is always a byte value; no globals are
 *          read and non-letters pass through unchanged.
 * @param[in] c  Character to convert.
 * @return Lowercase equivalent, or @p c unchanged; non-letters are unchanged.
 * @par Side Effects:
 *      None; pure conversion of a byte value.
 * @par Threading and Reentrancy:
 *      Single-threaded; no locks or globals, so safe from signal handlers.
 * @anchor spec_xtolower
 */
static inline int
xtolower(int c)
{
	return TOLOWER((unsigned char)c);
}

/**
 * @brief Map an ASCII lowercase letter to uppercase.
 * @details Locale-independent ASCII case conversion for the util module; see
 *          @ref arch_module_util (util.c/.h). @p c is cast to unsigned char
 *          before the lowercase test and bit operation, so negative ints and
 *          EOF are safe and the result is always a byte value; no globals are
 *          read and non-letters pass through unchanged.
 * @param[in] c  Character to convert.
 * @return Uppercase equivalent, or @p c unchanged; non-letters are unchanged.
 * @par Side Effects:
 *      None; pure conversion of a byte value.
 * @par Threading and Reentrancy:
 *      Single-threaded; no locks or globals, so safe from signal handlers.
 * @anchor spec_xtoupper
 */
static inline int
xtoupper(int c)
{
	return TOUPPER((unsigned char)c);
}

/**
 * @brief Write an unsigned decimal integer to a buffer without bounds checks.
 *
 * Writes digits and a NUL terminator, then reverses them in place. The caller
 * must guarantee room for the full decimal representation plus NUL.
 *
 * @details Writes digits and a NUL terminator, then reverses them in place;
 *          see @ref arch_module_util (util.c/.h). The caller owns @p buf and must
 *          guarantee room for the full decimal representation plus NUL; no
 *          destination capacity is checked and source and destination cannot
 *          overlap because only @p buf is written.
 * @param[out] buf     Destination buffer; must not be NULL.
 * @param[in]  number  Value to format.
 * @return Pointer to the NUL terminator.
 * @post `buf` contains the decimal representation and is NUL-terminated.
 * @par Side Effects:
 *      Direct writes to the caller-supplied destination buffer only.
 * @par Threading and Reentrancy:
 *      Single-threaded; touches only @p buf and locals, with no locks or
 *      globals, so safe from a signal handler when the buffer is not shared.
 * @warning Unbounded: a 64-bit value needs up to 21 bytes.
 * @anchor spec_numpcpy_unsafe
 */
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

/**
 * @brief Write a signed decimal integer, including the leading minus sign.
 *
 * Handles `INT64_MIN` without signed overflow by negating through uint64_t.
 *
 * @details Handles `INT64_MIN` without signed overflow by negating through
 *          uint64_t before delegating to @ref spec_numpcpy_unsafe; see
 * @param[out] buf     Destination buffer; must not be NULL.
 * @param[in]  number  Value to format.
 * @return Pointer to the NUL terminator.
 * @par Side Effects:
 *      Direct writes to the caller-supplied destination buffer only.
 * @par Threading and Reentrancy:
 *      Single-threaded; no locks or globals, so safe from a signal handler when
 *      the destination buffer is not shared.
 * @warning Unbounded: the caller supplies sufficient space.
 * @anchor spec_numpcpy_unsafe_signed
 */
static inline char *
numpcpy_unsafe_signed(char *buf, int64_t number)
{
	if (number < 0) {
		*buf++ = '-';
		/* negate through the unsigned domain (handles INT64_MIN safely) */
		number = (int64_t)(~(uint64_t)number + 1);
	}
	return numpcpy_unsafe(buf, (uint64_t)number);
}

/**
 * @brief Find the last occurrence of a byte within a bounded buffer.
 *
 * Equivalent to `memrchr` when `_GNU_SOURCE` is defined, with a portable
 * backwards-scan fallback. Callers rely on locating the final delimiter (for
 * example the `)` terminating a process name in `/proc/&lt;pid&gt;/stat`).
 *
 * @details Equivalent to `memrchr` when `_GNU_SOURCE` is defined, with a portable
 *          backward-scan fallback; see @ref arch_module_util (util.c/.h). The
 *          scan is limited to exactly @p n bytes and compares as unsigned char,
 *          so embedded NULs and high-byte values are handled correctly. Callers
 *          rely on locating the final delimiter, for example the `)` terminating
 *          a process name in `/proc/&lt;pid&gt;/stat`.
 * @param[in] s  Buffer to scan; need not be NUL-terminated.
 * @param[in] c  Byte to find, compared as unsigned char.
 * @param[in] n  Number of bytes to scan.
 * @return Pointer to the last matching byte, or NULL if absent.
 * @note Does not read beyond `s[n-1]`.
 * @par Side Effects:
 *      None; read-only scan of the caller-supplied buffer.
 * @par Threading and Reentrancy:
 *      Single-threaded; no locks or globals, so safe from a signal handler.
 * @anchor spec_strrchr_len
 */
static inline char *
strrchr_len(const char *s, int c, size_t n)
{
#ifdef _GNU_SOURCE
	return memrchr(s, c, n);
#else
	/* portable fallback: scan backwards for the last occurrence, so callers
	 * that rely on the final delimiter (e.g. /proc/<pid>/stat's comm field)
	 * behave identically whether or not _GNU_SOURCE is defined. */
	while (n) {
		n--;
		if ((unsigned char)s[n] == (unsigned char)c)
			return (char *)s + n;
	}
	return NULL;
#endif
}

/* DIE — call die(), injecting __FILE__/__LINE__/__func__ automatically.
 * Contract: @ref spec_die. */
#define DIE(...) die(__FILE__, __LINE__, __func__, __VA_ARGS__)

/* die — print "file:line: func(): fmt ... errno" to stderr, then exit.
 * Full contract: @ref spec_die in util.c. */
void die(const char *file, int line, const char *func, const char *fmt, ...);

/* ecalloc — zeroed nmemb*size allocation; terminates the process via
 * die() on failure instead of returning NULL.
 * Full contract: @ref spec_ecalloc in util.c. */
void *ecalloc(size_t nmemb, size_t size);

#endif /* UTIL_H */
