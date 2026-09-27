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

The repository currently registers **74 CTest tests**.

All earlier Project/Hub, Part, Persistence, Workbench/Viewer and Sketch regressions remain active. The long native Workbench stress test remains FULL-only.

The current transform/COPY regressions include:

- `sk07a.transform_core` and `sk07a.move_controller` — provider-independent mixed translation plus selection-first/command-first MOVE;
- `sk07b.transform_core`, `sk07b.common_transform_state` and `sk07b.transform_controller` — Rotate/positive uniform Scale/Mirror core, interaction and controller semantics;
- `sk07c.copy_interaction_state` — normal COPY state, frozen source/Base Point and repeated placement;
- `sk07c.copy_command` — atomic mixed duplication, fresh/non-reused EntityIds, Undo/Redo and schema-v4 high-water persistence;
- `sk07c.copy_controller` — zero-placement rejection, repeated independent placements, source-selection retention and stale-revision failure.
- `sk07d.repeat_last_command_controller` — one runtime repeat target, repeatable command recording, current-selection reuse, fresh COPY restart, non-overwrite by Select/grip/history/Delete and reset across Sketch edit sessions.
- `sk07e.space_cycle_edit_mode` — supported grip-role defaults/cycles, frozen selection/primary preservation, owner-vs-selection interaction-start geometry and non-compounding Reshape↔Move preview.

The existing `sk01.workbench_sketch_host` regression covers the grouped Select/Create/Modify surface, toolbar/Command-Line adapters including COPY, viewport Enter/Space Repeat Last Command routing, active-transform precedence, active-grip Space CycleEditMode precedence and text-focus keyboard behavior. It also checks that pointer movement preserves a token inside one PointRequest, replacing the request or finishing Sketch clears stale input without authored mutation, and non-empty-buffer Delete cannot fall through to geometry deletion. The existing `sk05a.part_sketch_direct_manipulation` controller regression additionally verifies preview cardinality, no-mutation cycling and atomic commit after Reshape↔Move switching.

`wb02.cad_input_session`, `wb02.cad_input_boundaries` and `wb02.global_cad_input_ui` cover provider-neutral CAD-input transport, dependency boundaries, semantic-generation lifetime, foreign-window/popup ownership, two-visible-shell isolation, ordinary text-editor ownership, application-shortcut preservation and fixed Command Line geometry/diagnostic behavior.

`b1.save_conflict` covers exact native-file checkpoints, sequential two-session lost-update prevention, cooperative Save guard ownership, content-change/replacement/DocumentId/missing-target conflicts and repeated successful Save.

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
<!-- section-id: internal.build-test.c2-history-benchmark -->
## C2 semantic history benchmark

The semantic history benchmark is maintainer evidence tooling and is opt-in. `SS2_ENABLE_C2_HISTORY_BENCHMARK` defaults to `OFF`, so ordinary product and CTest builds do not include the benchmark target.

Canonical Release evidence is run with:

```powershell
.\scripts\ss2-history-benchmark.ps1 `
  -BuildDir build\c2-history-benchmark `
  -Config Release `
  -OutputDir artifacts\c2-history-benchmark `
  -Samples 20 `
  -Warmup 3 `
  -MaxWorkingSetMB 4096 `
  -CellTimeoutMinutes 5
```

The runner executes semantic `DocumentSession` operations without Qt/Viewer/OCCT rendering in the timed path. It uses a working-set ceiling of the minimum of 25% physical RAM and 4 GiB, a five-minute per-cell timeout and BelowNormal child-process priority. These are benchmark-machine safety controls, not Product history limits.

Current C2 evidence is summarized in [History Performance](HISTORY_PERFORMANCE.md) and in the [C2 benchmark decision record](../../work/C2_HISTORY_BENCHMARK_RESULTS.md).
<!-- section-id: internal.build-test.docs-validation -->
## Documentation validation

`ss2 verify` invokes `scripts/ss2-docs.ps1 -Check -SelfTest`.

The documentation gate validates canonical pairs/section identifiers, deterministic links, Work Contract Documentation Impact and generated Product Browser freshness.

The Product Browser must be regenerated with `.\ss2.ps1 docs` whenever canonical documentation or its generator changes.
