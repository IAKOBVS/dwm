#!/bin/sh
# Bounded-parallel test runner.
#
# Usage: parallel-run.sh JOBS LOGDIR BIN...
# Runs each binary with at most JOBS in flight, captures per-binary output to
# LOGDIR/<name>.log, and exits non-zero if any suite failed. Each binary is
# executed with the caller's environment (ASAN_OPTIONS, UBSAN_OPTIONS, etc.).
set -u

jobs=$1
logdir=$2
shift 2

[ -n "${jobs:-}" ] || jobs=$(nproc 2>/dev/null || echo 4)
[ -n "${logdir:-}" ] || logdir=/tmp/dwm-parallel-logs
mkdir -p "$logdir"

# Ignore build artifacts (e.g. mock_x11.o) when callers pass a directory glob.
bins=""
for f in "$@"; do
	[ -x "$f" ] && bins="$bins $f"
done
set -- $bins

status_dir=$(mktemp -d "${TMPDIR:-/tmp}/dwm-status.XXXXXX") || exit 1
trap 'rm -rf "$status_dir"' EXIT HUP INT TERM

run_one='f="$1"; base=$(basename "$f"); \
if "$f" >"$LOGDIR/$base.log" 2>&1; then \
	printf "PASS\n" >"$STATUSDIR/$base"; \
else \
	printf "FAIL %s\n" "$?" >"$STATUSDIR/$base"; \
fi'

export LOGDIR="$logdir" STATUSDIR="$status_dir"
printf '%s\n' "$@" | xargs -r -P "$jobs" -I{} sh -c "$run_one" sh {}

pass=0
fail=0
for f in "$@"; do
	base=$(basename "$f")
	status=$(cat "$status_dir/$base" 2>/dev/null || printf 'MISSING\n')
	case "$status" in
	PASS*)
		pass=$((pass + 1))
		printf '  PASS  %s\n' "$base"
		;;
	*)
		fail=$((fail + 1))
		printf '  FAIL  %s (%s)\n' "$base" "$status"
		sed 's/^/        /' "$logdir/$base.log" 2>/dev/null | tail -20
		;;
	esac
done

printf '%s\n' "---"
printf 'Total: %s | Pass: %s | Fail: %s\n' "$((pass + fail))" "$pass" "$fail"
[ "$fail" -eq 0 ]
