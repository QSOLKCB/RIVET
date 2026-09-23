# RIVET GFX Contract v1

## Claim boundary

GFX v1 is the R2 software-surface contract.

It provides only:

- caller-owned RGBA8888 byte surfaces;
- explicit surface validation;
- half-open clipping;
- replace-only rectangle fill;
- transparent-zero 1-bit monochrome glyph/bitmap blit;
- overlap-safe in-place rectangular copy suitable for simple scroll/copy work.

It does **not** claim a window system, native widget toolkit, font engine, image decoder, alpha compositor, GPU path, platform backend, or performance advantage.

Machine-readable identity: `machine/gfx-v1.json`.

## ABI

The public header is `include/rivet/gfx.h`.

```text
RIVET_GFX_ABI_VERSION = 1
language baseline     = C99
pixel bytes           = 4
```

GFX v1 is separate from Core ABI v1. Adding the graphics contract does not redefine `include/rivet/rivet.h` or `rivet.core/v1`.

## Surface memory

A surface is attached to caller-owned bytes.

```text
byte 0 = R
byte 1 = G
byte 2 = B
byte 3 = A
```

This is a byte-order contract, not a host-endian `uint32_t` packing rule.

Surface attachment validates:

- non-null storage;
- non-zero dimensions;
- dimensions representable by the signed coordinate domain;
- at least four bytes per active pixel;
- stride large enough for one active row;
- stride × height without `size_t` overflow;
- buffer size sufficient for the declared stride and height.

GFX v1 allocates no surface memory and requires no heap.

## Coordinates and clip

Coordinates are signed `long`; dimensions are `unsigned long`.

A clip rectangle uses half-open bounds:

```text
[x, x + width)
[y, y + height)
```

`rivet_surface_set_clip()` intersects the requested rectangle with the surface bounds.

A fully excluded clip becomes an empty clip. Drawing into an empty clip is a successful no-op.

Every drawing operation validates the public caller-owned surface state before using it. Caller mutation that makes the surface/clip impossible fails with `RIVET_ERR_INVALID_ARGUMENT`.

## Fill

`rivet_surface_fill_rect()` clips the destination against both the surface and current clip.

Each covered pixel is replaced exactly with the supplied RGBA bytes.

There is no blending in GFX v1.

## Monochrome glyph/bitmap blit

`rivet_surface_blit_mono1()` consumes caller-owned 1-bit rows.

Rules:

- bits are MSB-first within each byte;
- bit 1 replaces the destination pixel with the supplied RGBA color;
- bit 0 is transparent and leaves the destination unchanged;
- source stride and total byte extent are validated before reading;
- destination clipping advances the source bit coordinates rather than changing glyph meaning.

This one primitive is sufficient for the initial built-in bitmap/glyph path. A font engine is not part of R2.

## Copy / scroll

`rivet_surface_copy_rect()` copies within one surface.

Rules:

- source is bounded to the physical surface;
- destination is bounded by both physical surface and current clip;
- destination clipping advances the source origin;
- horizontal overlap uses `memmove` semantics;
- vertical overlap copies bottom-up when required.

The source is read from the physical surface; the current clip constrains the destination only.

This is the only R2 scroll/copy primitive.

## Headless evidence adapter

R2 adds one evidence adapter:

```text
platform/headless/ppm.c
```

It writes the completed surface as binary PPM (P6), discarding alpha only for the evidence file.

This adapter:

- is not a window-system backend;
- does not establish POSIX, Win32, X11, Wayland, Cocoa, or other platform support;
- is not part of the GFX ABI;
- may rely on the host C library for file I/O.

Actual platform presentation remains later roadmap work.

## Reference vector

The deterministic proof is `examples/r2_gfx_proof.c`.

It performs:

1. full-surface background fill;
2. clipped rectangle fill;
3. 5×7 monochrome glyph blit;
4. in-surface copy.

Reference identity:

```text
surface       = 16 x 12 RGBA8888
stride        = 64 bytes
pixel FNV-1a64= 1c0020d75a3b782d
PPM SHA-256   = e525783bbdaaddd3cc193a855cd62b9a259d9074dc32125b79241dab3ed1d7cc
```

The FNV value covers all 768 active RGBA bytes in row-major byte order.

The PPM hash covers the exact P6 file emitted by the headless adapter.

These are conformance/evidence vectors, not performance claims.

## Non-goals

GFX v1 does not introduce:

- dynamic allocation;
- alpha blending/compositing;
- gradients;
- paths;
- antialiasing;
- scaling/filtering;
- image codecs;
- vector fonts or shaping;
- native windows;
- UI widgets;
- input;
- GPU APIs;
- SIMD or threading;
- dirty-region scheduling;
- display lists;
- caches;
- platform compositor integration.
