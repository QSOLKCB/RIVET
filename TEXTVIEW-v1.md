# RIVET Text Viewer Proof Contract v1

## Role

R4 proves that RIVET can host a useful non-browser application.

The text viewer is deliberately **application-specific**:

```text
apps/textview/
```

It consumes frozen Core v1, GFX v1 and UI v1. It does not add a new framework ABI or redefine those contracts.

Machine-readable identity: `machine/textview-v1.json`.

## Scope

The application provides:

- bounded open from caller-owned bytes;
- caller-owned line-index storage;
- exact forward search with one wrap;
- line and page navigation;
- deterministic read-only rendering;
- key-bound commands through UI v1;
- a bounded headless stdio file adapter for the executable proof.

It is a read-only viewer, so save/export of document content is intentionally not part of R4.

## Input bytes

R4 does not introduce the R7 text-decoding/document layer.

The proof viewer accepts only these frozen byte identities:

- LF `0x0a`;
- space `0x20`;
- `-` through `:` as numeric ASCII bytes `0x2d..0x3a`;
- uppercase `A..Z` as numeric ASCII bytes `0x41..0x5a`.

That set covers uppercase text, digits and the small punctuation vocabulary used by the proof application.

Unsupported document bytes return `RIVET_ERR_UNSUPPORTED`.

These are numeric ASCII bytes independent of the C implementation's execution character set.

## Memory model

The viewer allocates no heap.

The caller supplies:

- document bytes;
- line-offset array and capacity;
- search query bytes;
- rendering surface.

Open first validates/counts the whole document and required line count. If the caller's line-index capacity is too small, open fails with `RIVET_ERR_CAPACITY`.

Document bytes remain caller-owned and must outlive the viewer.

## Navigation

`top_line` identifies the first rendered line.

Operations:

- line up;
- line down;
- page up by a caller-declared visible-row count;
- page down by a caller-declared visible-row count.

Navigation saturates at the first/last line rather than wrapping.

## Search

`rivet_textview_find_next()` performs exact byte search.

Rules:

- query is non-empty;
- query bytes must belong to the supported non-newline display subset;
- search begins after the current match, otherwise at the current top line;
- one wrap to the beginning is permitted, but the current match is not reported again when it is the only occurrence;
- a found match records exact byte offset/length and moves `top_line` to the containing line;
- a miss returns `RIVET_ERR_NOT_FOUND` and preserves the previous match.

No case folding, regex, Unicode normalization or hidden index/cache is introduced.

## Rendering

The application renders through GFX v1.

It uses an application-owned 5x7 glyph table for the bounded R4 byte vocabulary. This table is not promoted into a RIVET font API.

Rendering:

- requires the viewport bounds to be wholly inside the validated surface;
- fills the viewport background;
- draws rows at an 8-pixel pitch;
- truncates horizontally at the viewport edge;
- highlights matched bytes using caller-supplied colors.

## Headless open adapter

The proof uses:

```text
platform/headless/text_file.c
```

This adapter reads into a caller-supplied fixed buffer and fails if capacity is exhausted.

It is proof/application infrastructure only. It does not establish a RIVET filesystem or platform backend before R5.

## Reference proof

The deterministic proof is `examples/r4_textview_proof.c`.

It:

1. opens `fixtures/r4_textview.txt` through the bounded headless reader;
2. creates a caller-owned line index;
3. registers UP / DOWN / FIND commands in Core v1;
4. projects those commands through UI v1 key bindings;
5. moves down two lines;
6. searches for numeric-ASCII `PIXELS`;
7. moves the viewport to the containing line;
8. renders the viewer and highlighted match through GFX v1;
9. writes a PPM evidence frame.

The exact pixel/PPM identities are measured by branch CI before PR publication and then frozen into this v1 contract.

## Non-goals

R4 does not introduce:

- browser/network semantics;
- HTML/CSS;
- generic text decoder;
- Unicode;
- lowercase rendering;
- text editing;
- document mutation;
- save/export of document content;
- filesystem abstraction;
- native window/input backend;
- generic font API;
- layout engine;
- image codec;
- heap allocation;
- threads;
- SIMD;
- GPU APIs;
- caches or search indexes.
