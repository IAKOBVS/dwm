#!/bin/sh
# dwm test runner
# Each test is a shell function writing PASS/FAIL to $td/result.
# Tests are registered in the TESTS heredoc at the bottom.
# Uses batch-wait jobserver (MAX_JOBS=8) to limit concurrency.

set -e

cd "$(dirname "$0")"
PROG="$(pwd)/dwm"
TD_ROOT="/tmp/dwm-test-$$"
MAX_JOBS=8
PASS=0
FAIL=0

trap 'rm -rf "$TD_ROOT"' EXIT INT TERM
mkdir -p "$TD_ROOT"

build_tests() {
	make -s clean 2>/dev/null
	make -s 2>&1
}

# Test: basic pure-logic tests compile and produce PASS
t_build_basic() {
	td=$1
	if [ -x test_pure_logic ]; then
		echo "PASS" > "$td/result"
	else
		echo "FAIL: test_pure_logic not built" > "$td/result"
	fi
}

# Test: segment dirty tracking tests compile and produce PASS
t_build_segments() {
	td=$1
	if [ -x test_segments ]; then
		echo "PASS" > "$td/result"
	else
		echo "FAIL: test_segments not built" > "$td/result"
	fi
}

# Test: run pure logic tests
t_run_pure_logic() {
	td=$1
	output=$(./test_pure_logic 2>&1)
	exit_code=$?
	if [ "$exit_code" -eq 0 ]; then
		echo "PASS" > "$td/result"
	else
		echo "FAIL: $output" > "$td/result"
	fi
}

# Test: run segment tests
t_run_segments() {
	td=$1
	output=$(./test_segments 2>&1)
	exit_code=$?
	if [ "$exit_code" -eq 0 ]; then
		echo "PASS" > "$td/result"
	else
		echo "FAIL: $output" > "$td/result"
	fi
}

# Register tests in TESTS heredoc
TESTS="$(cat <<'EOF'
t_build_basic
t_build_segments
t_run_pure_logic
t_run_segments
EOF
)"

# Main runner
build_tests

count=0
for t in $TESTS; do
	(
		mkdir -p "$TD_ROOT/$t"
		"$t" "$TD_ROOT/$t"
	) &
	count=$((count + 1))
	if [ "$count" -ge "$MAX_JOBS" ]; then
		wait
		count=0
	fi
done
[ "$count" -gt 0 ] && wait

# Report results
echo ""
echo "=== RESULTS ==="
for t in $TESTS; do
	r=$(cat "$TD_ROOT/$t/result" 2>/dev/null || echo "FAIL: no result")
	case "$r" in
		PASS*) PASS=$((PASS + 1)); echo "  PASS  $t" ;;
		*)     FAIL=$((FAIL + 1)); echo "  FAIL  $t ($r)" ;;
	esac
done
echo "---"
echo "Total: $((PASS + FAIL)) | Pass: $PASS | Fail: $FAIL"
[ "$FAIL" -eq 0 ] && echo "All tests passed." || echo "Some tests failed."
exit $FAIL
