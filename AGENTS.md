# Repository Guidelines

## Project Structure & Module Organization

This repository is a customized dwm 6.4 source tree. Core code lives at the
top level: `dwm.c` contains window-manager behavior, `drw.c`/`drw.h` handle
drawing, and `util.c`/`util.h` provide shared helpers. `dwm.h` holds project
types and declarations. Configuration is compile-time: edit `config.h` for a
local build, and keep reusable defaults in `config.def.h`. Build settings,
paths, compiler flags, and optional Xinerama support are in `config.mk`.
Patch files (`*.diff`), `.orig` files, and `dwm.c.rej` document prior patch
work and should be treated as reference artifacts unless intentionally updating
the patch history.

## Build, Test, and Development Commands

- `make` builds the `dwm` binary from `drw.c`, `dwm.c`, and `util.c`.
- `make clean` removes object files, the binary, and generated tarballs.
- `make clean install` rebuilds and installs `dwm` and its man page using the
  `PREFIX` and `MANPREFIX` values in `config.mk`; this may require root.
- `make dist` creates a `dwm-6.4.tar.gz` source archive.

Before building on a new system, ensure Xlib, Xft/fontconfig, Xinerama, and xcb
development headers match the flags in `config.mk`.

## Coding Style & Naming Conventions

Follow the existing suckless C style: C99, tabs for indentation, compact helper
functions, and minimal abstractions. Function names are lowercase without
underscores where practical, matching examples such as `applyrules` and
`buttonpress`. Keep declarations near related code, preserve existing macro
style, and avoid broad refactors while changing behavior. Format manually to
match surrounding code; no formatter is configured.

Every new feature MUST include:
- A **purpose comment** above the primary declaration or implementation
- **Inline comments** for non-obvious logic (bit operations, guard conditions,
  algorithm choices, performance trade-offs)
- **Corresponding unit tests** that exercise the primary path and at least one
  edge case

## Subagent & Parallelism

Development and testing leverage up to 5 concurrent subagent threads for
parallel work. Subagents build, test, and generate code independently and
report back results. This document describes the repository guidelines
that agents apply when working.

## Testing Guidelines

### Quick Start

```sh
cd tests
make run       # build and run all unit tests
make coverage  # clean build, run tests with gcov, show dwm.c coverage
```

### Test Architecture

Unit tests live in `tests/` and compile dwm.c with mock X11 headers (no real
X server needed). Every test `.c` file follows this pattern:

1. `#include "mock_x11.h"` + `#define DRW_H` + `#include "mock_drw.h"`
2. `#include "../dwm.h"` + `#include "../dwm.c"` — pulls in all dwm internals
3. A `main()` that initializes globals (`dpy`, `drw`, `selmon`, `scheme[]`) and
   calls test functions
4. `ASSERT()` / `ASSERT_EQ()` macros for pass/fail

### Current Test Files (833 tests total, 9 binaries)

- **test_pure_logic.c** (~1100 lines, 101 tests) — core dwm logic: linked-list
  ops (attach, detach, attachstack, detachstack), nexttiled, dirtomon,
  recttomon, wintoclient, gap_copy, setgaps (toggle/adjust/clamp/reset),
  tag/toggletag/toggleview/view, setmfact, incnmaster, zoom/togglefloating,
  setlayout, tile/monocle, updatebarpos, focusstack, swallowingclient,
  cachebuttons/cachekeys, ISVISIBLE/INTERSECT macros, applysizehints,
  createmon defaults, mousebuttonmatch fast-path, keypress mapped/unmapped

- **test_segments.c** (~230 lines, 17 tests) — bar_dirty_segments tracking:
  initial state, drawbar reset, drawbar early return, focus sets segments,
  updatestatus sets segments, setlayout sets segments, togglebar sets segments,
  setfullscreen(0) sets segments

- **test_arrange.c** (~420 lines, 42 tests) — arrange/arrangemon with tile
  and floating layouts (null, no clients, multi-monitor), tile geometry
  (1 client, 2 clients master/stack, nmaster=0, nmaster > n), monocle
  counting (1 client, 2 clients), resize/resizeclient geometry tracking
  and applysizehints clamping, setfocus (normal/neverfocus), showhide
  (null/visible/invisible), focus (sel changes, NULL finds visible),
  unfocus (with/without setfocus)

- **test_window_ops.c** (~310 lines, 29 tests) — togglebar toggles showbar and sets
  dirty segments, togglefloating (toggle, fullscreen noop, no-sel noop),
  seturgent flag, sendmon (changes monitor, detach/attach, same-monitor noop),
  unmanage (detach, destroy), manage geometry clamping,
  setclientstate (NormalState, WithdrawnState)

- **test_drw_cache.c** (~550 lines, 25 tests) — glyph-width cache in isolation:
  cache miss/hit returns same value, cache hit confirms stored width, ASCII
  uses direct extents, cache invalidate preserves value, empty/NULL/null-fonts
  returns 0, full-cache eviction no-infinite-loop, missing glyph returns 0,
  drw_fontset_getwidth_clamp limits/zero/n>width, linear probing across
  colliding slots, utf8decodebyte valid/continuation, utf8validate rejects
  surrogates/out-of-range, utf8decode zero-length input

- **test_events.c** (~560 lines, 62 tests) — focusin (different window, own window,
  no selection), clientmessage fullscreen (add, remove, toggle, unknown window,
  NetActiveWindow urgent), unmapnotify (send_event withdraw, non-send_event
  unmanage), destroynotify unmanage, configurerequest floating geometry
  (full mask, partial mask, non-client), expose (barwin, other),
  propertynotify (root WM_NAME, PropertyDelete early return, client
  WM_NORMAL_HINTS, WM_HINTS sets DIRTY_TAGS, WM_NAME on sel/non-sel),
  setfullscreen enter/exit/idempotent, togglefullscr with/without sel,
  enternotify (non-Normal mode, NotifyInferior, entering sel returns early,
  entering barwin returns early)

- **test_comprehensive.c** (~7136 lines, 428 tests) — broad dwm.c coverage
  across all 107 functions: applyrules/applysizehints, swallow/unswallow,
  buttonpress/click types/tag iteration, configure/configurenotify,
  configurerequest (floating/full/non-client), createmon, destroynotify,
  dirtomon, drawbar (segments/fullscreen freeze), enternotify, expose,
  focusmon/focusstack, getatomprop/getstate/gettextprop/title/winpid,
  grabkeys/grabbuttons, incnmaster, keypress, killclient, manage
  (new/clamping/swallow/transient/centered), mappingnotify, maprequest,
  monocle, motionnotify, movemouse (snap/cross-mir/throttle/early return),
  pop, propertynotify (all atoms), resize/resizeclient, resizemouse,
  restack, run events, scan (windows/iconic/override/transient),
  sendevent, sendmon, setclientstate, setfocus, setfullscreen,
  setgaps/setlayout/setmfact, seturgent, showhide, sighup/sigterm,
  spawn (fork/child/dmenucmd/null), tag/toggletag/toggleview/view,
  tagmon, textnw, tile, togglebar/togglefloating/togglefullscr,
  unmanage/unswallow/unmapnotify, updatebars/updatebarpos,
  updateclientlist, updategeom, updatenumlockmask, updatesizehints,
  updatestatus/updatetitle/updatewindowtype/updatewmhints,
  termforwin/isdescprocess/getparentprocess,
  wintoclient/wintomon, xerror (all variants), zoom

- **test_drw_safety.c** (~330 lines, 33 tests) — drw.c null/zero/edge-case
  crash-safety: drw_create (NULL display/colormap), drw_resize (NULL/zero
  dims), drw_rect (NULL/zero/empty/filled/multiple), drw_map (NULL/zero
  area), drw_text (NULL/zero/empty), drw_fontset_getwidth (NULL/zero/
  clamp), drw_setfontset (NULL), drw_setscheme (NULL), drw_free (NULL)

- **test_emoji_render_cache.c** (~200 lines, 87 tests) — emoji render cache
  (Phase 5): cache miss fills entry, subsequent hits reuse cached XImage,
  hash collision probes linearly, codepoint 0 does not cache,
  drw_emojicache_get returns NULL for cacheable but uncached emoji,
  per-dpy cache isolation, drw_free frees all cache entries

### Mock Infrastructure

- **mock_x11.h** — ~700 lines: all X11 types (Display, Window, Atom, XEvent
  union with all subtypes), X11 constants (modifiers, keysyms, atoms, CW*,
  etc.), and ~80 X11 function declarations

- **mock_x11.c** — stub implementations for all ~80 Xlib functions (safe
  no-ops), plus `ecalloc()`, `die()`, and xcb stubs

- **mock_drw.h** — `static inline` no-op stubs for all `drw_*()` functions

- **Forwarding headers** under `tests/include/X11/` and `tests/include/xcb/`
  redirect angle-bracket includes (`<X11/Xlib.h>`, `<xcb/xcb.h>`) to the mocks,
  keeping `-I tests/include` ahead of system includes

- **DWM_TEST define** — guards `main()` in `dwm.c` (`#ifndef DWM_TEST` …
  `#endif`) and suppresses `bar_dirty_segments = 0` in `drawbar()` so tests
  can verify segment bits after the function returns

- **test_drw_cache.c is self-contained** — it provides its own type stubs for
  `Display`, `XftFont`, `FcPattern` and does NOT include mock_x11.h, mock_drw.h,
  or the real dwm.c. It replicates the cache/UTF-8 logic from drw.c inline so
  tests never hit real X11 or Xft headers.

### Known Gotchas

- Tests that set `selmon = &local_monitor` MUST restore `selmon` to the
  heap-allocated original before the local variable goes out of scope,
  otherwise subsequent tests crash on dangling pointers
- Many dwm functions call `arrange()` → `tile()` which accesses `m->gap` — any
  local `Monitor` used with such a function must have `m.gap` allocated
- `toggletag()` preserves a client's last tag (won't XOR to 0) — tests must
  match actual dwm behavior, not assumed behavior
- `setmfact()` rejects values outside [0.05, 0.95]; absolute mode uses
  `arg->f - 1.0` which can yield >0.95 and be rejected
- `drawbar()` accesses `drw->fonts->h` at line 605, before the `showbar` and
  `bar_dirty_segments` early-returns — the global `drw` must have a valid
  font chain even for bar-drawing callers
- LSP errors about `GC` redefinition and missing `ft2build.h` are false
  positives from clangd seeing system X11 headers instead of mock redirects;
  actual compilation with `c99 -I include` succeeds
  `tests/include/X11/Xlib.h`, `mock_x11.h:26`)

### Adding a New Test

1. Create `tests/test_<feature>.c` following the existing pattern
2. The Makefile's `$(wildcard test_*.c)` picks it up automatically
3. Run `make` to verify it compiles
4. Add `cov_test_<feature>` to the `coverage` make target

### Pre-existing Notices

- `config.def.h` defines `termcmd[]` but it is never used (warning suppressed)
- `dwm.c:298,302,304` have signedness warnings in `buttonpress()` (X int v.
  unsigned tag index)
- `dwm.c:422-423` have signedness warnings in `clientmessage()` (long v.
  Atom (unsigned long))
- `dwm.c:892,1564,1681,1686,1964` have additional signedness warnings in
  grabkeys, setup, tile, updatenumlockmask

## Commit & Pull Request Guidelines

Recent history uses short, imperative subjects such as `remove binary`,
`optimize keypress`, and `README: remove`. Keep commit subjects concise and
focused, optionally prefixing the touched area (`config: ...`, `README: ...`).
Pull requests should describe the user-visible behavior change, list build
verification, mention any manual X-session testing, and call out updates to
`config.def.h`, patches, or installation defaults.

### IMPORTANT: Commit before switching branches

**Always commit all working-tree changes before checking out into another
branch.** Untracked files (new test files, docs, etc.) are invisible to git
until staged, and `git clean -fd` (often needed after a branch switch)
permanently deletes them.  Committing first ensures nothing is lost: use
`git add -A && git commit -m "<message>"` or `git stash` (which also saves
untracked files with `--include-untracked`).  Never rely on the working tree
being preserved across checkout when new files are present.

## Performance Optimizations

Five phases of drawbar optimization have been implemented; reference
`PERF-PROFILE.md`, `OPTIMIZE-STATUS.md`, and `OPTIMIZE-FULLSCREEN.md` for profiling data and remaining work.

### Phase 1: Glyph-Width Cache (`drw.c`)

`drw_fontset_getwidth()` uses a per-codepoint cache for non-ASCII (emoji)
glyphs (`drw.c:20-55`). The cache is a fixed-size open-addressing hash table
(`glyph_cache`, 64 entries) keyed by Unicode codepoint with linear probing.
`drw_fontset_invalidate_cache()` clears all entries on font change.

ASCII characters (< 0x7F) skip the cache and call `drw_font_getexts()` directly
since they have no emoji/PNG overhead. Non-ASCII goes through `glyph_getwidth()`
which checks the cache first, then calls `XftCharExists()` + `drw_font_getexts()`
on cache miss.

This avoids repeated `XftGlyphExtents` → `FT_Load_Glyph` → `png_read_*` →
`inflate` for emoji codepoints that appear in multiple bar redraws (tags, status
text, title text).

Cache correctness is tested in `tests/test_drw_cache.c` (25 tests) which
replicates the cache logic with minimal stubs (no X server required).

### Phase 2: Bar Dirty Tracking (`dwm.c`/`dwm.h`)

A `bar_dirty` flag (`dwm.h:257`) tracks whether bar content has actually
changed. `drawbar()` checks it early (`dwm.c:610`): if clean, it copies
the retained pixmap to the window via `drw_map()` and returns without
re-rendering text. After a full draw, `bar_dirty` is reset to 0.

This avoids the expensive Xft/FreeType/PNG-decompression path on every
`Expose`, `ConfigureNotify`, or geometry-only `restack` call. The pixmap
copy is negligible compared to glyph rendering.

### Phase 3: Segment-Level Dirty Tracking (`dwm.c`/`dwm.h`)

Phase 2's single `bar_dirty` flag was refined into a three-bit `bar_dirty_segments`
mask (`dwm.h:258-263`) with per-segment flags `DIRTY_STATUS`, `DIRTY_TAGS`,
`DIRTY_TITLE`. `drawbar()` draws only the dirty segments instead of the entire bar,
and the `!bar_dirty_segments` early-return applies to all monitors (not just selmon).

Content-change call sites set only the relevant segment bits:
- `focus()` → `DIRTY_TITLE | DIRTY_TAGS` — selected client changed
- `updatestatus()` → `DIRTY_STATUS | DIRTY_TITLE` — status text updated
- `propertynotify()` WM_HINTS → `DIRTY_TAGS` — urgency changed
- `propertynotify()` WM_NAME → `DIRTY_TITLE` — title changed (selected client only)
- `setlayout()` → `DIRTY_TAGS` — layout symbol changed
- `togglebar()` → `DIRTY_STATUS | DIRTY_TAGS | DIRTY_TITLE` — bar shown/hidden
- `swallow()` → `DIRTY_TITLE` — swallowed client title changed

`DIRTY_TITLE` is always set alongside `DIRTY_STATUS` because status text width
changes shift the title area boundary, requiring a title redraw. Tags-only and
title-only changes avoid the expensive emoji/status draw entirely.

### Phase 4: Fullscreen Bar Freeze (`config.h`/`dwm.c`)

`optimizefullscreen` (`config.def.h:93`, default 1) guards a check at the top
of `drawbar()` (`dwm.c:610-612`): when a fullscreen window is focused, the bar
is not drawn at all — no pixmap copy, no font work, no emoji. The bar freezes
at its pre-fullscreen state and resumes when the window exits fullscreen.

**What dwm still does during fullscreen:**
- Runs the event loop (`XNextEvent` dispatches normally)
- Handles client messages, configure, map, unmap, destroy for all windows
- Manages focus (if `lockfullscreen = 0`, user can refocus other windows)
- Processes `_NET_WM_STATE` for fullscreen toggle
- Runs `updatestatus()`, `focus()`, `propertynotify()` — these call `drawbar()`
  but it returns immediately at the `isfullscreen` check

**What dwm skips during fullscreen (`drawbar()` early-return):**
- `drw_text()` → XftDrawStringUtf8 → FreeType glyph rendering → emoji PNG decompress
- `drw_map()` → XCopyArea + XSync pixmap copy to bar window
- All per-segment drawing (status, tags, title, layout symbol, urgency rects)
- `XftFontLoadGlyphs`, `inflate`, `png_read_*` for status text

**What happens on un-fullscreen:**
- `setfullscreen(c, 0)` calls `arrange()` → `restack()` → `drawbar()`
- `isfullscreen` is now 0, so the guard passes
- `bar_dirty_segments` is 0 initially (never modified during fullscreen), so
  the `!bar_dirty_segments` early-return would normally skip the draw... but
  the pixmap is stale from before fullscreen. To force a fresh draw, `setfullscreen()`
  should set `bar_dirty_segments = DIRTY_ALL`. See `OPTIMIZE-FULLSCREEN.md` for
  this refinement.

**Config interaction:**
- `optimizefullscreen = 1` (default): freeze bar during fullscreen, zero CPU
- `optimizefullscreen = 0`: stock behavior — bar draws on top of fullscreen content
- `lockfullscreen = 1` (default): prevents focus cycling away from fullscreen window
- Both are independent knobs in `config.h`/`config.def.h`

### Phase 5: Emoji Render Cache (`drw.c`)

An `emoji_cache` in the `Drw` struct (`drw.h:39-42`, `drw.c:56-105`)
caches the rendered pixmap for each unique color emoji codepoint in a
32-entry open-addressing hash table with linear probing. On first
encounter in `drw_text()` (`drw.c:447-491`), the emoji is rendered
normally through `XftDrawStringUtf8` (which triggers
`XftFontLoadGlyphs` → `FT_Load_Glyph` → `png_read_*` → `inflate`)
and the resulting pixels are captured to a per-entry `XImage` via
`XGetImage`. On subsequent draws, `XPutImage` copies the cached
pixmap directly, bypassing the entire font-loading and PNG decompress
pipeline.

This avoids repeated `XftFontLoadGlyphs` + `inflate` for emoji
codepoints that appear in every drawbar redraw (status text). In
production with a 5-emoji status bar, perf shows:
- Samples: 87 → 14 (**−84%**)
- Cycles: 21.5M → 1.7M (**−92%**)
- `XftFontLoadGlyphs`: 0% (was 27.8%)
- `inflate`: 0% (was 30.3%)

Cache invalidation happens on font change (`drw_fontset_create`) and
drw destruction (`drw_free`). `XImage` pixmaps are freed during
invalidation. The cache uses `emojicachehash()` as a non-cryptographic
hash of the codepoint; eviction replaces the first colliding entry in
the probed slot.

Cache correctness is tested in `tests/test_drw_cache.c` alongside the
Phase-1 glyph-width cache tests (same file, distinct test groups).
