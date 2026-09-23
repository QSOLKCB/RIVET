# RIVET Architecture v2

## Core idea

RIVET separates four questions that modern application stacks often collapse together:

1. What does the application mean?
2. What capabilities does it require?
3. How are those capabilities presented?
4. Which machine and operating environment provide them?

```text
+--------------------------------------------------+
| APPLICATION                                      |
| domain state + application semantics             |
+--------------------------+-----------------------+
                           |
                           v
+--------------------------------------------------+
| COMMAND + CAPABILITY CONTRACT                    |
| stable operations + required/optional services   |
+--------------------------+-----------------------+
                           |
                           v
+--------------------------------------------------+
| RIVET PORTABLE CORE                              |
| bytes / text / event / command / config / stream |
+-----+----------------+----------------+----------+
      |                |                |
      v                v                v
+-----------+    +------------+   +------------+
| UI MODEL  |    | DOCUMENTS  |   | SERVICES   |
+-----------+    +------------+   +------------+
      |                |                |
      +----------------+----------------+
                       |
                       v
+--------------------------------------------------+
| PLATFORM ADAPTER                                 |
| window / surface / input / time / files / net    |
+--------------------------+-----------------------+
                           |
                           v
                 OS/API + CPU + compiler
```

## Authority

Application semantics are authoritative for application behaviour.

The platform adapter supplies machinery. The renderer supplies pixels. A menu supplies one presentation of commands.

None may silently redefine the application.

## Command model

Commands are stable, addressable capabilities. Illustrative IDs:

```text
document.open
document.save
document.save_as
view.source
view.reload
window.split_horizontal
network.proxy.configure
browser.javascript.disable
browser.cookies.inspect
history.export
```

One command can project into a menu item, toolbar control, keyboard binding, command palette, CLI, accessibility surface, or optional script binding.

The projection may change. The command identity remains explicit.

## Capability model

Applications declare required and optional capabilities. Illustrative IDs:

```text
surface.raster
input.keyboard
input.pointer
filesystem.read
filesystem.write
clipboard
timer.monotonic
network.tcp
network.http
network.tls
audio.pcm
threads
simd
document.html
document.css
script.javascript
```

A target advertises what it actually provides. RIVET determines whether the application can execute and which optional features are available.

## Platform and CPU are separate axes

```text
target identity =
    OS/API
  + CPU architecture
  + ABI
  + compiler/toolchain
  + declared capability set
```

Examples include Win32+x86, POSIX+x86_64, classic Mac OS+m68k, classic Mac OS+PowerPC, and AmigaOS+m68k.

The same CPU family may host very different platform capabilities.

## Graphics

The canonical graphics architecture is a CPU-produced software surface.

RIVET targets pixels, not GPUs. There is no RIVET GPU capability and no planned OpenGL, Vulkan, Direct3D, Metal, WebGL, WebGPU, shader, or GPU-compute rendering backend.

The host OS/window system is free to accelerate the final blit or compositing operation internally. That implementation detail is below the RIVET boundary.

Initial primitives should stay small: surface attachment/allocation, clipping, fill, line where justified, bitmap/glyph blit, image blit, and copy/scroll region.

UI widgets should generally draw onto this surface instead of requiring every platform port to implement an entire native widget family.

Performance work starts by reducing repaint/layout/materialisation work. SIMD or bounded threading may be considered only after measurement and reference-path conformance.

Optional native dialogs/services may exist behind capabilities.

See [RENDERING-v1.md](RENDERING-v1.md).

## UI

Widget scope is earned by real applications, not by framework fashion.

Likely early primitives are label, button, toggle, text input, list, scrollbar, menu, splitter, and tabs only when needed.

No component library is added because an architecture diagram has an empty box.

## Text

Canonical internal text should use UTF-8.

Platform adapters translate to host encodings where required.

A minimal built-in bitmap-font path is desirable as the portability baseline. Sophisticated font shaping/rasterisation is optional.

## Event model

The baseline runtime is single-threaded and event-driven.

Minimum event classes: startup/shutdown, keyboard, pointer when provided, resize/expose, timer, explicit service completion, and application-defined events.

Concurrency may improve throughput but may not be required merely to open a window and respond to input.

## Runtime and memory doctrine

RIVET separates **result identity** from **execution-plan identity**.

Where declared semantics are invariant, every successful execution for the same semantic inputs and declared profile must preserve the same correctness result across worker count, chunk size, memory budget, cache shape, or optional CPU optimization. An execution may instead fail explicitly with resource exhaustion when its declared budget cannot satisfy the minimum representation.

```text
RESULT IDENTITY != EXECUTION PLAN IDENTITY
BENCHMARK OBSERVATION != CORRECTNESS IDENTITY
```

Runtime work should prefer:

- explicit resident-memory budgets rather than allocate-until-failure behaviour;
- deterministic bounded chunking when work exceeds the resident budget;
- procedural regeneration instead of retaining reconstructible state;
- stream -> consume/reduce -> discard for transient representations;
- reference-path parity before optimized-path promotion;
- reusable state only when bound to complete effective-input identity.

The runtime does **not** pre-create scheduler, executor, cache, plugin, or backend hierarchies for hypothetical future use. A second real implementation must create the need before a general abstraction is introduced.

Exact donor sources and adoption boundaries are recorded in [RUNTIME-PLAN-v1.md](RUNTIME-PLAN-v1.md) and [DONORS.md](DONORS.md).

## Storage

RIVET favours explicit, versioned, inspectable state.

Native struct dumps are forbidden as portable formats.

Portable binary formats require magic/version, explicit widths, byte order, lengths/bounds, validation, and defined upgrade/rejection behaviour.

## Networking

Networking is optional.

The first core boundary should expose streams/capabilities rather than make one TLS implementation universal.

A future RIVET Relay may provide modern transport services to machines that cannot implement contemporary TLS/HTTP locally. The relay must not quietly become remote rendering: selected profiles should still parse, lay out, and render locally.

## Browser boundary

```text
apps/browser
   |
   +-- URL
   +-- HTTP
   +-- HTML
   +-- CSS
   +-- layout
   +-- image
   +-- history
   +-- bookmarks
   +-- downloads
   +-- optional scripting
```

Browser-specific complexity does not automatically enter `core/`.

## Intended source shape

```text
include/rivet/
core/
gfx/
ui/
net/
web/
platform/
arch/
adapters/
apps/
tests/
evidence/
machine/
```

Directories are created when they contain needed code, not in advance to make the project look larger.

## Version identities

Software release version, ABI version, capability-contract version, and document/browser contract versions are separate.

A release bump does not automatically redefine every contract.
