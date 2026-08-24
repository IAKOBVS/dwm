#!/usr/bin/env python3
"""Merge gcov output from multiple test binaries to produce combined dwm.c coverage.

Usage: python3 merge_gcov.py <test-source-dir> <test-binary-labels...>

Each binary label corresponds to a cov_test_<label> binary. The script:
1. Runs each binary
2. Runs gcov on dlass source
3. Merges all gcov outputs
4. Prints combined coverage summary
"""
import subprocess, sys, os, re, tempfile, shutil

SOURCES = ["../dwm.c", "../drw.c", "../util.c"]

def merge_gcovs(test_dir, test_labels):
    orig_dir = os.getcwd()
    os.chdir(test_dir)

    # Coverage data per source: line_no -> max count across all tests
    combined = {src: {} for src in SOURCES}
    src_lines = {src: {} for src in SOURCES}

    # Read source files to get total executable lines
    for src in SOURCES:
        src_path = os.path.join(test_dir, src)
        if os.path.exists(src_path):
            with open(src_path) as f:
                for i, line in enumerate(f, 1):
                    src_lines[src][i] = line

    tmpdir = tempfile.mkdtemp(prefix='cov_merge_')
    
    for label in test_labels:
        binary = f"cov_test_{label}"
        if not os.path.exists(binary):
            print(f"  SKIP {binary} (not found)")
            continue
        
        print(f"  RUN   {binary}")
        result = subprocess.run([f'./{binary}'], capture_output=True, text=True)
        if result.returncode != 0:
            print(f"  FAIL  {binary} (exit {result.returncode})")
            print(result.stderr[:500])
            continue
        
        # Find the gcda file
        import glob
        gcda_files = glob.glob(f"{binary}-*.gcda")
        if not gcda_files:
            print(f"  NOGC  {binary} (no gcda)")
            continue
        gcda = gcda_files[0]
        
        # Run gcov once per tracked source
        for src in SOURCES:
            result = subprocess.run(
                ['gcov', '-o', gcda, src],
                capture_output=True, text=True
            )
        
        # Parse each <src>.gcov
        # gcov format (lines with count):
        #   executed:  "        N: LLLL:source"   (8 spaces, count, colon, 1+ spaces, line, colon)
        #   uncovered: "    #####: LLLL:source"   (4 spaces, #####, colon, 1+ spaces, line, colon)
        #   non-exec:  "        -: LLLL:source"   (8 spaces, -, colon, 1+ spaces, line, colon)
        for src in SOURCES:
            gcov_file = os.path.join(
                test_dir, os.path.basename(src) + '.gcov')
            if not os.path.exists(gcov_file):
                continue
            table = combined[src]
            with open(gcov_file) as f:
                for line in f:
                    # Uncovered: "    #####:\s+\d+:"
                    m = re.match(r'^    #####:\s+(\d+):', line)
                    if m:
                        lineno = int(m.group(1))
                        if lineno not in table:
                            table[lineno] = 0
                        continue
                    # Covered: counter width varies with magnitude
                    m = re.match(r'^\s*(\d+):\s+(\d+):', line)
                    if m:
                        lineno = int(m.group(2))
                        count = int(m.group(1))
                        if lineno in table:
                            table[lineno] = max(table[lineno], count)
                        else:
                            table[lineno] = count
                        continue
            os.remove(gcov_file)
        # gcov regenerates sibling <src>.gcov files on every invocation;
        # drop them so the next source/binary parses its own fresh output
        for src in SOURCES:
            side = os.path.join(test_dir, os.path.basename(src) + '.gcov')
            if os.path.exists(side):
                os.remove(side)
    
    os.chdir(orig_dir)

    # Compute and print coverage stats per source
    for src in SOURCES:
        table = combined[src]
        total_executable = len(table)
        covered = sum(1 for c in table.values() if c > 0)
        uncovered_lines = [ln for ln, c in table.items() if c == 0]

        name = os.path.basename(src)
        print(f"\n=== Combined {name} coverage ===")
        print(f"  Executable lines: {total_executable}")
        print(f"  Covered:          {covered}")
        print(f"  Uncovered:        {len(uncovered_lines)}")
        if total_executable > 0:
            print(f"  Coverage:         {covered / total_executable * 100:.1f}%")

        if uncovered_lines:
            print(f"\n  Uncovered lines:")
            for ln in sorted(uncovered_lines):
                line_text = src_lines[src].get(ln, '').rstrip()
                print(f"    {ln}: {line_text[:80]}")

    return combined

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <test-dir> <label1> [label2 ...]")
        print(f"  Each label is the suffix after 'cov_test_'")
        sys.exit(1)
    
    test_dir = sys.argv[1]
    labels = sys.argv[2:]
    merge_gcovs(test_dir, labels)
