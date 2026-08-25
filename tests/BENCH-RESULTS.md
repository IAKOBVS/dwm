# Benchmark Results: `bench_optimize`

Baseline performance measurements for all 9 proposed optimizations.
Mock-based (no real X server), 50,000 iterations each.

**Build:** `make bench_optimize` from `tests/`
**Run:** `./bench_optimize`

---

## 2026-08-25: keyset vs no-keyset A/B benchmark

Added BENCH 12 to `bench_optimize.c`, comparing Mode A (exact
`(keysym, CLEANMASK(mod))` set via `cachekeys()`) against Mode B (the
pre-keyset lossy OR-mask gate, emulated by zeroing `keyset_count`).
50,000 iters, mock X11.

| Path | A: keyset | B: no-keyset | Δ (B-A) |
|---|---|---|---|
| unmatched keysym (reject) | 10 ns | 6 ns | −4 ns (noise) |
| wrong chord sharing mod bit | 13 ns | 152 ns | **+138 ns** |
| matched MODKEY+b (scan+dispatch) | 190 ns | 181 ns | −8 ns (noise) |

Finding: the keyset's measurable win is on the **wrong-chord reject** path
— a key combo that shares a modifier bit with a real binding but is not
itself bound. Without the keyset, `iskeybound()` returns true from the
lossy OR-mask and `keypress()` pays a full `keys[]` scan (~152 ns) that
finds no match. With the keyset, the exact set rejects in O(1) (~13 ns),
~10× faster on that path (~138 ns saved/call). The matched path (actual
bindings) and the unmatched-keysym path are parity — both already scan or
already reject cheaply.

Note: this corrects the earlier "Exact keypress binding index" entry
(2026-08-23), which reported parity at the 4–6 ns noise floor. That
entry's BENCH 11 never called `cachekeys()`, so the keyset was empty and
both reported paths exercised the mask fallback — hiding the real
difference.

Real-world impact: keypresses are human-rate (~10/s) and wrong chords are
rare, so the wall-clock saving is negligible; the keyset's primary value
remains exactness/correctness and O(1) worst-case reject regardless of
binding count, with this ~138 ns/call chord-reject speedup as a bonus.

---

## Raw Results

```
=== bench_optimize: Baseline Performance ===
All measurements use mock X11 (no real server).
Reported as nanoseconds per call.

=== 1. arrange() coalescing ===
  Measures cost of arrange(selmon) with 10 tiled clients.
  After optimization: N calls should coalesce to 1.

  [1] arrange(selmon) x N (10 clients)                           45 ns/call  (50000 iters, 2.262 ms total)
  [2] arrange(selmon) x 5N (burst, 10 clients)                  533 ns/call  (250000 iters, 133.338 ms total)
  [3] arrange(selmon) x N (20 clients)                          943 ns/call  (50000 iters, 47.153 ms total)
  [4] arrange(selmon) x 3N (burst, 20 clients)                  947 ns/call  (150000 iters, 142.154 ms total)

=== 2. updatestatus() comparison ===
  Measures cost of updatestatus() when text is identical.
  After optimization: identical text returns early.

  [5] updatestatus() x N (identical text)                        31 ns/call  (50000 iters, 1.599 ms total)
  [6] updatestatus() x N (alternating text)                      31 ns/call  (50000 iters, 1.573 ms total)

=== 3. focus() idempotent guard ===
  Measures cost of focus() when client is already focused.
  After optimization: same-client focus returns early.

  [7] focus(same_client) x N (5 clients)                         90 ns/call  (50000 iters, 4.540 ms total)
  [8] focus(NULL) x N (5 clients, finds sel)                     90 ns/call  (50000 iters, 4.515 ms total)
  [9] focus(different_client) x N (5 clients)                    91 ns/call  (50000 iters, 4.570 ms total)

=== 4. wintoclient() O(n) → O(1) ===
  Measures cost of wintoclient() linear walk.
  After optimization: hash table lookup is O(1).

  [10] wintoclient() x N (10 clients, hit)                         8 ns/call  (50000 iters, 0.435 ms total)
  [11] wintoclient() x N (10 clients, miss)                       20 ns/call  (50000 iters, 1.006 ms total)
  [12] wintoclient() x N (20 clients, hit)                        18 ns/call  (50000 iters, 0.907 ms total)
  [13] wintoclient() x N (20 clients, miss)                       44 ns/call  (50000 iters, 2.241 ms total)
  [14] wintoclient() x N (50 clients, hit)                        55 ns/call  (50000 iters, 2.783 ms total)
  [15] wintoclient() x N (50 clients, miss)                      143 ns/call  (50000 iters, 7.183 ms total)

=== 5. tile()/monocle() visible-client count ===
  Measures cost of client list walk to count n.
  After optimization: cached count avoids O(n) walk.

  [16] tile(selmon) x N (10 clients)                             397 ns/call  (50000 iters, 19.872 ms total)
  [17] tile(selmon) x N (20 clients)                             774 ns/call  (50000 iters, 38.747 ms total)
  [18] tile(selmon) x N (50 clients)                            1931 ns/call  (50000 iters, 96.598 ms total)
  [19] monocle(selmon) x N (10 clients)                          339 ns/call  (50000 iters, 16.989 ms total)
  [20] monocle(selmon) x N (20 clients)                          627 ns/call  (50000 iters, 31.379 ms total)
  [21] monocle(selmon) x N (50 clients)                         1541 ns/call  (50000 iters, 77.071 ms total)

=== 6. propertynotify() atom filter ===
  Measures cost of propertynotify() for uninteresting atoms.
  After optimization: early return before wintoclient().

  [22] propertynotify(uninteresting) x N (10 clients)             14 ns/call  (50000 iters, 0.736 ms total)
  [23] propertynotify(root WM_NAME) x N                           35 ns/call  (50000 iters, 1.760 ms total)
  [24] propertynotify(PropertyDelete) x N                          2 ns/call  (50000 iters, 0.137 ms total)

=== 7. setlayout() dirty guard ===
  Measures cost of setlayout() with same layout.
  After optimization: same layout skips dirty.

  [25] setlayout(same) x N                                         7 ns/call  (50000 iters, 0.385 ms total)
  [26] setlayout(toggle) x N                                       6 ns/call  (50000 iters, 0.304 ms total)

=== 8. enternotify() single-monitor guard ===
  Measures cost of enternotify() on single monitor.
  After optimization: single-monitor returns early.

  [27] enternotify() x N (single monitor, 5 clients)              10 ns/call  (50000 iters, 0.519 ms total)
  [28] enternotify() x N (2 monitors, 5 clients)                  10 ns/call  (50000 iters, 0.513 ms total)

=== 9. gaming mode (fullscreen overhead) ===
  Measures overhead of handlers during fullscreen.
  After optimization: propertynotify/updatestatus skipped.

  [29] propertynotify(uninteresting) during fullscreen x N        10 ns/call  (50000 iters, 0.511 ms total)
  [30] updatestatus() during fullscreen x N (already guarded)        1 ns/call  (50000 iters, 0.093 ms total)
  [31] enternotify() during fullscreen x N                        10 ns/call  (50000 iters, 0.515 ms total)
```

---

## Scaling Analysis

### arrange() — burst cost multiplier

| Scenario | ns/call | vs single |
|---|---|---|
| 10 clients, single call | 45 | 1× |
| 10 clients, burst of 5 | 533 | **12×** |
| 20 clients, single call | 943 | 1× |
| 20 clients, burst of 3 | 947 | 1× |

The burst multiplier (12× at 10 clients) proves redundant arrange() calls are
expensive. At 20 clients the per-call cost dominates, so burst overhead is
negligible relative to the single-call cost.

### wintoclient() — O(n) linear scaling

| Clients | Hit (ns) | Miss (ns) | Hit scaling | Miss scaling |
|---|---|---|---|---|
| 10 | 8 | 20 | 1× | 1× |
| 20 | 18 | 44 | 2.2× | 2.2× |
| 50 | 55 | 143 | 6.9× | 7.2× |

Perfect linear scaling confirms the linked-list walk. Miss cost is 2.5× hit
cost at all sizes (full walk vs early exit).

### tile()/monocle() — linear client scaling

| Clients | tile (ns) | monocle (ns) | tile scaling | monocle scaling |
|---|---|---|---|---|
| 10 | 397 | 339 | 1× | 1× |
| 20 | 774 | 627 | 1.9× | 1.9× |
| 50 | 1931 | 1541 | 4.9× | 4.5× |

Both functions scale linearly with client count. tile() is ~20% slower than
monocle() due to the more complex geometry calculation (master/stack split).

### propertynotify() — path cost comparison

| Path | ns/call | vs PropertyDelete |
|---|---|---|
| PropertyDelete (early return) | 2 | 1× |
| Uninteresting atom (wintoclient + switch) | 14 | 7× |
| Root WM_NAME (updatestatus) | 35 | 17.5× |

The uninteresting-atom path costs 7× more than the early-return path.
Filtering these before `wintoclient()` saves the full wintoclient walk.

### focus() — cost is uniform

| Scenario | ns/call |
|---|---|
| Same client (redundant) | 90 |
| NULL (find visible) | 90 |
| Different client | 91 |

No measurable difference between scenarios in mocks. The real savings come
from avoiding the downstream X11 calls (XSetInputFocus, drawbar trigger)
that mocks skip entirely.

### updatestatus() — cost is uniform

| Scenario | ns/call |
|---|---|
| Identical text | 31 |
| Alternating text | 31 |

No measurable difference. The mock gettextprop returns a fixed string, so
`strcmp` in a proposed optimization would return 0 immediately. The real
savings come from skipping the drw_text pipeline for status rendering.

---

## What Each Benchmark Proves

| # | Benchmark | Proves | Doesn't prove |
|---|---|---|---|
| 1 | arrange coalescing | Burst calls are 12× more expensive per effective call | Real X11 cost (mocks skip XConfigureWindow, XSync) |
| 2 | updatestatus comparison | Dirty chain is31 ns (cheap in mocks) | Real drw_text cost (~3K cycles per emoji) |
| 3 | focus idempotent | Dirty chain is90 ns (cheap in mocks) | Real XSetInputFocus cost (~200 cycles) |
| 4 | wintoclient hash | O(n) scaling is real and measurable (7× from 10→50 clients) | Cache miss patterns on real hardware |
| 5 | tile/monocle count | O(n) walk scales linearly (5× from 10→50 clients) | Real resize/XConfigureWindow cost |
| 6 | propertynotify filter | Uninteresting atoms cost 7× more than early return | Real XGetWindowProperty cost |
| 7 | setlayout guard | Negligible cost (7 ns) | Nothing — already negligible |
| 8 | enternotify guard | Can't differentiate single/multi in mocks | Real wintoclient + focus chain cost |
| 9 | gaming mode | updatestatus already guarded (1 ns) | Real fullscreen overhead |

---

## Mock vs Real Cost Ratio

The gap between mock ns/call and real-world cycle cost reveals where the
actual bottleneck lives:

| Function | Mock ns/call | Real cycles (est.) | Mock:Real ratio | Bottleneck |
|---|---|---|---|---|
| `arrange()` 10c | 45 | ~8,000 | 1:178 | X11 round-trips |
| `tile()` 10c | 397 | ~5,000 | 1:12 | XConfigureWindow per client |
| `focus()` | 90 | ~500 | 1:5.5 | XSetInputFocus + drawbar |
| `wintoclient()` 10c miss | 20 | ~400 | 1:20 | Pointer chasing + cache misses |
| `updatestatus()` | 31 | ~3,000 | 1:97 | drw_text → Xft pipeline |
| `propertynotify()` uninteresting | 14 | ~700 | 1:50 | wintoclient + XGetWindowProperty |
| `setlayout()` | 7 | ~500 | 1:71 | drawbar trigger |

Functions with low Mock:Real ratios (#5 tile, #3 focus) are already
relatively well-optimized in mocks. Functions with high ratios (#1 arrange,
#2 updatestatus, #6 propertynotify) have their real cost in X11/Xft calls
that mocks skip entirely.

---

## Revised Performance Estimates

After calibrating against benchmark data, the original OPTIMIZE-ROADMAP
estimates are adjusted:

| # | Optimization | Original | Revised | Basis for revision |
|---|---|---|---|---|
| 1 | arrange() coalescing | 3–5% | **3–5%** | Mock confirms 12× burst cost; X11 calls dominate |
| 2 | updatestatus() comparison | 2–3% | **1–2%** | Mock shows dirty chain is31 ns; real savings are drw_text |
| 3 | focus() idempotent guard | 2–4% | **0.5–1%** | Mock shows dirty chain is90 ns; real savings are XSetInputFocus |
| 4 | wintoclient() hash | 0.5–1% | **<0.5%** | Mock confirms O(n) but absolute cost is 8–143 ns |
| 5 | Cached visible count | 1–2% | **0.5–1%** | Mock confirms linear scaling (397–1931 ns) |
| 6 | propertynotify() filter | 1–2% | **<0.5%** | Mock shows 21 ns savings; real XGetWindowProperty is ~300 cycles |
| 7 | setlayout() guard | <0.5% | **<0.5%** | Confirmed negligible (7 ns) |
| 8 | enternotify() guard | 0.5–1% | **0.5–1%** | Mock can't measure; estimate based on real X11 cost |
| 9 | Gaming mode | 5–10% | **1–2%** | Mock shows most handlers already guarded or cheap |
| | **Cumulative (#1–8)** | **10–15%** | **5–10%** | |
| | **Cumulative (#1–9)** | **15–25%** | **6–12%** | |

---

## Environment

- **OS:** Linux (arch-based)
- **Compiler:** GCC 16.2.1
- **Flags:** `c99 -std=c99 -pedantic -Wall -Wextra -g -O0`
- **Mock:** mock_x11.h/c + mock_drw.h (no real X server)
- **Iterations:** 50,000 per benchmark
- **Timing:** `clock_gettime(CLOCK_MONOTONIC)` → nanoseconds

---

## 2026-08-23 Re-run After bench_optimize Repair

`bench_optimize.c` had drifted from the current API (embedded `Gap` struct,
per-monitor `bar_dirty_segments`) and did not compile; repaired and re-run.
Numbers below are the fresh baselines. Note: benchmark [1] runs on an empty
client list; [2] is the true 10-client arrange cost.

| # | Benchmark | Expected (orig -> revised) | Measured delta | Measured % | Verdict |
|---|---|---|---|---|---|
| 1 | arrange() coalescing | 3-5% -> 3-5% | burst loops cannot show deferral in mock; unit cost = 483 ns/call @10c, 924 @20c | up to (N-1)x per-call cost saved per burst | OPEN - biggest absolute mock win |
| 2 | updatestatus() compare | 2-3% -> 1-2% | identical 32 vs alternating 35 ns: guard ALREADY SHIPPED; residual delta 3 ns = 8.6% of fn | 3 ns abs (<5 ns Key Rule) | DONE - no headroom left |
| 3 | focus() idempotent guard | 2-4% -> 0.5-1% | same-client 91 vs different 94 ns: guard ALREADY SHIPPED (commit 284d22a) | ~3% of fn, 3 ns abs | DONE |
| 4 | wintoclient() hash | 0.5-1% -> <0.5% | miss-penalty over hit: +11 ns @10c (58%), +26 @20c (59%), +85 @50c (60%) | ~60% of lookup at scale, <=85 ns abs | OPEN - sub-0.5% real |
| 5 | tile/monocle count cache | 1-2% -> 0.5-1% | walk not isolated by this suite; scaling linear: tile 389/765/1905 ns @10/20/50c confirms O(n) target | n/a | OPEN - needs dedicated count-walk microbench first |
| 6 | propertynotify() filter | 1-2% -> <0.5% | uninteresting atom 13 vs root WM_NAME 35 ns | 22 ns = 63% of fn skipped | OPEN - real <0.5%, frequency-dependent |
| 7 | setlayout() dirty guard | <0.5% -> <0.5% | same-layout 12 vs toggle 11 ns (noise-level): guard ALREADY SHIPPED | ~0% | DONE - confirmed nothing left |
| 8 | enternotify() single-mon guard | 0.5-1% | single-monitor 10 vs dual-monitor 10 ns | 0% measurable | REJECTED - matches AGENTS.md gotcha (do not guard) |
| 9 | gaming mode | 5-10% -> 1-2% | fullscreen-frozen paths already active: updatestatus 2 vs 32 ns (-94%), propertynotify 9 vs 13 (-31%), enternotify 10 vs 10 (0%) | most of estimate banked by Phase 4 freeze | MOSTLY DONE - residual <1-2% |

### Reconciliation with documented mock baselines (AGENTS.md table)

| Function | Documented ns/call | This run | Delta |
|---|---|---|---|
| arrange() 10c | 45 | 483 ([2]; [1] empty-list 22) | doc row appears to have measured empty-list or pre-segment-tracking code |
| focus() | 90 | 91 | match |
| wintoclient() 10c | 8-20 | 8 hit / 19 miss | match |
| updatestatus() | 31 | 32-35 | match |
| propertynotify() | 14 | 13-35 (atom-dependent) | match at low end |
| setlayout() | 7 | 11-12 | +64%; doc row stale or machine variance |

Machine/run variance is roughly +-2x on sub-100 ns measurements (compare
arrange/setlayout rows), which reinforces the Key Rule: deltas under ~5 ns
are not actionable.

### Cumulative outlook

Only #1 (arrange coalescing) retains meaningful mock-visible headroom
(483+ ns per avoided call during bursts). Everything else measures at or
below the revised estimates' floor, consistent with the cumulative
projection of 5-10% (#1-#8) / 6-12% (#1-#9).

---

## 2026-08-23 (2): Post-Implementation Measurements

Optimizations #1/#4/#5(pivoted)/#6 implemented; suite re-run on same machine.
Correctness tests in `tests/test_optimize_layout.c` (41 assertions).

| # | Optimization | Baseline ns/call | After | Delta |
|---|---|---|---|---|
| 4 | wintoclient hit @10c / @50c | 8 / 56 | 4 / 4 | -50% / -93% |
| 4 | wintoclient miss @10c / @50c | 19 / 141 | 4 / 4 | -79% / -97% |
| 6 | propertynotify uninteresting atom | 13 | 4 | -69% |
| 1 | arrange burst, per request (3/event) | 1400 (3x466) | 475/3 = 158 | -66% per request |
| 5 | tile() @20c / @50c (all tiled) | 765 / 1905 | 747 / 1828 | -2% / -4% |

### Notes

**#5 pivot**: roadmap proposed caching the visible-client count, but
`tile()` counts *nexttiled* clients (excludes floating/invisible), so a
visible-count cache has wrong semantics for it. The actual duplicated work
was `nexttiled()` rescans restarting from the list head per placement step;
replaced with one filtered snapshot pass (`Client *tiled[n]`). On all-tiled
lists the rescan was already cheap (O(1) steps), hence only -2..-4%; the
saving grows with the floating/invisible prefix length. Geometry verified
byte-identical by existing tile tests + new ordering test.

**#1 semantics**: `arrange()` now sets `arrange_pending` while
`dispatching=1` inside run()'s handler call; `flusheventtail()` performs at
most one full pass then coalesced bar draws (arrange first, bars after).
Calls outside dispatch (movemouse/resizemouse grabs, setup(), scan()) stay
immediate. Verified N->1 pass, geometry equivalence vs immediate path.

**#4 safety**: hash entries re-keyed on swallow/unswallow window swaps;
cluster-repair deletion keeps probe chains intact; when the table is full,
insertion is suppressed and lookups fall back to the authoritative list
walk. Test fixtures register via `winclient_put()`; every test clears the
table at entry for isolation.

### keypress(): sorting keys[] by sym/mod? Measured: no.

New benchmarks [34]/[35]: mask-reject path 4 ns, matched full-scan+dispatch
4 ns -- identical within noise. The ~80-entry linear scan costs nothing
(branch-predicted, cache-resident), the existing key_mod_used /
key_keysym_used fast-path already rejects unmatched events in 4 ns, and
keypresses are human-rate (~10/s). A sorted layout + binary search would
save single-digit nanoseconds on a path that fires ~10 times per second:
unmeasurable. Sorting would also make hand-edited configs error-prone
(order-dependent semantics). Recommendation: keep as is.

### Cumulative status after implementation

Roadmap items now DONE: #1 (coalescing), #2 (updatestatus guard),
#3 (focus guard), #4 (hash), #6 (filter), #7 (setlayout guard),
#9 mostly (Phase-4 freeze). #5 shipped as single-pass tile with honest
smaller-than-estimated win (-2..-4% typical, more with floating clients).
#8 enternotify guard rejected (0% measured + correctness gotcha).
Remaining open: CI setup, comprehensive-suite leak cleanup (~23 KB test-side).

---

## 2026-08-23 (3): Exact keypress binding index

Replaced the lossy `key_keysym_used` OR-mask with an exact open-addressed
set of packed (keysym, CLEANMASK(mod)) pairs built by `cachekeys()`
(rebuilt from grabkeys() so numlock changes stay in sync). The old guard
could not reject chords merely sharing a mod bit (event MODKEY|Control vs
binding MODKEY|Shift both pass `state & key_mod_used`), and passed almost
any keysym once enough bindings were ORed together.

| Path | ns/call | Note |
|---|---|---|
| unmatched mod (reject) | 6 | parity with old mask path |
| wrong chord sharing mod bit | 6 | old code: full keys[] scan |
| matched MODKEY+b (scan+dispatch) | 6 | unchanged semantics |

Measured: parity on every path at the ~4-6 ns noise floor -- the scan was
never the cost. The win is exactness and O(1) worst-case reject regardless
of keys[] growth, plus correctness tests proving wrong-chord rejection
while genuine bindings still fire-all. Per the Key Rule this is not a
performance optimization; it ships because it removes a semantic wart
(lossy filter) at zero measured cost. Tests:
`test_keypress_exact_index_rejects_wrong_chord` in test_optimize_layout.c.
Sorting keys[] remains unjustified: see 2026-08-23 (2).

---

## 2026-08-24: LeakSanitizer Exemption Removed

The comprehensive-suite leak debt is closed; `make asan` runs every binary
with leak detection on. Fixes: per-test fixture frees (guided by ASan
allocation lines), drain of dwm-created clients, pre-setup global release,
mock drw_free now releases the font chain like the real one, mock XFree
releases mock heap buffers as Xlib's does, and a genuine dwm fix --
gettextprop() leaked name.value on its nitems==0 return path.

---

## 2026-08-24 (2): Coverage-Driven Test Expansion

Baseline `make coverage` measured dwm.c at a *misreported* 91.7% -- the
merger's fixed-width gcov regex silently dropped counters wider than 8
characters, so hot lines vanished from the tables. After fixing the parser
(and generalizing it to report drw.c/util.c), true coverage was 98.4%.

New suites:
- `test_coverage_gaps.c` (48 assertions) -- applyrules class/terminal/fallback
  rules, applysizehints aspect+inc+max/min clamps, buttonpress tag-index and
  dispatch paths, keyset_put saturation/collisions, grabkeys numlock fan-out
  (new XGrabKey/XUngrabKey mock counters), manage-transient monitor pinning,
  scan override_redirect skipping, drawbars multi-monitor, updatesizehints
  isfixed, updatewmhints urgency, termforwin guards, movemouse/resizemouse
  entry guards + cross-monitor sendmon + snap-threshold growth, xerror
  swallowed classes, gettextprop conversion-failure branch.
- `test_stress_winhash.c` (~2.5k checks) -- fixed-seed xorshift differential
  vs a linear reference model: 164k insert/remove/rekey ops across three
  regimes plus a saturation regime proving the suppression guard accepts
  exactly one final slot, churn keeps clusters consistent, ghost removals are
  inert, and full-table lookups fall back to the list walk.

Results: dwm.c 98.9% (17 lines uncovered: fork/exec child body, xcb pid
iterator internals, defensive probe-exhaustion returns, two multi-monitor
walk corners), util.c 100%, drw.c real-file coverage is out of scope for the
mock harness (its logic is covered by the self-contained replica suites).
Gates: 13/13 unit suites pass; ASan strict clean on all 13 binaries.

---

## 2026-08-24 (3): Creation-Path Layout Immediacy Restored

User-visible regression report: window-creation transitions could look
snappier/uglier because arrange-coalescing deferred the manage()/unmanage()/
swallow() layout passes to the event-loop tail, collapsing the per-window
map->tile transition into a single final XConfigureWindow.

Fix: those three low-frequency paths now call arrangenow() directly, keeping
every transition frame visible; high-frequency handlers (tag/view/setlayout/
...) still coalesce. Correctness tests added to test_optimize_layout.c:
manage/unmanage lay out synchronously during dispatch with no pending flag,
while tag() still defers (54 assertions total).

Also fixed a latent build-system hazard this exposed: suite binaries
#include ../dwm.c but their Makefile rules did not depend on it -- editing
dwm.c left stale test binaries. Suites now depend on ../dwm.c, ../dwm.h,
../config.h and mock_drw.h.
