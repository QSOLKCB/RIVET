CC ?= cc
CFLAGS ?= -std=c99 -O2 -Wall -Wextra -Wpedantic
BUILD_DIR ?= build

CORE = core/rivet.c
HEADER = include/rivet/rivet.h
GFX = gfx/raster.c
GFX_HEADER = include/rivet/gfx.h
UI = ui/ui.c
UI_HEADER = include/rivet/ui.h
TEXTVIEW = apps/textview/textview.c
TEXTVIEW_HEADER = apps/textview/textview.h
HEADLESS_PPM = platform/headless/ppm.c
HEADLESS_PPM_HEADER = platform/headless/ppm.h
HEADLESS_TEXT = platform/headless/text_file.c
HEADLESS_TEXT_HEADER = platform/headless/text_file.h

.PHONY: all gfx ui textview test test-gfx test-ui test-textview check-no-heap check-no-heap-gfx check-no-heap-ui check-no-heap-textview clean

all: $(BUILD_DIR)/rivet-headless

gfx: $(BUILD_DIR)/rivet-gfx-proof

ui: $(BUILD_DIR)/rivet-ui-proof

textview: $(BUILD_DIR)/rivet-textview-proof

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

$(BUILD_DIR)/test-ui: $(CORE) $(HEADER) $(GFX) $(GFX_HEADER) $(UI) $(UI_HEADER) tests/test_ui.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) -Iinclude $(CFLAGS) $(CORE) $(GFX) $(UI) tests/test_ui.c -o $@

$(BUILD_DIR)/rivet-ui-proof: $(CORE) $(HEADER) $(GFX) $(GFX_HEADER) $(UI) $(UI_HEADER) $(HEADLESS_PPM) $(HEADLESS_PPM_HEADER) examples/r3_ui_proof.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) -Iinclude -Iplatform/headless $(CFLAGS) $(CORE) $(GFX) $(UI) $(HEADLESS_PPM) examples/r3_ui_proof.c -o $@

$(BUILD_DIR)/test-textview: $(GFX) $(GFX_HEADER) $(HEADER) $(TEXTVIEW) $(TEXTVIEW_HEADER) tests/test_textview.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) -Iinclude -Iapps/textview $(CFLAGS) $(GFX) $(TEXTVIEW) tests/test_textview.c -o $@

$(BUILD_DIR)/rivet-textview-proof: $(CORE) $(HEADER) $(GFX) $(GFX_HEADER) $(UI) $(UI_HEADER) $(TEXTVIEW) $(TEXTVIEW_HEADER) $(HEADLESS_PPM) $(HEADLESS_PPM_HEADER) $(HEADLESS_TEXT) $(HEADLESS_TEXT_HEADER) examples/r4_textview_proof.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) -Iinclude -Iapps/textview -Iplatform/headless $(CFLAGS) $(CORE) $(GFX) $(UI) $(TEXTVIEW) $(HEADLESS_PPM) $(HEADLESS_TEXT) examples/r4_textview_proof.c -o $@

test: check-no-heap $(BUILD_DIR)/test-core
	./$(BUILD_DIR)/test-core

test-gfx: check-no-heap-gfx $(BUILD_DIR)/test-gfx
	./$(BUILD_DIR)/test-gfx

test-ui: check-no-heap-ui $(BUILD_DIR)/test-ui
	./$(BUILD_DIR)/test-ui

test-textview: check-no-heap-textview $(BUILD_DIR)/test-textview
	./$(BUILD_DIR)/test-textview

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

check-no-heap-ui:
	@if grep -En '(malloc|calloc|realloc|free)[[:space:]]*\(' $(UI) >/dev/null; then \
		echo "R3 UI path must not require heap allocation"; \
		exit 1; \
	fi

check-no-heap-textview:
	@if grep -En '(malloc|calloc|realloc|free)[[:space:]]*\(' $(TEXTVIEW) >/dev/null; then \
		echo "R4 text viewer must not require heap allocation"; \
		exit 1; \
	fi

clean:
	rm -rf $(BUILD_DIR)
