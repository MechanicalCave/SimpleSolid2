# PG-01E/E0 — Preliminary Project Geometry evidence and gap inventory (READ-ONLY DRAFT)

**Status:** **PRELIMINARY / NOT E0 FINAL PASS / NOT AN ACTIVATED PG-01E IMPLEMENTATION**  
**Prepared:** 2026-10-13  
**Inspected baseline:** main `5b495c4bbed8754d7a1f01e87a30716429db786d`. Accepted roadmap `work/PART_MODELING_V1_ROADMAP.md` **v1.32**, active work remains completed `RG-02A/G0`.  
**Owner decisions:** PG-01E is NEXT, deferred G1 is not a blocker; PG-01E D2-E1–E6 accepted only as direction to prepare bounded Work Contract (see [PR #311](https://github.com/MechanicalCave/SimpleSolid2/pull/311)). Exact proposed Work Contract is in separate [Draft PR #337](https://github.com/MechanicalCave/SimpleSolid2/pull/337). Its text, merges and activation remain unapproved.  
**This document's scope:** independent read-only source/test/accepted-evidence inspection. Does not execute new PG-01E runtime tests, demonstrate cold composite PG-01E cross-feature coverage, correct product docs or authorize code changes.

## E0-A. Accepted prior packages: distinguish prior verification from new PG-01E requirements

| Stage and existing scope | Immutable past evidence | Current test/source candidate inspected | Newly required PG-01E evidence |
| --- | --- | --- | --- |
| PG-01A — exact Kernel OCCT Line/Circle/Arc projection and fail-closed boundaries | [PG-01A evidence](PROJECTION_01A_KERNEL_ACCEPTANCE_EVIDENCE.md), [merged PR #300](https://github.com/MechanicalCave/SimpleSolid2/pull/300) squash `27268d4ec10fbc09721a16ab8f0e2d59bb54833f`; final native FULL stated **195/195 PASS** | Existing `tests/kernel_native/pm02p_e_kernel_lifecycle_test.cpp` calls real projection query, typed degenerate/unsupported/current provider checks; original specific FOCUSED records in linked evidence | **OPEN:** fresh exact G1-independent PG-01E cross-Feature source capture and one new combined linked consumer chain |
| PG-01B — authored linked Sketch material Edge source, effective Sketch/Profile, dependent Feature, atomic Break Link, native cold Save/Reopen | [PG-01B contract](PROJECTION_01B_ASSOCIATIVITY_CONTRACT.md), [merged PR #301](https://github.com/MechanicalCave/SimpleSolid2/pull/301) squash `8d78320daca8a72a1431fbece49296c3bfb83aec`; detailed iteration source [progress](PROJECTION_01B_PROGRESS_EVIDENCE.md) | Existing `tests/kernel_native/pm05e3_edge_feature_cold_rebuild_test.cpp` has actual native CreateProjectedSketchEdgesCommand, current dependent Cut, restored linked EntityIds after Save/Close/Reopen, separate OCCT provider and broken source. Original progress notes include transient/red runs: do not flatten them to an uninterrupted PASS | **OPEN:** PG-01E combined Edit/Undo/Redo/Broken+Repair/Break Link on a single current v16/modeling-v2 Part history with the selected Face capture modes |
| PG-01C — user-facing Edges source picking, current linked Profile/Feature, Tree and multi-selection Break Link | [PG-01C final acceptance](PROJECTION_01C_FINAL_ACCEPTANCE_AND_MERGE.md), [merged PR #304](https://github.com/MechanicalCave/SimpleSolid2/pull/304) squash `d928e8a5d1f5893b73de3e4fcd399d7e0483af83`, accepted full Windows **196/196 PASS** | Existing `tests/pm05f_r2_native_workbench_edit_test.cpp` exercises native Qt/OCCT Project Geometry, stage validation, exact linked Current Edge, some Break Link and cold reopen paths | **OPEN:** audited requirement-by-requirement coverage for all three accepted modes, not an invented per-gesture manual acceptance transcript |
| PG-01D — Planar Face/Face Boundary, manual multi-Face per-member partial support, v16/modeling-v2 strict rejection, Refine after Add | [PG-01D final acceptance](PROJECTION_01D_FINAL_ACCEPTANCE_AND_MERGE.md), [merged PR #309](https://github.com/MechanicalCave/SimpleSolid2/pull/309) squash `5adc94f1463ad1c5b0785b3027dc5d93f830a112` plus closure [PR #310](https://github.com/MechanicalCave/SimpleSolid2/pull/310). Exact-feature FULL [#38136149906](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38136149906), **25 core + 57 kernel + 117 desktop = 199/199 PASS** | Existing `tests/pm05f_r2_native_workbench_edit_test.cpp` contains actual Face Boundary wires + selection, geometric skipped cases and commands; `tests/part_document_store_test.cpp` explicitly checks typed `unsupported_modeling_semantics` and persisted v16/v2; PL/EN current Product docs describe the modes | **OPEN:** one PG-01E exact-head replay combining Face Boundary + linked Profile later Feature and v2 true cold Save/Reopen with a fresh kernel, plus acceptance matrix and performance data |

**Interpretation:** actual earlier package FULL PASS is evidence for accepted **earlier** behavior. No fresh PG-01E FOCUSED, FAST or FULL has been executed for this report. A single PASS of a large test executable does not prove every individual proposed D2-E2 chain, and a current test source location does not constitute new run evidence.

## E0-B. Confirmed specific canonical documentation drift

**Finding E0-DOC-01 — stale internal current-state statements**, directly visible on merged `main`:

- `docs/internal/PART_DOCUMENTS.md`, approximately **line 34**, says that "**planar Face boundary capture remains outside this tool**" when discussing the first PG-01C Project Geometry Edges UI. That statement is accurate as a **historical PG-01C** description but misleading if read as *current as-built tool coverage*; the accepted PG-01D now includes both Planar Face and Face Boundary.
- The same file, approximately **line 468**, says "**the user-facing Project Geometry tool**" and "**automatic Face-boundary projection**" are among "not yet implemented". This is an **unqualified stale present-tense claim** versus current manual Face Boundary acquisition and one user-facing Project Geometry tool. No **automatic adjacency/perimeter traversal** was delivered, so the corrected statement must preserve the explicit difference between *manual selectable Face Boundary* (available) and *automatic multi-Face traversal* (not available).
- Verified cross-check: `docs/product/pl/PARTS.md` heading "**Project Geometry — rzutowanie krawędzi...**" approximately line 324 and `docs/product/en/PARTS.md` Project Geometry section both document **Edges / Planar Face / Face Boundary**; [PG-01D final](PROJECTION_01D_FINAL_ACCEPTANCE_AND_MERGE.md) independently records implemented and Owner-accepted modes.

**Suggested strictly docs-only later remedy, NOT yet authorized or executed:** revise only the affected current-state paragraphs in `docs/internal/PART_DOCUMENTS.md`, retain historically scoped PG-01C facts where labelled as such, explicitly state the *manual* Face Boundary contract and no persistent Face source. If canonical Product docs are unaffected, no Browser rebuild is needed merely for Internal-only correction. The future exact approved PG-01E contract must allow the documentation source path and appropriate exact-head checks.

**Classification:** CONFIRMED DOC DRIFT, not a runtime defect. **No canonical doc changed in this E0 draft.**

## E0-C. Current v16/v2 compatibility boundary — observed source/test, not new execution

Existing `tests/part_document_store_test.cpp` explicitly tests the `modeling_semantics_version` field and typed `unsupported_modeling_semantics` rejection (source inspection). [PG-01D final](PROJECTION_01D_FINAL_ACCEPTANCE_AND_MERGE.md) establishes strict **new Part schema v16 and ModelingSemanticsVersion=2** distinction and Owner acceptance of no automatic or bulk v1 migration. This is a reader/refusal *policy*, not an obligation to make v1 documents load successfully. Real PG-01E E2 compatibility evidence on current main is **OPEN** until a new bounded replay and exact-head test run.

## E0-D. Unresolved PG-01E requirements; NO claims of completion

| Requirement | Current evidence scope | PG-01E status |
| --- | --- | --- |
| One complete v2 ordered Extrude/Revolve Add/Cut → linked Regular Profile → later Feature with mixed source Edges/Planar Face/Face Boundary | Isolated earlier PG-01B and PG-01D successes, tests exist | **OPEN — composite fixture not newly tested** |
| Supported upstream dimension edit, deletion/suppression, failed exact projection, broken source repair or Break Link and one Undo/Redo | Earlier native cold rebuild + UI lifecycle source inspected | **OPEN — verify exact combined current source stage and ref freshness** |
| Atomic Finish / no source on failure, partial geometric skip vs integrity errors, per-Face skip diagnostics | PG-01D accepted, current Workbench test source contains cases | **OPEN — prove exact PG-01E requirements and missing test entries** |
| v16/v2 Save/Close/Reopen fresh provider and modeling-v1 strict typed refusal with immutable legacy bytes | Product/PartStore earlier policy + tests present | **OPEN — current PG-01E exact-head combined coverage** |
| Deterministic performance/resource methodology, repeated controlled cycles, memory/handle deltas | Existing Windows test resources exist, no PG-01E repeated baseline published | **OPEN, no fabricated metrics or budget** |
| PL/EN + Internal/Browser parity | PL/EN present; E0-DOC-01 confirmed Internal drift | **OPEN for final audit/repair after activation** |
| PG-01E exact-head FOCUSED/FAST/FULL + Owner practical FINAL PASS and merge | Prior stage FULL only | **UNRUN / PENDING** |

## E0-E. Governance and next steps

1. Owner D2-E1–E6 scope direction is **accepted for contract drafting only**; [PR #311](https://github.com/MechanicalCave/SimpleSolid2/pull/311) is still open, **not merged**.
2. The bounded [contract PR #337](https://github.com/MechanicalCave/SimpleSolid2/pull/337) still needs **separate exact-text Owner acceptance** and independent Owner merge approval; PR #311 merge also separately gated.
3. Only **after** accepted documents are merged, a **separate `work/ACTIVE.yaml` PG-01E activation PR** may be proposed for Owner permission; main roadmap stays v1.32 and no product/tests change beforehand.
4. Once activated, execute E0–E6 within the exact accepted existing-test and conditional documentation allowlist. All required new production/source APIs or additional paths require a new bounded Owner amendment **before writing them**.
5. Deferred RG-02A/G1 and any RG-02 UX remain separate; **prior independent Owner UX consultation and approval before ANY RG-02 UI code** remain mandatory.

**No RG-02 Vertex schema, G1 test, production, Viewer, Part/kernel, accepted docs or CI file has been mutated in preparing this report.**

## Documentation Impact

Internal docs: not required (draft status report only)
PL/EN Product docs: not required
Browser: not required
Future formal E4 stage will explicitly assess these if Owner separately activates PG-01E.
