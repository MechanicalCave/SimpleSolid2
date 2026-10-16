# RG-02C/C1-P1-B1/H1 — native v20 writer: legacy test expectation correction, exact-path D2 addendum

**Status: D2 PROPOSAL / OWNER EXACT-SCOPE ACCEPTANCE REQUIRED. NO PRODUCTION TEST MUTATION OUTSIDE EXISTING 19 H1 PATHS IS AUTHORIZED BY THIS FILE.**
**Date:** 2026-10-16.
**Baseline:** protected `main` `b4265c53e5b6677af566733fdfc3deafc14fcad8`. Owner separately accepted architectural option **A** (one real current strict bounded analytic cylindrical Face and stage-qualified semantic SurfaceReference only), exact H1 D2/UX, and explicitly authorized guarded-squash #445 and #446. Both were independently Windows-green and merged, leaving H1 **ACTIVE with exactly 19 authorized paths**. H1 implementation [Draft/Ready PR #444](https://github.com/MechanicalCave/SimpleSolid2/pull/444) is NOT merged, technical FINAL PASS not requested.
**Authority:** AGENTS.md; frozen Constitution/Foundation 1.0; Architecture and accepted ADR-0014/-0016/-0017; Part Modeling roadmap v1.33; accepted `work/RG-02C_C1_P1B1_H1_STRICT_CYLINDRICAL_FACE_AXIS_HEADLESS_D2_WORK_CONTRACT_PROPOSAL.md` §2, including strict STOP if native RED requires a path outside the allowlist. DOC-01 impact declarations remain required.

## 1. Real Windows RED and reason to change historical tests

The first independent Ready FULL [Windows #3388](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38929996016) at actual implementation HEAD `eb62db628853099320be7af7f09d1658e2997280` **COMPLETED FAILURE**. The MSVC logs show **Core-only 25/25 PASS** and **native Kernel/OCCT Release 55/57**; the separate Desktop Qt/OCCT CTest never completed because native Kernel returned RED. The two failing registered tests were:

- `pm02jr2.add_surface_continuation`, `tests/kernel_native/pm02jr2_add_surface_continuation_test.cpp:1525`: checks `package.package->descriptor.domain_schema_version == 19` on a **new current-writer file** (not a genuine legacy v19 fixture).
- `pm05c2b.part_edge_topology`, `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp:3304`: checks `PartDocumentStore::current_schema_version == 19` although the Owner-approved H1 schema was deliberately bumped to **20**.

These are **real RED test assertions**, not a flaky runner, not OCCT geometry proof and not evidence that the writer should silently revert to v19. Do NOT bypass, exclude, skip, rewrite the writer to emit v19 for these tests, or report #3388 as PASS. Earlier correctly executed [FAST #3386](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38929211891) was **93/93 PASS** on its exact HEAD, and [native FOCUSED #3387](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38929754074) **1/1 PASS** on an earlier exact HEAD, including real native v20 Face Axis Save/Reopen/Cold Rebuild. Neither is the Ready FULL.

**Prevent further one-at-a-time RED:** read-only audit of all 146 repository `tests/**` C++ test sources (32 kernel-native plus 114 top-level) located **nine** files containing explicit legacy `current_schema_version == 19`, `domain_schema_version == 19` or equivalent **current-writer** expectations. Two are **already within H1 §2 allowlist**: `tests/pm04a_axis_schema_v12_test.cpp` (already updated in #444) and `tests/pm04e1_revolve_persistence_test.cpp` (two current writer/migration assertions can be updated without scope expansion). The **seven additional files** below are not currently allowed by H1 §2. Do not transform any true **old-file reader fixture** deliberately carrying schema 9/12/13/14/15/16/17/18/19; update only the expected schema version emitted by the **current writer**.

## 2. Requested EXACT D2 extension — seven test paths, no other changes

If Owner explicitly accepts this contract addendum, the exact list of H1 §2 allowed paths becomes **26 rather than 19** after separately governed integration. Add these **SEVEN** and only these:

| New H1 # | Exact path | Required current-writer assertion repair; observed source |
| --- | --- | --- |
| 20 | `tests/kernel_native/pm02jr2_add_surface_continuation_test.cpp` | One new-current-file descriptor check near line 1525 **19→20**, plus immediately adjacent stale current-schema comment only. Preserve genuine v15/v16 negative fixtures. |
| 21 | `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp` | One constant check near line 3304 **19→20**, immediately relevant native Edge Axis v19/v20 narrative only. Preserve genuine Edge-source persisted semantics and cold provider proof. |
| 22 | `tests/pm02e_sketch_support_schema_v9_test.cpp` | Three current-writer/rewrite descriptor checks near lines 394, 510, 625 **19→20**. Preserve actual schema-v9 legacy inputs and rejection semantics. |
| 23 | `tests/pm03a_datum_schema_v10_test.cpp` | Four current-writer/upgraded descriptor checks near lines 346, 401, 431, 633 **19→20**. Preserve actual legacy v16/v17/v18 and older inputs. |
| 24 | `tests/pm03e_datum_backed_sketch_test.cpp` | Two checks near lines 389 (store constant), 797 (current written descriptor) **19→20**. Preserve same Datum/Sketch source behavior. |
| 25 | `tests/pm05b1_edge_feature_schema_v14_test.cpp` | Three current-writer/current upgraded descriptor checks near lines 707, 974, 1080 **19→20**. Preserve actual deliberately mislabeled v13 / genuine older-record negatives. |
| 26 | `tests/pm05b3_generated_surface_reference_v14_test.cpp` | One current-writer descriptor check near line 505 **19→20**. Preserve the generated semantic Surface provenance and all old schema-v14 inputs. |

**Allowed mutation in each path:** only expectation literals for the *new v20 writer and its old-file-to-current-writer upgrade*, plus immediately adjacent accuracy-only comments/diagnostic labels; no unrelated test behavior, fixture normalization, alternate schema parser, new CMake target, production source logic, temporal ordering, skipped checks or removal of assertions. **No other paths**. The existing `tests/pm04e1_revolve_persistence_test.cpp` was already in H1 §2 and has two v19→v20 *current writer* checks around lines 301, 517; those can be updated within the current 19-path scope, but legacy v12/v13 Revolve files still remain genuine historical fixtures.

**New native/reader acceptance:** after the exact-path amendments are integrated, a genuinely completed separate Windows **FOCUSED**, independent **Draft FAST** and **Ready FULL** must run across real Core-only, Kernel-native OCCT Release and Desktop Qt/OCCT, with DOC-01 and exact final HEAD CLOSURE. Existing full 25/57/117 denominators are baseline, not assumed final figures. In the repaired tests verify that existing authentic old native files still parse and upgrade to **v20** with *unchanged authored identities*, and future/malformed kinds still reject. RED must be explained/fixed within an approved path or brought to Owner as a new explicit D2 gate. Never mark a skipped test as PASS.

## 3. Owner decision, formal activation and merge gates

1. **Owner exact-path D2 acceptance PENDING:** the user's latest `kontynuuj` authorized continued analysis and in-scope work, **not** prior acceptance of these unshown seven additional paths. The Owner must separately accept these precise paths/limitations.
2. Record exact Owner acceptance in this **governance-only** one-file proposal PR, run Windows exact-head CLOSURE, obtain independently explicit guarded-squash authority for that proposal and verify protected `main` after merge.
3. Independently amend the **already active H1 D2 Work Contract** by adding these exact seven test paths (20–26) to §2, preserving original 19, and pass Windows exact-head governance CLOSURE + separate guarded squash. There is no need to toggle `work/ACTIVE.yaml`: H1 remains active.
4. **Only after both governance PRs are integrated** may implementation #444 modify any of the seven new paths. Existing 19 authorized H1 paths may continue to be edited; no early test patch in the new paths to force GREEN.
5. Finish exact Real Windows FOCUSED / FAST / FULL / DOC-01 and factual ledger. Separate Owner technical FINAL PASS with deferred GAP accounting and independent protected-main guarded squash of #444. UX H2/Viewer/CLI/PL+EN product release remains outside this headless contract.

## 4. Explicitly unchanged product rules and GAPs

Strict chosen Face A; singular semantic Surface at authored history stage, strict bounded current Face with exactly one native realization and exact analytic cylinder query; no Face token/XYZ/radius persisted; all-around unoriented infinite Axis; stable AxisId/Undo/Redo and Revolve signed sweep. No B multi-Face carrier admission nor C patch discriminator; no fake native Fixture PASS. The inherited P1-B0 actual multi-Face resolved same-cylinder Surface witness and certain injected/reversed-topology cases remain expressly **GAP / NOT TESTED**.

## Documentation impact

Internal docs: not required
User/Product docs: not required
Reason: This is a governance-only exact test-path D2 addendum. It implements no user-facing feature. Actual H1 implementation must still update the already allowlisted as-built `docs/internal/PART_DOCUMENTS.md`, generate Browser via the official DOC-01 tool only, and run real Windows checks. The distinct H2 UI requires a separately accepted executable Work Contract.
