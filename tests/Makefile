.POSIX:

# Prefix the compiler with ccache when it is installed, so the many
# per-function test binaries can be rebuilt from cache. `command -v` keeps
# this a no-op on systems without ccache. Disable explicitly with
# `make CCACHE=` (or set CCACHE_DISABLE=1 for ccache's own bypass).
CCACHE := $(shell command -v ccache 2>/dev/null)
ifneq ($(CCACHE),)
CC := $(CCACHE) $(CC)
endif

CFLAGS  = -std=c99 -pedantic -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -g -O0
CFLAGS += -I . -I unit/include -I include -I ..
CFLAGS += -DDWM_TEST

LDFLAGS =

COV_CFLAGS  = -fprofile-arcs -ftest-coverage
COV_LDFLAGS = --coverage

ASAN_CFLAGS = -fsanitize=address -fno-omit-frame-pointer
ASAN_LDFLAGS = -fsanitize=address

# ASan+UBSan and TSan+UBSan builds are mutually exclusive and must not share
# artifacts, so each gets its own build directory (including its own mock
# object compiled with the same instrumentation).
ASAN_UBSAN_DIR = build/asan-ubsan
TSAN_UBSAN_DIR = build/tsan-ubsan

UBSAN_CFLAGS = -fsanitize=undefined -fno-sanitize-recover=undefined
ASAN_UBSAN_CFLAGS  = -fsanitize=address -fno-omit-frame-pointer $(UBSAN_CFLAGS)
ASAN_UBSAN_LDFLAGS = -fsanitize=address,undefined
TSAN_UBSAN_CFLAGS  = -fsanitize=thread -fno-omit-frame-pointer $(UBSAN_CFLAGS)
TSAN_UBSAN_LDFLAGS = -fsanitize=thread,undefined

MOCK_SRC = mock_x11.c
MOCK_OBJ = mock_x11.o

# Every per-function suite lives under unit/src/<module>/<function>.c;
# shared fixtures live under unit/include/. Discovery is recursive so adding
# a module directory is enough.
TEST_SRC   := $(shell find unit -name '*.c' | sort)
TEST_NAMES := $(notdir $(basename $(TEST_SRC)))
TEST_BIN   := $(addprefix bin/,$(TEST_NAMES))
INTEGRATION_NAMES := $(notdir $(basename $(wildcard unit/src/integration/*.c)))

# Suites that provide their own stubs and must NOT link mock_x11.o.
SELF_NAMES := selfcontained_drw_cache selfcontained_emoji_cache \
              strrchr_len die ecalloc \
              numpcpy_unsafe numpcpy_unsafe_signed \
              xisalpha xisdigit xisalnum xisascii xisblank xiscntrl \
              xisgraph xislower xisupper xisprint xispunct xisspace \
              xisxdigit xtolower xtoupper
SELF_BIN   := $(addprefix bin/,$(SELF_NAMES))
DWM_BIN    := $(filter-out $(SELF_BIN),$(TEST_BIN))

# Coverage/ASan binaries keep the legacy flat cov_test_<label> /
# asan_test_<label> names so merge_gcov.py and the existing docs keep working.
COV_TARGETS  := $(addprefix cov_test_,$(TEST_NAMES))
ASAN_TARGETS := $(addprefix asan_test_,$(TEST_NAMES))
COV_LABELS   := $(TEST_NAMES)

BENCH_BIN = bench_optimize

all: $(TEST_BIN) $(BENCH_BIN)

# Tests live in subdirectories but the build is flat (bin/); basenames are
# unique by construction (one test_<function>.c per directory).
vpath %.c $(sort $(dir $(TEST_SRC))) .

bin:
	mkdir -p bin

$(MOCK_OBJ): $(MOCK_SRC) mock_x11.h
	$(CC) $(CFLAGS) -c -o $@ $<

# Suites #include ../dwm.c (or drw.c/util.c) directly, so the embedded
# sources and their headers must be prerequisites — otherwise editing them
# leaves stale binaries that silently test old code.
DWM_DEPS = ../dwm.c ../dwm.h ../config.h ../drw.c ../drw.h ../util.c ../util.h \
           mock_drw.h $(wildcard unit/include/*.h)

# Standard per-function suite: dwm.c + mock X11/Xft.
bin/%: %.c $(MOCK_OBJ) $(DWM_DEPS) | bin
	$(CC) $(CFLAGS) -o $@ $< $(MOCK_OBJ) $(LDFLAGS)

# Self-contained suites provide their own stubs (drw cache replica, util
# helpers, portable strrchr_len) and must not link mock_x11.o.
$(SELF_BIN): bin/%: %.c $(DWM_DEPS) | bin
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(BENCH_BIN): bench_optimize.c $(MOCK_OBJ) $(DWM_DEPS)
	$(CC) $(CFLAGS) -o $@ $< $(MOCK_OBJ) $(LDFLAGS)

cov_test_%: %.c $(MOCK_OBJ) $(DWM_DEPS)
	$(CC) $(CFLAGS) $(COV_CFLAGS) -o $@ $< $(MOCK_OBJ) $(LDFLAGS) $(COV_LDFLAGS)

# Self-contained coverage builds
$(addprefix cov_test_,$(SELF_NAMES)): cov_test_%: %.c $(DWM_DEPS)
	$(CC) $(CFLAGS) $(COV_CFLAGS) -o $@ $< $(LDFLAGS) $(COV_LDFLAGS)

asan_test_%: %.c $(MOCK_OBJ) $(DWM_DEPS)
	$(CC) $(CFLAGS) $(ASAN_CFLAGS) -o $@ $< $(MOCK_OBJ) $(LDFLAGS) $(ASAN_LDFLAGS)

# Self-contained ASan builds
$(addprefix asan_test_,$(SELF_NAMES)): asan_test_%: %.c $(DWM_DEPS)
	$(CC) $(CFLAGS) $(ASAN_CFLAGS) -o $@ $< $(LDFLAGS) $(ASAN_LDFLAGS)

# --- isolated ASan+UBSan and TSan+UBSan builds -------------------------
ASAN_UBSAN_BIN  := $(addprefix $(ASAN_UBSAN_DIR)/,$(TEST_NAMES))
TSAN_UBSAN_BIN  := $(addprefix $(TSAN_UBSAN_DIR)/,$(TEST_NAMES))
ASAN_UBSAN_SELF := $(addprefix $(ASAN_UBSAN_DIR)/,$(SELF_NAMES))
TSAN_UBSAN_SELF := $(addprefix $(TSAN_UBSAN_DIR)/,$(SELF_NAMES))
ASAN_UBSAN_DWM  := $(filter-out $(ASAN_UBSAN_SELF),$(ASAN_UBSAN_BIN))
TSAN_UBSAN_DWM  := $(filter-out $(TSAN_UBSAN_SELF),$(TSAN_UBSAN_BIN))
ASAN_UBSAN_INTEGRATION_BIN := $(addprefix $(ASAN_UBSAN_DIR)/,$(INTEGRATION_NAMES))
TSAN_UBSAN_INTEGRATION_BIN := $(addprefix $(TSAN_UBSAN_DIR)/,$(INTEGRATION_NAMES))

$(ASAN_UBSAN_DIR) $(TSAN_UBSAN_DIR):
	mkdir -p $@

$(ASAN_UBSAN_DIR)/mock_x11.o: mock_x11.c mock_x11.h | $(ASAN_UBSAN_DIR)
	$(CC) $(CFLAGS) $(ASAN_UBSAN_CFLAGS) -c -o $@ $<

$(TSAN_UBSAN_DIR)/mock_x11.o: mock_x11.c mock_x11.h | $(TSAN_UBSAN_DIR)
	$(CC) $(CFLAGS) $(TSAN_UBSAN_CFLAGS) -c -o $@ $<

# Suites that include dwm.c/drw.c/util.c link the sanitized mock.
$(ASAN_UBSAN_DWM): $(ASAN_UBSAN_DIR)/%: %.c $(ASAN_UBSAN_DIR)/mock_x11.o $(DWM_DEPS) | $(ASAN_UBSAN_DIR)
	$(CC) $(CFLAGS) $(ASAN_UBSAN_CFLAGS) -o $@ $< $(ASAN_UBSAN_DIR)/mock_x11.o $(LDFLAGS) $(ASAN_UBSAN_LDFLAGS)

$(TSAN_UBSAN_DWM): $(TSAN_UBSAN_DIR)/%: %.c $(TSAN_UBSAN_DIR)/mock_x11.o $(DWM_DEPS) | $(TSAN_UBSAN_DIR)
	$(CC) $(CFLAGS) $(TSAN_UBSAN_CFLAGS) -o $@ $< $(TSAN_UBSAN_DIR)/mock_x11.o $(LDFLAGS) $(TSAN_UBSAN_LDFLAGS)

# Self-contained suites still need no mock object.
$(ASAN_UBSAN_SELF): $(ASAN_UBSAN_DIR)/%: %.c $(DWM_DEPS) | $(ASAN_UBSAN_DIR)
	$(CC) $(CFLAGS) $(ASAN_UBSAN_CFLAGS) -o $@ $< $(LDFLAGS) $(ASAN_UBSAN_LDFLAGS)

$(TSAN_UBSAN_SELF): $(TSAN_UBSAN_DIR)/%: %.c $(DWM_DEPS) | $(TSAN_UBSAN_DIR)
	$(CC) $(CFLAGS) $(TSAN_UBSAN_CFLAGS) -o $@ $< $(LDFLAGS) $(TSAN_UBSAN_LDFLAGS)

# Run every suite and propagate failures: a non-zero exit from any binary
# fails the make target.
test run: $(TEST_BIN)
	@status=0; \
	for t in $(TEST_BIN); do \
		printf "\033[1;34mRUNNING\033[0m $$t\n"; \
		./$$t || { printf "\033[1;31mFAILED \033[0m$$t\n"; status=1; }; \
	done; \
	exit $$status

cov_bins: $(MOCK_OBJ)
	$(MAKE) $(COV_TARGETS)

coverage: clean
	$(MAKE) $(MOCK_OBJ)
	$(MAKE) $(COV_TARGETS)
	@python3 merge_gcov.py . $(COV_LABELS) $(COV_MIN)

# Fail the build when any tracked source drops below its coverage floor.
coverage-check:
	$(MAKE) coverage COV_MIN='--min=dwm.c=95 --min=drw.c=80 --min=util.c=85'

asan: $(MOCK_OBJ)
	$(MAKE) CFLAGS="$(CFLAGS) $(ASAN_CFLAGS)" LDFLAGS="$(LDFLAGS) $(ASAN_LDFLAGS)" \
		$(ASAN_TARGETS)
	@# LeakSanitizer runs on every suite; fixtures are swept per-test and
	@# dwm-created clients are drained via unmanage() where needed.
	@status=0; \
	for t in $(ASAN_TARGETS); do \
		if [ -x "$$t" ]; then \
			printf "\033[1;34mRUNNING\033[0m $$t\n"; \
			./$$t || { printf "\033[1;31mFAILED \033[0m$$t\n"; status=1; }; \
		fi \
	done; \
	exit $$status

# Build and run every suite under ASan+UBSan in build/asan-ubsan/.
asan-ubsan: $(ASAN_UBSAN_BIN)
	@status=0; \
	for t in $(ASAN_UBSAN_BIN); do \
		printf "\033[1;34mRUNNING\033[0m $$t\n"; \
		ASAN_OPTIONS=detect_leaks=1:abort_on_error=1 \
		UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
		./$$t || { printf "\033[1;31mFAILED \033[0m$$t\n"; status=1; }; \
	done; \
	exit $$status

# Build and run every suite under TSan+UBSan in build/tsan-ubsan/.
tsan-ubsan: $(TSAN_UBSAN_BIN)
	@status=0; \
	for t in $(TSAN_UBSAN_BIN); do \
		printf "\033[1;34mRUNNING\033[0m $$t\n"; \
		TSAN_OPTIONS=halt_on_error=1 \
		UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
		./$$t || { printf "\033[1;31mFAILED \033[0m$$t\n"; status=1; }; \
	done; \
	exit $$status

# Sanitized integration suites only (same artifacts as the full targets).
asan-ubsan-integration: $(ASAN_UBSAN_INTEGRATION_BIN)
	@status=0; \
	for t in $(ASAN_UBSAN_INTEGRATION_BIN); do \
		printf "\033[1;34mRUNNING\033[0m $$t\n"; \
		ASAN_OPTIONS=detect_leaks=1:abort_on_error=1 \
		UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
		./$$t || { printf "\033[1;31mFAILED \033[0m$$t\n"; status=1; }; \
	done; \
	exit $$status

tsan-ubsan-integration: $(TSAN_UBSAN_INTEGRATION_BIN)
	@status=0; \
	for t in $(TSAN_UBSAN_INTEGRATION_BIN); do \
		printf "\033[1;34mRUNNING\033[0m $$t\n"; \
		TSAN_OPTIONS=halt_on_error=1 \
		UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
		./$$t || { printf "\033[1;31mFAILED \033[0m$$t\n"; status=1; }; \
	done; \
	exit $$status

# Clang-only integer sanitizer (builds into its own tree; the runner script
# decides which diagnostics are intentional wraparound in hash/PRNG code).
CLANG := $(shell command -v clang 2>/dev/null)
INTEGER_DIR = build/integer-sanitizer
INTEGER_CFLAGS  = -fsanitize=integer -fno-omit-frame-pointer
INTEGER_LDFLAGS = -fsanitize=integer
INTEGER_BIN  := $(addprefix $(INTEGER_DIR)/,$(TEST_NAMES))
INTEGER_SELF := $(addprefix $(INTEGER_DIR)/,$(SELF_NAMES))
INTEGER_DWM  := $(filter-out $(INTEGER_SELF),$(INTEGER_BIN))

$(INTEGER_DIR):
	mkdir -p $@

$(INTEGER_DIR)/mock_x11.o: mock_x11.c mock_x11.h | $(INTEGER_DIR)
	$(CLANG) $(CFLAGS) $(INTEGER_CFLAGS) -c -o $@ $<

$(INTEGER_DWM): $(INTEGER_DIR)/%: %.c $(INTEGER_DIR)/mock_x11.o $(DWM_DEPS) | $(INTEGER_DIR)
	$(CLANG) $(CFLAGS) $(INTEGER_CFLAGS) -o $@ $< $(INTEGER_DIR)/mock_x11.o $(LDFLAGS) $(INTEGER_LDFLAGS)

$(INTEGER_SELF): $(INTEGER_DIR)/%: %.c $(DWM_DEPS) | $(INTEGER_DIR)
	$(CLANG) $(CFLAGS) $(INTEGER_CFLAGS) -o $@ $< $(LDFLAGS) $(INTEGER_LDFLAGS)

integer-bins: $(INTEGER_BIN)

clean:
	rm -rf bin
	rm -rf build/asan-ubsan build/tsan-ubsan build/integer-sanitizer build/e2e
	rm -f $(BENCH_BIN) cov_test_* asan_test_* *.o *.gcda *.gcno *.gcov

.PHONY: all test run coverage coverage-check cov_bins asan asan-ubsan tsan-ubsan \
        asan-ubsan-integration tsan-ubsan-integration integer-bins clean
