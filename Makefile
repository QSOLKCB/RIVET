CC ?= cc
CFLAGS ?= -std=c99 -O2 -Wall -Wextra -Wpedantic
BUILD_DIR ?= build

CORE = core/rivet.c
HEADER = include/rivet/rivet.h
GFX = gfx/raster.c
GFX_HEADER = include/rivet/gfx.h
HEADLESS_PPM = platform/headless/ppm.c
HEADLESS_PPM_HEADER = platform/headless/ppm.h

.PHONY: all gfx test test-gfx check-no-heap check-no-heap-gfx clean

all: $(BUILD_DIR)/rivet-headless

gfx: $(BUILD_DIR)/rivet-gfx-proof

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/rivet-headless: $(CORE) $(HEADER) examples/r1_headless.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) -Iinclude $(CFLAGS) $(CORE) examples/r1_headless.c -o $@

$(BUILD_DIR)/test-core: $(CORE) $(HEADER) tests/test_core.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) -Iinclude $(CFLAGS) $(CORE) tests/test_core.c -o $@

$(BUILD_DIR)/test-gfx: $(GFX) $(GFX_HEADER) $(HEADER) tests/test_gfx.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) -Iinclude $(CFLAGS) $(GFX) tests/test_gfx.c -o $@

$(BUILD_DIR)/rivet-gfx-proof: $(GFX) $(GFX_HEADER) $(HEADER) $(HEADLESS_PPM) $(HEADLESS_PPM_HEADER) examples/r2_gfx_proof.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) -Iinclude -Iplatform/headless $(CFLAGS) $(GFX) $(HEADLESS_PPM) examples/r2_gfx_proof.c -o $@

test: check-no-heap $(BUILD_DIR)/test-core
	./$(BUILD_DIR)/test-core

test-gfx: check-no-heap-gfx $(BUILD_DIR)/test-gfx
	./$(BUILD_DIR)/test-gfx

check-no-heap:
	@if grep -En '(malloc|calloc|realloc|free)[[:space:]]*\(' $(CORE) >/dev/null; then \
		echo "R1 core must not require heap allocation"; \
		exit 1; \
	fi

check-no-heap-gfx:
	@if grep -En '(malloc|calloc|realloc|free)[[:space:]]*\(' $(GFX) >/dev/null; then \
		echo "R2 raster path must not require heap allocation"; \
		exit 1; \
	fi

clean:
	rm -rf $(BUILD_DIR)
