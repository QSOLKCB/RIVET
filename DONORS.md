# RIVET Donor Map

RIVET is a new project. Existing QSOL repositories provide bounded mechanisms, lessons, and evidence patterns; they do not automatically become RIVET dependencies or authorities.

## Primary donors

### QSOL-ARK

Useful mechanism:

- Minimum Recoverable Substrate;
- capability sets are explicit and not implicitly inherited;
- C99/offline-browser/constrained-execution portability ladder;
- computational archaeology;
- machine-readable recovery tiers.

RIVET adaptation:

- **Minimum Execution Substrate (MES)**;
- explicit evidence classes;
- portability as assumption budgeting.

### GAMES / Ternary Drift

Useful mechanism:

- platform-neutral C99 application core;
- one small Win32 platform file;
- software framebuffer;
- `waveOut`;
- no SDL/Electron/browser runtime in the native build;
- hard package-size budget;
- authoritative simulation separated from presentation/timing.

RIVET adaptation:

- simple portable core + thin platform adapter;
- software rendering baseline;
- measured size as a contract.

### PSYCLE-LINUX / C-Psycle preservation corpus

Useful mechanism:

- explicit UI/platform bridge;
- audio/UI separation;
- compatibility-first preservation;
- original behaviour as oracle;
- platform-specific implementation behind stable interfaces.

RIVET adaptation:

- platform adapters do not redefine application semantics;
- preservation evidence outranks redesign convenience.

### QSOL-MORPH

Useful mechanism:

```text
semantic program != machinery backend
```

RIVET adaptation:

```text
application meaning != execution machinery
```

### QSOL-MESH

Useful mechanism:

- workload meaning separated from execution placement;
- explicit fallback identity;
- deterministic reference verification;
- memory/resource planning.

RIVET adaptation:

- target machinery selected by capability;
- execution backend does not redefine application semantics.

### OPT

Useful mechanism:

- correctness outranks speed;
- keep reference path;
- bounded parallelism;
- working-set reduction;
- compact representations;
- performance budgets;
- measured host-aware promotion.

RIVET adaptation:

- optimisation only after semantic conformance;
- resource claims require evidence.

### RSH

Useful mechanism:

- software version, ABI version, semantic contract version kept separate;
- cross-runtime conformance;
- backend identity/fallback explicit.

RIVET adaptation:

- separate release/ABI/capability/document profile identities.

### GLUBALL / GALAXY

Useful mechanism:

- browser/native separation;
- bounded resident work;
- deterministic receipts;
- portable reference boundaries;
- browser visualisation not semantic authority.

### QSOLAI

Useful mechanism:

- machine-enforced architectural constraints;
- deterministic rejection of implementations that violate declared environment constraints.

RIVET adaptation:

- agent rules should actively reject stack creep, hidden runtime dependencies, and semantic duplication.

### QSOL-CONTROL

Useful mechanism:

- framework-free browser UI;
- display surface does not become authority;
- local-first/offline-conscious operator design.

### QSOL-BLUE-FORGE

Useful mechanism:

- hostile input -> violated invariant -> general mitigation -> permanent regression;
- do not merely blacklist one exploit string.

RIVET adaptation:

- parsers and network/document surfaces should be hardened by invariant, not payload whack-a-mole.

## Import rule

Before copying donor code:

1. record repository/ref/path;
2. record licence;
3. explain why reuse is smaller/better than clean implementation;
4. identify semantics being imported;
5. add conformance tests;
6. preserve notices where required.

Conceptual inspiration alone does not justify source copying.

## Exact donor sources for the runtime plan

The runtime plan borrows mechanisms from the following concrete records. Performance numbers and tuning constants do **not** transfer automatically.

- **GALAXY v0.6.0 — Stream -> Reduce -> Discard (fa1c76f):** <https://github.com/QSOLKCB/GALAXY/commit/fa1c76fb49664ae4cdd6dc090cccd702399c2b60> — transient-state elimination, bounded microtiles, retained negative optimization evidence.
- **GALAXY v0.5.0 (b2e8603):** <https://github.com/QSOLKCB/GALAXY/commit/b2e860309a04d7591c86f71d2b4ab1e5eec4c4d7> — canonical reference path, bounded worker-local state, guarded/measured CPU-path promotion.
- **GALAXY v0.4.0 (6f17a73):** <https://github.com/QSOLKCB/GALAXY/commit/6f17a734b9241359d36a9bf3d208b8527a456327> — logical scale separated from resident scale; topology/evidence accounting.
- **OPT v1.0.0 (41e2fc3):** <https://github.com/QSOLKCB/OPT/commit/41e2fc3677469839fb298dedefd0ce72caebcc68> — correctness-preserving optimization, bounded parallelism, reference equivalence.
- **OPT v1.1.0 (10c2e27):** <https://github.com/QSOLKCB/OPT/commit/10c2e27075b9174ef2f2d586c281dc1da45588b5> — explicit equivalence witnesses for reuse/parallelism.
- **OPT v1.2.0 (3441c6c):** <https://github.com/QSOLKCB/OPT/commit/3441c6ceacd4a6ca43ec46de758e2d5bc23d8ca7> — incremental execution, coalescing, shared materialization, early working-set reduction and measured CPU optimization patterns.
- **OPT-INC-001:** <https://github.com/QSOLKCB/OPT/blob/3441c6ceacd4a6ca43ec46de758e2d5bc23d8ca7/optimizations/OPT-INC-001-signature-bound-incremental-execution.md> — complete effective-input identity before reuse.
- **OPT-COAL-001:** <https://github.com/QSOLKCB/OPT/blob/3441c6ceacd4a6ca43ec46de758e2d5bc23d8ca7/optimizations/OPT-COAL-001-concurrent-duplicate-work-coalescing.md> — duplicate-work sharing only when caller semantics survive.
- **OPT-FAN-001:** <https://github.com/QSOLKCB/OPT/blob/3441c6ceacd4a6ca43ec46de758e2d5bc23d8ca7/optimizations/OPT-FAN-001-shared-materialization-fanout.md> — validate and consume the same immutable/versioned artifact.
- **OPT-REDUCE-001:** <https://github.com/QSOLKCB/OPT/blob/3441c6ceacd4a6ca43ec46de758e2d5bc23d8ca7/optimizations/OPT-REDUCE-001-early-working-set-reduction.md> — move/reduce work only under semantic equivalence.
- **QSOL-MESH PR #3 head e38fa71:** <https://github.com/QSOLKCB/QSOL-MESH/commit/e38fa71f1d8c1489bf4adf3538f2716c531e86a9> — explicit algorithms over infrastructure; no executor hierarchy before a second executor earns it.
- **IGM execution campaigns:** <https://github.com/QSOLKCB/igm/blob/9b2df317a463f53ebb9e71b9a3bac0610a8ba5c6/docs/EXECUTION_CAMPAIGNS.md> and <https://github.com/QSOLKCB/igm/tree/9b2df317a463f53ebb9e71b9a3bac0610a8ba5c6/runtime> — explicit memory budgets, deterministic chunking, correctness identity independent of workers/chunks/budgets.
- **Ternary Drift design:** <https://github.com/QSOLKCB/GAMES/blob/0ef476a5296faf008bcd0e25d0b080f608af47e3/TERNARYDRIFT/docs/DESIGN.md> — platform-neutral C99 semantics with a thin Win32 boundary.
- **C64 architecture:** <https://github.com/QSOLKCB/C64/blob/e8875f2f6d0872a94094381e054c27fba3894c0a/docs/ARCHITECTURE.md> — simple experience layer over mature machinery; advanced capability remains reachable.
- **GLUBALL v1.0.0:** <https://github.com/QSOLKCB/GLUBALL/releases/tag/v1.0.0> — frozen contract identity, sealed vectors and compact receipts.
- **QEC v170.2.0 / v170.2.1:** <https://github.com/QSOLKCB/QEC/releases/tag/v170.2.0> and <https://github.com/QSOLKCB/QEC/releases/tag/v170.2.1> — source+capability identity, immutable validated bytes, layered evidence without rewriting older identities.
- **QSOL-CONTEXT:** <https://github.com/QSOLKCB/QSOL-CONTEXT> — selective loading rather than all-state residency.
- **QSOL-FLOW:** <https://github.com/QSOLKCB/QSOL-FLOW> — smallest complete representation sufficient for the current task; conceptual donor only.
- **QNTOY v1.0.0:** <https://github.com/QSOLKCB/QNTOY/releases/tag/v1.0.0> — low-RAM/offline precedent.

See [RUNTIME-PLAN-v1.md](RUNTIME-PLAN-v1.md) for when each mechanism is allowed to enter RIVET.
