#!/usr/bin/env bash
# Clang integer sanitizer across every unit suite.
#
# UBSan's integer group flags intentional wraparound in the hash functions
# (keyset_home, winhash_home) and in the xorshift test PRNG (fuzz_rand), so
# diagnostics from those frames are allowlisted by function name. Anything
# else is a real finding and fails the script.
#
# Artifacts/logs: tests/build/integer-sanitizer/, tests/build/logs/
set -uo pipefail
cd "$(dirname "$0")"

if ! command -v clang >/dev/null 2>&1; then
	echo "SKIP: clang not installed"
	exit 0
fi

dir=build/integer-sanitizer
logdir=build/logs
mkdir -p "$logdir"

echo "==> building integer-sanitized binaries -> $dir"
if ! make -j"${JOBS:-$(nproc 2>/dev/null || echo 4)}" integer-bins >"$logdir/integer-build.log" 2>&1; then
	echo "FAIL: integer sanitizer build (see $logdir/integer-build.log)" >&2
	tail -40 "$logdir/integer-build.log" >&2
	exit 1
fi

log="$logdir/integer-sanitizer.log"
runlogs="$logdir/integer-runs"
: >"$log"
status=0
if UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=0 \
	sh parallel-run.sh "${SAN_JOBS:-${JOBS:-$(nproc 2>/dev/null || echo 4)}}" "$runlogs" "$dir"/*; then
	: # every suite exited 0
else
	status=1
fi
cat "$runlogs"/*.log >"$log" 2>/dev/null || true

python3 - "$log" <<'PY'
import re
import sys

path = sys.argv[1]
with open(path, errors="replace") as f:
    lines = f.read().splitlines()

blocks = []
cur = []
for line in lines:
    if "runtime error:" in line:
        if cur:
            blocks.append(cur)
        cur = [line]
    elif cur:
        if not line.strip() or line.startswith("SUMMARY:") or re.match(r"^==\d+==", line):
            blocks.append(cur)
            cur = []
        else:
            cur.append(line)
if cur:
    blocks.append(cur)

# Intentional wraparound lives in the hash functions and in util.h's
# character-class macros, which use unsigned subtraction for range checks.
# Test sources deliberately exercise overflow in PRNGs and replicas.
allow = ("keyset_home", "winhash_home", "fuzz_rand")
unexpected = []
for block in blocks:
    loc = block[0].split(": runtime error:")[0]
    if "unit/src/" in loc or "/util.h" in loc or "contradictions/" in loc:
        continue
    if any(any(name in line for name in allow) for line in block):
        continue
    unexpected.append("\n".join(block))

print(f"integer sanitizer: {len(blocks)} diagnostics, "
      f"{len(blocks) - len(unexpected)} intentional, {len(unexpected)} unexpected")
for block in unexpected[:10]:
    print(block, file=sys.stderr)
sys.exit(1 if unexpected else 0)
PY
py_status=$?
[ "$py_status" -ne 0 ] && status=1

if [ "$status" -eq 0 ]; then
	echo "PASS: integer sanitizer"
else
	echo "FAIL: integer sanitizer (see $log)" >&2
fi
exit "$status"
