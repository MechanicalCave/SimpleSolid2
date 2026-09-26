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

The compiled CTest suite contains 56 tests.

Project/Hub, PART-01 and WB-01 coverage remains active.

PERSIST-01 adds `persist01.native_document_container` coverage for the native ZIP+JSON package boundary, including round-trip read/write, mandatory entries, optional derived entries, unsupported container versions, malformed JSON, unsafe/duplicate ZIP entries, ZIP64 rejection and bounded physical file size.

SK-01 adds `sk01.part_sketch_host` and `sk01.workbench_sketch_host` coverage for Origin-plane support validation, stable SketchId creation, one-entry Undo/Redo restoration, Part schema v2 round-trip plus schema-v1 read compatibility, Sketch Tree presentation, Sketch-plane grid/camera alignment for XY/XZ/YZ, free runtime navigation without authored mutation, Finish Sketch and Save/Close/Reopen identity preservation.

SK-02A adds `sk02a.shared_2d_core` and `sk02a.shared_2d_boundaries` coverage for the host-neutral authored 2D core: finite Sketch-local U/V values, opaque stable model-local EntityId, exact-zero Line rejection without an epsilon policy, equal-coordinate-but-independent endpoints, add/find/erase semantics, identity non-reuse in a continuing model instance, independent value-copy state and dependency-boundary enforcement.

SK-02B adds `sk02b.part_sketch_model`, `sk02b.sketch_entity_lifecycle` and `sk02b.part_sketch_persistence` coverage for Part-owned Shared 2D value state, Add/Erase command history, live identity high-water, schema-v3 strict validation, v1/v2 backward readability and Save/Close/Reopen identity preservation.

SK-03A adds `sk03a.viewer_sketch_contracts`, `sk03a.sketch_viewport_mapping`, `sk03a.part_viewport_controller` and `sk03a.viewer_native_input` coverage for provider-neutral authored/preview scenes, Sketch-frame U/V↔3D conversion, ray→U/V failure behavior, active-Sketch presentation/token bindings, preview non-mutation, fail-closed runtime lifecycle, exclusive primary routing, cursor modes and real Qt/OCCT spatial input before/after camera orbit.

SK-04A adds `sk04a.sketch_interaction_state`, `sk04a.batch_delete` and `sk04a.line_commit_protocol` coverage. These prove neutral Select/Line state transitions, exact-zero suppression, one outstanding Line request, commit-success/failure anchor behavior, hierarchical Esc/Finish/Cancel, EntityId selection/reconciliation, atomic batch Delete validation, one-command/one-transaction/one-Undo semantics and three continuous committed Line segments producing exactly three Undo entries.

SK-04B adds `sk04b.viewer_selection_query_contracts`, `sk04b.part_viewport_selection_bridge` and `sk04b.viewer_native_selection_query` coverage. These prove finite logical query contracts and failure/no-hit distinction, active-Sketch token↔EntityId mapping, stale-token rejection, reverse highlight projection, independent routing/cursor configuration, runtime selection-box behavior, current-view Window/Crossing projected-Line classification, reference/origin/preview exclusion and real Qt/OCCT query behavior before/after orbit.

SK-04C adds `sk04c.part_sketch_interaction_controller` coverage for the single active Sketch interaction coordinator, continuous Line command/Undo granularity, transient preview, point/Ctrl selection, Window/Crossing rectangle selection, atomic Delete, history reconciliation and hierarchical Esc. Legacy Workbench/Sketch-host tests also verify the contextual Part ↔ Sketch Operations state.

SK-05A adds `sk05a.direct_manipulation_state`, `sk05a.line_geometry_command` and `sk05a.part_sketch_direct_manipulation`. Native grip/query coverage also extends `sk04b.viewer_native_selection_query`. Together they prove additive selection, semantic primary state, runtime hover/grips, owner-only endpoint Reshape, frozen-selection center Move, preview-only manipulation, atomic geometry commit/history and Save → Close → Reopen identity preservation.

WB-01B strengthens the native Windows overlay path without changing CAD semantics: the real Qt/OCCT selection query test repeatedly exercises OCCT-native rubber-band show/update/clear, the responsive ViewCube test verifies native-child composition, and the real Workbench stress test verifies that ViewCube is hosted as a native child of the native viewport. These automated checks prove lifecycle/non-regression mechanics but not the absence of visual framebuffer artifacts. WB-01B therefore also requires explicit manual visual verification on the exact Windows implementation build before closeout.

WB-01A adds or extends deterministic coverage for:

- empty-space selection clear and right-click no-op semantics;
- native provider detection/selection cleanup during scene replacement;
- recoverable provider exception containment;
- 100-iteration real Windows Workbench/Qt/OCCT stress behavior;
- repeated Origin Show/Hide and Undo/Redo;
- repeated Pan/Orbit/Zoom mixed with presentation refresh;
- responsive ViewCube geometry in wide and narrow Editor Surface layouts;
- Workspace folder browsing and explicit Create Folder;
- fail-closed Workspace path validation;
- neutral Open Document candidates and canonical DocumentSession reuse.

`wb01.viewer_native_smoke` runs without the offscreen Qt platform and validates basic real Qt/OCCT initialization/navigation.

`wb01a.workbench_native_stress` also runs natively. It exercises a real `CadWorkbench` with the real Qt/OCCT provider for 100 representative stress iterations.

Repository documentation validation runs outside CTest through `ss2 verify`.

Tests must not be weakened to obtain a pass.

CI-02 classifies CTest execution separately from the CI-01 exact-head documentation/closure tiers:

```text
FAST
  explicit label: tier-fast
  → broad short-running regression for frequent iteration
  → currently excludes the long native Workbench stress test

SUBSYSTEM
  explicit labels: subsystem-core/application/persistence/part/sketch/viewer/ui/project
  → one or more subsystem labels selected as a checkpoint
  → tests may belong to multiple subsystems

FULL
  no CTest label filter
  → every registered test, including tier-full-only/native stress coverage
  → mandatory runtime merge evidence
```

The canonical test command defaults to FULL. FAST and SUBSYSTEM are explicit:

```powershell
.\ss2.ps1 test -Tier fast
.\ss2.ps1 test -Tier subsystem -Subsystem sketch
.\ss2.ps1 test -Tier subsystem -Subsystem sketch,viewer,ui
.\ss2.ps1 test -Tier full
```

`-NoBuild` is a bounded CI optimization for a Test step that immediately follows a successful explicit Build. Normal local test commands still build incrementally before CTest. `-ListOnly` validates a selector without executing tests.

Production UI translation units are compiled once into `simplesolid2_ui`. The application and UI-oriented tests link that production library instead of compiling independent copies of the same `src/ui/*.cpp` implementation files.

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
