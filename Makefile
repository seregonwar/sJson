CC       ?= gcc
CFLAGS   ?= -std=c99 -Wall -Wextra -Wpedantic -Wshadow -O2
LDFLAGS  ?= -lm
SRC_DIR  ?= src
TEST_SRC ?= $(SRC_DIR)/test_json.c
TEST_BIN ?= $(SRC_DIR)/test_json
TEST_INC ?= $(SRC_DIR)/json_pal.h

.PHONY: all test clean asan

all: $(TEST_BIN)

$(TEST_BIN): $(TEST_SRC) $(TEST_INC)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

test: $(TEST_BIN)
	./$(TEST_BIN)

asan: CFLAGS += -fsanitize=address,undefined -g
asan: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f $(TEST_BIN)
