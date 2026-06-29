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

## Testing Guidelines

There is no automated test suite in this tree. Use `make clean && make` as the
baseline validation for every change. For window-management behavior, test in a
nested or disposable X session when possible, for example with Xephyr, then run
the built `./dwm` inside that display. Exercise changed keybindings, layouts,
tag behavior, fullscreen handling, and restart/swallow paths as applicable.

## Commit & Pull Request Guidelines

Recent history uses short, imperative subjects such as `remove binary`,
`optimize keypress`, and `README: remove`. Keep commit subjects concise and
focused, optionally prefixing the touched area (`config: ...`, `README: ...`).
Pull requests should describe the user-visible behavior change, list build
verification, mention any manual X-session testing, and call out updates to
`config.def.h`, patches, or installation defaults.

## Performance Optimizations

Four phases of drawbar optimization have been implemented; reference
`PERF-PROFILE.md`, `OPTIMIZE-STATUS.md`, and `OPTIMIZE-FULLSCREEN.md` for profiling data and remaining work.

### Phase 1: Text Extent Cache (`drw.c`/`drw.h`)

`drw_fontset_getwidth()` now caches computed text widths in a fixed-size
`ExtentCache` array (`drw.c:14-58`). The cache is keyed by string only — no
font hash is needed because `drw_fontset_invalidate_cache()` is called on every
font change (`drw_fontset_create()`, `drw_free()`), ensuring all cache entries
always belong to the current font.

This eliminates repeated `XftTextExtentsUtf8` → `XftGlyphExtents` →
`XftFontLoadGlyphs` → `FT_Load_Glyph` → `png_read_*` → `inflate` calls for
the same strings (tags, layout symbols, status text) across bar redraws.

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
