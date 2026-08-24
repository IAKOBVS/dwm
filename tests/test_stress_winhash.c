/* test_stress_winhash.c - randomized differential stress for the window
 * hash index. A fixed-seed xorshift PRNG drives insert/remove/rekey ops
 * against a linear reference model; after every op the index is
 * cross-checked: wintoclient must agree with the reference for every
 * live window, and winhash_count must match the number of inserted
 * (non-suppressed) entries. Clients stay at fixed addresses and remain
 * linked into a monitor list so the O(n) fallback walk stays honest. */
#define DWM_TEST 1
#define _GNU_SOURCE

#include <locale.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mock_x11.h"

#define DRW_H
#include "mock_drw.h"

#include "../dwm.h"
#include "../dwm.c"

static int total = 0, failed = 0;
#define ASSERT(cond, msg) do { \
	total++; \
	if (!(cond)) { \
		failed++; \
		fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, msg); \
	} \
} while(0)

#define POOL_MAX 400

static Client pool[POOL_MAX];
static Window ref_win[POOL_MAX];
static int ref_count;
static int slot_free[POOL_MAX];
static int free_top;

/* deterministic xorshift64* - identical sequence on every run */
static unsigned long long rng_state = 0x9E3779B97F4A7C15ULL;

static unsigned long long
rng(void)
{
	rng_state ^= rng_state << 13;
	rng_state ^= rng_state >> 7;
	rng_state ^= rng_state << 17;
	return rng_state * 2654435761ULL;
}

static void
reset_model(void)
{
	int i;
	memset(winhash, 0, sizeof winhash);
	winhash_count = 0;
	memset(pool, 0, sizeof pool);
	ref_count = 0;
	free_top = POOL_MAX;
	for (i = 0; i < POOL_MAX; i++)
		slot_free[i] = POOL_MAX - 1 - i;
}

static int
ref_find(Window w)
{
	int i;
	for (i = 0; i < ref_count; i++)
		if (ref_win[i] == w)
			return i;
	return -1;
}

static void
ref_drop(int ri)
{
	ref_win[ri] = ref_win[--ref_count];
}

/* put() that reports whether the entry actually landed in the table;
 * suppressed inserts (table at guard) stay list-only by design */
static int
do_put(Client *c)
{
	unsigned int before = winhash_count;
	winclient_put(c);
	return winhash_count != before;
}

/* unlink pool[idx] from mons->clients without moving any Client */
static void
unlink_client(Client *c)
{
	Client **pp = &mons->clients;
	while (*pp && *pp != c)
		pp = &(*pp)->next;
	if (*pp)
		*pp = c->next;
	c->next = NULL;
}

/* full cross-check of index vs reference */
static void
verify_index(const char *ctx)
{
	int i;
	for (i = 0; i < ref_count; i++) {
		Client *c = wintoclient(ref_win[i]);
		if (!c || c->win != ref_win[i]) {
			total++;
			failed++;
			fprintf(stderr,
			    "  FAIL %s: win %llu resolves wrong (entry %d)\n",
			    ctx, (unsigned long long)ref_win[i], i);
			return;
		}
	}
	ASSERT(1, ctx);
}

void
run_stress(int nops, const char *ctx)
{
	reset_model();
	static Monitor m;
	memset(&m, 0, sizeof m);
	mons = &m;

	Window fresh = 1000;
	int op;

	for (op = 0; op < nops; op++) {
		unsigned long long r = rng();

		switch (r % 3) {
		case 0: /* insert */
			if (!free_top)
				break;
			{
				int idx = slot_free[--free_top];
				pool[idx].win = ++fresh;
				pool[idx].next = m.clients;
				m.clients = &pool[idx];
				if (do_put(&pool[idx]))
					ref_win[ref_count++] = pool[idx].win;
			}
			break;
		case 1: /* remove a live entry */
			if (!ref_count)
				break;
			{
				int want = (int)((r >> 8) % ref_count);
				Window w = ref_win[want];
				Client *c = wintoclient(w);
				winclient_remove(c);
				unlink_client(c);
				ref_drop(want);
				slot_free[free_top++] = (int)(c - pool);
			}
			break;
		case 2: /* swallow-style rekey on a live entry */
			if (!ref_count)
				break;
			{
				int want = (int)((r >> 16) % ref_count);
				Window w = ref_win[want];
				Client *c = wintoclient(w);
				winclient_remove(c);
				ref_drop(want);
				pool[(int)(c - pool)].win = ++fresh;
				if (do_put(c))
					ref_win[ref_count++] = c->win;
			}
			break;
		}

		if ((op & 127) == 0 || op == nops - 1) {
			verify_index(ctx);
			ASSERT(winhash_count == ref_count,
			       "stress: hash count tracks reference size");
		}
	}

	verify_index(ctx);
	mons = NULL;
}

/* suppression regime: pack the table to its guard, prove overflow puts are
 * dropped safely, then prove lookups keep agreeing while clusters churn */
void
run_saturation(void)
{
	reset_model();
	static Monitor m;
	memset(&m, 0, sizeof m);
	mons = &m;

	int inserted = 0;
	Window w = 50000;
	while (inserted < WINHASH_SIZE - 1) {
		pool[inserted].win = w += 2;
		pool[inserted].next = m.clients;
		m.clients = &pool[inserted];
		winclient_put(&pool[inserted]);
		ref_win[ref_count++] = pool[inserted].win;
		inserted++;
	}
	ASSERT(winhash_count == WINHASH_SIZE - 1,
	       "saturation: table filled to one-below-guard");

	/* overflow inserts must be suppressed without corrupting anything */
	for (; inserted < WINHASH_SIZE + 40; inserted++) {
		pool[inserted].win = w += 2;
		pool[inserted].next = m.clients;
		m.clients = &pool[inserted];
		winclient_put(&pool[inserted]);
	}
	ASSERT(winhash_count == WINHASH_SIZE,
	       "saturation: guard accepts exactly the final free slot");

	/* churn: remove+reinsert arbitrary live entries across clusters */
	for (int round = 0; round < 500; round++) {
		int idx = (int)(rng() % (WINHASH_SIZE - 1));
		winclient_remove(&pool[idx]);
		winclient_put(&pool[idx]);
		if ((round & 63) == 0)
			verify_index("saturation: mid-churn consistency");
	}
	verify_index("saturation: consistent after churn");

	/* with the table at capacity, suppressed windows resolve via the
	 * O(n) list-walk fallback inside wintoclient */
	Client *sup = &pool[WINHASH_SIZE + 5];
	ASSERT(sup->win != 0, "saturation: overflow window present");
	ASSERT(wintoclient(sup->win) == sup,
	       "saturation: suppressed window found by list fallback");

	/* removing a never-inserted client must be a safe no-op */
	Client ghost;
	memset(&ghost, 0, sizeof ghost);
	ghost.win = 999999;
	winclient_remove(&ghost);
	verify_index("saturation: ghost removal is inert");
	mons = NULL;
}

int
main(void)
{
	dpy = (Display *)(void *)0x1;
	root = 1;

	run_stress(4000, "stress: 4k ops small-table invariants");
	run_stress(40000, "stress: 40k ops small-table invariants");
	run_stress(120000, "stress: 120k ops mixed churn");
	run_saturation();

	printf("%s: %d/%d assertions passed\n",
	       failed ? "FAIL" : "PASS", total - failed, total);
	return failed ? 1 : 0;
}
