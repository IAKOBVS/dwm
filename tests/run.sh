#!/bin/sh
# dwm test runner — builds and runs every per-function suite discovered
# under unit/src/<module>/ (not a hardcoded subset) and propagates failures.
# Concurrency is capped at MAX_JOBS parallel suites.

set -e

cd "$(dirname "$0")"
MAX_JOBS=8
TD="/tmp/dwm-run-$$"
trap 'rm -rf "$TD"' EXIT INT TERM
mkdir -p "$TD"

# Discover suite labels the same way the Makefile does; binaries are flat
# in bin/ and named after the test source basename.
SUITES="$(find unit -name '*.c' -exec basename {} .c \; | sort)"

make -s clean 2>/dev/null || true
make -s -j"$MAX_JOBS"

run_suite() {
	suite=$1
	out=$TD/$suite.log
	if "./bin/$suite" >"$out" 2>&1; then
		echo PASS >"$TD/$suite.status"
	else
		echo "FAIL: exit $?" >"$TD/$suite.status"
	fi
}

count=0
for t in $SUITES; do
	run_suite "$t" &
	count=$((count + 1))
	if [ "$count" -ge "$MAX_JOBS" ]; then
		wait || true
		count=0
	fi
done
wait || true

PASS=0
FAIL=0
for t in $SUITES; do
	status=$(cat "$TD/$t.status")
	if [ "$status" = "PASS" ]; then
		PASS=$((PASS + 1))
		echo "  PASS  $t"
	else
		FAIL=$((FAIL + 1))
		echo "  FAIL  $t ($status)"
		head -20 "$TD/$t.log" | sed 's/^/        /'
	fi
done
echo "---"
echo "Total: $((PASS + FAIL)) | Pass: $PASS | Fail: $FAIL"
[ "$FAIL" -eq 0 ] && echo "All tests passed." || echo "Some tests failed."
exit "$FAIL"
