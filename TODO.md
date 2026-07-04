# TODO

## Potential Optimizations

- Skip unnecessary root pointer work when dwm is effectively idle. `motionnotify()` is only useful for monitor switching on multi-monitor setups; keep the single-monitor fast path and consider disabling `PointerMotionMask` on the root window when pointer-driven monitor selection is not needed.

- Add a gaming/low-overhead mode. A runtime flag could suppress nonessential work while a fullscreen game is focused, such as root motion monitor switching, extra bar redraws, and status-triggered visual updates.

- Avoid redundant bar redraws. Audit `drawbars()` and `drawbar()` callers so focus, layout, and status changes only redraw when visible bar state actually changed.

- Avoid redundant arrange calls. Check paths that call `arrange()` after tag, fullscreen, swallow, unmanage, or floating changes, and skip arrange when client geometry or layout state is unchanged.

- Keep button and key fast paths simple. Button presses now use startup-derived button and mask caches before scanning `buttons[]`; keypress already filters on modifier state before scanning `keys[]`. Prefer exact low-cost prechecks over larger dispatch tables unless profiling shows real overhead.

- Consider reducing status update churn. External status scripts can wake dwm frequently through root window name changes; avoid high-frequency status updates during games or fullscreen sessions.

- Profile before adding complexity. Use `perf top`, `perf record`, or targeted logging during normal use and gaming to confirm whether time is spent in event handling, bar drawing, arranging, or X calls.

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
