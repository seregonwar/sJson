CC       ?= gcc
CFLAGS   ?= -std=c99 -Wall -Wextra -Wpedantic -Wshadow -O2
LDFLAGS  ?= -lm
UNAME_S := $(shell uname -s 2>/dev/null)
ifneq (,$(findstring MINGW,$(UNAME_S)))
GCC_ENV  ?= env TEMP=C:/msys64/tmp TMP=C:/msys64/tmp TMPDIR=/c/msys64/tmp
else
GCC_ENV  ?=
endif
SRC_DIR  ?= src
TEST_SRC  ?= $(SRC_DIR)/test_json.c
TEST_BIN  ?= $(SRC_DIR)/test_json
TEST_INC  ?= $(SRC_DIR)/json_pal.h
IMPL_SRC  ?= $(SRC_DIR)/sJson.c
BENCH_SRC     ?= benchmarks/bench_sjson.c
BENCH_BIN     ?= benchmarks/bench_sjson
COMPARE_SRC   ?= benchmarks/bench_compare.c
COMPARE_BIN   ?= benchmarks/bench_compare
ASSET_SRC     ?= benchmarks/bench_assets.c
ASSET_BIN     ?= benchmarks/bench_assets
CORPUS_SRC    ?= tests/json_corpus_runner.c
CORPUS_BIN    ?= tests/json_corpus_runner
COMPARE_LIBS  ?= -lcjson

.PHONY: all test corpus-test bench bench-compare bench-assets fetch-assets clean asan ubsan fuzz-smoke check

all: $(TEST_BIN)

$(TEST_BIN): $(TEST_SRC) $(TEST_INC) $(IMPL_SRC)
	$(GCC_ENV) $(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

test: $(TEST_BIN)
	./$(TEST_BIN)

asan: CFLAGS += -fsanitize=address,undefined -g
asan: $(TEST_BIN)
	./$(TEST_BIN)

ubsan: CFLAGS += -fsanitize=undefined -g
ubsan: $(TEST_BIN)
	./$(TEST_BIN)

fuzz-smoke:
	$(GCC_ENV) $(CC) $(CFLAGS) fuzz/fuzz_parse.c -o fuzz/fuzz_parse_smoke $(LDFLAGS)
	./fuzz/fuzz_parse_smoke

check: test corpus-test fuzz-smoke

$(BENCH_BIN): $(BENCH_SRC) $(TEST_INC) $(IMPL_SRC)
	$(GCC_ENV) $(CC) $(CFLAGS) -O3 -DNDEBUG $< -o $@ $(LDFLAGS)

bench: $(BENCH_BIN)
	./$(BENCH_BIN)

$(COMPARE_BIN): $(COMPARE_SRC) $(TEST_INC) $(IMPL_SRC)
	$(GCC_ENV) $(CC) $(CFLAGS) -O3 -DNDEBUG -DSJSON_BENCH_WITH_CJSON $< -o $@ $(LDFLAGS) $(COMPARE_LIBS)

bench-compare: $(COMPARE_BIN)
	./$(COMPARE_BIN)

$(ASSET_BIN): $(ASSET_SRC) $(TEST_INC) $(IMPL_SRC)
	$(GCC_ENV) $(CC) $(CFLAGS) -O3 -DNDEBUG -DSJSON_BENCH_WITH_CJSON $< -o $@ $(LDFLAGS) $(COMPARE_LIBS)

bench-assets: $(ASSET_BIN)
	./$(ASSET_BIN) benchmarks/assets/canada.json benchmarks/assets/citm_catalog.json benchmarks/assets/twitter.json

$(CORPUS_BIN): $(CORPUS_SRC) $(TEST_INC) $(IMPL_SRC)
	$(GCC_ENV) $(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

corpus-test: $(CORPUS_BIN)
	./$(CORPUS_BIN) tests/JSONTestSuite/test_parsing

fetch-assets:
	bash scripts/fetch_official_assets.sh

clean:
	rm -f $(TEST_BIN) $(BENCH_BIN) $(COMPARE_BIN) $(ASSET_BIN) $(CORPUS_BIN) fuzz/fuzz_parse_smoke
