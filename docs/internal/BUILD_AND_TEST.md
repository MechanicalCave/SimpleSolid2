# Build, Test and Run — As-built

<!-- doc-id: internal.build-test -->
<!-- document-kind: internal -->

<!-- section-id: internal.build-test.baseline -->
## Toolchain baseline

The repository baseline is:

- C++20;
- CMake 3.24+;
- Qt 6 Widgets;
- Windows-first;
- self-hosted Windows/MSVC PR gate.

OCCT is present in the machine-local environment, but the implemented PART-01 lifecycle does not use OCCT modeling or Viewer functionality.

<!-- section-id: internal.build-test.session -->
## Normal local session

The normal Owner workflow starts with:

```text
START_SS2.cmd
```

The SS2 PowerShell session provides canonical convenience commands including `ss2-status`, `ss2-run`, `ss2-docs` and `ss2-resume`.

`ss2-run` performs an incremental build and starts the current `SimpleSolid2.exe`.

`ss2-docs` regenerates the Product Browser from canonical Markdown.

<!-- section-id: internal.build-test.root-script -->
## Canonical repository commands

The root dispatcher is `ss2.ps1`.

Relevant commands are:

```powershell
.\ss2.ps1 setup
.\ss2.ps1 verify
.\ss2.ps1 configure
.\ss2.ps1 build
.\ss2.ps1 run
.\ss2.ps1 test
.\ss2.ps1 docs
.\ss2.ps1 status
```

Machine-local configuration is written under `.ss2-local/` and is not committed.

<!-- section-id: internal.build-test.tests -->
## Current executable/test gate

The compiled CTest suite contains eighteen tests.

The original ten cover Project metadata, ProjectSession, Recent Projects, Project Hub lifecycle, application smoke startup and Project Hub/Create UX.

Eight PART-01 tests cover:

- DocumentId / DocumentRevision / common properties;
- PartDocument staged transaction semantics;
- native Part persistence and schema rejection;
- DocumentSession command, Undo/Redo, save checkpoint and failed-save behavior;
- Workspace discovery and identity conflicts;
- Project + Part restart lifecycle;
- Project dirty-close guard and Save All;
- real offscreen Qt Part Workspace panel lifecycle.

Repository documentation validation runs outside CTest through `ss2 verify`.

Tests must not be weakened to obtain a pass.

<!-- section-id: internal.build-test.ci -->
## Windows PR gate

The GitHub workflow is:

```text
.github/workflows/windows-pr-gate.yml
```

It runs on the self-hosted Windows runner labeled `simplesolid2-native`.

The gate checks out the exact PR head SHA and then performs:

```text
exact checkout
→ .\ss2.ps1 docs
→ verify generated Browser is Git-clean
→ machine-local setup
→ ss2 verify
→ build
→ CTest
```

The explicit root `ss2.ps1 docs` step is a dispatcher regression check.

<!-- section-id: internal.build-test.docs-validation -->
## Documentation validation

`ss2 verify` invokes `scripts/ss2-docs.ps1 -Check -SelfTest`.

The documentation gate validates canonical pairs/section identifiers, deterministic links, Work Contract Documentation Impact and generated Product Browser freshness.

The Product Browser must be regenerated with `.\ss2.ps1 docs` whenever canonical documentation or its generator changes.
