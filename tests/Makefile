# This directory is meant to be placed at the repository root: test/
# Supports either include/uapi/vinput_uapi.h or kernel/uapi/vinput_uapi.h.
CC ?= cc
CFLAGS ?= -O2 -g
CPPFLAGS += -D_DEFAULT_SOURCE -I../include -I../kernel
CFLAGS += -std=gnu11 -Wall -Wextra -Werror

TESTS := test_vinput_io test_vinput_output test_vinput_capture

.PHONY: all run clean
all: $(TESTS)

$(TESTS): %: %.c test_common.c test_common.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $< test_common.c -o $@

run: all
	./run_tests.sh

clean:
	rm -f $(TESTS)
