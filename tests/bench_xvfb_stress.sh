#!/bin/sh
# dwm stability stress test
# Runs dwm under Xvfb with aggressive scripted workloads.
# Usage: bench_xvfb_stress.sh [-v|--valgrind] [dwm-binary] [label]

set -e

# --- Config ---------------------------------------------------------------
DWM_BIN="${2:-$(dirname "$0")/../dwm}"
LABEL="${3:-stress}"
USE_VALGRIND=0
PASS=0
FAIL=0

# Parse flags
if [ "$1" = "-v" ] || [ "$1" = "--valgrind" ]; then
	USE_VALGRIND=1
fi

# Validate dwm binary
if [ ! -x "$DWM_BIN" ]; then
	echo "FAIL: dwm binary not found at $DWM_BIN"
	exit 1
fi

# Check dependencies
for cmd in xdotool xsetroot timeout Xvfb; do
	if ! command -v "$cmd" >/dev/null 2>&1; then
		echo "SKIP: $cmd not found"
		exit 0
	fi
done

# --- Helpers --------------------------------------------------------------

TD="/tmp/dwm-stress-$$"
mkdir -p "$TD"
trap 'kill -9 "$XVFB_PID" "$DWM_PID" 2>/dev/null; rm -rf "$TD"' EXIT INT TERM

start_xvfb() {
	killall -9 Xvfb 2>/dev/null || true
	sleep 0.2
	Xvfb :99 -screen 0 1280x1024x24 +extension RENDER &>/dev/null &
	XVFB_PID=$!
	sleep 1
	export DISPLAY=:99
}

start_dwm() {
	if [ "$USE_VALGRIND" -eq 1 ]; then
		valgrind --leak-check=full --show-leak-kinds=all \
			--track-origins=yes --log-file="$TD/valgrind.log" \
			"$DWM_BIN" &>/dev/null &
	else
		"$DWM_BIN" &>/dev/null &
	fi
	DWM_PID=$!
	sleep 0.5

	if ! kill -0 "$DWM_PID" 2>/dev/null; then
		echo "FAIL: dwm crashed on startup"
		exit 1
	fi
}

phase() {
	echo "  PHASE: $*"
}

report() {
	name="$1"
	result="$2"
	if [ "$result" -eq 0 ]; then
		echo "    PASS  $name"
		PASS=$((PASS + 1))
	else
		echo "    FAIL  $name ($result)"
		FAIL=$((FAIL + 1))
	fi
}

check_dwm_alive() {
	if ! kill -0 "$DWM_PID" 2>/dev/null; then
		echo "    CRASH: dwm died during test"
		return 1
	fi
	return 0
}

# ==========================================================================
# Main
# ==========================================================================

echo "=== dwm stability stress test ==="
echo "Binary: $DWM_BIN"
echo "Valgrind: $USE_VALGRIND"
echo ""

start_xvfb
echo "  Xvfb started (PID $XVFB_PID)"

start_dwm
echo "  dwm started (PID $DWM_PID)"
echo ""

# -------------------------------------------------------------------------
# Phase A: All non-ASCII status flood (Unicode U+0080 to U+10FFFF)
# -------------------------------------------------------------------------
phase "A: All non-ASCII status flood (1.1M codepoints)"
A_FAIL=0
python3 -c '
import subprocess
# Generate all valid Unicode non-ASCII characters (excluding surrogates)
chars = [chr(i) for i in range(0x80, 0x110000) if not (0xD800 <= i <= 0xDFFF)]
chunk_size = 200
for i in range(0, len(chars), chunk_size):
    chunk = "".join(chars[i:i+chunk_size])
    subprocess.run(["xsetroot", "-name", chunk], stderr=subprocess.DEVNULL)
' || true
check_dwm_alive || A_FAIL=1
report "all non-ASCII status flood" "$A_FAIL"

# -------------------------------------------------------------------------
# Phase B: Window storm (create/destroy 50 windows)
# -------------------------------------------------------------------------
phase "B: Window storm (50 windows)"
B_FAIL=0
for i in $(seq 1 50); do
	timeout 0.3 xev -geometry 200x100+$((i*10))+$((i*5)) &>/dev/null &
	if [ $((i % 10)) -eq 0 ]; then
		sleep 0.05
		check_dwm_alive || { B_FAIL=1; break; }
	fi
done
sleep 0.5
# Clean up any lingering xev processes
killall xev 2>/dev/null || true
sleep 0.3
check_dwm_alive || B_FAIL=1
report "window storm" "$B_FAIL"

# -------------------------------------------------------------------------
# Phase C: Fullscreen toggle hammer (100 toggles)
# -------------------------------------------------------------------------
phase "C: Fullscreen toggle hammer (100 toggles)"
C_FAIL=0
for i in $(seq 1 100); do
	xdotool key --window root alt+f 2>/dev/null || true
	if [ $((i % 20)) -eq 0 ]; then
		sleep 0.01
		check_dwm_alive || { C_FAIL=1; break; }
	fi
done
report "fullscreen toggle" "$C_FAIL"

# -------------------------------------------------------------------------
# Phase D: Tag-switching race + window creation
# -------------------------------------------------------------------------
phase "D: Tag-switching race (9 tags x 5 windows)"
D_FAIL=0
for tag in 1 2 3 4 5 6 7 8 9; do
	xdotool key --window root "alt+$tag" 2>/dev/null || true
	sleep 0.02
	# Launch a window on this tag
	timeout 0.3 xev -geometry 200x100 &>/dev/null &
	sleep 0.05
	check_dwm_alive || { D_FAIL=1; break; }
done
xdotool key --window root alt+1 2>/dev/null || true
sleep 0.3
killall xev 2>/dev/null || true
report "tag-switching race" "$D_FAIL"

# -------------------------------------------------------------------------
# Phase E: Togglebar spam + status updates
# -------------------------------------------------------------------------
phase "E: Togglebar spam (50 cycles)"
E_FAIL=0
for i in $(seq 1 50); do
	xdotool key --window root alt+b 2>/dev/null || true
	xsetroot -name "toggle $i ⚡🔥✨" 2>/dev/null || true
	if [ $((i % 10)) -eq 0 ]; then
		check_dwm_alive || { E_FAIL=1; break; }
	fi
done
# Ensure bar is visible
xdotool key --window root alt+b 2>/dev/null || true
if [ $((i % 2)) -eq 1 ]; then
	xdotool key --window root alt+b 2>/dev/null || true
fi
sleep 0.2
report "togglebar spam" "$E_FAIL"

# -------------------------------------------------------------------------
# Phase F: Rapid keybinding hammer
# -------------------------------------------------------------------------
phase "F: Keybinding hammer (200 random keys)"
F_FAIL=0
KEYS="alt+shift+Return alt+shift+space alt+j alt+l alt+h alt+k alt+m alt+ctrl+space"
for i in $(seq 1 200); do
	for key in $KEYS; do
		xdotool key --window root "$key" 2>/dev/null || true
	done
	if [ $((i % 50)) -eq 0 ]; then
		check_dwm_alive || { F_FAIL=1; break; }
	fi
done
report "keybinding hammer" "$F_FAIL"

# -------------------------------------------------------------------------
# Final check
# -------------------------------------------------------------------------
echo ""

# Kill dwm (SIGKILL — no SIGTERM handler, so SIGKILL is needed)
kill -9 "$DWM_PID" 2>/dev/null || true
wait "$DWM_PID" 2>/dev/null || true

# Kill Xvfb
kill -9 "$XVFB_PID" 2>/dev/null || true
wait "$XVFB_PID" 2>/dev/null || true

# Check Valgrind log for errors
VALGRIND_OK=0
if [ "$USE_VALGRIND" -eq 1 ]; then
	phase "Valgrind leak check"
	if [ -f "$TD/valgrind.log" ]; then
		LEAKS=$(grep -c "definitely lost" "$TD/valgrind.log" || true)
		if [ "$LEAKS" -gt 0 ]; then
			echo "    FAIL: $LEAKS definite leaks"
			grep "definitely lost" "$TD/valgrind.log"
			VALGRIND_OK=1
		else
			echo "    PASS: no definite leaks"
		fi
		# Show summary
		grep -E "in use at exit|total heap usage|ERROR SUMMARY" "$TD/valgrind.log" || true
	else
		echo "    FAIL: no valgrind log"
		VALGRIND_OK=1
	fi
fi

echo ""
echo "=== RESULTS ==="
echo "Passed: $PASS | Failed: $FAIL"
if [ "$USE_VALGRIND" -eq 1 ]; then
	echo "Valgrind leaks: $VALGRIND_OK"
fi
[ "$FAIL" -eq 0 ] && [ "$VALGRIND_OK" -eq 0 ] && echo "All tests passed." || echo "Some tests failed."
exit $((FAIL + VALGRIND_OK))
