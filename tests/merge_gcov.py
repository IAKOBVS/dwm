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

def merge_gcovs(test_dir, test_labels, src_file="../dwm.c"):
    orig_dir = os.getcwd()
    os.chdir(test_dir)
    
    # Coverage data: line_no -> max count across all tests
    combined = {}
    src_lines = {}
    
    # Read source file to get total executable lines
    src_path = os.path.join(test_dir, src_file)
    if os.path.exists(src_path):
        with open(src_path) as f:
            for i, line in enumerate(f, 1):
                src_lines[i] = line

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
        
        # Run gcov
        result = subprocess.run(
            ['gcov', '-o', gcda, src_file],
            capture_output=True, text=True
        )
        
        # Parse dwm.c.gcov
        # gcov format (lines with count):
        #   executed:  "        N: LLLL:source"   (8 spaces, count, colon, 1+ spaces, line, colon)
        #   uncovered: "    #####: LLLL:source"   (4 spaces, #####, colon, 1+ spaces, line, colon)
        #   non-exec:  "        -: LLLL:source"   (8 spaces, -, colon, 1+ spaces, line, colon)
        gcov_file = os.path.join(test_dir, 'dwm.c.gcov')
        if os.path.exists(gcov_file):
            with open(gcov_file) as f:
                for line in f:
                    # Uncovered: "    #####:\s+\d+:"
                    m = re.match(r'^    #####:\s+(\d+):', line)
                    if m:
                        lineno = int(m.group(1))
                        if lineno not in combined:
                            combined[lineno] = 0
                        continue
                    # Covered: "        \d+:\s+\d+:"
                    m = re.match(r'^        (\d+):\s+(\d+):', line)
                    if m:
                        lineno = int(m.group(2))
                        count = int(m.group(1))
                        if lineno in combined:
                            combined[lineno] = max(combined[lineno], count)
                        else:
                            combined[lineno] = count
                        continue
            os.remove(gcov_file)
    
    os.chdir(orig_dir)
    
    # Compute coverage stats
    total_executable = len(combined)
    covered = sum(1 for c in combined.values() if c > 0)
    uncovered_lines = [ln for ln, c in combined.items() if c == 0]
    
    print(f"\n=== Combined dwm.c coverage ===")
    print(f"  Executable lines: {total_executable}")
    print(f"  Covered:          {covered}")
    print(f"  Uncovered:        {len(uncovered_lines)}")
    if total_executable > 0:
        print(f"  Coverage:         {covered / total_executable * 100:.1f}%")
    
    if uncovered_lines:
        print(f"\n  Uncovered lines:")
        for ln in sorted(uncovered_lines):
            line_text = src_lines.get(ln, '').rstrip()
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
