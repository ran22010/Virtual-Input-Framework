# Compiler & Build Flags
CC      ?= gcc
CFLAGS  ?= -O2 -Wall -Wextra -I../include -I../kernel

# 1. Automatically discover all unit test source files (test_*.c)
TEST_SRCS = $(wildcard test_*.c)
TEST_BINS = $(TEST_SRCS:.c=)

# Default target: build all discovered unit test binaries
all: $(TEST_BINS)

# 2. Individual Unit Test Dependencies (linking required kernel sources)
test_ringbuf: test_ringbuf.c ../kernel/vinput_ringbuf.c
	$(CC) $(CFLAGS) $^ -o $@

test_macro: test_macro.c ../kernel/vinput_macro.c ../kernel/vinput_ringbuf.c
	$(CC) $(CFLAGS) $^ -o $@

test_sched: test_sched.c ../kernel/vinput_sched.c ../kernel/vinput_ringbuf.c
	$(CC) $(CFLAGS) $^ -o $@

test_stats: test_stats.c ../kernel/vinput_stats.c
	$(CC) $(CFLAGS) $^ -o $@

# Generic fallback rule for standalone unit tests with no kernel C dependencies
test_%: test_%.c
	$(CC) $(CFLAGS) $< -o $@

# 3. Run all C unit tests in sequence
run-unit: all
	@echo "=========================================="
	@echo " Running C Unit Test Suite"
	@echo "=========================================="
	@for test in $(TEST_BINS); do \
		echo "--> Executing $$test"; \
		./$$test || exit 1; \
	done
	@echo "=========================================="
	@echo " All Unit Tests Passed!"
	@echo "=========================================="

# 4. Run end-to-end kernel integration tests (requires root/sudo)
run-integration:
	@echo "=========================================="
	@echo " Running System Integration Test"
	@echo "=========================================="
	@sudo ./integration_test.sh

# Run everything (Unit tests first, then Integration tests)
run-all: run-unit run-integration

# Clean test binaries and temporary output files
clean:
	rm -f $(TEST_BINS) *.o *.log

.PHONY: all run-unit run-integration run-all clean