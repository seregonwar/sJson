CC      ?= gcc
CFLAGS  ?= -std=c99 -Wall -Wextra -Wpedantic -Wshadow -O2
LDFLAGS ?= -lm

.PHONY: all test clean asan

all: test_json

test_json: test_json.c json_pal.h
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

test: test_json
	./test_json

asan: CFLAGS += -fsanitize=address,undefined -g
asan: test_json

clean:
	rm -f test_json
