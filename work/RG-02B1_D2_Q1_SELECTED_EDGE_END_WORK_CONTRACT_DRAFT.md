# RG-02B1/D2-Q1 — selected material Edge-local END provenance: Owner-accepted prospective executable Work Contract

**STATUS (Owner complete-text D2 acceptance, 2026-10-13): ACCEPTED AS PROSPECTIVE Q1 EXECUTABLE SCOPE, NOT MERGED / NOT ACTIVE / NO IMPLEMENTATION AUTHORITY.** Owner responded **“akceptuję - kontynuuj”** to the request for the entire bounded Q1 contract in Draft PR #356, including the owning-DocumentId hardening, with reviewed exact-text HEAD `0a226abd5553340475963ef88c71bb7d264ccfb8` (Windows governance #2868 SUCCESS; FOCUSED/FAST/FULL SKIPPED). This acceptance covers the full twelve-path implementation scope, typed semantic/API bounds, Q1-01…08 required future tests and DOC-01 obligations **as future work only**. It does **not** approve merging PR #356, changing `work/ACTIVE.yaml`, writing implementation code/tests, claiming any Q1 test PASS or activating Q2+. Independent exact-head PR-specific merge permission and a separately authorized/merged Q1 ACTIVE switch are still mandatory.
**Date:** 2026-10-13
**Repository:** `MechanicalCave/SimpleSolid2`; proposal based on `main` `596ec70fd10d5931c09a20dd5111f709812f0692`.
**Milestone:** Part Modeling roadmap **ACCEPTED v1.33**, RG-02B1 **Q1 only**.
**Governance:** `AGENTS.md` → Constitution/Foundation v1.0 → Architecture → accepted ADR-0014/0016/0017 (+ ADR-0009, 0012, 0013) → `work/ACTIVE.yaml` → program roadmap → accepted Work Contract → code/tests → DOC-01.
**Current ACTIVE:** `work/RG-02B1_D2_Q0_EXACT_3D_EDGE_QUERY_WORK_CONTRACT_DRAFT.md` is still pointed to, even though its Q0 implementation PR #354 was squash merged. **No Q1 mutation of code, tests, as-built docs, schema or ACTIVE is authorized by this draft.**
**Input:** planning-only Draft PR #355 and `work/RG-02B1_D2_Q1_EDGE_LOCAL_END_PROVENANCE_SCOPE_PLAN.md` §0–5; accepted design `work/RG-02B1_D2_EDGE_POINT_SEMANTIC_DESIGN_PROPOSAL.md` (merged PR #349); accepted Datum UX basis `work/RG-02B1_DATUM_EDGE_POINT_WORK_CONTRACT_DRAFT.md` (merged PR #345); Q0 contract #352, activation #353 and production #354 merged. The historical headers/statuses of these input documents do not override live GitHub.
**Owner decision recorded:** the **entire bounded Q1 contract below** was accepted once as prospective D2 scope at PR #356 reviewed HEAD `0a226abd5553340475963ef88c71bb7d264ccfb8`. No incremental public API/signature/allowed-path or UX micro-approvals are solicited. Any subsequently discovered out-of-scope requirement is a hard STOP requiring an explicit amended Owner decision.

## 1. GOAL — deliberately bounded authorable END semantics

Create one Part-owned, provider-neutral, **strictly stage-scoped, selected-material-Edge-local terminal reference** that remains the **same semantic END** over repeated current evaluation, upstream Extrude height change and a completely independent cold OCCT provider **where its producer-semantic endpoint is independently certifiable**. The key must distinguish:

- chosen strict `MaterialEdgeReference` (the *specific* authorable material Edge), and
- a certified *semantic terminal discriminator* of that chosen Edge, not an OCCT first/last slot, native token, world XYZ, curve parameter, nearest Vertex or screen hit.

**Exactly supported initial Q1 family:** one current strictly Resolved material **bounded Line or genuinely open circular Arc** with exactly two distinct native terminal realizations, for which the *indicated* terminal has **one already independently Resolved** producer-semantic `FeaturePointAddress` (existing three-Surface meaning), uniquely mapped to one of the current Edge's own two native terminals. The new **composite** Edge+terminal key stays distinct for each selected Edge, even if two/three Edges terminate in one shared native Vertex/semantic global Point. In the canonical rectangular Extrude all **12 strict Lines / 24 Edge-terminal uses / 8 shared three-Edge corners** must pass. This is the defensible Owner D2-P2 option **A**, not an unsupported claim that option **B** (a new independent Edge-producer terminal discriminator for globally Ambiguous/Unsupported Points) is already proven.

**Fail-closed coverage boundary:** When the global Point is Ambiguous/Unsupported/Missing, or the chosen source branch or current physical END lacks a unique certification, Q1 explicitly declines to author that END. This does **not** redefine the global Point, claim universal END support, nor silently satisfy future Q2's complete feature acceptance for these families. Noncanonical Fillet/Chamfer/Revolve/half-cylinder cases may remain not-authorable. If Owner requires END coverage there now, STOP and seek a separate narrowly evidenced terminal-provenance D2 amendment, not a coordinate or provider-order fallback.

**Important classification:** Q1 produces a **durable-ready in-memory Part-local semantic reference value and a read-only runtime resolver**. Q1 does **not persist that value in native Part v16**; Q2+ must separately accept a new Datum authored source, schema compatibility, frame orientation and Command/Undo/Save meaning. A Part-local value is qualified externally by its owning `DocumentId`. Neither `FeatureId` nor `DocumentRevision` is a substitute for `DocumentId`. "Cold rebuild" tests mean carrying this semantic value across a fresh native Part evaluation/reopen, not claiming it was already serialized.

## 2. SOURCE AUDIT AND ENGINEERING RATIONALE

The current `MaterialEdgeReference` in `src/part/include/simplesolid2/part/semantic_topology_reference.hpp` stores `BodyStageRef`, `FeatureCurveAddress` (semantic producer and exactly two Surface roles), and an `EdgeBranchDiscriminator` (`SingularAtAuthoredStage`, `BetweenSemanticPoints`, `AtSingleSemanticPoint`). It distinguishes a bounded material Edge, **not its two ends**. `FeaturePointAddress` is a semantic producer + triple-Surface intersection, not an arbitrary native endpoint. Changing its meaning globally is outside Q1.

`src/part/feature_evaluation.cpp` currently: (a) authors/resolves a strict material Edge against exactly one `BodyStageTopologyCatalog`; (b) requires independently Resolved Point references when disambiguating certain branches; (c) records only **runtime** `BodyVertexTopologyRecord.incident_material_edges` and diagnostic `provider_point`; (d) preserves Point/Curve meanings via explicit producer and provider lineage, but has **no independent stable Edge-terminal producer role** for intrinsically Ambiguous/Unsupported global Points. The `pm05b2.strict_edge_resolver` negative test expressly prohibits promoting a globally Ambiguous Point to Resolved using incidental Edge connectivity. Preserve it.

Merged Q0 `IExactBoundedEdgeGeometryQuery` returns paired *transient* terminal positions and unoriented tangent lines, exact bounded length and half-length MID. Its `ExactBoundedEdge3d.current_terminal_positions_world[0/1]` array has **no semantic slot meaning**. The provider already identifies the native two terminal `TopoDS_Vertex` objects of its exact runtime Edge and has `inventoryVertexToken`; therefore the smallest correct bridge is an **additional runtime-only, same-slot pair of `RuntimeVertexToken`** in Q0's result, allowing Part to match a uniquely certified semantic terminal by **current Vertex token equality**, never by XYZ, OCCT order or proximity. This is an additive **D2 public-result change**, expressly authorized only if Owner accepts this full contract.

Native E0/`pm05c2b.part_edge_topology` already proves 12 strict box material Edges, 24 independently Resolved endpoint uses, 8 trihedral corners, cold provider/height edit and same-XYZ replacement decoys. **E0 is NOT Q1 proof.** Q0 runs FAST #2861 **93/93**, FULL #2862 **199/199** on code SHA `6d53ba47162ca58df25e295aa12efa5d3a47d174`, docs-only #2864 PASS at final Q0 HEAD. Q0 direct new-path `Standard_Failure` injection remains not directly tested; Q1 does not misstate or silently expand that claim. **All new Q1-01…Q1-08 tests are UNRUN.**

## 3. OWNER D2: EXACT PROPOSED PUBLIC TYPES AND FUNCTIONS

Place the *Part semantic value* in `src/part/include/simplesolid2/part/semantic_topology_reference.hpp`:

```cpp
// Initially certified-point-backed only. Struct versioning/serialization
// must be separately designed if Q2+ later persists it.
struct StableEdgeLocalTerminalKey final {
    FeaturePointAddress certified_semantic_point;
    [[nodiscard]] bool valid() const noexcept {
        return certified_semantic_point.valid();
    }
    friend bool operator==(const StableEdgeLocalTerminalKey&,
                           const StableEdgeLocalTerminalKey&) = default;
};

struct SelectedMaterialEdgeEnd final {
    MaterialEdgeReference source_edge;
    StableEdgeLocalTerminalKey terminal;
    [[nodiscard]] bool valid() const noexcept {
        return source_edge.valid() && terminal.valid();
    }
    friend bool operator==(const SelectedMaterialEdgeEnd&,
                           const SelectedMaterialEdgeEnd&) = default;
};
```

The *value as a whole* is Edge-local: `{source_edge, certified_semantic_point}`. The Point component is **independently** certified as a real semantic Point; the same `FeaturePointAddress` on a different material Edge does **not** make those two END references equal. The struct does **not** assert that its Point field is intrinsically Edge-local, nor provide a hidden, unjustified `End0/End1` variant. No new Part schema or new global `FeaturePointAddress` kind.

In `src/kernel/include/simplesolid2/kernel/exact_bounded_edge_geometry.hpp`, **append** the following member to `ExactBoundedEdge3d` after existing fields to preserve the existing field order/source uses:

```cpp
// Evaluation-local native terminal membership only.
// Token[i], position[i] and tangent[i] denote the SAME actual physical
// terminal of the SAME current Body+Edge; i is not a semantic key.
std::array<RuntimeVertexToken, 2> current_terminal_vertex_tokens{};
```

Only successful Q0 queries can populate it, and `ok` must mean exactly two distinct valid tokens in the same `RuntimeSolidHandle` vertex inventory, paired with the two existing Q0 END positions/tangent lines. All eight existing `ExactEdge3dStatus` results and failure payload rules remain unchanged; no appended token may be retained on non-`ok`. Runtime token carries **zero durable authority**. If OCCT cannot certify the two matching Edge-owned native Vertex objects **in the exact bound Body**, return the existing typed Q0 failure with **no geometry**; no XYZ matching or new heuristics.

In `src/part/include/simplesolid2/part/feature_evaluation.hpp`, **append** to the existing `PartEvaluation` (without reordering existing fields) `std::optional<core::DocumentId> source_document_id;` as **runtime-only evaluation ownership evidence**, populated by `evaluatePart` from `document.documentId()` in `src/part/feature_evaluation.cpp`. This field is neither native persistence nor a new global identity framework. A default-constructed/synthetic PartEvaluation without the owner ID cannot authorize Q1 source resolution. Also declare the following **read-only Q1 result/query boundary** (exact names/signatures are D2 proposal to accept in one go):

```cpp
enum class SelectedEdgeEndStatus {
    resolved,
    invalid_context,
    edge_missing,
    edge_ambiguous,
    edge_unsupported,
    terminal_missing,
    terminal_ambiguous,
    terminal_unsupported,
    native_geometry_unavailable,
};

struct SelectedEdgeEndResolution final {
    SelectedEdgeEndStatus status{SelectedEdgeEndStatus::invalid_context};
    std::optional<kernel::RuntimeEdgeToken> current_edge;
    std::optional<kernel::RuntimeVertexToken> current_terminal;
    std::optional<kernel::Point3> current_position_world;
    std::optional<kernel::UnorientedUnitTangent3> current_tangent_line;
    std::optional<kernel::ExactEdge3dStatus> native_error;
    [[nodiscard]] bool resolved() const noexcept; // strict all-fields invariant
};

struct SelectedEdgeEndAuthoringResult final {
    SelectedEdgeEndStatus status{SelectedEdgeEndStatus::invalid_context};
    std::optional<SelectedMaterialEdgeEnd> reference;
    [[nodiscard]] bool ok() const noexcept; // strict resolved+valid invariant
};

// Caller supplies the actual owning PartDocument, not a self-attested
// raw revision. Its DocumentId AND current revision must match the
// evaluation's source_document_id AND source_revision.
// source_stage must resolve one currently successful Feature result with
// its exact paired result_solid and complete result_topology.
[[nodiscard]] SelectedEdgeEndAuthoringResult
authorSelectedMaterialEdgeEnd(
    const PartDocument& owning_document,
    const PartEvaluation& evaluation,
    const BodyStageRef& source_stage,
    kernel::RuntimeEdgeToken picked_material_edge,
    kernel::RuntimeVertexToken indicated_current_terminal,
    kernel::IExactBoundedEdgeGeometryQuery& provider);

[[nodiscard]] SelectedEdgeEndResolution
resolveSelectedMaterialEdgeEnd(
    const PartDocument& owning_document,
    const SelectedMaterialEdgeEnd& authored,
    const PartEvaluation& evaluation,
    kernel::IExactBoundedEdgeGeometryQuery& provider);
```

The header may directly include `kernel/exact_bounded_edge_geometry.hpp` to name the 3D geometry-result types; no new generic platform reference framework. The Q1 reference has no Body/token/session data; its Part-local address is qualified by the owning Document's `DocumentId` at the read boundary. The resolution result is a disposable runtime snapshot; `current_position_world` and `current_tangent_line` are **observations**, not identity nor signed Datum frame. `native_error` is present exactly for `native_geometry_unavailable` when the Q0 query supplies a typed failure. Failed results carry **no** Edge/Vertex/position/tangent publication and no stale last-good observation.

## 4. ALGORITHM, PRECEDENCE, AUTHORING AND COLD REPLAY

**Shared stage admission**: require valid semantic source, `evaluation.source_document_id == owning_document.documentId()` and `evaluation.source_revision == owning_document.revision()` (do **not** accept a caller-supplied bare revision as independent authority); also require the stage's `FeatureId` to exist in the owning Document. Inspect `PartEvaluation.features` for exactly one `FeatureEvaluation` of `source_stage.feature_id` whose `status == up_to_date`, `result_solid` exists and whose complete `result_topology.stage == source_stage`. Reject missing evaluation owner, cross-document mix, invalid/duplicate/wrong stage, stale revision or unavailable current Feature without using `current_topology` from final Body by substitution. Runtime request/session lease beyond evaluation revision remains the owning future application/command's responsibility and cannot be fabricated by this pure Part query. The owner check protects identity scoping; the caller must still use the authentic owning document evaluation (not fabricate a manually assembled `PartEvaluation`). Q2 commit must revalidate DocumentId, revision, session lease, exact stage and all dependencies.

**Edge first:** require `authorMaterialEdgeReference` followed by `resolveMaterialEdgeReference` to yield exactly the input picked Edge; for existing specialized PG-01D `AtSingleSemanticPoint` do not silently switch to projection-only admission. On replay, re-resolve the *stored exact* `MaterialEdgeReference` at that stored stage; none/one/many map Missing/Resolved/Ambiguous/Unsupported as declared. A different source FeatureId with identical XYZ is never equivalent.

**Terminal then:** in the stage's complete vertex catalog, find exactly **two distinct current physical endpoints** of the chosen Edge, checked against the Q0 current terminal token pair. To author, require exactly one of those two tokens equals `indicated_current_terminal`. Require its Vertex record to have exactly **one** independently Resolved `FeaturePointAddress`, matching exactly **one** `FeaturePointResolution` with `status == resolved`, one current vertex and that same token. Verify no duplicate competing resolved Point maps to the same native Vertex; if ambiguous, reject. Never infer that Point's resolution from Edge incidence; incidence is a membership check only. Reject nonmaterial seams/partitions, a disconnected/not-on-this-Edge indicated token, repeated endpoints, closed/periodic/unsupported native curve, and incomplete catalog.

**Q0 coupling:** only after strict Part semantics, `bindExactEdgeToBody` the exact stage `result_solid` + resolved current Edge; `queryExactBoundedEdge3d` with that same Body; check geometry complete, Line/open Arc, two distinct valid inventoried token pairs and exactly matching semantic endpoint current token. Select the associated position/tangent by token equality, **not by Q0 array slot, coordinates, first/last parameter, length ranking or direction**. Permuting *whole* Q0 terminal tuples must not affect semantic resolution. No new numeric modeling tolerance in Part: native tolerances remain Q0's accepted local policy and never select an END candidate.

**Cold/replay:** `resolveSelectedMaterialEdgeEnd` repeats all current-stage strict Edge and independent certified Point checks using the stored semantic value, finds exactly the one current endpoint token, then Q0 binds the current new Body and derives geometry. If upstream edits move the terminal, it follows **provenance**, not its former XYZ. If source disappears, splits, acquires competing claims or different Feature identity, return typed Missing/Ambiguous/Unsupported with empty output. Do not resurrect via old runtime token, previous geometry or nearest match. Do not mutate `PartDocument`, `FeaturePointResolution`, history, save state or UI.

**Typed failure precedence (one authoritative result, no partial success):**
1. Invalid/mismatched stage, revision or incomplete current context → `invalid_context`.
2. Existing strict Edge resolver decides source Missing/Ambiguous/Unsupported → corresponding `edge_*`.
3. Selected Edge resolves but requested Point source has zero/multiple/unsupported semantic realization, or the candidate is no longer a uniquely selected Edge's own terminal → `terminal_missing`, `terminal_ambiguous`, or `terminal_unsupported`, respectively. No independent global Point status is promoted.
4. Only with strict current semantic source resolved, Q0 provider failure / unsupported analytic geometry → `native_geometry_unavailable` and exact underlying existing `ExactEdge3dStatus`; no geometry payload.
5. `resolved` iff one exact Edge+its END is both semantically certified **and** completely, currently geometrically materialized.

## 5. EXACT FUTURE SOURCE/TEST/DOCUMENTATION ALLOWLIST

**Only AFTER the complete accepted contract has independently merged AND a separate accepted/merged `ACTIVE.yaml` activation points to it**, allow the following **twelve exact paths** on a fresh production branch; NO wildcard permission:

| # | Exact path | Allowed bounded change |
| --- | --- | --- |
| 1 | `src/part/include/simplesolid2/part/semantic_topology_reference.hpp` | Typed `StableEdgeLocalTerminalKey` and composite selected-Edge END semantic value only |
| 2 | `src/part/include/simplesolid2/part/feature_evaluation.hpp` | Append runtime-only `PartEvaluation.source_document_id`, typed Q1 statuses/results and two read-only author/resolve declarations |
| 3 | `src/part/feature_evaluation.cpp` | Populate evaluation owner DocumentId; check owner+revision+stage; strict Edge+certified Point admission/replay/Q0 runtime-token association; no modification of existing global Point resolver meaning |
| 4 | `src/kernel/include/simplesolid2/kernel/exact_bounded_edge_geometry.hpp` | One appended runtime-only endpoint-token pair on Q0 result |
| 5 | `src/kernel_occt/solid_modeling_kernel.cpp` | Current-inventory `TopoDS_Vertex`→`RuntimeVertexToken` lookup for existing paired Q0 output, fail-closed atomic success |
| 6 | `tests/pm05b2_strict_edge_resolver_test.cpp` | Registered semantic negative/alias/branch/structural Q1 unit regressions, preserve old negatives |
| 7 | `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp` | Registered true Part+OCCT 12/24/8, independent provider, height edit and real unsupported Q1 proof |
| 8 | `tests/kernel_native/pm02p_e_kernel_lifecycle_test.cpp` | Registered current-inventory Q0 terminal-token pair, real Line/open Arc/closed and generation guards |
| 9 | `docs/internal/PART_DOCUMENTS.md` | Canonical as-built strict Part EDGE+certified-point END semantics, bounded limitations |
| 10 | `docs/internal/PART_EDGE_PROJECTION_KERNEL.md` | Q0 additional transient token field, precise Q1 caller contract, preserve 2D projection and existing sections |
| 11 | `docs/browser/index.html` | **Generated-only** (invoke `./ss2.ps1 docs`), exact freshness after canonical Markdown changes; never hand-edit |
| 12 | `work/RG-02B1_D2_Q1_SELECTED_EDGE_END_ACCEPTANCE_EVIDENCE.md` | New work-only real source SHA/Windows CTest evidence and Owner acceptance ledger, no fabricated PASS |

**Explicit exclusions:** `work/ACTIVE.yaml` in the implementation PR (separate governance activation only), any `src/ui/**`, `src/viewer/**`, `src/application/**`, `src/part/part_document_store.cpp`, `src/part/datum*`, `src/kernel/include/simplesolid2/kernel/solid_modeling.hpp`, all other `src/**` and `tests/**`, CMake, CI workflows, schema v16/modeling-semantics v2, Foundation, ADRs, roadmap, unrelated issue #302, G1 and PM-06. If a necessary include/build/test seam cannot fit the list, **STOP** and request explicit exact-path amendment *before* mutation, not a hidden helper, broad search/replace or weakened test.

## 6. Q1-01…Q1-08 — FINITE ACCEPTANCE MATRIX (ALL NEW CASES UNRUN)

| Gate | Native or semantic source | Required objective evidence |
| --- | --- | --- |
| **Q1-01** 12/24 | `pm05c2b.part_edge_topology` | Actual OCCT box at exact Body stage: **12** unique strict authorable Edges, exactly **24** certified composite Edge-local terminal references; for each Edge two distinct semantics; Q0 runtime slot swap has no effect |
| **Q1-02** 8 shared corners | `pm05c2b.part_edge_topology` | At **8** corners, Edge A/B/C END references have their own selected `MaterialEdgeReference`, share legitimate current corner Point but remain **three distinct composite source meanings**; direct B+corner author unaffected by an independently recorded transient hint on A; this is **headless semantics only**, not a tested viewport Point-first pick |
| **Q1-03** fresh provider | `pm05c2b.part_edge_topology` | Carry the exact Q1 reference values into a completely independent real OCCT provider/evaluation and optionally a cold v16 Part Save/Reopen; **24/24** preserve semantic terminal, native tokens may differ, no cached Body/Vertex/geometry required; the Q1 source itself is not serialized |
| **Q1-04** upstream height | `pm05c2b.part_edge_topology` | Edit original Extrude extent while preserving Feature identity: all still-defensible keys stay attached to their original bottom/top corner semantics; moved point follows upstream shape, not original XYZ; physically same-looking **new FeatureId** leaves old key Missing |
| **Q1-05** wrong/stale/branch | `pm05b2.strict_edge_resolver` + native `pm05c2b` | Wrong `BodyStageRef`, missing/mismatched owning `DocumentId`, stale owning Document revision, missing Edge, extra bounded branch, deleted certified Point, foreign runtime Vertex, native Body mismatch all fail typed, no current END/geometry, no false Resolved |
| **Q1-06** negative families | `pm05c2b.part_edge_topology` + `pm05b2.strict_edge_resolver` | Half-cylinder ambiguous, Fillet/Chamfer/Revolve/unsupported Point variants retain **original global Point statuses**; selected Edge does not upgrade intrinsic Ambiguous Point; support only where independent same-Edge certification **actually passes**, otherwise typed not-authorable; record precise family counts |
| **Q1-07** closed/degenerate/slot | `pm02p.e_kernel_lifecycle` + semantic test | Q0 full circle/periodic/degenerate rejection, same selected Edge's indistinguishable terminal cases, duplicated native token, reversed/relabeled native terminal ordering and one whole-tuple swap: no silently persisted ordinal and no false valid END |
| **Q1-08** current Q0 coupling | Both registered native targets | Selected reference resolves strict exact stage, binds **same current Body**, joins unique certified Vertex to Q0 token→position/unoriented tangent tuple by identity; wrong Body and provider mismatch preserve Q0 failure; **no** Datum, signed Frame3, Reverse, MID source persistence, UI or native Part v17 |

**Tests must actually distinguish semantic identity from geometric equality.** Compare full semantic source values over replay and separately verify current native geometry by guarded diagnostics. Include at least one explicitly expected rejection in every negative family. Do not declare Q1-06 supported merely because current geometry has two visible endpoints.

**Registered CTests already exist:** `pm05b2.strict_edge_resolver` in `tests/CMakeLists.txt`; `pm05c2b.part_edge_topology` and `pm02p.e_kernel_lifecycle` in `tests/kernel_native/CMakeLists.txt`. Their existing source tests must remain unweakened and no new CMake entry is authorized.

## 7. VALIDATION, EXACT-HEAD CI AND OWNER CLOSEOUT

1. **Governance before source:** Owner explicitly accepts the **full exact text** of this bounded Q1 Work Contract; its own Draft governance PR is checked/marked Ready and receives independent Owner PR-specific squash-merge permission; verify actually merged on `main`. Prepare a separate `work/ACTIVE.yaml`-only activation proposal referencing its exact merged path, preserve Foundation v1.0, roadmap v1.33 and history; obtain distinct Owner activation scope approval, exact-HEAD Windows governance and separate Owner merge permission; verify merged before writing ANY of the twelve implementation paths.
2. **FOCUSED after activation:** native `pm05c2b.part_edge_topology`, native `pm02p.e_kernel_lifecycle` and semantic/Desktop `pm05b2.strict_edge_resolver`. Use the **current** CI's tested focused-message/dispatch grammar; verify each actual target executed, don't mistake green classifier or skipped jobs for a passing native test.
3. **FAST after source settling:** run actual Windows FAST against the committed code/test HEAD; record counts and any RED (including CI #302 build-vs-CTest mismatch if encountered), do not suppress tests.
4. **Ready-for-Review FULL:** run Windows FULL on exact code HEAD with Core, kernel-native, Desktop, docs and aggregate; synchronize canonical internal docs and regenerate `docs/browser/index.html` from Markdown, then obtain exact-HEAD doc validation. If documentation-only HEAD follows tested code HEAD, use a verified two-SHA compare to prove no source/test change and **report the split evidence precisely** instead of asserting FULL on docs HEAD.
5. **Completion:** written Q1-01…08 witness table with actual source SHA, registered CTest and Windows run URLs/results; zero false Resolved; fail-closed negative matrices; test source hash/paths and docs freshness; Owner technical FINAL PASS on the bounded Q1 contract plus **separate explicit permission to merge production PR**. A single green CI badge is not Owner acceptance. Keep Q2+/Datum product and native persistence unactivated.

## 8. HARD D2/D3 STOP CONDITIONS

**STOP, no silent workaround**, if: the selected Edge has no strict unique semantic source at the exact stage; a runtime endpoint cannot be paired to one inventoried current native Vertex; endpoint role requires guessing B-Rep order, coordinate/nearness, seam orientation or a screen tolerance; provider gives multiple current true terminal candidates; the only Point address is Ambiguous/Unsupported; a half-cylinder/Fillet/Chamfer/Revolve family lacks proved terminal provenance; a changed producer can be re-bound only via similar geometry; adding a Part/kernal source file, CMake/CI/test registration, schema change or new tolerance is required; a Q0 existing successful native result would cease to carry complete valid data; or there is no single correct status under the contract. Report the exact blocking witness/source/required amendment as **BLOCKED**, do not claim PASS.

**Out of Q1:** global `FeaturePointAddress` repair, new general Edge-local producer-terminal roles for unsupported families, deferred G1, MID source persistence, independent off-line Point, user hit-stack/hover/Edge-first or Point-first implementation, Datum Plane constructor, signed frame, deterministic Reverse, Undo/Redo/Save of a future new Datum source, product PL/EN UI documentation, Assembly/Drawing/Axis, PM-06, Mirror/Pattern. Owner's accepted **one-tool Datum UX** and shared-corner selected Edge behavior remain binding for future Q2, but **are not implemented or tested in Q1**.

**Single point of failure:** a strictly Resolved **Edge** still does not guarantee a **durably referenceable terminal**. If 12/24/8 fails with independently certified producer Points, the core Q1 key has no admissible basis; stop rather than implement a surrogate. Even successful canonical Q1 cannot claim universal END across unsupported global Point families.

## Documentation impact

Internal docs: **required** — accepted public Part Edge-terminal semantic value/resolution, additive current-only Kernel result membership, failure/stage/provider boundaries; canonical `docs/internal/PART_DOCUMENTS.md` and `docs/internal/PART_EDGE_PROJECTION_KERNEL.md`; regenerate `docs/browser/index.html` with `./ss2.ps1 docs`.

User/Product docs: **not required** — Q1 adds **no** user-visible Datum constructor, UI, persisted schema, input grammar or production capability. Future Q2+ must declare full PL/EN Product/Browser impact when the tool is implemented.

## 9. OWNER COMPLETE-TEXT D2 DECISION — RECORDED

**Accepted as one prospective executable Work Contract (2026-10-13):** the **complete** RG-02B1/D2-Q1 bounded reference/query API, certified-point-backed initial support limit, owning `DocumentId`/live revision read admission, additive *runtime-only* Q0 terminal-token pairing, twelve exact future paths, Q1-01…08 unrun proof matrix, failure classification, Windows validation and documentation obligations. Exact text reviewed at PR #356 HEAD `0a226abd5553340475963ef88c71bb7d264ccfb8` before this governance-only status annotation; this annotation neither adds a new implementation requirement nor changes the previously accepted API.

**Still required:** PR #356 exact-HEAD governance validation and **separate explicit Owner permission to squash merge this specific PR**, followed by an independently Owner-approved, tested and merged `work/ACTIVE.yaml`-only Q1 activation. Until then Q0 remains ACTIVE. This text/scope approval is **not** permission to merge PR #356, alter ACTIVE, implement Q1 or mark Q1 tests PASS. No speculative Q2+ phase is activated.
