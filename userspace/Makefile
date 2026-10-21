CC ?= cc
CPPFLAGS += -D_POSIX_C_SOURCE=200809L -I../include
CFLAGS ?= -O2 -g -std=gnu11 -Wall -Wextra -Werror
LDFLAGS ?=
LDLIBS ?=

SOURCES := main.c macro.c recorder.c player.c macro_file.c
OBJECTS := $(SOURCES:.c=.o)
TARGET := vinput

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
