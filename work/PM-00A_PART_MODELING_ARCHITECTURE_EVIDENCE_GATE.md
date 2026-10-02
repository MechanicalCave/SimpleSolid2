# PM-00A — Part Modeling Architecture Evidence Gate

**Status:** ACTIVE  
**Owner acceptance:** 2026-10-02  
**Decision class:** D2 architecture-evidence contract; no durable Part Feature schema or user-facing solid-modeling activation  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Program authority:** `work/PART_MODELING_V1_ROADMAP.md` v1.3  
**Entry gate:** G0 Sketcher profile-authoring readiness PASS on `16df8d1940a1f438a66cbd6964a881ba24718c0d`  
**Activation rule:** production/evidence implementation begins only after explicit Owner acceptance and synchronization through `work/ACTIVE.yaml`

## 1. Goal

Produce sufficient technical evidence to freeze the minimum single-Body / ordered-Feature architecture safely before SS2 introduces durable solid-modeling data.

PM-00A must answer the high-risk questions that would otherwise become expensive schema/reference migrations later:

- what exact semantic boundary crosses from Profile into the geometry Kernel;
- how generated/modified/deleted topology evidence can support SS2 semantic references without becoming SS2 identity;
- what support-frame contract remains stable across provider/topology changes;
- how authored validity, reference resolution and geometry execution remain distinct;
- what numerical/refine/healing policy is part of modeling semantics;
- what runtime/session information must never be required for cold reconstruction;
- what minimum identity/snapshot rules future Part/Assembly boundaries require.

PM-00A is evidence-first. It is not the first solid-feature delivery contract.

## 2. Entry state

PM-00A may be activated only if all remain true:

1. G0 readiness is recorded PASS;
2. Foundation v1.0 and accepted ADRs remain unchanged or any relevant superseding ADR is explicitly accepted;
3. Part Modeling v1 Roadmap remains accepted;
4. no conflicting production Work Contract is active;
5. the exact activation candidate passes the repository governance gate.

At proposal time G0 is complete, but PM-00A is still inactive.

## 3. Scope IN

PM-00A may implement only the minimum technical seams, probes, tests and provider adapters required to collect architecture evidence.

Allowed scope includes:

- Phase A0 Verification Topology;
- provider-neutral Kernel request/result types needed by the accepted evidence cases;
- a bounded OCCT provider behind that neutral boundary;
- exact Profile boundary-use extraction/adaptation required by E01–E10;
- test-only or evidence-only representative Extrude/Cut/Fillet/Revolve operations sufficient to study topology generation and lineage;
- support-frame evidence helpers;
- topology-reference candidate/resolution prototypes that do not become durable production schema;
- B-Rep validation/evidence;
- cold-model-rebuild harnesses;
- generation/session isolation probes;
- numerical/refine/healing evidence;
- required CMake/build/test/CI changes for kernel-native verification;
- evidence reports and ADR candidates.

PM-00A may touch production source only where necessary to establish the minimal provider-neutral seam that later production work would need anyway. Any such seam must remain smaller than the evidence it supports and must not smuggle in Body/Feature persistence or a generalized modeling framework.

## 4. Scope OUT

PM-00A does not authorize:

- durable BodyId/FeatureId schema in user documents;
- persisted solid Feature history;
- Part Feature Tree product implementation;
- user-facing Extrude/Cut/Revolve/Fillet/Chamfer commands;
- user-facing solid preview/edit UI;
- planar-face Sketch support as product behavior;
- projection product UI;
- Axis product role;
- multi-body;
- Assembly/Drawing implementation;
- occurrence/mate/solver infrastructure;
- OCAF/provider topology identity as SS2 authored truth;
- geometry-similarity-only automatic persistent naming;
- global/general parametric dependency framework;
- authored constraints/solver;
- broad Kernel API designed for hypothetical future features;
- migration of existing Part persistence schema solely to store PM-00A evidence.

## 5. Phase A0 — Verification Topology

Phase A0 is mandatory and precedes E01–E10.

### 5.1 Required build/test topology

Establish and prove three distinct modes:

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
    -> full Workbench / Viewer / presentation integration
```

The existing semantic/core mode must not regress.

### 5.2 Required A0 deliverables

A0 must establish:

1. current core-only no-Qt/no-OCCT verification remains green;
2. minimal provider-neutral Kernel boundary required by PM-00A evidence;
3. OCCT-backed kernel-native **Release** build/test lane with Qt discovery explicitly disabled;
4. stable `subsystem-kernel`;
5. one authoritative regression metadata declaration path or an equivalent fail-closed mechanism preventing classification drift;
6. validation that every registered regression has coherent tier/subsystem metadata;
7. Release kernel evidence target(s) suitable for B-Rep validity and topology-lineage scenarios;
8. cold-model-rebuild harness distinct from CMake build-tree clean/warm state;
9. exact-head proof that semantic/core, kernel-native and desktop modes all pass on one candidate.

### 5.3 Pipeline constraints

Preserve public verification semantics:

- FOCUSED — exact target/test iteration evidence only;
- FAST — broad draft iteration;
- SUBSYSTEM — explicit subsystem checkpoint; add `kernel`;
- FULL — mandatory merge evidence and must cover desktop + semantic/core + kernel-native;
- DOCS/CLOSURE — existing trusted-FULL suffix rules unchanged.

Do not introduce a new public tier merely for kernel-native tests.

Do not add path-based dependency guessing such as treating every `src/part/**` edit as fully covered by only `part` tests.

Do not introduce Ninja migration, sccache/ccache, PCH, Unity Build, distributed workers or sharding without measured evidence that PM-00A/PM-01 has created a material bottleneck.

### 5.4 Test graph discipline

Where many topology scenarios share expensive setup/provider code, prefer a bounded shared executable with independently reportable CTest cases over one heavy executable per scenario.

This optimization must not merge semantically distinct assertions into one opaque result or weaken FULL.

### 5.5 A0 completion gate

E01–E10 evidence may be accepted only after A0 PASS is recorded on an exact revision.

A0 PASS does not activate durable Part schema or product solid modeling.

## 5A. Phase A0 progress record

### A0.1 — Build/Test Architecture

**Status:** COMPLETED — PASS  
**Exact candidate:** `5a3dc358a4e3c9828789db6294d0b8a140d6207a`  
**Windows FULL:** #1199 — PASS

A0.1 established and proved on one exact revision:

- desktop build/test graph: PASS;
- semantic/core Release with Qt and OCCT disabled: PASS;
- kernel-native Release with OCCT required and Qt disabled: PASS;
- `subsystem-kernel` selection: PASS;
- fail-closed tier/subsystem metadata validation: PASS;
- distinct CI-04 persistent build-tree fingerprint mode for kernel-native: PASS;
- first real OCCT B-Rep construction/validity smoke evidence: PASS.

No durable Body/Feature schema, persistent topology reference, Part Feature Tree behavior or user-facing solid operation was introduced.

**A0 remains INCOMPLETE.** The next required slice is A0.2 — Evidence Harness Foundation: the minimal provider-neutral Kernel boundary, bounded OCCT adapter/evidence surface, B-Rep validation/lineage harness foundation and cold-model-rebuild harness. E01–E10 remain blocked until the full A0 completion gate passes.

### A0.2 — Evidence Harness Foundation

**Status:** COMPLETED — PASS  
**Exact candidate:** `8438cd739e55a390b1df8d4b7ddaa9e6c070e237`  
**Windows FULL:** #1206 — PASS

A0.2 established and proved on one exact revision:

- provider-neutral `simplesolid2_kernel` input/evidence types compile in semantic/core mode with Qt and OCCT absent;
- Part evaluates a durable Profile into exact neutral Line/Circle/Arc boundary uses with O/U/V/N frame and semantic provenance;
- technical whole-curve provider seam state is stripped from neutral semantic input;
- bounded `simplesolid2_kernel_occt` builds only in kernel-native mode and keeps TopoDS/Qt/provider handles out of public headers;
- OCCT constructs and validates a real Profile face including an inner circular hole;
- neutral evidence reports B-Rep validity/topology counts and provenance-to-generated-edge evidence without exporting provider topology identity;
- dependency-boundary regression prevents neutral Kernel -> Part/Sketch/Application/Viewer/UI/provider leakage and Part -> kernel_occt leakage;
- cold-model rebuild PASS: authored Rectangle + Circle-hole Profile is saved, runtime session/input/provider/B-Rep scope is destroyed, the file is reopened, Profile reevaluated, and neutral input/evidence reproduce deterministically;
- desktop, core-only and kernel-native Release all PASS on the same exact candidate;
- combined `part,kernel,persistence` subsystem selection and complete unfiltered desktop verification PASS.

No Body/Feature persistence, Part Feature Tree, persistent topology naming or user-facing solid operation was introduced.

### Phase A0 completion gate

**Status:** COMPLETED — PASS  
**A0 source candidate:** `8438cd739e55a390b1df8d4b7ddaa9e6c070e237`  
**Windows FULL:** #1206 — PASS

Phase A0 is complete. The Verification Topology and Evidence Harness Foundation required before E01–E10 are now present and exact-head verified.

The next legal PM-00A work is the E01–E10 architecture-evidence matrix. E01–E10 may use the bounded neutral Kernel/OCCT evidence seams established by A0, but must not expand them into durable Body/Feature schema or product solid-modeling behavior.

## 6. Evidence model and invariants

### 6.1 Authored intent versus evaluated geometry

Authored semantic state remains authoritative.

OCCT TopoDS handles, topology order, AIS objects, tessellation, provider caches and generated-history handles are runtime/evaluation evidence only.

### 6.2 Reference resolution states

The evidence resolver must distinguish at least:

- Resolved;
- Missing;
- Ambiguous;
- Unsupported.

A false-positive Resolved result is an integrity failure.

A resolver that returns Ambiguous/Missing for every supported upstream edit also fails the product goal.

### 6.3 Three validity layers

Evidence must keep separate:

1. authored-structure validity;
2. reference-resolution validity;
3. geometric executability/result validity.

A structurally valid authored intent may have unresolved/failed evaluated output without becoming malformed persistence by implication.

### 6.4 Support frame

Evidence must define a complete stable right-handed support frame:

`O / U / V / N`

It must not derive durable orientation from:

- camera;
- Viewer;
- arbitrary OCCT face parameterization;
- topology enumeration order.

### 6.5 Profile-to-Kernel input

The Kernel boundary consumes exact semantic/evaluated boundary uses with:

- loop identity/context where required;
- outer/inner role;
- orientation;
- trimmed fragments;
- supported exact curve representation;
- provenance back to semantic source/boundary use.

Tessellation or screen-space polylines are not valid modeling input.

## 6A. Frozen E01–E10 execution matrix

The expected outcomes for E01–E10 are frozen before individual probes in:

`work/PM-00A_E01_E10_EVIDENCE_MATRIX.md`

The matrix is an evidence oracle, not a durable Part-schema or PM-00B architecture decision. Probe implementation must not silently rewrite expected outcomes to match provider behavior. A contradiction is recorded as evidence and escalated under the active STOP rules.

## 7. Mandatory evidence cases

### E01 — Profile with hole → Extrude

Prove:

- exact outer loop + inner hole input;
- valid solid result;
- semantic roles for start cap/end cap;
- side provenance tied to concrete profile-boundary use;
- no reliance on Face[n]/Edge[n].

### E02 — Extrude → Cut → Fillet-style lineage

Using evidence-only representative operations:

- produce downstream topology;
- modify an upstream authored dimension/input;
- reevaluate;
- inspect generated/modified/deleted mappings;
- determine which stable semantic references should remain Resolved and why.

### E03 — referenced edge splits

Prove that one prior semantic target becoming multiple candidates never silently resolves to the first fragment.

Expected result must be deterministic under the proposed reference contract: Ambiguous/Missing unless a separately defined aggregate selector legitimately applies.

### E04 — candidates merge

Prove behavior when two previously distinct candidates become one.

Do not silently preserve incompatible prior meaning.

### E05 — geometrically similar topology

Create deliberately similar faces/edges and prove that geometry proximity/similarity alone cannot produce Resolved.

### E06 — full Revolve / periodic seam

Use a full-rotation probe to expose seam/periodic-surface behavior.

A technical seam created by the provider must not accidentally become durable semantic meaning.

### E07 — cold model rebuild

Destroy:

- DocumentSession/runtime owner;
- evaluated B-Rep;
- provider objects;
- provider-generated history/cache.

Reconstruct only from legal authored/evidence inputs and reevaluate.

Required stable cases must recover the same semantic resolution outcomes without previous-process handles.

### E08 — support-frame stability

Change accepted upstream dimensions/topology while preserving semantic support.

The support frame must not flip or rotate 180 degrees arbitrarily.

### E09 — stale generation/session result

Start/evaluate evidence under one revision/session context, replace that context, and prove the old result cannot publish as current.

### E10 — tolerance/refine matrix

Prove modeling outcomes are independent of:

- zoom;
- pick aperture;
- screen-space tolerance;
- Viewer display settings.

Collect evidence for:

- modeling tolerance policy;
- refine/unify/healing behavior;
- deterministic result/version semantics.

## 7A. Evidence execution progress

### E01 — Profile with hole → Extrude

**Status:** COMPLETED — PASS  
**Exact source candidate:** `476258b74751a251c1c4fdf7dabaa03b3da63976`  
**Windows FULL:** #1228 — PASS  
**False-Resolved:** 0

See `work/PM-00A_E01_PROFILE_EXTRUDE_EVIDENCE.md`.

E01 supports semantic cap roles and side roles derived from exact Profile boundary-use provenance. OCCT topology remains transient provider evidence; no durable selector schema is frozen by this result.

### E05 — geometrically similar topology

**Status:** COMPLETED — PASS  
**Exact source candidate:** `7e0d6109cf927a2e8d85d2794b5718dcc2feab22`  
**Windows FULL:** #1252 — PASS  
**False-Resolved:** 0

See `work/PM-00A_E05_SIMILARITY_FALSE_POSITIVE_EVIDENCE.md`.

E05 proves that geometry similarity is diagnostic only. Equal-area/same-axis decoys do not determine identity; an exact-geometry replacement under a new semantic source remains a different target; proximity does not override surviving provenance; and similarity-only evidence cannot return Resolved.

The added neutral face diagnostics are transient evidence and do not freeze a durable selector schema.

**Next:** execute E02 multi-stage Extrude -> Cut -> Fillet-style lineage using the accepted E03/E04 split/merge semantics.

### E03 + E04 — split/merge cardinality

**Status:** COMPLETED — PASS  
**Exact source candidate:** `30886e0d47a8f0bbfba5ab9048cf22fb9fa7be25`  
**Kernel-focused:** #1274 — PASS  
**Windows FULL:** #1275 — PASS  
**False-Resolved:** 0

See `work/PM-00A_E03_E04_SPLIT_MERGE_CARDINALITY_EVIDENCE.md`.

E03/E04 establish fail-closed singular-reference cardinality across real OCCT Boolean history: split -> Ambiguous, deletion -> Missing, merge collapse -> Ambiguous without independent semantic meaning, and undeclared aggregate semantics -> Unsupported.

They also prove that provider history asymmetry is not identity authority: Modified/Deleted bookkeeping can support diagnostics and reconstruction evidence but cannot choose a semantic winner by itself.

**Next:** execute E02 multi-stage Extrude -> Cut -> Fillet-style lineage using the accepted E03/E04 split/merge semantics. E07 cold-rebuild matrix follows after E02.

### E02 — multi-stage Extrude → Cut → Fillet-style lineage

**Status:** COMPLETED — PASS  
**Exact source candidate:** `953cdca42978916754f6ae6cc68d86352aad35ac`  
**Kernel-focused:** #1285 — PASS  
**Windows FULL:** #1286 — PASS  
**False-Resolved:** 0

See `work/PM-00A_E02_MULTISTAGE_LINEAGE_EVIDENCE.md`.

E02 proves that producer/consumed stage participates in semantic reference meaning, that accepted lineage can remain Resolved across upstream rebuilds without provider-handle continuity, and that downstream geometric failure is separate from reference resolution.

Provider Boolean/Fillet history remains transient evidence only. Geometry used to pick a concrete test edge inside an already Resolved semantic Cut-face context is probe plumbing, not persistent selector semantics.

**Next:** execute E07 cold-rebuild parity across accumulated E01/E02/E03/E04/E05 stable and fail-closed cases.

## 8. Acceptance asymmetry

Completion requires **zero false Resolved** results in the accepted PM-00A matrix.

This rule is intentionally stricter than ordinary UX convenience because a false positive can silently redirect downstream design intent.

At the same time, each case declared stable by the accepted reference contract must actually remain Resolved after its supported edit and cold rebuild.

## 9. Open-decision outputs

PM-00A gathers evidence and proposes, but does not itself silently freeze D2/D3 product semantics beyond its accepted contract.

It must produce Owner-reviewable recommendations for:

- O-01 single-Body valid-result/no-effect/multi-solid semantics;
- O-04 support-frame construction and re-support;
- O-05 topology-reference selector/lineage/split/merge guarantees;
- O-09 downstream failure/Delete/Suppress/retry/history semantics;
- O-11 modeling tolerances/refine/healing/modelingSemanticsVersion;
- required O-12 ID/snapshot/read-boundary foundations.

It must also produce an O-03/O-06 scope matrix for:

- exact projected curve support planned for v1;
- first production operation variants;
- explicit v1 versus later exclusions.

PM-00B owns the architecture freeze from this evidence.

## 10. STOP conditions

STOP and return to Owner review if PM-00A evidence requires or strongly implies:

- Foundation change;
- new domain ownership direction;
- Part or Assembly depending on `viewer::` types;
- persistence of provider/OCAF topology identity as authored truth;
- topology ordinal/tree-row/Viewer-token identity;
- automatic reference identity based only on geometry similarity;
- durable Body/Feature schema before reference semantics are reviewed;
- multi-body as a prerequisite;
- global/general dependency framework as a prerequisite;
- Assembly occurrence/solver infrastructure as a prerequisite;
- public Viewer API changes;
- changing CAD input ownership;
- hidden schema migration;
- a production user-facing solid tool merely to collect evidence.

A0 verification-infrastructure defects that remain inside this contract may be fixed. Crossing an architecture/product boundary requires explicit Owner decision.

## 11. Expected implementation surface

After activation, bounded changes may include:

- root/src/tests CMake;
- `scripts/ss2-build.ps1`, `scripts/ss2-configure.ps1`, `scripts/ss2-test.ps1`, `scripts/ss2-check.ps1`;
- bounded helpers under `scripts/ci/`;
- `ss2.ps1`;
- `.github/workflows/windows-pr-gate.yml`;
- new provider-neutral Kernel headers/source in a narrowly scoped module;
- bounded OCCT kernel provider source;
- test/evidence harnesses;
- `benchmarks/**` only if measurement is evidence-required rather than product behavior;
- internal architecture/build documentation;
- `work/**`.

UI/product source is out of scope unless a tiny non-semantic compile/link adaptation is strictly required by moving OCCT discovery/targets. Any behavioral UI change is STOP.

Existing Part semantic source may be touched only to expose/use provider-neutral evidence seams; it must not add persistent Body/Feature schema or user-facing modeling semantics.

## 12. Verification

During implementation:

- use FOCUSED for exact local slices;
- use `SUBSYSTEM part,kernel,persistence` checkpoints when meaningful;
- keep draft PR iterations FAST where classifier permits;
- verification-infrastructure changes remain FULL-sensitive per existing CI rules.

Before PM-00A completion:

- Phase A0 exact-head PASS;
- complete E01–E10 accepted evidence matrix PASS;
- semantic/core exact-head PASS;
- kernel-native Release exact-head PASS;
- complete desktop FULL exact-head PASS;
- required docs/browser freshness PASS;
- explicit evidence that false-Resolved count is zero;
- cold-model-rebuild PASS for required stable cases.

Because PM-00A changes verification infrastructure and Kernel architecture seams, the final implementation candidate requires an exact-head Windows FULL. A later work-only completion suffix may use CLOSURE only under existing trusted-FULL rules.

## Documentation impact

Internal docs: required  
User/Product docs: not required  
Reason: PM-00A changes architecture evidence, Kernel/provider boundaries and repository verification topology without activating user-visible solid modeling.

Internal documentation must cover, as delivered:

- Kernel provider boundary;
- semantic/core versus kernel-native versus desktop build/test topology;
- Part/reference architecture evidence;
- cold-model-rebuild evidence;
- numerical/modeling-semantics evidence.

If implementation would change user-visible product behavior, STOP and reclassify scope before proceeding.

The deterministic Product Browser must remain current where canonical documentation changes require regeneration.

## 14. Deliverables

PM-00A completion requires:

1. Phase A0 verification-topology report;
2. exact-head evidence for all three build modes;
3. PM-00A E01–E10 results matrix;
4. reference survival/failure matrix;
5. support-frame evidence;
6. exact Profile-to-Kernel boundary evidence;
7. numerical/tolerance/refine/healing recommendation;
8. cold-model-rebuild evidence;
9. zero-false-Resolved statement backed by the matrix;
10. proposed O-01/O-04/O-05/O-09/O-11/O-12 resolutions;
11. O-03/O-06 product-scope matrix;
12. Owner-reviewable Part Feature Architecture ADR candidate;
13. PM-00B Work Contract candidate.

## 15. Completion boundary

PM-00A completes when the evidence is sufficient for an Owner architecture decision.

Completion does not mean the proposed architecture is automatically accepted.

After PM-00A:

- production mutation stops;
- PM-00B remains a separate Owner architecture-freeze gate;
- no durable Body/Feature schema is introduced until PM-00B accepts it;
- PM-01 First Solid Vertical Slice remains inactive until PM-00B completion and separate Work Contract acceptance.

## 16. Activation boundary

This file is **ACTIVE** after explicit Owner acceptance on 2026-10-02.

Activation is authorized only after the synchronized activation candidate passes the repository governance gate.

The accepted implementation order begins with **Phase A0 — Verification Topology**. E01–E10 evidence may not be accepted before A0 PASS.

No acceptance of PM-00A authorizes durable Body/Feature schema, Part Feature Tree product behavior or user-facing solid modeling.
