# Build, Test and Run — As-built

<!-- doc-id: internal.build-test -->
<!-- document-kind: internal -->

<!-- section-id: internal.build-test.baseline -->
## Toolchain baseline

The repository baseline is C++20, CMake 3.24+, Qt 6 Widgets/Test, OpenCASCADE for the native CAD Viewer, Windows-first development and the self-hosted Windows/MSVC PR gate.

OCCT is a required machine-local product-build dependency. Public Viewer contracts remain provider-neutral.

The canonical local environment adds Qt and OCCT runtime DLL directories to PATH so the product and native Viewer tests can execute after build.

<!-- section-id: internal.build-test.session -->
## Normal local session

The normal Owner workflow starts with `START_SS2.cmd`.

The SS2 PowerShell session provides canonical convenience commands including `ss2-status`, `ss2-run`, `ss2-docs` and `ss2-resume`.

`ss2-run` performs an incremental build and starts `SimpleSolid2.exe`. `ss2-docs` regenerates the Product Browser.

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
.\ss2.ps1 test -Tier fast
.\ss2.ps1 test -Tier subsystem -Subsystem sketch
.\ss2.ps1 test -Tier subsystem -Subsystem sketch,viewer,ui
.\ss2.ps1 test -Tier full
.\ss2.ps1 docs
.\ss2.ps1 status
```

Machine-local configuration is written under `.ss2-local/` and is not committed.

<!-- section-id: internal.build-test.tests -->
## Current executable/test gate

The compiled CTest suite contains **58 tests**.

All earlier Project/Hub, Part, Persistence, Workbench/Viewer and Sketch regressions remain active. In particular, the native selection-query test still exercises real Qt/OCCT grip lifecycle/hit testing before and after camera orbit, while the long native Workbench stress test remains FULL-only.

SK-06A adds:

- `sk06a.circle_arc_model_persistence` — mixed Line/Circle/Arc model identity, canonical validation, semantic add/update/delete/history, schema-v4 persistence, Save/load identity preservation and malformed unknown-kind rejection;
- `sk06a.circle_arc_interaction_state` — Circle Center+Radius and Arc Start/Through/End state/preview semantics, CW/CCW and short/long Arc canonicalization, duplicate/collinear failure, mixed frozen-selection Move and owner-only Circle/Arc reshape invariants.

SK-06A also generalizes existing provider/controller paths so the pre-existing native/query/direct-manipulation tests execute against semantic curve presentation and the DPI-aware state-based square grip implementation.

At exact head `518e8f3a5feb56a04d2c2067553d8697e02ceadf`, Windows FULL #440 passed Build and **58/58** unfiltered CTest tests. In that run `tier-fast` contained 57 tests; the long native Workbench stress test remained the sole `tier-full-only` test.

Repository documentation validation runs outside CTest through `ss2 verify`. Tests must not be weakened to obtain a pass.

CI-02 execution tiers remain:

```text
FAST
  explicit label: tier-fast
  → broad short-running regression for frequent iteration
  → excludes the long native Workbench stress test

SUBSYSTEM
  explicit labels: subsystem-core/application/persistence/part/sketch/viewer/ui/project
  → one or more subsystem labels selected as a checkpoint

FULL
  no CTest label filter
  → every registered test, including tier-full-only/native stress coverage
  → mandatory runtime merge evidence
```

The canonical test command defaults to FULL; `-NoBuild` remains a bounded CI optimization after an explicit successful Build. Production UI translation units compile once into `simplesolid2_ui`.

<!-- section-id: internal.build-test.ci -->
## Windows PR gate

The GitHub workflow is `.github/workflows/windows-pr-gate.yml`. CI-01 exact-head DOCS/CLOSURE behavior remains intact; CI-02 adds a bounded FAST runtime iteration tier.

```text
FAST
  draft PR with runtime source changes only
  → exact checkout
  → machine-local setup
  → ss2 verify
  → Build
  → tier-fast CTest only

FULL
  ready-for-review runtime change
  or any tests/CMake/scripts/workflow verification-infrastructure change
  → exact checkout
  → docs dispatcher/freshness
  → machine-local setup
  → ss2 verify
  → Build once
  → full unfiltered CTest with -NoBuild

DOCS
  docs/governance-only suffix
  → exact checkout
  → regenerate Browser and require Git-clean output
  → documentation validation/self-test
  → no CAD build or CTest

CLOSURE
  work/** bookkeeping-only suffix
  → exact checkout
  → documentation/governance validation/self-test
  → no CAD build or CTest
```

Draft status is an iteration signal only. FAST is never accepted as merge evidence. Marking a runtime PR ready for review triggers FULL; every later runtime change on a ready PR also requires FULL.

Changes to `tests/**`, `scripts/**`, any `CMakeLists.txt`, `ss2.ps1`, `ss2.cmd` or `.github/workflows/**` fail closed to FULL even while the PR is draft.

For a PR that already has a successful `windows-msvc-full` job on an exact ancestor SHA, the classifier may still examine only the suffix after that trusted FULL SHA for DOCS/CLOSURE. FAST results never become trusted FULL evidence.

The classifier remains fail-closed. Unknown/mixed verification-sensitive paths select FULL. A stable final `windows-msvc` summary check succeeds only when the selected tier succeeds.

PR title/body edits do not trigger or cancel verification. Content synchronization triggers verification, and `ready_for_review` explicitly triggers the merge-candidate FULL transition.

The explicit root `ss2.ps1 docs` dispatcher/freshness regression remains part of FULL and DOCS. FULL builds once; its Test step uses `-NoBuild` to avoid invoking the same build a second time.
<!-- section-id: internal.build-test.docs-validation -->
## Documentation validation

`ss2 verify` invokes `scripts/ss2-docs.ps1 -Check -SelfTest`.

The documentation gate validates canonical pairs/section identifiers, deterministic links, Work Contract Documentation Impact and generated Product Browser freshness.

The Product Browser must be regenerated with `.\ss2.ps1 docs` whenever canonical documentation or its generator changes.
