# ADR Candidate — Part Feature Architecture, References and Modeling Semantics

**Proposed ADR number:** ADR-0014  
**Status:** PROPOSED — OWNER REVIEW REQUIRED; NOT ACCEPTED  
**Decision class:** D2 Part architecture  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Evidence authority:** PM-00A final source candidate `ac34a713c6fee4a53d513bfcce2a2044ce1a0ffb`, Windows FULL #1320  
**Related:** ADR-0004, ADR-0005, ADR-0008, ADR-0009, ADR-0012, ADR-0013  
**Candidate source:** `work/PM-00A_ARCHITECTURE_RECOMMENDATIONS.md`

> This file is an Owner-review candidate only. It is not listed in `work/ACTIVE.yaml -> accepted_adrs`, does not authorize production mutation and must not be treated as an accepted ADR.

## Context

Part Modeling v1 needs durable Body/Feature semantics before the first solid feature is persisted. PM-00A collected real OCCT and semantic/core evidence for Profile-to-Kernel input, topology lineage, split/merge/similarity failure modes, full-Revolve seams, cold rebuild, support frames, stale publication and numerical/refine behavior.

The evidence rules out provider topology identity, geometry-similarity identity, Viewer identity and runtime-session continuity as durable foundations.

## Proposed decision

### 1. Single Body and ordered Features

A Part v1 owns exactly one durable Body with a typed Part-local BodyId distinct from DocumentId. Empty Body is valid.

The Body result is produced by an ordered sequence of typed Features. A successful current result is exactly one valid solid. Multi-solid and no-effect outcomes are explicit non-success result classes in v1 and never silently activate multi-body behavior.

A newly finishing Feature is committed only from a currently valid accepted preview/result. An already-authored Feature that later becomes invalid because of upstream edits remains authored with its FeatureId and inputs so it can be repaired.

### 2. Semantic topology references

Durable topology references are provider-neutral semantic selectors.

A selector identifies the producing semantic object/stage and a semantic role/provenance meaning. Exact storage layout is defined with the first schema, but it must not contain OCCT handles, topology ordinals, Viewer tokens or geometry-similarity identity.

Resolution is fail-closed:

- zero semantic candidates -> Missing;
- one -> Resolved;
- more than one -> Ambiguous;
- undeclared meaning -> Unsupported.

Provider history is transient evidence used during evaluation. Geometry diagnostics may corroborate or help UI repair but cannot independently make a target Resolved.

### 3. Stage-scoped evaluation

Producer/consumed stage participates in reference meaning. A downstream input is resolved in its declared semantic context rather than by searching the final Body globally.

Reference resolution and geometric feasibility are separate. A resolved input can feed an operation that fails geometrically.

### 4. Support frames

Every accepted Sketch support provides a deterministic right-handed metric O/U/V/N frame. Camera, Viewer state, provider topology traversal and raw provider UV orientation are not frame authority.

Origin-plane frames remain the accepted ADR-0009/E08 mappings.

Re-support preserves authored local U/V geometry by default and changes the host mapping. Missing/ambiguous target fails closed.

Planar-face support remains Unsupported until a dedicated deterministic semantic face-frame algorithm is accepted; no OCCT UV/first-edge fallback is implied by this ADR.

### 5. Failure, Blocked, Delete and Suppress

Authored Feature identity is distinct from evaluation status.

An authored feature may be UpToDate, Failed, Blocked or Suppressed. Failed/Blocked features retain authored intent. Downstream features do not consume stale last-good B-Reps as current truth.

Delete and Suppress are distinct semantic commands:

- Delete removes the feature from authored history and lets dependents resolve/fail against the new sequence;
- Suppress preserves identity/inputs while removing the feature contribution for evaluation.

Both are ordinary Undoable mutations. Visibility is presentation and does not suppress evaluation.

### 6. Publication freshness

Evaluated result publication is authorized only for the current durable Document, current DocumentRevision, current runtime session generation/lease and current request/evaluation generation.

Session/request generations are runtime-only. Cancelled, failed or stale requests permanently lose publication authority. Geometry equality never restores authority.

### 7. Numerical and modeling-semantics policy

Modeling behavior has a durable modeling-semantics version distinct from container version, Part schema version, EngineeringRevision and runtime DocumentRevision.

For initial Part-v1 policy:

- display/pick/tessellation tolerances are not modeling inputs;
- exact semantic Profile geometry crosses into Kernel;
- no iterative fuzzy escalation or silent gap healing;
- provider fuzzy value is explicit and defaults to 0 unless an owning operation contract accepts another fixed policy;
- provider precision constants are implementation diagnostics, not authored tolerance;
- refine/unify policy is explicit per operation and covered by topology-reference regressions;
- a change that can alter evaluated geometry/topology/reference outcomes requires an explicit modeling-semantics compatibility/version decision.

### 8. Identity and read boundary

Typed Part-local BodyId/FeatureId/DatumId/PublishedReferenceId are externally qualified by DocumentId.

DocumentRevision remains runtime freshness. Runtime session/request generation and native file checkpoints are never persisted CAD identity.

Future Part read snapshots are immutable/provider-neutral. Future Assembly may consume only an accepted Part read/published-reference contract; it does not own or inspect internal feature/provider topology.

Transform direction is explicit at domain boundaries using the convention `parent_from_local`:

`P_parent = T_parent_from_local(P_local)`.

## Proposed product-scope consequence

The initial production slice is deliberately narrow: one existing valid Profile -> one-sided Extrude Add -> one Body, with complete edit/history/persistence lifecycle. Later Part-v1 packages add Cut, planar-face support/projection, Revolve and constant Fillet/Chamfer under the same reference/failure/numerical rules.

Exact scope is recorded in `work/PM-00A_ARCHITECTURE_RECOMMENDATIONS.md`.

## Rejected directions

- persisting TopoDS/OCAF/provider topology as authored identity;
- Face[n]/Edge[n]/tree-row/Viewer-token identity;
- geometry similarity/proximity as automatic rebinding authority;
- stale last-good geometry as current semantic truth after evaluation failure;
- screen/pick tolerance as modeling tolerance;
- increasing fuzzy tolerance until an operation succeeds;
- deriving planar-face Sketch orientation from arbitrary provider UV/first edge;
- implementing a global dependency framework or Assembly solver as a prerequisite;
- multi-body as a prerequisite for Part v1.

## Consequences

Positive:

- design intent remains provider-neutral and cold-rebuildable;
- topology failures are explicit instead of silently retargeted;
- operation failure remains repairable;
- runtime stale results cannot overwrite newer state;
- provider/refine policy changes become reviewable compatibility events.

Costs:

- evaluator and UI must expose explicit Failed/Blocked/Ambiguous/Missing states;
- durable reference selectors need semantic provenance/stage information;
- modeling-semantics versioning adds compatibility obligations;
- face-supported Sketch work requires an additional deterministic-frame decision before activation.

## Acceptance / next step

If the Owner accepts this candidate in PM-00B, materialize it as the next accepted ADR with the Owner-accepted wording and update `work/ACTIVE.yaml -> accepted_adrs`.

Until then this candidate has no architecture authority.
