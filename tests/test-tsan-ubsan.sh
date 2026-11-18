#!/usr/bin/env bash
# Build and run every suite under TSan+UBSan.
# Artifacts: tests/build/tsan-ubsan/   Log: tests/build/logs/tsan-ubsan.log
set -euo pipefail
cd "$(dirname "$0")"

logdir=build/logs
mkdir -p "$logdir"
log="$logdir/tsan-ubsan.log"

echo "==> TSan+UBSan (all suites) -> $log"
if make tsan-ubsan 2>&1 | tee "$log"; then
	echo "PASS: TSan+UBSan"
else
	echo "FAIL: TSan+UBSan (see $log)" >&2
	exit 1
fi
