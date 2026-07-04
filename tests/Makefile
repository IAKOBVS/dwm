.POSIX:

CFLAGS  = -std=c99 -pedantic -Wall -Wextra -Wno-unused-parameter -g -O0
CFLAGS += -I include -I ..
CFLAGS += -DDWM_TEST

LDFLAGS =

COV_CFLAGS  = -fprofile-arcs -ftest-coverage
COV_LDFLAGS = --coverage

MOCK_SRC = mock_x11.c
MOCK_OBJ = mock_x11.o
TEST_SRC = $(wildcard test_*.c)
TEST_BIN = $(TEST_SRC:.c=)

all: $(TEST_BIN)

$(MOCK_OBJ): $(MOCK_SRC) mock_x11.h
	$(CC) $(CFLAGS) -c -o $@ $<

%: %.c $(MOCK_OBJ)
	$(CC) $(CFLAGS) -o $@ $< $(MOCK_OBJ) $(LDFLAGS)

cov_%: %.c $(MOCK_OBJ)
	$(CC) $(CFLAGS) $(COV_CFLAGS) -o $@ $< $(MOCK_OBJ) $(LDFLAGS) $(COV_LDFLAGS)

run: $(TEST_BIN)
	@for t in $(TEST_BIN); do \
		printf "\033[1;34mRUNNING\033[0m $$t\n"; \
		./$$t; \
	done

coverage: clean
	$(MAKE) $(MOCK_OBJ) cov_test_pure_logic cov_test_segments cov_test_arrange cov_test_window_ops cov_test_events cov_test_drw_cache
	./cov_test_pure_logic
	./cov_test_segments
	./cov_test_arrange
	./cov_test_window_ops
	./cov_test_events
	./cov_test_drw_cache
	@gcov -o cov_test_pure_logic-test_pure_logic ../dwm.c 2>/dev/null || true
	@gcov -o cov_test_segments-test_segments ../dwm.c 2>/dev/null || true
	@gcov -o cov_test_arrange-test_arrange ../dwm.c 2>/dev/null || true
	@gcov -o cov_test_window_ops-test_window_ops ../dwm.c 2>/dev/null || true
	@gcov -o cov_test_events-test_events ../dwm.c 2>/dev/null || true
	@echo "=== Coverage summary (dwm.c) ==="
	@grep -E '^    [0-9]+:' dwm.c.gcov 2>/dev/null | head -25 || echo "(no gcov data)"

clean:
	rm -f $(TEST_BIN) cov_test_* *.o *.gcda *.gcno *.gcov

.PHONY: all run coverage clean
