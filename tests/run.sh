#!/bin/sh
# dwm test runner — builds and runs every suite discovered under unit/ and
# contradictions/, with parallelism defaulting to nproc.
set -e

cd "$(dirname "$0")"
JOBS=${JOBS:-$(nproc 2>/dev/null || echo 4)}
export JOBS

TD="/tmp/dwm-run-$$"
trap 'rm -rf "$TD"' EXIT INT TERM
mkdir -p "$TD"

# Discovery must match the Makefile (which also includes contradictions/).
SUITES="$(find unit contradictions -name '*.c' -exec basename {} .c \; | sort)"

make -s clean 2>/dev/null || true
make -s -j"$JOBS"

set -- $(for t in $SUITES; do printf './bin/%s ' "$t"; done)
if sh parallel-run.sh "$JOBS" "$TD/logs" "$@"; then
	echo "All tests passed."
	exit 0
fi
echo "Some tests failed (logs in $TD/logs)."
exit 1
