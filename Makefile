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
PLATFORM_COMMON = platform/platform.c
PLATFORM_POSIX = platform/posix/platform_posix.c
PLATFORM_WIN32 = platform/win32/platform_win32.c
PLATFORM_HEADER = include/rivet/platform.h
DOCUMENT = web/stream_url_utf8.c web/html_css.c web/layout_image.c
DOCUMENT_HEADER = include/rivet/document.h
DOCUMENT_ABI_OBJECTS = $(BUILD_DIR)/r7-abi-stream.o $(BUILD_DIR)/r7-abi-html.o $(BUILD_DIR)/r7-abi-layout.o
BROWSER = apps/browser/browser.c
BROWSER_HEADER = include/rivet/browser.h

.PHONY: all gfx ui textview platform-posix document browser test test-gfx test-ui test-textview test-platform test-document test-document-charset test-document-abi test-browser check-no-heap check-no-heap-gfx check-no-heap-ui check-no-heap-textview check-no-heap-platform check-no-heap-document check-no-heap-browser clean

all: $(BUILD_DIR)/rivet-headless

gfx: $(BUILD_DIR)/rivet-gfx-proof

ui: $(BUILD_DIR)/rivet-ui-proof

textview: $(BUILD_DIR)/rivet-textview-proof

platform-posix: $(BUILD_DIR)/rivet-platform-posix

document: $(BUILD_DIR)/rivet-document-proof

browser: $(BUILD_DIR)/rivet-web1-proof

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

$(BUILD_DIR)/test-platform: $(CORE) $(HEADER) $(PLATFORM_COMMON) $(PLATFORM_HEADER) tests/test_platform.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) -Iinclude $(CFLAGS) $(CORE) $(PLATFORM_COMMON) tests/test_platform.c -o $@

$(BUILD_DIR)/rivet-platform-posix: $(CORE) $(HEADER) $(PLATFORM_COMMON) $(PLATFORM_HEADER) $(PLATFORM_POSIX) examples/r5_platform_proof.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) -Iinclude $(CFLAGS) $(CORE) $(PLATFORM_COMMON) $(PLATFORM_POSIX) examples/r5_platform_proof.c -o $@

$(BUILD_DIR)/test-document: $(DOCUMENT) $(DOCUMENT_HEADER) $(HEADER) tests/test_document.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) -Iinclude $(CFLAGS) $(DOCUMENT) tests/test_document.c -o $@

$(BUILD_DIR)/test-document-charset: $(CORE) $(DOCUMENT) $(DOCUMENT_HEADER) $(HEADER) tests/test_document_charset.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) -Iinclude $(CFLAGS) $(CORE) $(DOCUMENT) tests/test_document_charset.c -o $@

$(BUILD_DIR)/r7-abi-stream.o: web/stream_url_utf8.c $(DOCUMENT_HEADER) $(HEADER) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) -Iinclude $(CFLAGS) -c web/stream_url_utf8.c -o $@

$(BUILD_DIR)/r7-abi-html.o: web/html_css.c $(DOCUMENT_HEADER) $(HEADER) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) -Iinclude $(CFLAGS) -c web/html_css.c -o $@

$(BUILD_DIR)/r7-abi-layout.o: web/layout_image.c $(DOCUMENT_HEADER) $(HEADER) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) -Iinclude $(CFLAGS) -c web/layout_image.c -o $@

$(BUILD_DIR)/test-document-abi: $(DOCUMENT_ABI_OBJECTS) $(DOCUMENT_HEADER) $(HEADER) tests/test_document_abi.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) -Iinclude $(CFLAGS) -fshort-enums tests/test_document_abi.c $(DOCUMENT_ABI_OBJECTS) -o $@

$(BUILD_DIR)/rivet-document-proof: $(DOCUMENT) $(DOCUMENT_HEADER) $(HEADER) examples/r7_document_proof.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) -Iinclude $(CFLAGS) $(DOCUMENT) examples/r7_document_proof.c -o $@

$(BUILD_DIR)/test-browser: $(CORE) $(GFX) $(UI) $(DOCUMENT) $(BROWSER) $(BROWSER_HEADER) $(DOCUMENT_HEADER) $(UI_HEADER) $(GFX_HEADER) $(HEADER) tests/test_browser.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) -Iinclude $(CFLAGS) $(CORE) $(GFX) $(UI) $(DOCUMENT) $(BROWSER) tests/test_browser.c -o $@

$(BUILD_DIR)/rivet-web1-proof: $(CORE) $(GFX) $(UI) $(DOCUMENT) $(BROWSER) $(BROWSER_HEADER) $(DOCUMENT_HEADER) $(UI_HEADER) $(GFX_HEADER) $(HEADER) $(HEADLESS_PPM) $(HEADLESS_PPM_HEADER) examples/r8_browser_proof.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) -Iinclude -Iplatform/headless $(CFLAGS) $(CORE) $(GFX) $(UI) $(DOCUMENT) $(BROWSER) $(HEADLESS_PPM) examples/r8_browser_proof.c -o $@

test: check-no-heap $(BUILD_DIR)/test-core
	./$(BUILD_DIR)/test-core

test-gfx: check-no-heap-gfx $(BUILD_DIR)/test-gfx
	./$(BUILD_DIR)/test-gfx

test-ui: check-no-heap-ui $(BUILD_DIR)/test-ui
	./$(BUILD_DIR)/test-ui

test-textview: check-no-heap-textview $(BUILD_DIR)/test-textview
	./$(BUILD_DIR)/test-textview

test-platform: check-no-heap-platform $(BUILD_DIR)/test-platform
	./$(BUILD_DIR)/test-platform

test-document: check-no-heap-document $(BUILD_DIR)/test-document
	./$(BUILD_DIR)/test-document

test-document-charset: check-no-heap-document $(BUILD_DIR)/test-document-charset
	./$(BUILD_DIR)/test-document-charset

test-document-abi: check-no-heap-document $(BUILD_DIR)/test-document-abi
	./$(BUILD_DIR)/test-document-abi

test-browser: check-no-heap-browser $(BUILD_DIR)/test-browser
	./$(BUILD_DIR)/test-browser

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

check-no-heap-platform:
	@if grep -En '(malloc|calloc|realloc|free)[[:space:]]*\(' $(PLATFORM_COMMON) $(PLATFORM_POSIX) $(PLATFORM_WIN32) >/dev/null; then \
		echo "R5 platform backends must not require heap allocation"; \
		exit 1; \
	fi

check-no-heap-document:
	@if grep -En '(malloc|calloc|realloc|free)[[:space:]]*\(' $(DOCUMENT) >/dev/null; then \
		echo "R7 document engine must not require heap allocation"; \
		exit 1; \
	fi

check-no-heap-browser:
	@if grep -En '(malloc|calloc|realloc|free)[[:space:]]*\(' $(BROWSER) >/dev/null; then \
		echo "R8 WEB1 browser must not require heap allocation"; \
		exit 1; \
	fi

clean:
	rm -rf $(BUILD_DIR)
