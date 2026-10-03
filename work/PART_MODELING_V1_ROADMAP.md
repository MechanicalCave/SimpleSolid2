# Part Modeling v1 — Program Roadmap

**Status:** ACCEPTED — PROGRAM FROZEN; IMPLEMENTATION NOT ACTIVATED  
**Version:** 1.3  
**Owner acceptance:** 2026-10-02  
**Previous accepted version:** 1.2 — 2026-10-02  
**Decision class:** D2 program sequencing and Part-v1 scope freeze; individual D2/D3 architecture/product decisions remain owned by the package that explicitly closes them  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Current baseline:** `main` after merged SR-03, `6bc83dc47dd4b101c881bd7df4455e6bcdba80f0`  
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
| 2 | **PM-00B — Part Feature Architecture Freeze** | Owner-approved ADR/contract freeze for first solid workflow based on PM-00A evidence | future separate Work Contract / governance gate |
| 3 | **PM-01 — First Solid Vertical Slice: Body / Feature / Extrude Add** | minimum durable Body/Feature architecture plus one complete Extrude Add lifecycle | future separate Work Contract |
| 4 | **PM-02 — Datum Plane / Sketch Support / Extrude Cut** | offset datum from stable source, Sketch on datum, Cut, downstream recompute/failure behavior | future separate Work Contract |
| 5 | **PM-03 — Face Support / Semantic Topology References / Projection** | planar-face support, repair, exact bounded projection and resolver expansion without silent rebinding | future separate Work Contract |
| 6 | **PM-04 — Axis / Datums / Revolve** | Axis semantics, remaining required datums and complete Revolve Add/Cut lifecycle | future separate Work Contract |
| 7 | **PM-05 — Edge Features: Fillet / Chamfer** | complete edge-feature lifecycle with lineage/refine handling and repair | future separate Work Contract |
| 8 | **PM-06 — Part v1 Completion / Published References / Evidence** | close accepted Part-v1 scope, minimal future-Assembly read contract, docs and performance evidence; no Assembly implementation | future separate Work Contract |

No package number authorizes mutation by itself.

### 6.1 Why the audited PM-01/PM-02 infrastructure split is not preserved literally

The audited v0.2 draft correctly requires minimal Body/Feature schema work before Extrude, but the accepted program deliberately forbids a long standalone "infrastructure first" implementation phase.

PM-01 therefore combines only the **minimum** Body/Feature identity, persistence, evaluation, tree projection and Kernel API required to prove one real Extrude Add vertical workflow.

Architecture that cannot be justified by PM-01's concrete workflow remains out of scope.

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

The audited design uses O-01...O-12 as a durable decision ledger. They remain open until explicitly closed by the package below.

| ID | Decision | Program direction | Must be closed by |
| --- | --- | --- | --- |
| O-01 | exact single-Body semantics: creation, Empty, valid one-solid result, no-effect/multi-solid outcomes | one Body in v1; do not alias BodyId to DocumentId | PM-00B before PM-01 schema |
| O-02 | exact datum constructors and Mid semantics | only constructors justified by v1 workflows; no provider-parameter Mid shortcut | owning datum package, no later than PM-04 |
| O-03 | projection curves, snapshot/refresh, atomic face-boundary capture | snapshot intent; exact supported curves or explicit UnsupportedCurve; no silent approximation | product matrix in PM-00A; final before PM-03 |
| O-04 | support-frame construction, re-support and orientation | full stable O/U/V/N rule; preserve local 2D by default; no silent fallback | PM-00B before support schema |
| O-05 | topology-reference selector, lineage and split/merge guarantees | semantic producer/stage/role lineage, fail closed, user repair; no Face[n]/Edge[n] | evidence in PM-00A; freeze in PM-00B |
| O-06 | exact variants/inputs per modeling operation | freeze a bounded v1 matrix; lifecycle completeness over variant breadth | first matrix PM-00A; each owning operation contract |
| O-07 | Sketch Axis representation/editing/direction/visibility | Axis excluded from Profiles; any valid straight Sketch line may still be an axis source without role conversion | PM-04 before Shared-2D mutation |
| O-08 | Published References/local frames and v1 UI scope | stable typed interface identity; no Assembly implementation | semantics by PM-00B; UI/final scope PM-06 |
| O-09 | downstream failure commit, Delete, Suppress, retry/history semantics | preserve repairable downstream intent; separate Delete from Suppress; no hidden history entries | PM-00B before evaluator/commands |
| O-10 | visibility, auto-hide, history inspection and performance budgets | visibility never changes evaluation; measure before optimize | each owning UI package; final budget by PM-06 |
| O-11 | numerical tolerances, refine/healing, exact Profile->Kernel input, modeling semantics version | explicit versioned policy; no display tolerance leakage or escalating fuzzy success | evidence PM-00A; freeze PM-00B before first persisted solid |
| O-12 | ID scopes, Part snapshot/read contract, transform direction, durable version vs DocumentRevision | distinguish Document/Body/local IDs and runtime revision; no Assembly solver | foundations PM-00B; minimal interface PM-06 |

A package must not silently make a ledger decision owned by a later package if doing so would constrain that later decision durably. If a dependency is discovered, STOP and amend sequencing explicitly.

## 10. PM-00B — Part Feature Architecture Freeze

PM-00B is intentionally short and governance-heavy.

It converts accepted PM-00A evidence into:

- accepted Part Feature Architecture ADR(s);
- frozen O-01/O-04/O-05/O-09/O-11 and required O-12 foundations;
- the accepted first product-scope slice from O-03/O-06;
- a bounded PM-01 production Work Contract;
- synchronized persistence/reference/failure semantics documentation.

PM-00B must not become a second research phase or a general modeling-framework implementation.

## 11. PM-01 — First Solid Vertical Slice: Body / Feature / Extrude Add

PM-01 is the first authorized production-solid package only after G0, PM-00A and PM-00B pass.

Minimum complete workflow:

`existing valid Profile -> Extrude Add -> Body -> edit -> recompute -> Undo/Redo -> Save -> Close -> Reopen -> cold rebuild`.

PM-01 owns only the minimal implementation required for that workflow:

- BodyId / FeatureId and one-Body validation;
- minimal ordered Feature state and dependency edges;
- schema migration preserving existing SketchId/ProfileId/EntityId;
- exact Profile->Kernel boundary required by Extrude;
- minimal provider-neutral Kernel API + OCCT adapter;
- Extrude Add variants explicitly frozen by PM-01;
- cap/side reference lineage required by later consumers;
- deterministic recompute/status;
- Tree/Properties/visibility needed to operate the feature;
- semantic edit/delete behavior in the accepted subset;
- complete GUI/semantic API parity;
- documentation and Windows manual acceptance.

No speculative framework beyond the first real Feature.

## 12. PM-02 — Datum Plane / Sketch Support / Extrude Cut

Primary vertical scenario:

`Extrude Add -> accepted datum plane -> Sketch -> Profile -> Extrude Cut -> edit upstream -> recompute or structured downstream failure`.

Start with datum definitions that do not require unresolved face topology (for example accepted Origin-based offset). Face-derived support belongs to PM-03 unless PM-00B explicitly proves and freezes the required reference path.

The package owns full lifecycle for the exact datum/Cut variants it activates.

## 13. PM-03 — Face Support / Semantic Topology References / Projection

PM-03 expands the proven resolver into user-facing topology-dependent workflows:

- Sketch support on planar model face;
- deterministic support frame and repair;
- manual projection;
- atomic selected-face-boundary capture including holes;
- exact supported curve conversion;
- explicit UnsupportedCurve / Missing / Ambiguous behavior;
- bounded Refresh/Reproject policy without silent overwrite of edited snapshot geometry;
- split/merge/delete repair workflows.

Snapshot projection provenance is informational unless an explicit live dependency is separately accepted. A live Sketch support and snapshot projected geometry are distinct concepts.

## 14. PM-04 — Axis / Datums / Revolve

PM-04 closes O-07 and the remaining datum constructors required for Revolve.

Program constraints:

- any non-degenerate straight Sketch line may act as a Revolve axis without duplication or forced role conversion;
- an optional Axis role is excluded from region/profile generation;
- direction/orientation is semantic and not derived from arbitrary OCCT edge orientation;
- Revolve references a concrete source identity;
- full-rotation seam/topology behavior must extend the PM-00A/PM-00B reference contract rather than bypass it.

Revolve Add/Cut lifecycle completeness is mandatory for the variants activated by this package.

## 15. PM-05 — Edge Features: Fillet / Chamfer

PM-05 adds edge-consuming features only after topology-reference semantics are proven in earlier packages.

It must cover:

- semantic edge inputs at the correct consumed stage;
- deterministic mapping through prior operations;
- fixed accepted parameter variants;
- preview/Finish/Edit;
- upstream changes that preserve, split, merge or remove an input;
- structured Failed/Blocked behavior and repair;
- refine/unify history handling;
- Delete/Suppress/Undo/Redo;
- Save/Reopen/cold rebuild.

No silent use of a stale shape from an earlier stage.

## 16. PM-06 — Part v1 Completion / Published References / Evidence

PM-06 is not a bucket for lifecycle work deferred from earlier tools.

Every earlier package must already close edit/Undo/persistence/error behavior for the functionality it introduced.

PM-06 closes only cross-package product obligations:

- minimal accepted Published References / Part-read contract for future Assembly;
- complete Part-v1 acceptance matrix;
- final performance/resource baselines and accepted budgets;
- documentation/product-browser completion;
- migration matrix;
- end-to-end Windows acceptance;
- explicit list of deferred variants/features.

It does **not** implement Assembly occurrences, mates, solver, BOM, configurations or inter-document parameter solving.

## 17. Common tool-delivery contract

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

## 18. Reference and evaluation safety contract

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

## 19. Persistence and identity direction

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

## 20. Product exclusions for Part Modeling v1

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

## 21. Program acceptance evidence

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

## 22. Documentation impact

This roadmap is governance/program documentation, not as-built product documentation.

Each active PM package must declare its own Documentation Impact.

When a package changes actual behavior, update the relevant:

- `docs/internal/`;
- PL/EN Product documentation;
- persistence/reference architecture docs;
- generated `docs/browser/index.html`;
- regression/acceptance evidence.

Planned Part features must not be documented as already implemented.

## 23. Activation boundary

Program v1.3 is accepted and frozen. **PM-00A is completed** after A0 plus E01-E10 evidence and the Owner-review synthesis. No later PM package is active.

Current legal next action:

**Owner review of the proposed PM-00A Part Feature Architecture ADR candidate and the proposed PM-00B Part Feature Architecture Freeze Work Contract.**

G0 and PM-00A are complete. PM-00A's final source candidate `ac34a713c6fee4a53d513bfcce2a2044ce1a0ffb` passed Windows FULL #1320. The evidence and recommendations do not themselves freeze D2 architecture.

PM-00B remains inactive until explicit Owner acceptance of `work/PM-00B_PART_FEATURE_ARCHITECTURE_FREEZE.md`. PM-01 and all product solid-modeling packages remain inactive.

After PM-00B, PM-01 still requires a separate production Work Contract and explicit Owner acceptance.

Part Modeling v1 program acceptance must never be interpreted as permission to skip those boundaries.
