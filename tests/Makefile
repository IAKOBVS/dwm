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

BENCH_BIN = bench_optimize

all: $(TEST_BIN) $(BENCH_BIN)

$(MOCK_OBJ): $(MOCK_SRC) mock_x11.h
	$(CC) $(CFLAGS) -c -o $@ $<

%: %.c $(MOCK_OBJ)
	$(CC) $(CFLAGS) -o $@ $< $(MOCK_OBJ) $(LDFLAGS)

# Self-contained test — does NOT include mock_x11.h or dwm.c,
# provides its own X11 stubs.  Must not be linked with mock_x11.o.
test_emoji_render_cache: test_emoji_render_cache.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

cov_%: %.c $(MOCK_OBJ)
	$(CC) $(CFLAGS) $(COV_CFLAGS) -o $@ $< $(MOCK_OBJ) $(LDFLAGS) $(COV_LDFLAGS)

# Self-contained coverage build
cov_test_emoji_render_cache: test_emoji_render_cache.c
	$(CC) $(CFLAGS) $(COV_CFLAGS) -o $@ $< $(LDFLAGS) $(COV_LDFLAGS)

asan_%: %.c $(MOCK_OBJ)
	$(CC) $(CFLAGS) $(ASAN_CFLAGS) -o $@ $< $(MOCK_OBJ) $(LDFLAGS) $(ASAN_LDFLAGS)

# Self-contained ASan build
asan_test_emoji_render_cache: test_emoji_render_cache.c
	$(CC) $(CFLAGS) $(ASAN_CFLAGS) -o $@ $< $(LDFLAGS) $(ASAN_LDFLAGS)

test run: $(TEST_BIN)
	@for t in $(TEST_BIN); do \
		printf "\033[1;34mRUNNING\033[0m $$t\n"; \
		./$$t; \
	done

COV_LABELS = pure_logic segments arrange window_ops events drw_cache emoji_render_cache comprehensive drw_safety

cov_bins: $(MOCK_OBJ)
	$(MAKE) $(addprefix cov_test_,$(COV_LABELS))

coverage: clean
	$(MAKE) $(MOCK_OBJ)
	$(MAKE) $(addprefix cov_test_,$(COV_LABELS))
	@python3 merge_gcov.py . $(COV_LABELS)
	@echo "=== Per-test gcov files (last run) ==="
	@gcov -o cov_test_events-test_events ../dwm.c 2>/dev/null | grep -E 'Lines executed' || true
	@echo ""
	@grep -E '^    [0-9]+:' dwm.c.gcov 2>/dev/null | head -25 || true

asan: $(MOCK_OBJ)
	$(MAKE) CFLAGS="$(CFLAGS) $(ASAN_CFLAGS)" LDFLAGS="$(LDFLAGS) $(ASAN_LDFLAGS)" \
		asan_test_pure_logic asan_test_segments asan_test_arrange \
		asan_test_window_ops asan_test_events asan_test_drw_cache \
		asan_test_emoji_render_cache asan_test_drw_safety
	@for t in asan_test_*; do \
		if [ -x "$$t" ]; then \
			printf "\033[1;34mRUNNING\033[0m $$t\n"; \
			./$$t; \
		fi \
	done

clean:
	rm -f $(TEST_BIN) $(BENCH_BIN) cov_test_* asan_test_* *.o *.gcda *.gcno *.gcov

.PHONY: all test run coverage clean
