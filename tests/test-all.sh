#!/usr/bin/env bash
# Run the full test matrix:
#   1. plain unit tests              (make test)
#   2. all suites under ASan+UBSan   (test-asan-ubsan.sh)
#   3. all suites under TSan+UBSan   (test-tsan-ubsan.sh)
#   4. integration suites sanitized  (test-integration-sanitized.sh)
# Logs: tests/build/logs/
set -uo pipefail
cd "$(dirname "$0")"

JOBS=${JOBS:-$(nproc 2>/dev/null || echo 4)}
SAN_JOBS=${SAN_JOBS:-$JOBS}
export JOBS SAN_JOBS

status=0

run() {
	name="$1"
	shift
	echo
	echo "########## $name ##########"
	if "$@"; then
		echo "PASS: $name"
	else
		echo "FAIL: $name" >&2
		status=1
	fi
}

run "plain unit tests"       make test
run "ASan+UBSan"             ./test-asan-ubsan.sh
run "TSan+UBSan"             ./test-tsan-ubsan.sh
run "integration sanitized"  ./test-integration-sanitized.sh
run "integer sanitizer"      ./test-integer-sanitizer.sh
run "e2e (Xvfb)"             ./test-e2e.sh

echo
if [ "$status" -eq 0 ]; then
	echo "ALL TEST CONFIGURATIONS PASSED"
else
	echo "SOME TEST CONFIGURATIONS FAILED (see tests/build/logs/)" >&2
fi
exit "$status"
