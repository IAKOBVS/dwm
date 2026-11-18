#!/usr/bin/env bash
# Xvfb end-to-end smoke test against an ASan+UBSan build of dwm.
#
# Verifies real X behaviour that the mocks cannot: tiling geometry, tag
# hiding, fullscreen, and the XKillClient fallback. Skips cleanly when Xvfb,
# xdotool, or xwininfo are unavailable.
#
# Artifacts/logs: tests/build/e2e/
set -euo pipefail
cd "$(dirname "$0")"

for tool in Xvfb xdotool xwininfo; do
	if ! command -v "$tool" >/dev/null 2>&1; then
		echo "SKIP: $tool not installed"
		exit 0
	fi
done

dir=build/e2e
mkdir -p "$dir"
CC=${CC:-cc}

echo "==> building sanitized dwm and e2e client"
"$CC" -g -O1 -std=c99 -fsanitize=address,undefined -fno-omit-frame-pointer \
	-I/usr/X11R6/include -I/usr/include/freetype2 \
	-D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=700L \
	'-DVERSION="6.4"' -DXINERAMA \
	../drw.c ../dwm.c ../util.c -o "$dir/dwm" \
	-L/usr/X11R6/lib -lX11 -lXinerama -lfontconfig -lXft -lX11-xcb -lxcb -lxcb-res
"$CC" -O1 -o "$dir/e2e_client" e2e_client.c -lX11

n=99
while [ -e "/tmp/.X$n-lock" ] && [ "$n" -lt 120 ]; do
	n=$((n + 1))
done
export DISPLAY=":$n"

dwm_pid=
c1_pid=
c2_pid=
xvfb_pid=

cleanup() {
	set +e
	[ -n "$c1_pid" ] && kill "$c1_pid" 2>/dev/null
	[ -n "$c2_pid" ] && kill "$c2_pid" 2>/dev/null
	[ -n "$dwm_pid" ] && kill "$dwm_pid" 2>/dev/null
	sleep 0.2
	[ -n "$xvfb_pid" ] && kill "$xvfb_pid" 2>/dev/null
	wait 2>/dev/null
}
trap cleanup EXIT

fail() {
	echo "FAIL: $*" >&2
	[ -f "$dir/dwm.log" ] && tail -40 "$dir/dwm.log" >&2
	exit 1
}

Xvfb "$DISPLAY" -screen 0 1280x800x24 -nolisten tcp >"$dir/xvfb.log" 2>&1 &
xvfb_pid=$!
for _ in $(seq 1 100); do
	xdotool getdisplaygeometry >/dev/null 2>&1 && break
	sleep 0.05
done
xdotool getdisplaygeometry >/dev/null 2>&1 || fail "Xvfb did not start"

ASAN_OPTIONS=detect_leaks=0:abort_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
	"$dir/dwm" >"$dir/dwm.log" 2>&1 &
dwm_pid=$!
sleep 0.7
kill -0 "$dwm_pid" 2>/dev/null || fail "dwm exited during startup"

rm -f "$dir/w1" "$dir/w2"
"$dir/e2e_client" e2e-one "$dir/w1" >/dev/null 2>&1 &
c1_pid=$!
"$dir/e2e_client" e2e-two "$dir/w2" >/dev/null 2>&1 &
c2_pid=$!

wait_wid() {
	for _ in $(seq 1 100); do
		[ -s "$1" ] && return 0
		sleep 0.05
	done
	return 1
}
wait_wid "$dir/w1" || fail "client 1 did not start"
wait_wid "$dir/w2" || fail "client 2 did not start"
wid1=$(cat "$dir/w1")
wid2=$(cat "$dir/w2")
sleep 0.6

geom() {
	xdotool getwindowgeometry --shell "$1" |
		awk -F= '/^X=/{x=$2}/^Y=/{y=$2}/^WIDTH=/{w=$2}/^HEIGHT=/{h=$2}END{print x, y, w, h}'
}

read -r x1 y1 w1 h1 <<<"$(geom "$wid1")"
read -r x2 y2 w2 h2 <<<"$(geom "$wid2")"
[ "${w1:-0}" -gt 0 ] && [ "${w2:-0}" -gt 0 ] || fail "managed clients have no geometry"
# Launch order is not deterministic: identify master by the larger width and
# require the mfact gap (0.55 => ~55/45 split).
if [ "$w1" -ge "$w2" ]; then big=$w1; small=$w2; else big=$w2; small=$w1; fi
[ "$big" -gt "$small" ] || fail "tile produced equal widths ($w1, $w2)"
frac=$((big * 100 / (big + small)))
[ "$frac" -ge 50 ] && [ "$frac" -le 65 ] || fail "master fraction ${frac}% is outside 50-65% ($w1, $w2)"

# Tag 2 has no clients: both windows must be parked off-screen.
xdotool key --clearmodifiers alt+2
sleep 0.5
read -r x1 y1 w1 h1 <<<"$(geom "$wid1")"
read -r x2 y2 w2 h2 <<<"$(geom "$wid2")"
[ "$x1" -lt 0 ] && [ "$x2" -lt 0 ] || fail "tag switch did not hide clients (x1=$x1 x2=$x2)"

# Back to tag 1: both visible again.
xdotool key --clearmodifiers alt+1
sleep 0.5
read -r x1 y1 w1 h1 <<<"$(geom "$wid1")"
read -r x2 y2 w2 h2 <<<"$(geom "$wid2")"
[ "$x1" -ge 0 ] && [ "$x2" -ge 0 ] || fail "tag return did not restore clients (x1=$x1 x2=$x2)"

# Fullscreen the selected client and verify it covers the screen.
xdotool key --clearmodifiers alt+k
sleep 0.2
fw=$(xdotool getwindowfocus)
[ "$fw" = "$wid1" ] || [ "$fw" = "$wid2" ] || fail "focus is not a managed client ($fw)"
xdotool key --clearmodifiers alt+f
sleep 0.5
read -r fx fy fw_w fw_h <<<"$(geom "$fw")"
[ "$fw_w" -eq 1280 ] && [ "$fw_h" -eq 800 ] || fail "fullscreen geometry ${fw_w}x${fw_h} != 1280x800"

xdotool key --clearmodifiers alt+f
sleep 0.5
read -r fx fy fw_w fw_h <<<"$(geom "$fw")"
[ "$fw_w" -ne 1280 ] || [ "$fw_h" -ne 800 ] || fail "fullscreen did not restore tiling"

# The helper does not advertise WM_DELETE_WINDOW, so killclient must use
# XKillClient and the *currently selected* window must disappear. Focus may
# legitimately follow the pointer after leaving fullscreen, so re-query it.
target=$(xdotool getwindowfocus)
[ "$target" = "$wid1" ] || [ "$target" = "$wid2" ] || fail "kill target is not a client ($target)"
xdotool key --clearmodifiers alt+q
sleep 0.5
if xwininfo -id "$target" >/dev/null 2>&1; then
	fail "killclient left window $target alive"
fi

kill -0 "$dwm_pid" 2>/dev/null || fail "dwm exited during the test"
if grep -qE 'ERROR: AddressSanitizer|ERROR: LeakSanitizer|runtime error' "$dir/dwm.log"; then
	fail "sanitizer report in dwm.log"
fi

echo "PASS: Xvfb E2E (tile, tags, fullscreen, killclient)"
