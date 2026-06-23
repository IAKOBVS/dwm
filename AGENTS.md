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
