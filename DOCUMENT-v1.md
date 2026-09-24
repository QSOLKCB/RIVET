# RIVET Document Contract v1

## Purpose

Document v1 is the bounded R7 document engine.

It provides small, deterministic reference semantics for:

- URL parsing;
- byte streams;
- strict UTF-8 decoding;
- a strict HTML subset;
- a strict CSS subset;
- bounded layout;
- PPM/P6 image decode;
- explicit link/form/image capability discovery.

It does **not** provide a browser, network transport, scripting, history, downloads, or browser-style error recovery.

Machine-readable identity: `machine/document-v1.json`.

## ABI and source

Public header:

```text
include/rivet/document.h
RIVET_DOCUMENT_ABI_VERSION = 1
language baseline          = C99
```

Reference sources:

```text
web/stream_url_utf8.c
web/html_css.c
web/layout_image.c
```

The implementation requires no heap allocation.

Caller-owned storage is used for decoded code points, HTML nodes, CSS rules, layout boxes, and image pixels.

## Byte streams

`rivet_byte_stream` is an immutable byte-source view with a bounded cursor.

Read semantics:

- the caller supplies the destination buffer and requested byte count;
- reads return up to the requested count;
- EOF is a successful zero-byte read;
- peek returns `RIVET_ERR_NOT_FOUND` at EOF;
- no hidden buffering is introduced.

## URL subset

Document v1 accepts absolute HTTP/HTTPS URLs only.

Supported URL identity:

- scheme: HTTP or HTTPS, ASCII case-insensitive;
- authority separator: `//`;
- host: ASCII letters, digits, dot and hyphen;
- optional decimal port 1..65535;
- optional path/query/fragment;
- percent escapes must contain two hexadecimal digits.

URL input must be visible numeric ASCII bytes `0x21..0x7e`.

Not supported in URL v1:

- userinfo;
- IPv6 literals;
- IDNA/domain Unicode;
- URL canonicalisation;
- relative-URL resolution;
- percent decoding.

Slices reference caller-owned input bytes.

## UTF-8

`rivet_utf8_decode()` implements strict UTF-8 scalar decoding.

It rejects:

- invalid leading/continuation bytes;
- overlong forms;
- surrogate code points;
- truncated sequences;
- values above U+10FFFF.

Decoded scalars are written only when caller capacity is sufficient.

## HTML subset

Document v1 intentionally uses a strict subset rather than browser recovery semantics.

Supported elements:

```text
html
head
body
p
h1
h2
a
form
input
img
br
style
text
```

Supported semantic attributes:

```text
a      href
form   action
input  name value
img    src width height
```

Attribute values must use double quotes.

Unknown attributes are parsed safely but do not gain semantics.

Void elements:

```text
input
img
br
```

Rules:

- nesting must be explicit and correctly matched;
- maximum element stack depth is 32;
- exactly one HTML root and one body are required;
- duplicate head/body roots are rejected;
- image nodes require src, width and height;
- image dimensions are positive decimal values no greater than 4096;
- HTML text must be valid UTF-8;
- NUL is rejected;
- character/entity references are not implemented in v1 and `&` is rejected;
- comments, script, malformed-tag recovery, implicit closing and unknown tags are unsupported.

The parser performs a validation/count pass before writing nodes, so insufficient caller capacity fails explicitly.

## Content capability discovery

Parsed content may require these independent capabilities:

```text
document.html
document.css
document.links
document.forms
image.ppm
```

Links and forms are separate capabilities: parsing HTML does not imply that an application must expose navigation or form submission.

The returned capability order is deterministic.

## CSS subset

Selectors are element selectors only:

```text
body p h1 h2 a form input img
```

Supported properties:

```text
color
background-color
margin-top
margin-bottom
```

Color syntax is exactly `#RRGGBB`.

Margins are non-negative decimal pixel values from 0 through 1024.

Unknown selectors/properties, duplicate declarations, malformed syntax and values beyond the bound fail explicitly.

CSS comments, classes, IDs, combinators, inheritance tables, cascading specificity, media queries and external stylesheets are outside v1.

Rules apply in source order; later matching declarations replace earlier matching declarations for the same property.

## Layout subset

Layout v1 is a deterministic reference flow, not a complete CSS formatting model.

Reference text metrics:

```text
glyph advance = 6 px per Unicode scalar
line height   = 8 px
input box     = 80 x 12 px
```

Block elements:

```text
body p h1 h2 form
```

Inline content includes text and links.

Images and inputs are replaced boxes.

Layout:

- wraps at the caller-declared viewport width;
- collapses ASCII HTML whitespace for measurement;
- emits caller-owned text/image/input boxes;
- uses parent text colour plus supported element-rule overrides;
- applies declared block top/bottom margins;
- carries source slices rather than copying document text;
- fails explicitly when box capacity, coordinates or width constraints cannot be satisfied.

The implementation is intentionally scalar and simple. R11 owns measured work-elimination/representation optimisation.

## PPM/P6 image reference codec

R7 proves bounded image decoding with Netpbm P6.

Supported form:

- magic `P6`;
- decimal width/height;
- maximum sample value exactly 255;
- header whitespace/comments;
- maximum dimension 4096 x 4096;
- exactly width × height × 3 RGB source bytes.

Output is caller-owned RGBA8888 byte storage with alpha fixed to 255.

PNG, JPEG, GIF, SVG, animation, colour management and resampling are not claimed by `image.ppm`.

## Hostile-data rule

Malformed external data is not repaired heuristically.

The reference implementation must reject invalid syntax, invalid UTF-8, malformed nesting, numeric overflow, depth overflow, capacity exhaustion, truncated image data and unsupported grammar explicitly.

Sanitizer and regression tests retain reduced hostile-input reproductions.

## Deterministic R7 proof

The proof uses:

```text
fixtures/r7_document.html
fixtures/r7_image.ppm
examples/r7_document_proof.c
```

It verifies one semantic chain:

```text
bounded bytes
 -> byte stream
 -> strict UTF-8
 -> HTML
 -> CSS
 -> link URL
 -> capability requirements
 -> layout boxes
 -> PPM RGBA decode
```

Reference identity:

```text
HTML bytes          = 428
HTML SHA-256        = 0e39a26117b5a8774e3a06e76d00053362ddfacf280f1304df938619e06b0484
HTML FNV-1a64       = 3987ce4034e4f3fc
HTML nodes          = 15
CSS rules           = 5
content capabilities= 5
UTF-8 sample scalars= 12
layout viewport     = 120 px
layout boxes        = 6
document height     = 46 px
layout FNV-1a64     = 0209c1501da9396c
PPM bytes           = 23
PPM SHA-256         = 69d84c9c40bbfe1bfa0519120af54a299af34be4eebb31bb6a34b67aaae22f00
RGBA FNV-1a64       = 8a4318bc590ba10d
link secure         = 1
link port           = 443
```

The fixture hashes bind checkout bytes; the canonical layout hash serializes box fields explicitly rather than hashing native struct padding, so it remains comparable across 32-bit/64-bit and endian targets.

## Non-goals

Document v1 does not introduce:

- network I/O;
- HTTP;
- TLS;
- browser chrome;
- history/bookmarks;
- downloads;
- relative URL resolution;
- generic URL canonicalisation;
- browser HTML recovery;
- HTML entities;
- DOM mutation;
- selectors beyond element selectors;
- complete CSS cascade/inheritance;
- font loading/shaping;
- PNG/JPEG/GIF/SVG;
- forms submission;
- JavaScript;
- accessibility tree;
- incremental layout;
- caches;
- threads;
- SIMD;
- GPU APIs.

Those capabilities must be added by later versioned profiles rather than inferred from R7.
