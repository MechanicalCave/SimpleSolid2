# Part Modeling v1 — Program Roadmap

**Status:** ACCEPTED — PROGRAM FROZEN; PM-01 COMPLETED — PASS; PM-02P COMPLETED — PASS; PM-02 COMPLETED — PASS; PM-03 COMPLETED — PASS; PM-04 COMPLETED — PASS; PM-05 ACTIVE  
**Version:** 1.29  
**Owner acceptance:** 2026-10-06 — PM-05A D2 conclusions explicitly accepted; PM-05B authorized. v1.29 is activation-state synchronization only  
**Previous accepted version:** 1.28 — 2026-10-06  
**Decision class:** D2 program sequencing and Part-v1 scope freeze; individual D2/D3 architecture/product decisions remain owned by the package that explicitly closes them  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Current active checkpoint:** PM-05F — documentation / cumulative automated evidence / Owner Windows acceptance under `work/PM-05_EDGE_FEATURES.md`  
**Upstream readiness authority:** `work/SKETCH_ROADMAP.md` v1.9  
**Source design:** Owner Part Modeling v1 draft 0.1 plus architecture-audited draft 0.2 reviewed and accepted as the basis for this program on 2026-10-02

## 1. Purpose and authority

This roadmap makes the accepted Part Modeling v1 direction durable in the repository so it cannot disappear with chat/session context.

The Owner accepts:

- the Part Modeling v1 product direction and exclusions in this document;
- the mandatory gate order in this document;
- the split into bounded future packages;
- the architecture-evidence-first rule from the audited v0.2 design;
- the rule that no package, open decision or acceptance obligation may be silently dropped merely because the active Work Contract changes.

This roadmap is **not blanket authorization to implement Part Modeling v1**.

Only a separately Owner-accepted Work Contract named by `work/ACTIVE.yaml -> active_work` authorizes production mutation. A future package may resolve choices deliberately left open here, but it must not silently contradict this roadmap, Foundation, accepted ADRs or an earlier frozen package decision.

If the Owner later supersedes a frozen program decision, this roadmap must be amended explicitly.

## 2. Context reconstruction order

While this roadmap is referenced by `work/ACTIVE.yaml`, every Part-v1 implementation session must read:

1. `AGENTS.md`;
2. Constitution / Foundation / Architecture;
3. relevant accepted ADRs;
4. `work/ACTIVE.yaml`;
5. this roadmap;
6. `work/SKETCH_ROADMAP.md` v1.9 when the Sketcher readiness gate or Shared-2D behavior is relevant;
7. the active bounded Work Contract;
8. `governance/DOCUMENTATION.md`.

Repository/governance remains authoritative over chat handoffs.

## 3. Program entry state

At program freeze:

- SR-01 Interaction Correctness and Profile UX is completed;
- SR-02 Sketch Interaction & Presentation Latency is completed;
- SR-03 Responsive Workbench Shell is completed and merged to `main`;
- the next required action is the Sketcher profile-authoring readiness re-test;
- no production CAD Work Contract is active;
- Part Feature Tree, Body/Feature persistence, Kernel modeling operations and solid modeling remain inactive.

The initial readiness review found concrete blockers and required SR-01 -> SR-02 -> SR-03 before re-test. Completion of those stabilization packages does not itself constitute readiness PASS.

## 4. Part Modeling v1 product direction — frozen

Part Modeling v1 targets the first useful **single-Body** Part modeling workflow with full lifecycle support for every tool that is actually delivered.

Program scope includes, in bounded packages:

- one Body in Part v1, with Body identity distinct from Document identity;
- ordered Part Features and deterministic evaluation;
- Extrude Add and Cut;
- Revolve Add and Cut;
- Chamfer;
- Fillet;
- dynamic picking/hover of model geometry where required by a tool;
- construction planes, axes and points required by accepted workflows;
- Sketch support on Origin, accepted construction planes and planar model faces;
- manual projection of model edges into Sketch;
- automatic capture/projection of the selected planar face boundary, including holes, within the exact curve support accepted by the owning package;
- complete edit / preview / Finish / Cancel / Undo / Redo / Delete / visibility / Save / Close / Reopen lifecycle for each delivered tool;
- explicit failure and repair behavior for lost/ambiguous references;
- provider-neutral persistence of design intent rather than B-Rep/provider handles.

The strongest product invariant is not merely "create a solid". A user must be able to edit an earlier stage, understand the consequences, repair broken references where supported and reopen the same authored model without relying on the previous process cache.

### 4.1 Scope deliberately not frozen at program level

This roadmap does **not** yet freeze:

- exact C++ type names or class hierarchy;
- exact persistent selector representation;
- schema version number;
- exact numerical tolerance values;
- exact OCCT algorithms/options;
- exact UI geometry or visual styling;
- all operation variants;
- all datum constructors;
- exact projection refresh/merge UX;
- exact Axis editing matrix;
- exact Published References UI;
- exact performance budgets.

Those decisions belong to the package identified in Section 9 and require evidence/Owner approval where D2/D3.

## 5. Cross-program invariants

Every package preserves:

- authored semantic intent is authoritative; B-Rep/tessellation/cache/provider objects are derived;
- Part owns Body/Feature/datum/support/reference meaning;
- Shared 2D owns authored 2D geometry and region/profile semantics;
- Application/DocumentSession coordinates commands, transactions, history and evaluation but does not become a second Part model;
- Kernel API is provider-neutral; OCCT stays behind the provider boundary;
- Qt/Viewer/tree rows/presentation tokens/topology ordinals are never durable CAD identity;
- durable mutation remains GUI/AI/Script -> semantic Command -> Validation -> Transaction -> owning Document -> Evaluation;
- ambiguous or stale identity/reference fails closed;
- a clean rebuild from authored state and legal dependencies is a first-class invariant;
- one accepted authored change creates one Undo step; preview/hover/Cancel/no-op do not author state;
- presentation failure does not roll back a valid authored commit;
- no speculative universal framework is introduced without demonstrated need.

## 6. Mandatory execution order

The order is strict unless the Owner explicitly amends this roadmap:

| Order | Gate / package | Required outcome | Activation |
| --- | --- | --- | --- |
| G0 | **Sketcher profile-authoring readiness re-test** | prove current Sketcher can deliberately author practical Profiles after SR-01/02/03 | **COMPLETED — PASS on `16df8d1940a1f438a66cbd6964a881ba24718c0d`** |
| 1 | **PM-00A — Part Modeling Architecture Evidence Gate** | first establish Verification Topology (semantic/core, kernel-native, desktop), then prove reference lineage, support frames, Profile->Kernel boundary, numerical policy and cold rebuild assumptions before durable solid schema | **COMPLETED — PASS; final source candidate `ac34a713c6fee4a53d513bfcce2a2044ce1a0ffb`, Windows FULL #1320; Owner-review synthesis prepared** |
| 2 | **PM-00B — Part Feature Architecture Freeze** | Owner-approved ADR/contract freeze for first solid workflow based on PM-00A evidence | **COMPLETED — PASS; source candidate `4cf065c844a523be4e7c2ae88b4d78a1ae89d7e9`, Windows FULL #1323** |
| 3 | **PM-01 — Extrude Feature Vertical Slice: Body / Feature / Add / Cut** | durable Body/Feature architecture plus complete Extrude Add/Cut OneSide/Midplane lifecycle | **COMPLETED — PASS; Owner final manual acceptance 2026-10-03; runtime FULL #1369, docs #1370** |
| 4 | **PM-02P — Body Semantic Topology Evidence Gate** | prove complete Face/Edge/Vertex accounting, carrier Surface/Curve/Point semantics, deterministic planar frames, split/merge/delete behavior and prospective dynamic Sketch support before production schema/API | **COMPLETED — PASS; runtime FULL #1387; Owner accepted D2 synthesis 2026-10-04** |
| 5 | **PM-02 — Body Semantic Topology / Face-Supported Sketch** | production semantic topology catalog and picking plus Sketch/Profile on arbitrary resolved planar Body surfaces, reusing PM-01 Extrude Add/Cut | **COMPLETED — PASS; runtime FULL #1473, docs #1474, Owner final manual Windows PASS 2026-10-05** |
| 6 | **PM-03 — Datum Reference Geometry** | bounded Offset Datum Plane + Datum-backed Sketch support on PM-02 semantic references | **COMPLETED — PASS; Owner final Windows PASS 2026-10-05; translucent Origin/Datum plane fill deferred as PM-06 presentation polish** |
| 7 | **PM-04 — Axis / Revolve** | Axis semantics plus complete Revolve Add/Cut lifecycle using accepted reference geometry | **COMPLETED — PASS; runtime FULL #1578 attempt 2; Owner final Windows PASS 2026-10-06** |
| 8 | **PM-05 — Edge Features: Fillet / Chamfer** | complete explicit multi-Edge Fillet/Chamfer lifecycle with connected-corner, lineage/refine handling and repair | **ACTIVE — Owner accepted 2026-10-06; PM-05A/B/C/D/E COMPLETED — PASS; current checkpoint PM-05F** |
| 9 | **PM-06 — Part v1 Completion / Published References / Evidence** | close accepted Part-v1 scope, minimal future-Assembly read contract, docs and performance evidence; no Assembly implementation | future separate Work Contract |

No package number authorizes mutation by itself.

### 6.1 Why the audited PM-01/PM-02 infrastructure split is not preserved literally

The audited v0.2 draft correctly requires minimal Body/Feature schema work before Extrude, but the accepted program deliberately forbids a long standalone "infrastructure first" implementation phase.

PM-01 therefore combines only the Body/Feature identity, persistence, evaluation, tree projection, bounded Viewer presentation and Kernel API required to prove the accepted Extrude Feature family: multiple ordered Add/Cut Features with OneSide/Midplane. ADR-0015 explicitly supersedes the earlier single-Add product-scope slice before production mutation began.

Architecture that cannot be justified by PM-01's concrete workflow remains out of scope.

### 6.2 Why PM-02 sequencing is amended after PM-01

Owner review after PM-01 concluded that implementing Datum first as only an Origin-based offset-plane special case would create a temporary support/reference architecture before Body topology is available.

ADR-0016 therefore deliberately moves semantic Body topology and face-supported Sketch ahead of Datum. PM-02P is an evidence-only gate that must prove complete topology accounting, carrier geometry, Edge/Vertex lineage and deterministic planar support frames before production PM-02 schema/API is accepted.

Projection remains an accepted Part-v1 product concern but is independent of this critical path. It is not part of PM-02P, production PM-02 or PM-03 Datum activation and requires separately accepted ownership before implementation.

## 7. G0 — Sketcher profile-authoring readiness re-test

G0 is the entry gate for Part architecture work. It is not a solid-modeling Work Contract.

**G0 result:** COMPLETED — PASS on 2026-10-02 on exact candidate `16df8d1940a1f438a66cbd6964a881ba24718c0d`. Evidence is recorded in `work/G0_SKETCHER_PROFILE_AUTHORING_READINESS_RETEST.md`.

Use the accepted criteria in `work/SKETCH_ROADMAP.md` v1.9. At minimum the supported Windows evidence must cover:

- deliberate creation/editing of Line, Circle, Arc and Rectangle;
- usable Regular/Construction workflow;
- read-only inspection/measurement;
- precise numeric/unit-aware point entry;
- Polar and Dynamic Input;
- OSNAP/Tracking/Inference sufficient for deliberate placement;
- Trim / Extend / Extend-Both with accepted identity/history behavior;
- Undo/Redo and Save/Close/Reopen;
- deliberate creation of valid Profile geometry without automatic gap healing or Viewer-dependent closure.

**If G0 fails:** do not activate PM-00A. Record concrete findings and close them through separately accepted bounded stabilization work, then repeat G0.

**If G0 passes:** no solid modeling becomes active automatically. PM-00A may then be proposed and explicitly accepted.

## 8. PM-00A — Part Modeling Architecture Evidence Gate

### Goal

Prove the minimum semantic contracts required to freeze single-Body / Feature evaluation architecture **before** persistent solid-modeling data or the first production Feature is introduced.

### Phase A0 — Verification Topology

PM-00A begins with an obligatory verification-topology slice before E01–E10 architecture evidence.

This phase exists to keep Part modeling evidence aligned with SS2 ownership boundaries. It is **not** a separate speculative CI program and must not invent a broad Kernel framework before concrete PM-00A evidence requires it.

The accepted verification topology has three distinct build/test modes:

```text
semantic/core
    Qt forbidden
    OCCT forbidden
    -> provider-independent Core / Sketch / Part / Application semantics

kernel-native
    Qt forbidden
    OCCT required
    -> provider-neutral Kernel boundary + OCCT provider + Part/kernel evidence

desktop
    Qt required
    OCCT required
    -> full Workbench / Viewer / picking / presentation integration
```

The existing semantic/core boundary remains authoritative and must not regress when Kernel support is introduced. Part/domain semantics that do not intrinsically require a geometry provider remain testable without Qt and OCCT.

Before E01–E10 may be accepted as PM-00A evidence, Phase A0 must establish:

1. preservation of the current core-only no-Qt/no-OCCT build and its semantic test authority;
2. the smallest provider-neutral Kernel boundary required by the active PM-00A evidence cases;
3. an OCCT-backed **kernel-native Release** build/test lane with Qt discovery explicitly disabled;
4. a stable `subsystem-kernel` test classification/aggregate in addition to the existing subsystem labels;
5. repository verification that every registered regression has explicit, valid verification metadata and cannot silently fall outside the intended subsystem/tier graph;
6. a Release kernel evidence target suitable for B-Rep validity, generated/modified/deleted lineage, split/merge/refine and tolerance cases;
7. a **cold-model-rebuild harness** that destroys runtime DocumentSession/evaluated B-Rep/provider/cache state, reloads authored inputs and reevaluates from legal durable intent;
8. exact-head proof that semantic/core, kernel-native and desktop modes all pass on the same candidate revision.

Build-tree "clean/warm" state and model-evaluation "cold rebuild" are separate concepts. Deleting/reconfiguring a CMake build tree is not evidence that authored Part intent can reconstruct its geometry and references without previous-process runtime state.

### Phase A0 pipeline rules

The existing `FOCUSED / FAST / SUBSYSTEM / FULL / DOCS / CLOSURE` public verification model remains the default. PM-00A does not authorize a new developer-facing test tier merely because kernel-native tests are added.

Expected use:

- FOCUSED remains the cheapest exact target/test iteration mechanism;
- FAST remains broad draft iteration evidence;
- SUBSYSTEM gains `kernel` and may combine `part,kernel,persistence` for explicit checkpoints;
- FULL remains mandatory merge evidence and must include the complete desktop regression plus semantic/core and kernel-native verification;
- DOCS/CLOSURE trusted-FULL behavior remains unchanged.

Do not add path-based pseudo-dependency analysis such as automatically assuming every `src/part/**` edit is fully covered by only Part tests. Selection remains explicit/fail-closed until measured repository growth proves a stronger mechanism necessary.

### Phase A0 test-graph discipline

Part Modeling will introduce many geometry/reference scenarios. The default design is to avoid one heavy executable per scenario when a shared harness can provide independent CTest cases.

A bounded provider test executable may expose multiple exact scenario cases (for example lineage split/merge/refine variants) while each case remains separately reportable in CTest. This reduces repeated compile/link cost without reducing FULL coverage or merging distinct semantic assertions into one opaque pass/fail result.

Test metadata must have one authoritative declaration path. The implementation may choose the CMake helper shape, but it must prevent drift between:

- registered test;
- FAST/FULL-only classification;
- subsystem labels;
- aggregate build target membership.

A metadata self-test must fail closed when a registered regression is unclassified or carries contradictory verification metadata.

### Phase A0 performance rule

Do not pre-emptively introduce Ninja migration, compiler cache, PCH, Unity Build, distributed workers or test sharding.

First measure the real PM-00A/PM-01 build and test graph on the supported Windows machine. Further CI/build optimization requires separate measured evidence if the new graph becomes a material development bottleneck.

### Evidence scope

PM-00A may build bounded technical probes and test-only/minimal provider adapters required to answer architecture questions. It must not expose a production Body/Feature schema or user-facing solid-modeling feature.

Required evidence areas:

1. exact Profile boundary uses -> Kernel input, including outer loop, holes, trimmed fragments, orientation and provenance;
2. topology lineage for generated / modified / deleted outputs across representative Extrude -> Cut -> Fillet-style operations;
3. semantic roles such as start cap / end cap / side derived from a specific profile-boundary use, never Face[n]/Edge[n];
4. split / merge / delete / geometrically-similar candidate behavior;
5. cold rebuild after process/cache teardown;
6. full stable support frame O/U/V/N independent of camera, Viewer and arbitrary provider parameterization;
7. separation of authored-structure validity, reference-resolution validity and geometric executability;
8. numerical/tolerance/refine/healing policy evidence;
9. generation/session isolation for stale preview/evaluation results;
10. Part identity/snapshot assumptions required later by Assembly without implementing Assembly.

### Phase A0 completion gate

E01–E10 architecture evidence may begin only after the active PM-00A contract records Phase A0 PASS on an exact revision. A Phase A0 failure is a bounded verification/infrastructure finding inside PM-00A unless resolving it would cross a D2/D3 ownership, dependency or persistence boundary; such a boundary requires Owner review.

Phase A0 PASS does **not** authorize persistent Body/Feature schema, Part Feature Tree product behavior or a user-facing solid operation.

### Mandatory PM-00A evidence cases

- **E01:** Profile with a hole -> Extrude; exact boundary and cap/side provenance.
- **E02:** Extrude -> Cut -> Fillet-style lineage; edit upstream dimension and recompute mapping.
- **E03:** one referenced edge splits; never silently select the first fragment.
- **E04:** previous candidates merge; return the contractually correct Missing/Ambiguous outcome unless a separately defined aggregate reference applies.
- **E05:** geometrically similar faces/edges; no false Resolved by proximity alone.
- **E06:** full Revolve probe with seam/similar surfaces to expose periodic-topology risks.
- **E07:** remove runtime caches/process state and reconstruct from authored probe inputs.
- **E08:** support frame survives dimension/topology change without accidental 180-degree flip.
- **E09:** stale preview/result cannot publish after revision/session-context replacement.
- **E10:** tolerance/refine matrix proves no dependence on zoom, pick aperture or display settings.

### PM-00A acceptance asymmetry

A false-positive reference resolution is an integrity failure.

Completion requires **zero false Resolved** in the accepted matrix. However, a resolver that returns Ambiguous/Missing for every edited model also fails the product goal. Cases declared stable in the accepted matrix must resolve automatically after edit and cold rebuild.

### PM-00A deliverables

- Phase A0 verification-topology report with exact-head semantic/core, kernel-native and desktop evidence;
- architecture evidence report;
- reference survival/failure matrix;
- support-frame evidence;
- numerical/modeling-semantics recommendation;
- proposed resolutions for O-01/O-04/O-05/O-09/O-11 and the required foundations of O-12;
- O-03/O-06 product-scope matrix for the first production slice and later v1 packages;
- Owner-reviewable Part Feature Architecture ADR candidate;
- PM-00B Work Contract candidate.

### PM-00A STOP conditions

STOP for Owner review if evidence suggests:

- persisting OCCT/OCAF/provider topology identity as SS2 authored truth;
- using topology ordinals, tree position or Viewer tokens as durable references;
- geometry-similarity alone as automatic identity;
- changing Foundation/provider ownership;
- a global universal parametric/dependency framework as a prerequisite;
- multi-body as a prerequisite;
- durable schema before reference semantics are resolved;
- Part/Assembly domain dependency on `viewer::` math types;
- implementing Assembly/occurrence/solver infrastructure to make Part work.

## 9. Decision ledger and owning package

The audited design uses O-01...O-12 as a durable decision ledger. Decisions remain open until explicitly closed by their owning package; PM-00B now closes the ADR-0014 foundations and records the accepted initial O-03/O-06 matrix.

| ID | Decision | Program direction | Must be closed by |
| --- | --- | --- | --- |
| O-01 | exact single-Body semantics: creation, Empty, valid one-solid result, no-effect/multi-solid outcomes | one Body in v1; do not alias BodyId to DocumentId | **CLOSED by ADR-0014 / PM-00B** |
| O-02 | exact datum constructors and Mid semantics | only constructors justified by v1 workflows; no provider-parameter Mid shortcut | owning datum package PM-03 |
| O-03 | projection curves, snapshot/refresh, atomic face-boundary capture | snapshot intent; exact supported curves or explicit UnsupportedCurve; no silent approximation | **INITIAL MATRIX ACCEPTED by PM-00B; final in separately activated Projection ownership, outside PM-02/PM-03 critical path** |
| O-04 | support-frame construction, re-support and orientation | full stable O/U/V/N rule; preserve local 2D by default; no silent fallback | **FOUNDATION CLOSED by ADR-0014; dynamic carrier-frame amendment accepted by ADR-0016, evidence PM-02P** |
| O-05 | topology-reference selector, lineage and split/merge guarantees | semantic producer/stage/role lineage, fail closed, user repair; no Face[n]/Edge[n] | **FOUNDATION CLOSED by ADR-0014; topology/carrier extension accepted by ADR-0016; PM-02P PASS and 2026-10-04 Owner D2 acceptance add bounded semantic branch/provenance discrimination for singular Edge/Curve, otherwise Ambiguous** |
| O-06 | exact variants/inputs per modeling operation | freeze a bounded v1 matrix; lifecycle completeness over variant breadth | **EXTRUDE MATRIX AMENDED/CLOSED for PM-01 by ADR-0015; later operations remain owned by their packages** |
| O-07 | Sketch Axis representation/editing/direction/visibility | Origin X/Y/Z are built-in AxisReferences; authored Axis is Part-owned and derived from a Regular/Construction Sketch Line without conversion; independent visibility; no new Shared-2D Axis role | **CLOSED by Owner-accepted PM-04 Work Contract 2026-10-06** |
| O-08 | Published References/local frames and v1 UI scope | stable typed interface identity; no Assembly implementation | semantics by PM-00B; UI/final scope PM-06 |
| O-09 | downstream failure commit, Delete, Suppress, retry/history semantics | preserve repairable downstream intent; separate Delete from Suppress; no hidden history entries | **CLOSED by ADR-0014 / PM-00B** |
| O-10 | visibility, auto-hide, history inspection and performance budgets | visibility never changes evaluation; measure before optimize | **Profile-consumption visibility closed for PM-01 by ADR-0015; later tools/final budget remain owning-package/PM-06 work** |
| O-11 | numerical tolerances, refine/healing, exact Profile->Kernel input, modeling semantics version | explicit versioned policy; no display tolerance leakage or escalating fuzzy success | **CLOSED by ADR-0014 / PM-00B** |
| O-12 | ID scopes, Part snapshot/read contract, transform direction, durable version vs DocumentRevision | distinguish Document/Body/local IDs and runtime revision; no Assembly solver | **FOUNDATIONS CLOSED by ADR-0014 / PM-00B; minimal interface PM-06** |

A package must not silently make a ledger decision owned by a later package if doing so would constrain that later decision durably. If a dependency is discovered, STOP and amend sequencing explicitly.

## 10. PM-00B — Part Feature Architecture Freeze

**Status:** COMPLETED — PASS. Owner accepted 2026-10-03; ADR-0014 is accepted and synchronized; source candidate `4cf065c844a523be4e7c2ae88b4d78a1ae89d7e9` passed Windows FULL #1323.

PM-00B is intentionally short and governance-heavy.

It converts accepted PM-00A evidence into:

- accepted Part Feature Architecture ADR(s);
- frozen O-01/O-04/O-05/O-09/O-11 and required O-12 foundations;
- the accepted first product-scope slice from O-03/O-06;
- a bounded PM-01 production Work Contract;
- synchronized persistence/reference/failure semantics documentation.

PM-00B must not become a second research phase or a general modeling-framework implementation.

## 11. PM-01 — Extrude Feature Vertical Slice: Body / Feature / Add / Cut

**Status:** COMPLETED — PASS. Owner accepted activation 2026-10-03 under ADR-0015 and reported final manual Windows PASS 2026-10-03.

PM-01 is the first authorized production-solid package.

**Completion evidence:** runtime remediation through H7 is merged; Windows FULL #1369 PASS on `aaab5610f8f7c6fa3036d8123849523d06dd66e1`; internal + PL/EN Product docs/Product Browser Windows DOCS #1370 PASS; Owner final manual acceptance PASS on `afecfa9884bfeb53d5d0bb84ff110b3b280cf8d2`. PM-02 is not activated by this completion.

Minimum complete workflow:

`existing valid Profile -> Extrude Add -> Body -> additional Extrude Add/Cut -> edit/recompute -> Undo/Redo -> Save/Close/Reopen -> cold rebuild`.

PM-01 owns the bounded implementation required for that workflow:

- BodyId / FeatureId and one-Body validation;
- ordered Feature state and evaluation;
- schema migration preserving current Sketch/Profile/Entity identity;
- Profile visibility policy with automatic hiding when consumed;
- exact Profile -> Kernel boundary;
- provider-neutral production Kernel API + OCCT Extrude/Fuse/Cut;
- Add/Cut with OneSide/Midplane and OneSide Reverse;
- dynamic single-draft preview with one Finish;
- bounded provider-neutral solid/preview Viewer presentation, without topology picking;
- cap/side semantic lineage required by later consumers;
- deterministic recompute/status and Delete/Suppress;
- Tree/Properties bidirectional Profile <-> Feature identification;
- complete GUI/Command Line/semantic API parity;
- documentation and Windows manual acceptance.

ADR-0015 supersedes the earlier one-sided Add-only O-06 slice before PM-01 production mutation began. No speculative framework beyond the accepted Extrude Feature family is authorized.

## 12. PM-02P — Body Semantic Topology Evidence Gate

**Status:** COMPLETED — PASS. Owner accepted the final D2 synthesis/recommendation on 2026-10-04.

Authority:

- `adr/ADR-0016-body-semantic-topology-carrier-geometry-and-face-supported-sketch.md`;
- `work/PM-02P_BODY_SEMANTIC_TOPOLOGY_EVIDENCE.md`;
- `work/PM-02P_TOPOLOGY_EVIDENCE_MATRIX.md`;
- `work/PM-02P_TOPOLOGY_EVIDENCE_REPORT.md`;
- `work/PM-02P_REFERENCE_SURVIVAL_MATRIX.md`;
- `work/PM-02_PRODUCTION_ARCHITECTURE_RECOMMENDATION.md`.

Final runtime evidence:

- exact source `e259fef5849bc4707ddcbdda8e75795b8f3fa5b4`;
- Windows FULL #1387 PASS;
- merged runtime main `a0dad01dd584eb1b1c39a4cb282b0bbc009000aa`;
- synthesis/closure #1388 PASS;
- synthesis merged main `72cbf070b4e3a64a737fce25137621b82d6dbee5`.

PM-02P proved:

- complete Face/Edge/Vertex accounting after every successful tested Body stage;
- semantic Surface/Curve/Point carrier meaning distinct from bounded topology;
- stable and fail-closed lineage through current Extrude Add/Cut Boolean topology;
- explicit seam/representation-artifact handling;
- deterministic planar carrier O/U/V/N frames;
- cold-rebuild and stale-generation behavior;
- prospective face-supported Sketch movement without authored world-placement mutation;
- bounded dependency-cycle rejection without a universal dependency graph.

Final required metrics are zero for false Resolved, unaccounted Face/Edge/Vertex, frame instability, cold semantic mismatch and stale acceptance.

Owner D2 acceptance additionally freezes this production rule:

> A singular Edge/Curve selector may use a bounded semantic branch/provenance discriminator only when the producing operation supplies defensible semantic meaning. If no such meaning exists, the selector remains Ambiguous.

PM-02P PASS authorizes only preparation of the production PM-02 Work Contract.

## 13. PM-02 — Body Semantic Topology / Face-Supported Sketch

**Status:** COMPLETED — PASS. Owner accepted the exact production Work Contract and all four UX/Viewer design inputs on 2026-10-04 and reported final post-remediation Windows PASS on 2026-10-05.

Completed authority:

`work/PM-02_BODY_SEMANTIC_TOPOLOGY_FACE_SUPPORTED_SKETCH.md`.

Checkpoint state:

- `PM-02A — production evaluated Body-stage topology catalog + typed runtime Face/Edge/Vertex tokens` — **COMPLETED — PASS**; runtime candidate `58368a6ada5f6c30e03cb845682dbf5408ecb763`, Windows FULL #1397 PASS, merged runtime main `b95f72105b8242eedce9cd79dd8bf910f3ccb715`;
- `PM-02B — Surface/Face production semantics` — **COMPLETED — PASS**; final runtime candidate `05434516fea0892aac2babae09da772043987f29`, Windows FULL #1400 PASS, merged final runtime main `65d074577bc89e3507288118b17e99b12288a881`;
- `PM-02C — Edge/Curve and Vertex/Point semantic catalog` — **COMPLETED — PASS**; runtime candidate `9135baf1d749f01eeb5dcae4f6ea7cffc856d8a0`, Windows FULL #1402 PASS, merged runtime main `b128f7445b2705ec9577afddcdbf7f9c81594aa0`;
- `PM-02D — topology-aware Body presentation, Viewer picking, View Styles and semantic inspection` — **COMPLETED — PASS**; final runtime candidate `c325fcf681afff44ca61abb02fd239495db5d67f`, Windows FULL #1421 PASS, merged final runtime main `9dd893ffa0e9cd1e5287b9590c02b317d543cbd3`;
- `PM-02E — Sketch support schema v9 + deterministic frame resolver` — **COMPLETED — PASS**; final runtime candidate `8b5e2c6844c93648e1884464d138d9a98e2a7a9a`, Windows FULL #1431 PASS, merged runtime main `e3452d4d0051564f332319a7945344655e91aa3a`;
- `PM-02F — stage-aware Sketch/Profile evaluation` — **COMPLETED — PASS**; final runtime candidate `87d4043f9114952df8228681e4cf16937c6f664b`, Windows FULL #1434 PASS, merged runtime main `f620d7e9888c09a6c4e9854bca6b3092699c1fe3`;
- `PM-02G — create/re-support Sketch on arbitrary planar Body Surface` — **COMPLETED — PASS**; final runtime candidate `f45ed8ee09aabe098ab88bc7e47a0c614841fb40`, Windows FULL #1443 PASS, merged runtime main `17778bdfe80d0411031b458ef4f6d582eccc205b`;
- `PM-02H — existing Extrude Add/Cut from face-supported Sketch` — **COMPLETED — PASS**; final runtime candidate `0540a762be42b3f4329e7032545e652b21332486`, Windows FULL #1446 PASS, merged runtime main `16818f4b47d4ec2f1c18410856e2b1c1ba51f2c0`;
- `PM-02I — lifecycle, repair, persistence and regression matrix` — **COMPLETED — PASS**; final runtime/evidence candidate `b642a5af625a48d668a21efeaf3d78bfb9bf7c79`, Windows FULL #1451 PASS, merged runtime/evidence main `a26d7e981b160973825603bc3c11f37d5ab594a8`;
- `PM-02J — documentation and Owner Windows acceptance` — **COMPLETED — PASS**; post-remediation runtime FULL #1473 PASS, docs #1474 PASS, bookkeeping #1476 PASS, Owner Windows re-test PASS 2026-10-05.

Primary vertical scenario:

`existing Body -> select any resolved planar Body Face -> semantic Surface support -> Sketch -> Profile -> existing PM-01 Extrude Add/Cut -> edit upstream -> semantic re-resolution -> recompute or structured failure`.

PM-02 must not special-case only top/bottom caps. All Body Faces are accounted; arbitrary resolved planar cap/lateral/Cut-exposed Faces are eligible Sketch supports. Non-planar Faces remain fully catalogued/selectable but are explicitly unsupported as standard planar Sketch support.

The completed PM-02 production contract owns the delivered evaluated stage topology catalog/picking, deterministic planar carrier frames, topology-aware Body presentation/View Styles, direct topology selection, topology Properties/inspection, schema-v9 Sketch-support migration, face-supported Sketch/re-support, stage-aware evaluation, lifecycle/persistence and repair semantics.

General durable Edge/Vertex persistence is not introduced speculatively; evaluated semantic Edge/Curve and Vertex/Point catalogs are required, while durable serialized selectors remain consumer-driven.

Projection is explicitly out of PM-02.

## 14. PM-03 — Datum Reference Geometry

**Status:** ACTIVE — Owner accepted the exact bounded Work Contract and UI/interaction amendments on 2026-10-05.

Active authority:

`work/PM-03_DATUM_REFERENCE_GEOMETRY.md`.

PM-03 deliberately delivers **Offset Datum Plane only** as the first construction-reference vertical slice. Datum Axis and Datum Point remain deferred until a concrete accepted consumer justifies them.

Accepted sources are Origin XY/XZ/YZ planes, any uniquely resolved planar semantic Body Surface at an explicit Body stage, and an existing Datum Plane. The package adds durable DatumId, deterministic offset-frame evaluation, bounded Datum dependency/cycle rules, Datum-backed Sketch support, persistence/lifecycle, one shared GUI/Command Line draft, Extrude-style default 10 mm preview, presentation-only plane/Body intersection overlay, and Reference Geometry tree grouping/visibility.

PM-03 must reuse PM-02 semantic SurfaceReference/BodyStageRef meaning. No provider-native topology identity, geometry-similarity rebinding, global dependency graph, Projection prerequisite, Datum Axis or Datum Point is authorized by this package.

PM-03A semantic Datum foundation + schema v10 is **COMPLETED — PASS**; exact candidate `41ea901c873f1453e27b2b4973332ecb5295388c` passed Windows FULL #1485 and merged to main as `542d7b50fcd3f7566326f1b2c849d9453a3dcd00`.

PM-03B Datum evaluator + dependency/cycle semantics is **COMPLETED — PASS**; exact candidate `e12f6c606132eadab5283372dee1a9f3f3008abd` passed Windows FULL #1491 and merged to main as `42b36d427bdb4c5c75b4961444576c7905032eb2`.

PM-03C commands + Extrude-style draft / Command Line parity is **COMPLETED — PASS**. C1 exact candidate `c633a9714602801e1253d119198ea9e46f73f9a9` passed Windows FULL #1496 and merged as `4147eba3697dfc345f49762508e92293b6373ff5`; C2 exact candidate `8b9185261e2cf32006f6af7a8c8a9da5e7c9ec04` passed Windows FULL #1507 and merged as `ad9e64df059091556bca2dc18515fa25a5cc5a32`. Completion evidence: `work/PM-03C_COMMANDS_DRAFT_COMMAND_LINE_COMPLETION.md`.

PM-03D Viewer / intersection overlay / Tree / Properties is **COMPLETED — PASS**. D1 exact candidate `b236e54028a64ba00569a995b592b4ab8de2cd17` passed Windows FULL #1511 and merged as `28c86f2cb62c5c9156cd2d1ca3f305432957eedf`; D2 exact candidate `adc9dd5c1f970932a03aa43422a105643db9052d` passed Windows FULL #1514 and merged as `3aa23632892853dbf9bf51574e5b2c762c2ac426`. Completion evidence: `work/PM-03D_VIEWER_TREE_PROPERTIES_COMPLETION.md`.

PM-03E Datum-backed Sketch + existing Extrude is **COMPLETED — PASS**. Exact candidate `f4c691c50ce28bc311076b94925b719c889e8172` passed Windows FULL #1520 and merged as `66f31391894eb6a0adfb430d4dd78b37e749f150`. Completion evidence: `work/PM-03E_DATUM_BACKED_SKETCH_EXTRUDE_COMPLETION.md`.

PM-03F lifecycle / persistence / docs / Owner acceptance is **COMPLETED — PASS**. F1 exact runtime evidence passed Windows FULL #1522; F2a live Datum draft-preview remediation passed Windows FULL #1523; final docs candidate `439abd15b9e7ec1c4c8b2177cd8ea31f6debda0e` passed workflow #1524 and merged as `4e9ecdb4d51f7f81be799bbc0778b98808799f3d`. Owner final Windows workflow PASS was reported on 2026-10-05.

Owner acceptance explicitly defers one presentation-only item: add a coherent neutral translucent fill to both Origin planes and Datum planes during PM-06 final Part-v1 polish. This does not defer Datum semantics, identity, lifecycle, persistence, repair, picking, intersection overlay or downstream modeling.

Projection is not a prerequisite for Datum and remains separately gated.

## 15. PM-04 — Axis / Revolve

**Status:** COMPLETED — PASS; Owner final Windows acceptance 2026-10-06.

Active authority:

`work/PM-04_AXIS_REVOLVE.md`.

PM-04 closes O-07 with two AxisReference variants only: built-in Origin X/Y/Z and Part-owned authored Axis derived from one non-degenerate Sketch Line. Authored Axis has durable AxisId, independent visibility and explicit repair; it does not add a new Shared-2D Axis role and it is not Datum Axis.

The Revolve family is complete within the accepted bounded matrix: Add/Cut, OneSide/Midplane, default 360°, OneSide Reverse, full Create/Edit/preview/Finish/Cancel/Suppress/Delete/Undo/Redo/Save/Reopen/cold-rebuild lifecycle and GUI/Command Line parity. Axis/Profile admission is coplanar and fail-closed; material may touch or lie along the Axis but may not cross it internally.

Full-rotation periodic seams remain accounted representation artifacts unless they are true material boundaries; PM-04 must extend the existing semantic topology catalog rather than bypass it.

PM-04A Axis semantic model + persistence is **COMPLETED — PASS**. Exact candidate `9a01ec707c12ac699dcd1d5b9e1a4e7d8bc2578a` passed Windows FULL #1531 and merged to main as `4c6ed90e59e365fe6d53a88c68184b6332743749`. Completion evidence: `work/PM-04A_AXIS_SEMANTIC_SCHEMA_V12_COMPLETION.md`.

PM-04B Axis lifecycle / Tree / Properties / Viewer is **COMPLETED — PASS**. B1 exact candidate `37bdbd55cbb8ee2fc2120c88d0de80358c47b6aa` passed Windows FULL #1533 and merged as `3d6d36c7e2253c0f071090d96d836abfe6c44d57`; B2 exact candidate `da5e64dcaca0238391e4710f2edaacdfd198413b` passed Windows FULL #1544 and merged as `e6660287c19ab245d622fad470e6d58323aeca24`. Completion evidence: `work/PM-04B_AXIS_LIFECYCLE_TREE_VIEWER_COMPLETION.md`.

PM-04C Revolve semantic/kernel operation + topology catalog is **COMPLETED — PASS**. C1 exact candidate `57cb332f2c6e8e3032a3647df3c8457d4d1583a3` passed Windows FULL #1547 and merged as `704879e90aa56f8338164d1ccdb918d122d0e03b`; C2 exact candidate `6e627749b850524c0dc3730ed19c7f9506b37536` passed Windows FULL #1549 and merged as `70d1b5986fa87d736626afbe858a6c096eabd36d`. Completion evidence: `work/PM-04C_REVOLVE_SEMANTIC_KERNEL_TOPOLOGY_COMPLETION.md`.

PM-04D Revolve draft / Operations / Command Line / preview is **COMPLETED — PASS**. D1 exact candidate `24975b9bdd8f2e97cc11f2c7fef77bdbc9c48de3` passed Windows FULL #1551 and merged as `84bddafb57eab4f7776b78621b81369c2917899b`; D2 exact candidate `c275045387b7fa928dcc955ba3837565796b1328` passed Windows FULL #1555 and merged as `92348a8af84679ee2ce77b099d9c337dfec2dc84`. Completion evidence: `work/PM-04D_REVOLVE_DRAFT_OPERATIONS_PREVIEW_COMPLETION.md`.

PM-04E integrated lifecycle / repair / persistence is **COMPLETED — PASS**. E1 exact candidate `dad92ce54fb0cf69ae96b7210d5f49bfce3f9402` passed Windows FULL #1560 and merged as `a559f7b2d6a667e4d4b34cbfd7af6f2d93cd65e1`; E2 exact candidate `bdf87a29b5cdc27c608023af863e32c44dc55ef1` passed Windows FULL #1562 and merged as `e9a1056da4ec3639458aa1cdf4faacfc1621fcc9`; E3 exact candidate `b7dcd55ab9c04fdf208a534f6e09de7ffac0e00f` passed Windows FULL #1563 and merged as `1a7e1228a91b23f89bf13f6bb5c07c8abe9afae3`. Completion evidence: `work/PM-04E_REVOLVE_LIFECYCLE_PERSISTENCE_COMPLETION.md`.

PM-04F Axis-designation remediation / documentation / final acceptance is **COMPLETED — PASS**. Exact final runtime/docs candidate `8d6f082137a573aab08a4eee3b383a9923d98a49` passed Windows FULL #1578 attempt 2 with core 25/25, kernel-native 47/47 and desktop 104/104, including canonical documentation/Product Browser verification. The Owner executed the supported Windows acceptance workflow on 2026-10-06 and reported PASS with no errors. Completion evidence is recorded in `work/PM-04_FINAL_ACCEPTANCE_MATRIX.md`.

PM-04 production mutation authority is closed. PM-05 Edge Features is **ACTIVE** under `work/PM-05_EDGE_FEATURES.md`; PM-05A, PM-05B, PM-05C, PM-05D and PM-05E are COMPLETED — PASS and PM-05F is the only active checkpoint. Datum Axis, Projection and PM-06 remain separately gated.

## 16. PM-05 — Edge Features: Fillet / Chamfer

**Status:** ACTIVE — Owner accepted 2026-10-06. PM-05A/B/C/D/E are COMPLETED — PASS. Current checkpoint: **PM-05F — documentation / cumulative automated evidence / Owner Windows acceptance**.

PM-05 adds edge-consuming features only after topology-reference semantics are proven in earlier packages. The accepted production direction is explicit 1..N material-Edge Fillet/Chamfer with mandatory common connected-corner behavior; a single-Edge-only production fallback is forbidden.

PM-05A is COMPLETED — PASS. The evidence package is merged as `aa9e86f6b88ebf429390f7a4764a534b400e757b`; exact runtime/test candidate `c70980d63d836ea54d1f88be29082023e44b1c38` passed Windows FULL #1605 and the final report-sync head `d08b76dae60456df5cd0300ac81c971eb5febc00` passed Windows CLOSURE #1606. The Owner explicitly accepted the D2 conclusions in `work/PM-05A_EDGE_FEATURE_TOPOLOGY_EVIDENCE.md` on 2026-10-06 and authorized PM-05B.

PM-05B is COMPLETED — PASS. B1 exact candidate `abb758a17e52849db4345e03dc84915543718ad4` passed Windows FULL #1613 and merged as `bfe197b78ff1bc81a8e2e4bd26f2f3e204b429ab`; B2 exact candidate `9b6c6318599ec85300c7b3ebb0ebc24720d5c287` passed Windows FULL #1617 and merged as `13e2c14505be6d25b733b95c71ca891d16a7e4b4`; B3 generated-Surface provenance exact candidate `384835c58c5823e5f5ac20c619eb960f3a5acb7a` passed Windows FULL #1624 and merged as `cab3cea663a07102e1749d08dc027a85764db790`. Completion evidence: `work/PM-05B_DURABLE_EDGE_REFERENCE_SCHEMA_RESOLVER_COMPLETION.md`.

PM-05C is COMPLETED — PASS. C1 exact candidate `2f55b8382b6fe8022ad26843a7f0eb4dcb15bd1c` passed Windows FULL #1632 and merged as `9547b986ccbd4dc7533c446c797eb5c4886ad0a6`; C2 exact candidate `e5826858a1ef2580a3c00913f94791653696dc50` passed Windows FULL #1660 and merged as `85c7df1395c59a35b1a80fdf446cdb9c2f77775c`. Completion evidence: `work/PM-05C_EDGE_FEATURE_KERNEL_EVALUATION_TOPOLOGY_COMPLETION.md`.

PM-05D is COMPLETED — PASS. Exact candidate `7f57726b368c8a5011f8007369f3de945ae0d833` passed Windows FULL #1697 with final `windows-msvc` aggregate PASS and merged as `83ece0b03889e11f8dc7769498d949caee3c0be1` (#290). Completion evidence: `work/PM-05D_WORKBENCH_VIEWER_PREVIEW_COMMAND_LINE_COMPLETION.md`.

PM-05E is COMPLETED — PASS. Exact candidate `88e07b1a2dbdb6ded73d0c2f90dd0a1443242cbe` passed Windows FULL #1721 with core-only 25/25, kernel-native 54/54 and desktop 112/112; final `windows-msvc` aggregate PASS and squash merge #292 is `1a1491f7687f32089c1424025123329c74d7b6ca`. Completion evidence: `work/PM-05E_EDGE_FEATURE_LIFECYCLE_PERSISTENCE_COMPLETION.md`.

PM-05F may now complete the accepted documentation / cumulative automated evidence / Owner supported-Windows acceptance checkpoint on top of the closed E lifecycle/persistence behavior. PM-05 is not closed until the final Owner workflow explicitly PASSes.

PM-05F must keep runtime semantics frozen except for bounded remediation required by failing final acceptance evidence; PM-06 and excluded PM-05 variants remain separately gated.

## 17. PM-06 — Part v1 Completion / Published References / Evidence

PM-06 is not a bucket for lifecycle work deferred from earlier tools.

Every earlier package must already close edit/Undo/persistence/error behavior for the functionality it introduced.

PM-06 closes only cross-package product obligations:

- minimal accepted Published References / Part-read contract for future Assembly;
- complete Part-v1 acceptance matrix;
- final performance/resource baselines and accepted budgets;
- documentation/product-browser completion;
- migration matrix;
- end-to-end Windows acceptance;
- explicit list of deferred variants/features;
- close the Owner-accepted cross-package plane-presentation obligation: one coherent neutral translucent fill treatment for both Origin planes and Datum planes.

It does **not** implement Assembly occurrences, mates, solver, BOM, configurations or inter-document parameter solving.

## 18. Common tool-delivery contract

Every user-facing Part-v1 tool must satisfy, in its accepted variant scope:

1. one semantic operation regardless of GUI/Command Line adapter;
2. selection before/after command where meaningful, with explicit admissible-input filters;
3. unit-aware validated parameters;
4. transient preview; no authored mutation before Finish;
5. one mutating Finish = one transaction/Undo; Cancel/no-op = no authored history;
6. re-edit restores semantic inputs/values and preserves the object's durable ID;
7. correct response to changed inputs, missing/ambiguous references and stale generation;
8. structured diagnostics identify the object and failing input;
9. name/Properties/visibility/Delete/dependencies where meaningful;
10. Undo/Redo and Save/Close/Reopen;
11. GUI and semantic API use the same validation/meaning;
12. automated tests + Windows manual workflow + required docs.

A package may narrow variants. It may not deliver a half-lifecycle tool and defer basic edit/history/persistence to PM-06.

## 19. Reference and evaluation safety contract

Program-wide reference states include at least:

- Resolved;
- Missing;
- Ambiguous;
- Unsupported.

The resolver may use provider generated/modified/deleted history as evidence but never as SS2 authored identity by itself.

A reference records the semantic source context required by its contract: document/Body scope where applicable, producing stage, sub-element kind and semantic provenance selector.

Rules:

- split never means "take first fragment";
- merge never silently preserves two incompatible prior meanings;
- aggregate/set semantics exist only for a separately declared aggregate reference type;
- coordinates/proximity/hash may support diagnostics/ranking but are not independent identity;
- cold rebuild must not require previous-process topology handles or session-only history.

Evaluation distinguishes:

1. authored-structure validity;
2. resolution of existing references;
3. geometric executability.

A structurally valid authored model may be saved with downstream Failed/Blocked evaluation state. A missing evaluated face is not automatically a malformed persistent document.

A last-good B-Rep may be shown only as clearly diagnostic stale geometry. It is not the current valid Body and cannot feed new modeling operations as current truth.

## 20. Persistence and identity direction

Part v1 persists intent, semantic IDs, parameters, dependencies, typed references and accepted authored presentation state.

It does not persist B-Rep/provider handles as authored history truth.

Identity direction:

- DocumentId identifies the Part definition;
- BodyId is distinct from DocumentId;
- FeatureId/DatumId/PublishedReferenceId are typed Part-local identities addressed with DocumentId outside the Part;
- Sketch EntityId remains Sketch-local and requires SketchId outside that Sketch;
- DocumentRevision is runtime freshness, not a durable engineering/model version;
- schema version and modeling-semantics version are separate concerns;
- Undo/branching must not allow abandoned IDs to be silently reused where that would violate existing identity rules.

Exact schemas remain PM-00B/owning-contract decisions.

## 21. Product exclusions for Part Modeling v1

Unless this roadmap is explicitly amended, v1 excludes:

- multi-body modeling;
- Assembly modeling, occurrences and mate/constraint solver;
- Drawing implementation;
- Loft, Sweep, Shell, Draft, Pattern and solid Mirror;
- advanced/variable-radius blends;
- general surface modeling/direct face editing;
- global inter-document parametric dependency graph;
- mandatory Sketch constraint solver as a prerequisite;
- Sketch on non-planar surfaces;
- silent repair of gaps;
- automatic guessing of lost/ambiguous references;
- silent projection approximation;
- import/export as a mandatory v1 requirement;
- arbitrary feature reorder/insertion unless a later package explicitly accepts it;
- a universal CAD framework built only for hypothetical future operations.

## 22. Program acceptance evidence

The final Part-v1 acceptance matrix is cumulative.

Mandatory themes include:

- old `.ss2part` migration preserving current Sketch/Profile/Entity identities;
- Profile with holes -> solid without tessellation dependence;
- edit earlier feature -> deterministic recompute or explicit downstream failure;
- face-support split/merge/loss -> correct resolve or explicit repair state;
- no-effect / detached / multi-solid-invalid cases according to O-01/O-06;
- Cancel/rejected edit does not partially mutate or consume durable identity incorrectly;
- Delete vs Suppress semantics are explicit and reversible as contracted;
- visibility/history inspection never changes evaluation implicitly;
- Save -> Close -> Reopen and cold rebuild restore authored intent and statuses;
- stale preview/evaluation cannot publish into a newer revision/session;
- topology-reference matrix contains zero false Resolved;
- stable-reference cases actually remain resolved after supported edits and cold reopen;
- exact boundary fragments/provenance work for holes/trimmed arcs and later edge features;
- full Revolve/seam and refine/simplification cases obey versioned reference/numerical policy;
- tests remain layered: semantic tests possible without Qt/OCCT, provider geometry tests, desktop integration tests.

Numerical performance budgets are measured and accepted before the owning package closes; lack of measurement is not evidence of adequate responsiveness.

## 23. Documentation impact

This roadmap is governance/program documentation, not as-built product documentation.

Each active PM package must declare its own Documentation Impact.

When a package changes actual behavior, update the relevant:

- `docs/internal/`;
- PL/EN Product documentation;
- persistence/reference architecture docs;
- generated `docs/browser/index.html`;
- regression/acceptance evidence.

Planned Part features must not be documented as already implemented.

## 24. Activation boundary

Program v1.25 is activation-state synchronized against the accepted frozen Part-v1 program. **G0, PM-00A, PM-00B, PM-01, PM-02P, PM-02 and PM-03 are COMPLETED — PASS. PM-04 is ACTIVE at PM-04F; PM-04A, PM-04B, PM-04C, PM-04D and PM-04E are COMPLETED — PASS.**

Owner accepted ADR-0016 and PM-02P evidence activation on 2026-10-03, accepted the final PM-02P D2 synthesis/recommendation on 2026-10-04, and explicitly accepted the exact PM-02 production Work Contract plus all four referenced UX/Viewer design inputs on 2026-10-04.

PM-02 and PM-03 production mutation authority are closed. Current legal production authority is the Owner-accepted PM-04 Work Contract: **`work/PM-04_AXIS_REVOLVE.md`**.

PM-02A production accounting is closed: runtime candidate `58368a6ada5f6c30e03cb845682dbf5408ecb763` passed Windows FULL #1397 and was squash-merged to main as `b95f72105b8242eedce9cd79dd8bf910f3ccb715`.

PM-02B Surface/Face semantics are closed: final runtime candidate `05434516fea0892aac2babae09da772043987f29` passed Windows FULL #1400 and was squash-merged to main as `65d074577bc89e3507288118b17e99b12288a881`. Completion evidence is `work/PM-02B_SURFACE_FACE_SEMANTICS_COMPLETION.md`.

PM-02C Edge/Curve and Vertex/Point semantics are closed: runtime candidate `9135baf1d749f01eeb5dcae4f6ea7cffc856d8a0` passed Windows FULL #1402 and was squash-merged to main as `b128f7445b2705ec9577afddcdbf7f9c81594aa0`. Completion evidence is `work/PM-02C_EDGE_CURVE_VERTEX_POINT_SEMANTICS_COMPLETION.md`.

PM-02D topology-aware Body presentation, Viewer picking, View Styles and semantic inspection are closed: final runtime candidate `c325fcf681afff44ca61abb02fd239495db5d67f` passed Windows FULL #1421 and was squash-merged to main as `9dd893ffa0e9cd1e5287b9590c02b317d543cbd3`. Completion evidence is `work/PM-02D_BODY_PRESENTATION_VIEWER_PICKING_INSPECTION_COMPLETION.md`.

PM-02E Sketch support schema v9 + deterministic frame resolver is closed: final runtime candidate `8b5e2c6844c93648e1884464d138d9a98e2a7a9a` passed Windows FULL #1431 and was squash-merged to main as `e3452d4d0051564f332319a7945344655e91aa3a`. Completion evidence is `work/PM-02E_SKETCH_SUPPORT_SCHEMA_V9_COMPLETION.md`.

PM-02F stage-aware Sketch/Profile evaluation is closed: final runtime candidate `87d4043f9114952df8228681e4cf16937c6f664b` passed Windows FULL #1434 and was squash-merged to main as `f620d7e9888c09a6c4e9854bca6b3092699c1fe3`. Completion evidence is `work/PM-02F_STAGE_AWARE_SKETCH_PROFILE_EVALUATION_COMPLETION.md`.

PM-02G create/re-support Sketch on arbitrary planar Body Surface is closed: final runtime candidate `f45ed8ee09aabe098ab88bc7e47a0c614841fb40` passed Windows FULL #1443 and was squash-merged to main as `17778bdfe80d0411031b458ef4f6d582eccc205b`. Completion evidence is `work/PM-02G_FACE_SUPPORTED_SKETCH_AUTHORING_COMPLETION.md`. The final gate includes the post-#1439 remediation that made support selection draft-only and Finish the single transaction boundary.

PM-02H existing Extrude Add/Cut from face-supported Sketch is closed: final runtime candidate `0540a762be42b3f4329e7032545e652b21332486` passed Windows FULL #1446 and was squash-merged to main as `16818f4b47d4ec2f1c18410856e2b1c1ba51f2c0`. Completion evidence is `work/PM-02H_FACE_SUPPORTED_EXTRUDE_ADD_CUT_COMPLETION.md`. PR #216 / FULL #1445 is superseded evidence only; PR #217 is the selected final runtime candidate because its regression discovers support from the actual evaluated topology catalog.

PM-02I lifecycle, repair, persistence and regression matrix is closed: final runtime/evidence candidate `b642a5af625a48d668a21efeaf3d78bfb9bf7c79` passed Windows FULL #1451 and was squash-merged to main as `a26d7e981b160973825603bc3c11f37d5ab594a8`. Completion evidence is `work/PM-02I_LIFECYCLE_REPAIR_PERSISTENCE_SURVIVAL_COMPLETION.md`. The integrated regression closes repairable Profile Delete, producer-support Delete rejection, Undo/Redo, stale support mutation, split/delete/alias/same-geometry failure, explicit re-support repair, native persistence and cold rebuild without runtime-token authority.

PM-02 is **COMPLETED — PASS**. Final accepted evidence is post-remediation runtime FULL #1473 on `b9e70c684c2d054da21662ef3a8ae56ac59f60c0`, remediation docs DOCS #1474 on `1dbaef95b2ea48f8e529425a154b795984d6551f`, bookkeeping #1476 PASS, and Owner Windows re-test PASS on 2026-10-05 against current main `d8d73e213f24d5b82c5a7a13dbeba07282520a40`.

PM-03 is **COMPLETED — PASS**. Final accepted evidence includes F1 Windows FULL #1522, F2a Windows FULL #1523, documentation workflow #1524 on exact docs candidate `439abd15b9e7ec1c4c8b2177cd8ea31f6debda0e` merged as `4e9ecdb4d51f7f81be799bbc0778b98808799f3d`, and Owner final Windows workflow PASS on 2026-10-05. The Owner explicitly accepted the missing neutral translucent plane fill as a presentation-only defer to PM-06, to be fixed coherently for both Origin and Datum planes.

PM-04 is ACTIVE at PM-04F under the Owner-accepted Work Contract. Exact contract candidate `cc84e21b3178af73ddb06b58d4482a5aba67abee` passed Windows PR gate #1526 and merged to main as `8c464cb7447b8fef7d07b10cc6f16eb849b0c41e`. PM-04A, PM-04B, PM-04C, PM-04D and PM-04E are COMPLETED — PASS; PM-05, PM-06 product scope and Projection remain separately gated.
