.POSIX:

CFLAGS  = -std=c99 -pedantic -Wall -Wextra -Wno-unused-parameter -g -O0
CFLAGS += -I include -I ..
CFLAGS += -DDWM_TEST

LDFLAGS =

COV_CFLAGS  = -fprofile-arcs -ftest-coverage
COV_LDFLAGS = --coverage

ASAN_CFLAGS = -fsanitize=address -fno-omit-frame-pointer
ASAN_LDFLAGS = -fsanitize=address

MOCK_SRC = mock_x11.c
MOCK_OBJ = mock_x11.o
TEST_SRC = $(wildcard test_*.c)
TEST_BIN = $(TEST_SRC:.c=)

# Coverage/ASan targets are derived from the source list so new test files
# picked up by the wildcard are automatically covered (no manual lists).
COV_TARGETS  = $(addprefix cov_,$(TEST_BIN))
ASAN_TARGETS = $(addprefix asan_,$(TEST_BIN))
COV_LABELS   = $(subst test_,,$(TEST_BIN))

BENCH_BIN = bench_optimize

all: $(TEST_BIN) $(BENCH_BIN)

$(MOCK_OBJ): $(MOCK_SRC) mock_x11.h
	$(CC) $(CFLAGS) -c -o $@ $<

# Suites #include ../dwm.c directly, so the embedded source (and the
# headers it pulls in) must be prerequisites — otherwise editing dwm.c
# leaves stale binaries that silently fail/pass against old code.
DWM_DEPS = ../dwm.c ../dwm.h ../config.h mock_drw.h

%: %.c $(MOCK_OBJ) $(DWM_DEPS)
	$(CC) $(CFLAGS) -o $@ $< $(MOCK_OBJ) $(LDFLAGS)

# Self-contained test — does NOT include mock_x11.h or dwm.c,
# provides its own X11 stubs.  Must not be linked with mock_x11.o.
test_emoji_render_cache: test_emoji_render_cache.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

cov_%: %.c $(MOCK_OBJ) $(DWM_DEPS)
	$(CC) $(CFLAGS) $(COV_CFLAGS) -o $@ $< $(MOCK_OBJ) $(LDFLAGS) $(COV_LDFLAGS)

# Self-contained coverage build
cov_test_emoji_render_cache: test_emoji_render_cache.c
	$(CC) $(CFLAGS) $(COV_CFLAGS) -o $@ $< $(LDFLAGS) $(COV_LDFLAGS)

asan_%: %.c $(MOCK_OBJ) $(DWM_DEPS)
	$(CC) $(CFLAGS) $(ASAN_CFLAGS) -o $@ $< $(MOCK_OBJ) $(LDFLAGS) $(ASAN_LDFLAGS)

# Self-contained ASan build
asan_test_emoji_render_cache: test_emoji_render_cache.c
	$(CC) $(CFLAGS) $(ASAN_CFLAGS) -o $@ $< $(LDFLAGS) $(ASAN_LDFLAGS)

# Run every suite and propagate failures: a non-zero exit from any binary
# fails the make target (previously failures were silently ignored).
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
	@python3 merge_gcov.py . $(COV_LABELS)
	@echo "=== Per-test gcov files (last run) ==="
	@gcov -o cov_test_events-test_events ../dwm.c 2>/dev/null | grep -E 'Lines executed' || true
	@echo ""
	@grep -E '^    [0-9]+:' dwm.c.gcov 2>/dev/null | head -25 || true

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

clean:
	rm -f $(TEST_BIN) $(BENCH_BIN) cov_test_* asan_test_* *.o *.gcda *.gcno *.gcov

.PHONY: all test run coverage clean
