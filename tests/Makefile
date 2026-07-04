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

all: $(TEST_BIN)

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

run: $(TEST_BIN)
	@for t in $(TEST_BIN); do \
		printf "\033[1;34mRUNNING\033[0m $$t\n"; \
		./$$t; \
	done

coverage: clean
	$(MAKE) $(MOCK_OBJ) cov_test_pure_logic cov_test_segments cov_test_arrange cov_test_window_ops cov_test_events cov_test_drw_cache cov_test_emoji_render_cache
	./cov_test_pure_logic
	./cov_test_segments
	./cov_test_arrange
	./cov_test_window_ops
	./cov_test_events
	./cov_test_drw_cache
	./cov_test_emoji_render_cache
	@gcov -o cov_test_pure_logic-test_pure_logic ../dwm.c 2>/dev/null || true
	@gcov -o cov_test_segments-test_segments ../dwm.c 2>/dev/null || true
	@gcov -o cov_test_arrange-test_arrange ../dwm.c 2>/dev/null || true
	@gcov -o cov_test_window_ops-test_window_ops ../dwm.c 2>/dev/null || true
	@gcov -o cov_test_events-test_events ../dwm.c 2>/dev/null || true
	@echo "=== Coverage summary (dwm.c) ==="
	@grep -E '^    [0-9]+:' dwm.c.gcov 2>/dev/null | head -25 || echo "(no gcov data)"

asan: $(MOCK_OBJ)
	$(MAKE) CFLAGS="$(CFLAGS) $(ASAN_CFLAGS)" LDFLAGS="$(LDFLAGS) $(ASAN_LDFLAGS)" \
		asan_test_pure_logic asan_test_segments asan_test_arrange \
		asan_test_window_ops asan_test_events asan_test_drw_cache \
		asan_test_emoji_render_cache
	@for t in asan_test_*; do \
		if [ -x "$$t" ]; then \
			printf "\033[1;34mRUNNING\033[0m $$t\n"; \
			./$$t; \
		fi \
	done

clean:
	rm -f $(TEST_BIN) cov_test_* asan_test_* *.o *.gcda *.gcno *.gcov

.PHONY: all run coverage clean
