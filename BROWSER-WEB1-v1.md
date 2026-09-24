# RIVET Browser WEB1 Contract v1

## Purpose

WEB1 is the first bounded RIVET Browser profile.

It composes the frozen R1-R7 contracts into a downstream, non-JavaScript browser application. Browser-specific state remains outside the portable core.

Machine-readable identity: `machine/browser-web1-v1.json`.

## ABI and source

Public header:

```text
include/rivet/browser.h
RIVET_BROWSER_ABI_VERSION = 1
language baseline          = C99
```

Reference source:

```text
apps/browser/browser.c
```

WEB1 requires no heap allocation. Browser bytes, HTML nodes, CSS rules, layout boxes, history, bookmarks, download scratch space, and pixels remain caller-owned and explicitly bounded.

## Resource service boundary

WEB1 does not make one HTTP/TLS stack universal.

A host supplies:

```text
browser.resource.fetch
browser.download.sink   (optional when downloads are disabled)
```

The fetch callback resolves an absolute URL into bounded caller-owned bytes. A modern native host, deterministic fixture provider, or future RIVET Relay may implement the service.

Transport machinery is not browser semantics.

## Inspectable configuration

WEB1 configuration is a strict UTF-8/numeric-ASCII line format:

```text
RIVET-WEB1 1
home=<absolute-http-or-https-url>
downloads=0|1
user-css=<single-line Document-v1 CSS>
```

Unknown lines, reordered fields, malformed URLs, malformed booleans, CR line endings, trailing bytes, or invalid CSS fail explicitly.

The configuration bytes remain caller-owned. The parsed configuration stores slices into those bytes.

## Browsing semantics

WEB1 supports:

- absolute HTTP/HTTPS navigation through the resource service;
- one current document;
- bounded history with back/forward semantics;
- bounded bookmarks with duplicate rejection;
- bounded selected-link traversal with wrap;
- opening the selected absolute link;
- reloading and returning home;
- downloading the selected resource through the explicit download sink;
- view-source mode;
- user CSS applied after document CSS;
- line-step scrolling;
- keyboard-first command projection;
- local software-surface rendering.

A failed fetch or malformed document does not enter history. A load that has begun but fails document/layout validation invalidates the active loaded-document state rather than exposing partially validated content.

## Commands and default keys

Stable command IDs:

```text
browser.back
browser.forward
browser.reload
browser.home
browser.link.next
browser.link.open
browser.bookmark.add
browser.download
view.source
view.scroll.up
view.scroll.down
```

Default keyboard projection:

```text
Alt+B       back
Alt+F       forward
Ctrl+R      reload
Alt+H       home
N           next link
Enter       open selected link
Ctrl+K      add bookmark
Ctrl+D      download selected link
Ctrl+S      toggle source
Up/Down     scroll
```

The command identities remain independent of this projection.

## Rendering

WEB1 renders to the frozen R2 RGBA8888 software surface.

The reference renderer:

- reserves a 10-pixel browser chrome row;
- renders document text from R7 layout boxes using a small scalar 5x7 bitmap baseline;
- maps lowercase glyphs to the same baseline shapes as uppercase;
- renders unsupported glyph shapes as a visible question-mark fallback;
- uses CSS-derived foreground/background values from the R7 layout;
- highlights the selected link;
- renders image and input boxes as bounded placeholders in WEB1;
- renders raw document bytes in view-source mode;
- performs no GPU/API rendering.

Inline image fetch/decode integration is deliberately not required by WEB1; R7 already proves the bounded PPM codec and later browser profiles may connect richer resource presentation without redefining WEB1.

## History, bookmarks, and downloads

History and bookmarks are in-memory caller-owned arrays of bounded URL records.

Durable persistence adapters are not part of WEB1. A host may export or persist these records without changing browser semantics.

Downloads are explicit byte transfers to the host-supplied sink. WEB1 does not silently assume filesystem write access.

## User styles

The configured `user-css` line is parsed with Document v1 CSS and appended after document style rules, so later user declarations override matching supported properties.

## Footprint evidence

The deterministic proof prints:

- browser state bytes;
- total caller-owned proof resident bytes;
- deterministic document-mode pixel FNV-1a64;
- deterministic source-mode pixel FNV-1a64.

The R8 CI footprint step additionally records the built proof executable's byte size with `wc -c` and its text/data/BSS section sizes with the platform `size` tool. Executable-size evidence is therefore a build-environment observation rather than a value emitted by the proof process itself.

Footprint observations are evidence, not correctness identity across architectures.

Reference raster identity at 120 × 64:

```text
document-mode FNV-1a64 = 75be6cc92698ac1a
source-mode   FNV-1a64 = 5cf7c63a1fa3d9b4
```

The first x86-64 hosted proof observation records `sizeof(rivet_browser) = 848` bytes. The proof's `proof_resident_bytes` total explicitly sums every simultaneously live proof-owned buffer, state structure, command/render object, counter, event and pointer, including the 120 × 64 RGBA surface. These measurements are environment evidence rather than portable ABI-size requirements.

## Non-goals

WEB1 does not add:

- JavaScript;
- cookies;
- browser-style malformed HTML recovery;
- relative URL resolution;
- HTTP/TLS implementation inside the browser;
- form submission;
- durable history/bookmark storage;
- tabs;
- pointer-first chrome;
- inline image fetch/render integration;
- font shaping;
- incremental layout/caches;
- threads/SIMD;
- GPU rendering.

Those capabilities require later profiles or service implementations rather than being inferred from WEB1.
