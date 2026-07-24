# TODO

## Now

Fix swallowing: disallow to swallow random terminals, when can't determine which terminal to swlalow

## Potential Optimizations

Status: `[OPEN]` = not started, `[PARTIAL]` = partially done, no tag = done.

- `[OPEN]` **Skip unnecessary root pointer work when idle.** `motionnotify()` returns immediately on single-monitor, but `PointerMotionMask` is still registered in `setup()` (dwm.c:1661), so X11 delivers events that are immediately discarded. Consider conditionally removing the mask via `XSelectInput` when only one monitor is present, or removing it entirely while a fullscreen client is focused.

- `[OPEN]` **Add a runtime gaming/low-overhead mode.** Suppress nonessential work while a fullscreen game is focused: root motion monitor switching, status-triggered bar redraws, `propertynotify` processing for non-critical atoms, and `updatestatus()` calls. `optimizefullscreen` only skips `drawbar()` — many ancillary handlers still run.

- `[PARTIAL]` **Avoid redundant bar redraws.** Segment-level dirty tracking (`DIRTY_STATUS/TAGS/TITLE`) is live and effective, but callers lack before/after guards: `focus()` dirties segments even when the same client is still focused, `updatestatus()` dirties even when `stext` hasn't changed, `setlayout()` dirties even when the layout symbol is unchanged. Add value comparisons before setting segment bits.

- `[OPEN]` **Avoid redundant `arrange()` calls.** All 25+ callers invoke `arrange()` unconditionally — no check for whether client geometry or layout state actually changed. Add guard conditions at hot call sites (tag, fullscreen, swallow, unmanage, floating toggles) or introduce an `arrange_pending` coalescing flag (mirroring `bar_draw_pending`) to defer to the event-loop tail.

- `[OPEN]` **Reduce status update churn.** External status scripts can fire `XA_WM_NAME` changes on root at high frequency. `updatestatus()` runs on every one with no debouncing, rate-limiting, or before/after `stext` comparison. Consider: comparing new text vs. old before dirtying segments; skipping entirely when fullscreen is focused; or capping to ~10 Hz via a timestamp guard.

- `[OPEN]` **`arrange_pending` coalescing.** `bar_draw_pending` defers `drawbars()` to the event-loop tail, but there is no equivalent for `arrange()` — each separate event in a batch calls `arrange()` immediately. Introduce `arrange_pending` so that burst events (e.g., window creation) produce only one arrange pass.

- `[OPEN]` **Skip `updatestatus()` during fullscreen.** When the bar is frozen by `optimizefullscreen`, processing status updates is wasted work. Add a guard at the top of `updatestatus()` or in `propertynotify()`: if `optimizefullscreen && selmon->sel && selmon->sel->isfullscreen`, return early.

- `[OPEN]` **`propertynotify` early atom filtering.** Every property change on root or any client dispatches through `propertynotify()`, which calls `XGetWindowProperty` for interesting atoms. Return early for uninteresting atoms with a cheap constant-time check before any X11 call.

- `[OPEN]` **Cache visible-client counts in tile/monocle.** Both `tile()` and `monocle()` walk the full client list on every arrange to count visible clients. Cache this count and update it incrementally on attach/detach/showhide to avoid O(n) walks on every layout pass.

- `[OPEN]` **Single-monitor `enternotify` guard.** Same pattern as `motionnotify`: on single-monitor setups, `EnterWindowMask` events are delivered but `enternotify()` (dwm.c:694) still runs through the full `wintoclient`/`wintomon`/`focus` chain. Either conditionally mask the event, or add a fast-path return when `!mons->next`.

- `[OPEN]` **Simplify `updategeom()`.** The code itself already says `/* TODO: updategeom handling sucks, needs to be simplified */` at dwm.c:463. The multi-monitor geometry update path (Xinerama, monitor add/remove, fullscreen resize) is convoluted and a source of subtle bugs. Simplification may also reduce unnecessary arrange/draw churn on geometry changes.

## Hidden Bug Hunting Plan

To systematically flush out remaining hidden bugs (memory leaks, crashes, logic flaws, and protocol errors), we can use a multi-tiered approach utilizing existing testing tools plus some deeper analysis.

### Phase 1: Dynamic Stress Testing (Xvfb + Valgrind / ASan)
1. **Valgrind Memory Audit**: Run `./bench_xvfb_stress.sh --valgrind`. Valgrind will track every `malloc()` and `free()`. If `dwm` leaks memory when mapping/unmapping clients, or accesses memory after it is freed (Use-After-Free), Valgrind will flag the exact line number.
2. **ASan (AddressSanitizer) Builds**: Compile `dwm` using your existing ASan flags (`ASAN_CFLAGS`). Run the stress test against the ASan-instrumented binary. ASan is much faster than Valgrind and will instantly crash with a stack trace if there are any array out-of-bounds reads/writes.

### Phase 2: Static Analysis Audit
1. **GCC Analyzer**: Your `Makefile` already includes `-fanalyzer`. Capture and audit the output of a clean `make clean && make` to see if it flags any null-pointer dereferences or dead code.
2. **Clang-Tidy / Cppcheck**: Run strict linters over `dwm.c` and `drw.c` to identify unsafe C practices (like integer truncation, uninitialized variables, or implicit casts that could cause unexpected behavior on different architectures).

### Phase 3: X11 Protocol Edge-Case Audit (Manual & Automated)
1. **[DONE] Error Handler Audit**: Audited `xerror` and `xerrorstart` in `dwm.c`. Identified that unhandled `BadValue` errors were causing the `startx` crash loops. Fixed all `XCreatePixmap`, `XDrawRectangle`, and padding underflow edge cases in `drw.c`, and successfully implemented Xvfb-based tests in the `mock_x11` suite to prevent regressions.
2. **X Event Fuzzing**: Write a small C program that injects garbage or malformed `XEvent` structs directly into `dwm`'s event handlers (`configurenotify`, `clientmessage`, `propertynotify`) to see if unexpected event data crashes the WM.

### Phase 4: Patch Interoperability
1. **Review Patch Seams**: Identify all patches applied to this build (e.g., Color Emoji, Systray, Alpha, AttachAside).
2. **Check Shared State**: Review how these patches interact with the global `Client` list and monitor states. For example, if the systray patch resizes the bar, does the alpha patch handle the new dimensions correctly?

## Missing Test Edge Cases

**Line coverage**: 99.93% of dwm.c (1348 lines covered, only dead code at line 231 uncovered).  
**Total tests**: 878 across 9 binaries in `tests/` (up from 833).

Of the 18 identified gaps, 22 new tests were added covering all High and Medium priority items. Two items remain:

| Priority | Function | Missing edge case | Status |
|----------|----------|-------------------|--------|
| Low | `focusmon` | Wrap past end/beginning of empty monitor list | Already tested by `test_focusmon_switches_*` and `test_focusmon_prev` — wrap is exercised |
| Low | `keypress` | Numlock-only modifier early return | Requires mock keymap infrastructure (modmap/keysyms) — low impact |
| Low | `scan` | `XQueryTree` failure; `XGetWindowAttributes` failure; already-mapped windows | `test_scan_no_windows` covers XQueryTree returning 0; `test_scan_xgetwindowattr_fail` covers attr fail; iconic/override/transient all covered |

### Coverage Summary (22 new tests added)

| Edge case | Test function |
|-----------|---------------|
| `applysizehints` incw=0, mina=maxa=0 | `test_applysizehints_incw_zero` |
| `applysizehints` baseismin=false | `test_applysizehints_baseismin_false` |
| `buttonpress` click past last tag | `test_buttonpress_past_last_tag` |
| `buttonpress` ClkMasterTag dispatch | `test_buttonpress_clkmastertag` |
| `buttonpress` ClkRootWin no-match | `test_buttonpress_clkrootwin` |
| `configurenotify` non-root window | `test_configurenotify_non_root` |
| `configurenotify` fullscreen resize loop | `test_configurenotify_fullscreen_resize` |
| `enternotify` NotifyGrab mode | `test_enternotify_grab_mode` |
| `enternotify` NotifyUngrab mode | `test_enternotify_ungrab_mode` |
| `enternotify` own window early return | `test_enternotify_own_window` |
| `focus` NULL arg with valid selmon | `test_focus_null_selmon_ok` |
| `focus` idempotent (same client) | `test_focus_idempotent` |
| `focusstack` all clients invisible | `test_focusstack_all_invisible` |
| `monocle` 2+ clients | `test_monocle_multiple_clients` |
| `propertynotify` unsupported atom | `test_propertynotify_unsupported_atom` |
| `propertynotify` transient non-sel | `test_propertynotify_transient_non_sel` |
| `resize` floating client with increment | `test_resize_floating` |
| `setlayout` non-zero arg | `test_setlayout_arrange_monitor_null_gap` |
| `tile` nmaster > n | `test_tile_nmaster_gt_n` |
| `unmapnotify` non-client window | `test_unmapnotify_non_client` |
| `updatesizehints` PBaseSize + PMinSize | `test_updatesizehints_min_from_base` |
| `updatesizehints` only PSize (all defaults) | `test_updatesizehints_only_psize` |
