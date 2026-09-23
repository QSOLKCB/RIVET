# RIVET UI Contract v1

## Claim boundary

UI v1 is the R3 keyboard-first input and lean-menu contract.

It provides only:

- a portable logical key-event representation;
- exact key+modifier bindings to existing command IDs;
- one caller-owned menu projection over existing command IDs;
- keyboard navigation and activation for that menu;
- deterministic menu rendering through GFX v1;
- a tiny built-in uppercase 5x7 bitmap alphabet sufficient for the R3 proof.

It does **not** claim a native keyboard backend, pointer input, text editor, font engine, widget toolkit, command palette, accessibility backend, native menu, or platform window.

Machine-readable identity: `machine/ui-v1.json`.

## ABI

The public header is `include/rivet/ui.h`.

```text
RIVET_UI_ABI_VERSION = 1
language baseline    = C99
```

UI v1 depends on the existing Core v1 command registry and GFX v1 software surface. It does not redefine either contract.

## Logical keyboard events

A key event contains:

- logical key identity;
- exact modifier bitset;
- press/release state.

Printable command keys use ASCII identities from 0x20 through 0x7e.

UI v1 also freezes these logical navigation keys:

```text
RIVET_KEY_UP
RIVET_KEY_DOWN
RIVET_KEY_ENTER
RIVET_KEY_ESCAPE
```

Modifier bits are:

```text
SHIFT
CTRL
ALT
META
```

A future platform adapter is responsible for translating native keyboard events into this logical representation. R3 itself does not claim native keyboard support.

## Keymap projection

A `rivet_keymap` is caller-owned immutable binding data mapping an exact key+modifier pair to a stable Core command ID.

Validation rejects:

- null storage for a non-empty map;
- invalid logical keys;
- unknown modifier bits;
- null/empty command IDs;
- duplicate key+modifier bindings.

Key release does not dispatch a command.

An unmatched key returns `RIVET_ERR_NOT_FOUND`.

A matched press dispatches the existing command through Core v1 and propagates that command result.

The keymap does not copy, rename, wrap, or redefine commands.

## Menu projection

A `rivet_menu` is one presentation of existing command IDs.

Each item contains:

- display label;
- command ID.

The menu stores caller-owned item references and no heap state.

Keyboard behavior while visible:

```text
UP      previous item, wrapping
DOWN    next item, wrapping
ENTER   dispatch selected command
ESCAPE  hide menu
```

Other keys and key releases return `RIVET_ERR_NOT_FOUND`.

When hidden, the menu does not consume command input.

## Presentation independence

The central R3 invariant is:

```text
MENU ITEM != COMMAND
KEY BINDING != COMMAND
PRESENTATION REMOVED != CAPABILITY REMOVED
```

The deterministic R3 proof activates `demo.save` through the menu, hides the menu, then activates `demo.open` through `CTRL+O` using the same Core command registry.

The commands remain addressable independently of the menu presentation.

## Menu rendering

The menu renders through GFX v1 only.

Each row is 11 pixels high:

- 2 pixels top padding;
- 7 pixel glyph height;
- 2 pixels bottom padding.

The built-in glyph contract is deliberately narrow:

- uppercase ASCII `A` through `Z`;
- space.

Unsupported label glyphs return `RIVET_ERR_UNSUPPORTED`.

This is a tiny bitmap presentation aid, not a general font engine.

The caller supplies normal/selected foreground and background colors plus the menu bounds.

Insufficient bounds return `RIVET_ERR_CAPACITY`.

A hidden menu renders as a successful no-op.

## Why there is no command palette yet

The R3 roadmap permits a simple command palette only if it is smaller than alternative duplicated UI logic.

At this phase:

- key bindings already project commands without duplicating command behavior;
- the menu already projects commands without duplicating command behavior.

A palette would add search/text-input state before a real application requires it.

Therefore the second presentation does **not** earn a third abstraction yet.

## Reference vector

The deterministic proof is `examples/r3_ui_proof.c`.

It performs:

1. background raster fill;
2. menu creation for OPEN / SAVE / QUIT;
3. DOWN navigation to SAVE;
4. menu rendering with SAVE selected;
5. ENTER dispatch of `demo.save`;
6. menu removal;
7. direct `CTRL+O` dispatch of `demo.open`.

Reference identity:

```text
surface        = 96 x 48 RGBA8888
selected item  = SAVE
pixel FNV-1a64 = 8e022a6d842ff8e5
PPM SHA-256    = c7feae9354c6934df1b199aa869845e0bae2cf46bb0cabce4e78930095fb21fd
open count     = 1
save count     = 1
quit count     = 0
menu visible   = 0 after command-independence check
```

The PPM freezes the last visible menu frame before presentation removal.

These are conformance/evidence vectors, not performance claims.

## Non-goals

UI v1 does not introduce:

- native keyboard translation;
- pointer/mouse/touch input;
- text editing;
- Unicode text rendering;
- lowercase glyphs;
- font shaping;
- font loading;
- command palette;
- generic widget hierarchy;
- native menus;
- accessibility backend;
- window system;
- image widgets;
- layout engine;
- threads;
- SIMD;
- GPU APIs;
- caches or retained scene graphs.
