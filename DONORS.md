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
