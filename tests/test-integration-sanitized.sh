#!/usr/bin/env bash
# Run only tests/unit/src/integration/* under both sanitizer configurations.
# Artifacts: tests/build/{asan,tsan}-ubsan/
# Logs:      tests/build/logs/{asan,tsan}-ubsan-integration.log
set -euo pipefail
cd "$(dirname "$0")"

logdir=build/logs
mkdir -p "$logdir"
status=0

for cfg in asan-ubsan tsan-ubsan; do
	log="$logdir/$cfg-integration.log"
	echo "==> $cfg integration suites -> $log"
	if make "$cfg-integration" 2>&1 | tee "$log"; then
		echo "PASS: $cfg integration"
	else
		echo "FAIL: $cfg integration (see $log)" >&2
		status=1
	fi
done

exit "$status"
