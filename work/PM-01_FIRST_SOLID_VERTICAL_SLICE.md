# PM-01 — First Solid Vertical Slice: Body / Feature / Extrude Add

**Status:** PROPOSED — NOT ACTIVE; EXPLICIT OWNER ACCEPTANCE REQUIRED  
**Decision class:** D2 production Work Contract with bounded D0/D1 implementation inside ADR-0014  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Program authority:** `work/PART_MODELING_V1_ROADMAP.md` v1.4  
**Architecture authority:** `adr/ADR-0014-part-feature-architecture-references-and-modeling-semantics.md`  
**Entry gate:** PM-00B COMPLETED — PASS is required before activation

## 1. Goal

Deliver the first production solid-modeling vertical slice:

`existing valid Profile -> Extrude Add -> single Body -> edit/recompute -> Undo/Redo -> Save -> Close -> Reopen -> cold rebuild`.

PM-01 proves the smallest durable Body/Feature implementation that satisfies ADR-0014 with one real user-facing solid operation. It is not a general feature-framework package.

## 2. Activation rule

This Work Contract is a proposal only.

It becomes active only after PM-00B is completed and the Owner explicitly accepts PM-01 scope. PM-00B completion, ADR-0014 acceptance or Part Modeling v1 roadmap acceptance does not activate production mutation.

Before activation, no durable Body/Feature schema, migration or user-facing Extrude Add implementation is authorized.

## 3. Scope IN after Owner acceptance

PM-01 may implement only what is required for the first solid vertical slice:

- exactly one durable Part-v1 Body with typed Part-local BodyId distinct from DocumentId;
- ordered typed Feature records with typed Part-local FeatureId and non-aliasing/high-water identity allocation;
- the minimum persistent schema migration from the current Part schema while preserving existing SketchId/ProfileId/EntityId meaning;
- a durable modeling-semantics version distinct from container/schema/EngineeringRevision/DocumentRevision;
- Extrude Add from one existing valid Part Profile;
- one-sided linear distance normal to the accepted Sketch support frame, with explicit direction/reverse semantics;
- first Add creating the Body solid and later Add features succeeding only when the accepted result is exactly one attached valid solid;
- explicit non-success for no-effect, detached/multi-solid or geometrically invalid results;
- exact Profile boundary/provenance transfer to the provider-neutral Kernel boundary, including holes;
- minimal OCCT-backed evaluation required for Extrude Add, without provider identity becoming authored state;
- semantic cap/side lineage sufficient to preserve ADR-0014 reference meaning for future consumers;
- deterministic feature evaluation/status using UpToDate / Failed / Blocked / Suppressed where applicable;
- edit, retry/reevaluate, Delete and Suppress semantics required by ADR-0014;
- transient preview with Finish/Cancel and revision/context revalidation before authored commit;
- Part Feature Tree / Properties / visibility surfaces only to the extent required to operate and inspect this slice;
- one semantic command/transaction meaning shared by GUI and non-GUI callers;
- Undo/Redo, Save/Close/Reopen and true cold rebuild from authored state;
- structured diagnostics for invalid Profile, failed geometry, Blocked downstream state and stale context;
- tests and Windows manual acceptance for the delivered workflow;
- required as-built and PL/EN product documentation plus regenerated Product Browser.

## 4. Frozen Extrude Add variant

The PM-01 product variant is deliberately narrow:

- input: exactly one existing valid Profile;
- support: the Profile's currently accepted deterministic Sketch support frame;
- extent: one finite positive Length;
- direction: support normal with explicit reverse/direction choice;
- result: exactly one valid solid in the single Body;
- holes in the Profile remain holes in the produced solid.

Excluded variants include symmetric/two-sided extent, draft, thin-wall, up-to-face/up-to-next/through-all, multiple Profiles in one Feature and multi-body output.

PM-01 may support multiple ordered Extrude Add Feature instances only under the same single-Body rule; it must not introduce a second Body.

## 5. Required semantic behavior

Finish commits only a currently valid accepted preview/result. Rejected Finish, Cancel and no-op do not mutate authored state or consume durable identity beyond already-accepted high-water rules.

Editing an existing Feature preserves FeatureId. If an upstream edit makes an authored Feature invalid, the Feature remains authored and repairable; downstream features cannot consume stale last-good geometry as current truth.

Reference resolution and geometric feasibility remain separate. Missing/Ambiguous/Unsupported reference states fail closed. Provider generated/modified/deleted history and geometry similarity remain evidence/diagnostics only.

Display, camera, tessellation, pick aperture, OSNAP and Viewer state are never modeling inputs. Fuzzy/healing escalation is forbidden. Any refine/unify choice used by Extrude Add must be explicit and regression-covered under the accepted modeling-semantics policy.

If evaluation is synchronous, PM-01 need not invent an async publication manager. If asynchronous publication is introduced, ADR-0014 freshness authority is mandatory.

## 6. Persistence and migration

The first Body/Feature schema change must be explicit and versioned.

Migration must preserve current durable document properties, Origin visibility, Sketch identity/content, Profile identity/intent, current high-water ID rules and user-selected length unit. Older valid Parts without Body/Feature state migrate to a valid Empty Body state without inventing a solid Feature.

No B-Rep, TopoDS/OCAF handle, topology ordinal, Viewer token, tree row or runtime session/request generation is serialized as authored CAD identity.

Save/Close/Reopen must reconstruct accepted Body/Feature intent and reevaluate from legal durable inputs without previous-process provider/cache state.

## 7. Scope OUT

PM-01 does not authorize:

- Extrude Cut;
- Revolve;
- Fillet or Chamfer;
- Datum/Construction Plane implementation;
- Sketch support on model faces;
- projection/reprojection;
- deterministic planar-face frame derivation;
- edge/face repair UI beyond the references actually required by PM-01;
- Assembly, Drawing or occurrence infrastructure;
- Published References UI or final Assembly read interface;
- multi-body;
- arbitrary feature reorder/insertion;
- Loft, Sweep, Shell, Draft, Pattern or solid Mirror;
- general surface/direct-face editing;
- a global dependency framework;
- a mandatory Sketch constraint solver;
- generalized numerical healing or adaptive fuzzy retries;
- public Viewer API changes unless separately Owner-approved.

## 8. Acceptance evidence

Completion requires exact-head automated and manual evidence covering at least:

- migrate/open an existing pre-PM-01 Part with Sketch/Profile identity preserved and an Empty Body;
- valid Profile -> first Extrude Add -> exactly one valid solid;
- Profile with a hole -> correct solid with hole;
- reverse/direction and unit-aware distance edit;
- attached subsequent Add coverage if multiple Add instances are delivered;
- detached/multi-solid/no-effect/invalid Profile outcomes fail explicitly according to ADR-0014;
- edit upstream Profile -> deterministic recompute or explicit Failed/Blocked state without stale last-good truth;
- Feature edit preserves FeatureId;
- Delete and Suppress are distinct, Undoable and reconstruct correctly;
- Cancel/rejected Finish/stale context causes no partial authored mutation;
- Undo/Redo restores authored Feature identity and accepted state;
- Save -> Close -> Reopen and cold rebuild reproduce authored intent and semantic cap/side outcomes;
- zero false Resolved topology-reference outcomes in the PM-01 matrix;
- camera/display/pick state cannot alter modeling result;
- no fuzzy escalation or silent gap healing;
- semantic/core, kernel-native and desktop verification remain green on the same final source candidate;
- supported Windows manual workflow passes.

## 9. STOP conditions

STOP and return to the Owner if implementation would require:

- changing ADR-0014 semantics;
- persisting provider topology identity or geometry similarity as authored identity;
- adding multi-body;
- introducing planar-face support/frame semantics;
- adding a public Viewer API;
- introducing Assembly/global dependency infrastructure;
- broadening Extrude variants beyond Section 4;
- changing Foundation ownership;
- an unplanned schema meaning that cannot be expressed within this contract.

## Documentation impact

Internal docs: required
User/Product docs: required
Reason: PM-01 introduces the first durable Body/Feature schema and user-visible Extrude Add workflow, including persistence, failure and lifecycle behavior.

## 10. Completion boundary

PM-01 completion authorizes only the delivered Extrude Add vertical slice. PM-02 and later Part-v1 packages remain separately gated by their own Work Contracts and any still-open roadmap decisions.
