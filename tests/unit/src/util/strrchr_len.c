/* test_strrchr_len.c - verify the portable (no _GNU_SOURCE) fallback of
 * strrchr_len() finds the LAST delimiter. This is the path production uses
 * because config.mk does not define _GNU_SOURCE, and /proc/<pid>/stat's comm
 * field may itself contain ')'. Deliberately does NOT define _GNU_SOURCE. */
#include <stdio.h>
#include <string.h>

#include "util.h"

static int total = 0, failed = 0;
#define ASSERT(cond, msg) do { \
	total++; \
	if (!(cond)) { \
		failed++; \
		fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, msg); \
	} \
} while(0)

int
main(void)
{
	const char *stat = "1234 (my)proc) S 4321 1234 1234 0 -1";
	char *p;

	/* last ')' must win even though an earlier one is present */
	p = strrchr_len(stat, ')', strlen(stat));
	ASSERT(p != NULL && *p == ')',
		"returns a pointer to the last ')'");
	ASSERT(p != NULL && strncmp(p, ") S 4321", 8) == 0,
		"fields after the last ')' are reachable");

	/* a single occurrence still works */
	p = strrchr_len("abc)def", ')', 7);
	ASSERT(p != NULL && strcmp(p, ")def") == 0,
		"single delimiter resolved");

	/* absent delimiter returns NULL, including a zero-length window */
	ASSERT(strrchr_len("abcdef", ')', 6) == NULL,
		"absent delimiter returns NULL");
	ASSERT(strrchr_len("abcdef", ')', 0) == NULL,
		"zero-length window returns NULL");

	/* only the first n bytes are considered */
	ASSERT(strrchr_len("ab)cd)ef", ')', 4) == "ab)cd)ef" + 2,
		"bounded scan stops before a later delimiter");
	ASSERT(strrchr_len("ab)cd)ef", ')', 6) == "ab)cd)ef" + 5,
		"bounded scan includes a delimiter at the window edge");

	printf("%d/%d assertions passed\n", total - failed, total);
	return failed ? 1 : 0;
}
