# TODO

## Potential Optimizations

- Skip unnecessary root pointer work when dwm is effectively idle. `motionnotify()` is only useful for monitor switching on multi-monitor setups; keep the single-monitor fast path and consider disabling `PointerMotionMask` on the root window when pointer-driven monitor selection is not needed.

- Add a gaming/low-overhead mode. A runtime flag could suppress nonessential work while a fullscreen game is focused, such as root motion monitor switching, extra bar redraws, and status-triggered visual updates.

- Avoid redundant bar redraws. Audit `drawbars()` and `drawbar()` callers so focus, layout, and status changes only redraw when visible bar state actually changed.

- Avoid redundant arrange calls. Check paths that call `arrange()` after tag, fullscreen, swallow, unmanage, or floating changes, and skip arrange when client geometry or layout state is unchanged.

- Keep button and key fast paths simple. Button presses now use startup-derived button and mask caches before scanning `buttons[]`; keypress already filters on modifier state before scanning `keys[]`. Prefer exact low-cost prechecks over larger dispatch tables unless profiling shows real overhead.

- Consider reducing status update churn. External status scripts can wake dwm frequently through root window name changes; avoid high-frequency status updates during games or fullscreen sessions.

- Profile before adding complexity. Use `perf top`, `perf record`, or targeted logging during normal use and gaming to confirm whether time is spent in event handling, bar drawing, arranging, or X calls.
