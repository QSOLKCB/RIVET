CC ?= cc
CFLAGS ?= -std=c99 -O2 -Wall -Wextra -Wpedantic
BUILD_DIR ?= build

CORE = core/rivet.c
HEADER = include/rivet/rivet.h

.PHONY: all test check-no-heap clean

all: $(BUILD_DIR)/rivet-headless

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/rivet-headless: $(CORE) $(HEADER) examples/r1_headless.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) -Iinclude $(CFLAGS) $(CORE) examples/r1_headless.c -o $@

$(BUILD_DIR)/test-core: $(CORE) $(HEADER) tests/test_core.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) -Iinclude $(CFLAGS) $(CORE) tests/test_core.c -o $@

test: check-no-heap $(BUILD_DIR)/test-core
	./$(BUILD_DIR)/test-core

check-no-heap:
	@if grep -En '(malloc|calloc|realloc|free)[[:space:]]*\(' $(CORE) >/dev/null; then \
		echo "R1 core must not require heap allocation"; \
		exit 1; \
	fi

clean:
	rm -rf $(BUILD_DIR)
