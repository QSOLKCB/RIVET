# RIVET Rendering Contract v1

## Rule

> **RIVET targets pixels, not GPUs.**

The canonical RIVET rendering result is a CPU-produced software pixel surface.

This rule applies equally to historical and modern systems. A modern machine does not gain a separate GPU-oriented RIVET architecture merely because a GPU is present.

## Canonical pipeline

```text
APPLICATION / DOCUMENT STATE
            |
            v
        RIVET UI
            |
            v
   LAYOUT + PAINT DECISIONS
            |
            v
   CPU SOFTWARE RASTERIZER
            |
            v
      PIXEL SURFACE
            |
            v
     PLATFORM PRESENT
            |
            v
          DISPLAY
```

RIVET owns the pipeline through the completed pixel surface.

The host platform owns presentation after that boundary.

## GPU exclusion

RIVET does not provide or target:

- OpenGL;
- Vulkan;
- Direct3D;
- Metal;
- WebGL;
- WebGPU;
- CUDA or GPU compute;
- shader-language rendering;
- GPU-specific scene graphs;
- GPU-specific texture/resource ownership;
- a second GPU renderer that duplicates software-renderer semantics.

This is not a temporary R0 omission. It is a constitutional architectural boundary.

## Host compositor boundary

An operating system, display server, window manager, driver, emulator, or compositor may internally use GPU acceleration to copy, scale, composite, or display the completed RIVET surface.

That does not make RIVET GPU-accelerated and does not create a GPU requirement.

```text
HOST USES GPU         != RIVET TARGETS GPU
PRESENTATION MECHANISM != RIVET PIXEL SEMANTICS
```

RIVET should not require knowledge of the host's acceleration mechanism.

## Why

The portability target spans machines for which a common GPU API either does not exist or would multiply platform code dramatically.

The rendering problem RIVET initially needs to solve is dominated by:

- text;
- rectangles;
- borders;
- images;
- clipping;
- scrolling;
- selection;
- document paint;
- application chrome.

These are valid CPU/software-rendering workloads.

Modern CPUs provide substantially more execution capacity than the older CPUs RIVET also intends to support. RIVET therefore treats modern CPU capacity as margin for the same simple architecture rather than justification for a second rendering stack.

## Performance doctrine

Performance work begins by reducing work.

Preferred order:

1. avoid repaint when nothing changed;
2. track dirty/damaged regions;
3. relayout only affected content;
4. bound decoded images and working sets;
5. reuse glyphs and safe computed results;
6. use compact display/layout representations;
7. measure;
8. consider optional CPU SIMD or bounded threading only when evidence justifies the added code.

```text
DO LESS BEFORE DOING IT FASTER
```

No amount of benchmark pressure turns a GPU renderer into a required roadmap step.

## Reference path

The scalar software renderer remains the semantic reference.

Optional CPU-side optimisations must preserve declared output semantics and retain a conformance path to the reference implementation where practical.

## Browser consequence

The RIVET Browser must be designed around bounded document/layout/paint work rather than assuming hardware compositing.

Initial browser profiles should prefer:

- bounded style/layout state;
- visible-region work;
- dirty-region painting;
- explicit image budgets;
- user control over expensive features;
- no JavaScript requirement for WEB1;
- no GPU-dependent web feature requirement.

A web profile that fundamentally requires a GPU is outside RIVET and cannot become a RIVET Browser requirement, capability, or conformance profile.

## Specialised applications

An application may integrate external GPU code outside the RIVET rendering contract.

Such code:

- is not a RIVET rendering capability;
- may reduce that application's portability;
- must not become required by the RIVET core, UI, document engine, or Browser;
- must not claim conformance to a RIVET software-rendering profile unless that profile actually executes.

## Evidence

Rendering evidence should identify at least:

- source revision;
- target identity;
- raster profile/version;
- surface dimensions and pixel format;
- test vector/workload;
- output hash or other declared observable;
- binary/working-set evidence when claimed;
- whether optional CPU optimisations were active.

GPU model is not part of canonical RIVET rendering identity.
