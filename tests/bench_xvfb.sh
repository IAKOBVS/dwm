#!/bin/sh
# Benchmark dwm under Xvfb - measures user CPU, system CPU, wall time
# Usage: bench_xvfb.sh <dwm-binary> <label>

DWM_BIN="$1"
LABEL="$2"

killall -9 Xvfb 2>/dev/null
sleep 0.3

Xvfb :99 -screen 0 1280x1024x24 &>/dev/null &
XVFB_PID=$!
sleep 1

export DISPLAY=:99
export TIMEFMT="${LABEL}: user %U sys %S real %E"

# Run dwm under time + timeout
{ time timeout -s KILL 3 "$DWM_BIN" &>/dev/null; } 2>&1 &
DWM_PID=$!
sleep 0.5

# Workload phase 1: 20 rapid emoji-heavy status updates
for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do
    xsetroot -name "status $i ⚡🔥💯✨🚀📦 testing emoji rendering path 1234567890" 2>/dev/null
    sleep 0.01
done

# Workload phase 2: 10 tag switches + togglebar cycles
for i in 1 2 3 4 5 6 7 8 9 10; do
    xdotool key --window root alt+b 2>/dev/null
    xdotool key --window root alt+b 2>/dev/null
    xdotool key --window root alt+2 2>/dev/null
    xdotool key --window root alt+1 2>/dev/null
    sleep 0.01
done

# Workload phase 3: Create 3 windows
for i in 1 2 3; do
    xclock -d -update 1 &>/dev/null &
    sleep 0.1
done
sleep 0.5

# Fullscreen toggle
xdotool key --window root alt+f 2>/dev/null
sleep 0.1
xdotool key --window root alt+f 2>/dev/null
sleep 0.1

# Clean up windows
killall xclock 2>/dev/null

# Wait for dwm to finish
wait "$DWM_PID" 2>/dev/null

kill "$XVFB_PID" 2>/dev/null
wait "$XVFB_PID" 2>/dev/null
