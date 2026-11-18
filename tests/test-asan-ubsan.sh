#!/usr/bin/env bash
# Build and run every suite under ASan+UBSan.
# Artifacts: tests/build/asan-ubsan/   Log: tests/build/logs/asan-ubsan.log
set -euo pipefail
cd "$(dirname "$0")"

logdir=build/logs
mkdir -p "$logdir"
log="$logdir/asan-ubsan.log"

echo "==> ASan+UBSan (all suites) -> $log"
if make asan-ubsan 2>&1 | tee "$log"; then
	echo "PASS: ASan+UBSan"
else
	echo "FAIL: ASan+UBSan (see $log)" >&2
	exit 1
fi
