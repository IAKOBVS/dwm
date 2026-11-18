# Codebase Review -- Potential Improvements

Generated: 2026-08-23. Findings from parallel exploration of `dwm.c`, `drw.c`,
`config.*`, build files, and the `tests/` suite. Items already tracked in
`TODO.md` / `OPTIMIZE-*.md` are referenced rather than duplicated.

## Update (2026-08-23, post-implementation)

Items 1, 2, 3, 4, 5, 6, 7, 8, 11 (ASan gap), and 12 are **resolved**.
Item 1's table below was corrected during implementation: the volume-vs-gaps
rows were false positives (the pairs differ by the Shift modifier) and a fifth
real collision (`MODKEY+space`: zoom + setlayout) was found. Consciously not
done: item 9 range-gating (eviction logic is sound; scheme keying shrinks the
blast radius), item 10 XftFontMatch hardening (practically unreachable),
CI setup. `bench_optimize.c` was repaired en route (stale `Gap` pointer /
global `bar_dirty` API); it builds and runs again.

Enabling ASan on `test_comprehensive` (item 11) immediately paid off: it
exposed 13 tests that freed their local Monitor while `selmon`/`mons` still
pointed at it (the AGENTS.md "Known Gotchas" violation), causing a real
heap-use-after-free once any later test called `setup()` → `updategeom()`.
All 13 now restore globals before freeing. `test_drw_safety` leaked its mock
Displays (15 × XOpenDisplay without close) — fixed. Coverage follow-up (same day): merged gcov reporting now
covers dwm.c/drW.c/util.c with a padding-safe regex (the old parser
under-reported); dwm.c stands at 98.9% with 13 suites / ~3.8k
assertions, ASan strict clean everywhere.

Leak-sanitizer follow-up completed same day: LeakSanitizer now runs strict on every
suite with no exemptions. `test_comprehensive` fixtures are freed at each
test end (ASan-allocation-line-guided insertion), clients created inside
dwm by manage/maprequest/scan are drained via `unmanage(c, 1)`, setup-held
globals are released before re-setup, and two real fixes came out of it:
`gettextprop()` now XFrees the value buffer on its nitems==0 early-return,
and the mock's `drw_free` mirrors the real one by releasing the font chain.

## Priority Summary

| # | Severity | Area | Finding |
|---|----------|------|---------|
| 1 | HIGH | `config.h` | Five duplicate keybindings all fire simultaneously |
| 2 | HIGH | `Makefile` | `dist` tarball omits `dwm.h` -- cannot compile |
| 3 | HIGH | git | 10 compiled test binaries tracked in git |
| 4 | MED | `drw.c` | Emoji render cache bakes in fg/bg colors of first draw |
| 5 | MED | `drw.c` | Emoji cache survives `drw_resize` -> out-of-bounds `XCopyArea` |
| 6 | MED | tests | `make test` swallows failures (exit codes ignored) |
| 7 | MED | tests | killall / killatfullscreen functions have zero coverage |
| 8 | MED | Makefile | No header dependency tracking (`-MMD`) -> stale-object bugs |
| 9 | LOW-MED | `drw.c` | All non-ASCII codepoints flood the 32-slot emoji cache |
| 10 | LOW | `drw.c` | Latent infinite loop if `XftFontMatch()` returns NULL |
| 11 | LOW | CI | No CI anywhere; ASan target skips largest suite |
| 12 | INFO | docs | AGENTS.md describes XGetImage/XPutImage; code uses XCopyArea |

## 1. Correctness Bugs

### 1.1 [HIGH] Duplicate keybindings fire together -- `config.h`

`keypress()` dispatches every matching binding (no break after match), so each
of these pairs executes both actions on one keypress:

| Key | Fires both | Lines (pre-fix) |
|---|---|---|
| `MODKEY+s` | `dsite` spawn AND SCREENSHOT spawn | `config.h:174`, `config.h:182` |
| `MODKEY+c` | `dclip` spawn AND COLOR_PICKER spawn | `config.h:175`, `config.h:180` |
| `MODKEY+Shift+0` | OBS spawn AND gaps reset | `config.h:178`, `config.h:197` |
| `ALTKEY+equal` | volume up "2" AND `cfan` | `config.h:186`, `config.h:187` |
| `MODKEY+space` | `zoom` AND `setlayout(tile)` | `config.h:164`, `config.h:168` |

(The volume-down/gaps+1 and volume-up/gaps-1 pairs initially reported here do
NOT collide -- one side has Shift, so CLEANMASK differs.)

Resolution applied: first occurrence kept, second commented out with a note;
GAP_RESET moved to `ALTKEY|Shift+0`; `cfan` moved to `ALTKEY+F9`.

Also lost functionality vs `config.def.h` worth restoring consciously:
no `quit` binding (`config.h:199` commented), no `tagmon`
(`config.h:171-172` commented), no send-to-tag `Shift+N` in TAGKEYS,
no B2/B3 client buttons.

### 1.2 [HIGH] `dist` ships an uncompilable tarball

`Makefile:33-34` copies `drw.h util.h ... dwm.c` but omits `dwm.h`.
Fix: add `dwm.h` to the `cp` list.

### 1.3 [MED] Emoji cache captures scheme colors -- `drw.c`

On miss (`drw.c:646-648`) the emoji renders onto the drawable already filled
with the current scheme background; the region is then copied verbatim into
the cache pixmap. On hit (`drw.c:634-636`) the pixmap blits back unchanged.
A codepoint first drawn under `SchemeNorm` shows wrong colors under
`SchemeSel` or any other scheme / inverted draw. Cache key is codepoint-only.

Fix options: extend key with fg/bg pixels (or scheme index + invert), or
invalidate the cache inside `drw_setscheme()` (`drw.c:454-457`). Related:
`drw_setfontset()` (`drw.c:442-447`) swaps fonts without invalidating the
glyph-width cache (latent API hazard; dwm only calls `drw_fontset_create`,
which does invalidate). Also note `glyph_cache` is process-global while
`emoji_cache` is per-Drw -- inconsistent ownership.

### 1.4 [MED] Emoji cache survives `drw_resize` -- `drw.c:272-286`

`drw_resize` replaces the backing pixmap but deliberately keeps cached pixmaps
sized to the old bar height. After a bar-height change (font change ->
`updatebarpos` -> `drw_resize`), the blit requests `eslot->w x h` with the new
`h`, reading outside the cached pixmap -> BadMatch + glitched output.
Fix: call `emoji_cache_invalidate(drw)` in `drw_resize`.

### 1.5 [LOW-MED] Non-emoji codepoints flood the emoji cache

`drw.c:611-651`: any decoded codepoint > 0x7F with nonzero width (accents,
CJK, box-drawing, symbols) is pixmap-cached, not just color emoji. With 32
slots this evicts real emoji and widens the blast radius of 1.3 (a CJK title
cached under one scheme blits wrong colors under another).
Fix: gate caching on verified color-emoji ranges, or accept eviction cost.

Cache thrash note: when >32 distinct codepoints rotate, each redraw misses and
pays pixmap create/free per draw (`emoji_cache_insert`, `drw.c:126-144`).
Consider occupancy counter / LRU-ish reset. Eviction logic itself is sound.

### 1.6 [LOW] Latent infinite loop if `XftFontMatch()` returns NULL

`drw.c:704-725`: if font matching fails, neither `nomatches` is recorded nor
does `text` advance; the outer `while(1)` re-decodes the same character
forever. Practically unreachable (fontconfig nearly always matches) but cheap
to harden: treat `match == NULL` like the no-glyph case
(`nomatches` + `text += utf8charlen`).

### 1.7 [INFO] Misc defensive gaps

- `drw_map` (`drw.c:732-740`): no zero-size guard before `XCopyArea`.
- `util.c:35`: `die()` appends errno text even when irrelevant; `util.c:48`
  message reads `"calloc():calloc:"`. Cosmetic.
- `transient.c` manual fixture has no exit path in its event loop; consider
  moving to `tests/` with timeout/auto-exit.

## 2. Test Suite Gaps

Current measured inventory: ~899 assertions across 9 binaries (AGENTS.md's
"878 tests" is stale; several files grew).

### 2.1 Functions never called by any test

- `killall()` (`dwm.c:1506-1516`) -- zero references.
- `killatfullscreen_stop()` (`dwm.c:1518-1523`) -- zero references.
- `killatfullscreen_start()` (`dwm.c:1525-1536`) -- fork/sleep/HUP sequencing
  completely untested.
- setfullscreen integration hooks (`dwm.c:1542`, `dwm.c:1553`) never asserted.
  Root cause: untestable seam -- tests compile against local `config.h`
  (`#ifdef DWM_TEST` empties the process list) and the functions exec the real
  `killall` binary. Suggested fix: injectable function pointer (pattern already
  exists as `xerrorxlib`) so argument assembly can be asserted without exec.

### 2.2 Weak or partial coverage

- sighup/sigterm (`test_comprehensive.c:903-914`): asserts only
  `running == 0`; never checks sighup -> restart=1 vs sigterm -> restart=0.
  Also `test_sighup` does not reset `running = 1` first (can pass vacuously).
- `updatestatus()` unchanged-text guard (`dwm.c:2153-2154`): no identical-vs-
  different-text correctness tests (violates AGENTS.md checklist item #2).
- `swallow()` setting `DIRTY_TITLE` (`dwm.c:248`): no test asserts it.
- Real `drw.c` caches are tested only via inline replicas
  (`test_drw_cache.c`, `test_emoji_render_cache.c`) -- replica drift risk.
  Only `test_drw_safety.c` compiles real `drw.c`, and just NULL paths.
  Consider linking real `drw.c` into a cache test or adding drift checks.
- `getrootptr()`: no direct True/False path tests despite mock support
  (`mock_querypointer_return`).
- `isuniquegeom()` (`dwm.c:954`) and all XINERAMA branches of `updategeom()`
  / `setup()`: not compiled in test builds (no `-DXINERAMA`, no Xinerama
  mocks). Adding stubs + the flag unlocks multi-monitor geometry coverage.
- `util.c` zero direct tests; `dwm.h` inline helpers `strcpy_len` /
  `stpcpy_len` / `stpncpy_len` lack truncation edge-case tests.

### 2.3 Test infrastructure improvements

- `tests/Makefile:49-53`: `make test` ignores exit status of each binary --
  make reports success even when suites fail. Highest-value one-line fix.
- `asan` target (`tests/Makefile:69-73`) builds 8 targets but omits
  `test_comprehensive` (largest, most pointer-heavy suite).
- Coverage target hardcodes `COV_LABELS` (`tests/Makefile:55`) -- new test
  files are auto-built but NOT auto-covered (contradicts AGENTS.md).
- `run.sh` covers only 2 of 9 binaries; stale.
- Order dependence: single `main()` chains tests sharing globals
  (`selmon`, `mons`, `running`, `bar_draw_pending`);
  `test_bar_draw_pending_initial` must self-reset because others pollute it.
- ~15 vacuous `ASSERT(1, ...)` smoke tests that cannot fail
  (`test_updatenumlockmask`, `test_spawn`, `test_updatestatus`, ...).
- Magic numbers: sentinel window `42`/`99999`; raw mask `7` instead of
  named segment flags (`test_segments.c:371`).
- `make_monitor` called 237x vs `free(m)` 126x in test_comprehensive --
  leaks invisible without LeakSanitizer; fixing the asan gap surfaces them.
- No CI at all (no `.github/workflows`). Minimal win: workflow running
  `make test && make asan && make coverage` with failure propagation.

## 3. Build System

- **Header deps** (`Makefile:20`): objects depend only on `config.h
  config.mk`; editing `dwm.h`/`drw.h` does not trigger rebuilds. Add
  `-MMD -MP` to CFLAGS and `-include $(OBJ:.o=.d)`.
- **LTO mismatch** (`config.mk:32-33`): `-flto` in CFLAGS but LDFLAGS lacks
  `-flto -O2`, so link-time optimization is neutered. Align or drop.
- `-Wall -pedantic` but no `-Wextra`; AGENTS.md documents several signedness
  warnings that stricter flags would contextualize.
- `-march=native` makes installed binary machine-specific (fine locally;
  breaks the `dist` story).
- Hard-coded `/usr/X11R6/{include,lib}` legacy paths; pkg-config cleaner.
- `dist: clean` deletes the fresh binary before packaging (upstream quirk).

## 4. Repo Hygiene

- **HIGH**: 9 compiled test binaries tracked in git (`git ls-files` shows
  ELF executables incl. `tests/test_comprehensive`, orphan `tests/test_dbg`
  with no source). Fix: `git rm --cached tests/test_*` for binaries and add
  ignore patterns.
- `.gitignore:4-6` dead negations (`!tests/*.c` etc.) mislead readers --
  likely how binaries got committed. Remove and add real patterns:
  `*.tar.gz`, `*.gcda`, `*.gcno`, `perf.data*`, `gmon.out`, test executables.
- Foreign entries `dwmblocks`/`dwmblocks-fast` belong to another project.
- Patch/reference artifacts (`.orig`, `.rej`, `*.diff`) are sanctioned by
  AGENTS.md; consider relocating to `patches/` for a cleaner top level.

## 5. Documentation Drift

- AGENTS.md Phase 5 says the emoji cache captures pixels via XGetImage and
  blits via XPutImage; actual `drw.c` uses pixmaps + XCopyArea (3 call sites),
  and mocks accordingly lack XGetImage/XPutImage. Reconcile doc to code.
- AGENTS.md test counts stale (see 2 above).
- TODO.md already tracks remaining performance work (arrange coalescing,
  updatestatus debouncing, gaming mode, enternotify guard, cached visible
  counts); this review intentionally does not duplicate those.

## Suggested Order of Work

1. De-duplicate six colliding bindings in `config.h` (user-visible today).
2. Fix `make test` exit-code swallowing; add CI wrapper around
   `make test && make asan && make coverage`.
3. `emoji_cache_invalidate(drw)` in `drw_resize`; decide scheme-keying (1.3)
   or range-gating (1.5) for the emoji cache.
4. Add `dwm.h` to `dist`; add header deps (`-MMD`); align LTO flags.
5. Un-track committed test binaries; repair `.gitignore`.
6. Tests: killall family behind injectable seam + setfullscreen hook asserts;
   sighup restart=1 assertion; updatestatus idempotence; swallow DIRTY_TITLE.
7. Harden `XftFontMatch()==NULL` fall-through; reconcile AGENTS.md docs.
