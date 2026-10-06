# PM-05 — Edge Features: Fillet / Chamfer

**Status:** ACTIVE — OWNER ACCEPTED 2026-10-06  
**Decision class:** D2 production Work Contract  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Program authority:** `work/PART_MODELING_V1_ROADMAP.md` v1.27 — PM-05  
**Architecture authority:** ADR-0014, ADR-0016, ADR-0017  
**Predecessor:** PM-04 Axis / Revolve — COMPLETED PASS  
**Candidate baseline:** main `abeed19a8f1120fd149a119cd383b933c7cd4021`  
**Owner acceptance:** 2026-10-06 — exact bounded contract accepted, including multi-edge production scope, connected-corner behavior, full lifecycle and Part-toolbar Create/Modify grouping  
**Production mutation:** PM-05A evidence/instrumentation only; PM-05B+ durable production mutation remains BLOCKED until explicit Owner acceptance of the PM-05A conclusions

## 1. Goal

PM-05 adds two production ordered Part Features:

- **Fillet** — constant-radius edge blend;
- **Chamfer** — equal-distance edge chamfer.

They consume explicit semantic **material Edge** inputs from one declared upstream Body stage, operate on one or more explicitly selected Edges in one Feature, support connected-corner transitions, and complete the full Part Feature lifecycle.

PM-05 is the first production package in which strict bounded material Edge meaning is a durable Feature input. It therefore must prove and freeze the missing durable strict-Edge selector before persistence is implemented.

A single-Edge-only production fallback is explicitly forbidden. PM-05 must not ship a temporary one-Edge tool intended to be replaced later.

## 2. Product toolbar organization

As part of PM-05, the normal Part Modeling toolbar is reorganized semantically, analogous to the grouped Sketcher toolbar.

Required normal Part-mode grouping:

```text
Create:
[ Sketch ] [ Datum Plane ] [ Extrude ] [ Revolve ]

Modify:
[ Fillet ] [ Chamfer ]
```

Rules:

- grouping labels are user-facing semantic organization, not new domain objects;
- this change does not introduce a new Part-level Select command solely for toolbar symmetry;
- normal neutral Viewer/Tree selection remains the selection surface outside active tools;
- Sketch Edit keeps its own existing Sketcher toolbar organization;
- no standalone GUI Axis creation button returns;
- the canonical English feature names are **Fillet** and **Chamfer**;
- future localization may display user-facing translated labels, but durable/code terminology remains Fillet / Chamfer.

## 3. Accepted Fillet variant

Part-v1 PM-05 Fillet is exactly:

```text
Constant Radius Fillet
input: 1..N explicit material Edge references
parameter: one common Radius > 0
```

All selected Edges in one Fillet Feature use the same authored radius.

In scope:

- one Edge;
- multiple disconnected Edges;
- multiple connected Edges;
- Edges meeting at shared corners/vertices;
- common connected-corner transition topology produced by the provider;
- editing the explicit Edge set and Radius.

Out of scope:

- variable radius;
- per-Edge radius values;
- face fillet;
- full-round fillet;
- setback controls;
- chordal-radius variants;
- automatic geometry-similarity expansion;
- hidden tangent-chain authored expansion.

A provider may create required transition/corner geometry for the explicit selected set. It may not silently add an un-authored material Edge to the semantic input set.

## 4. Accepted Chamfer variant

Part-v1 PM-05 Chamfer is exactly:

```text
Equal Distance Chamfer
input: 1..N explicit material Edge references
parameter: one common Distance > 0
```

All selected Edges in one Chamfer Feature use the same authored distance.

In scope:

- one Edge;
- multiple disconnected Edges;
- multiple connected Edges;
- Edges meeting at shared corners/vertices;
- common connected-corner transition topology produced by the provider;
- editing the explicit Edge set and Distance.

Out of scope:

- distance + angle;
- distance1 + distance2;
- per-Edge distances;
- vertex-only chamfer;
- face-based chamfer;
- automatic geometry-similarity expansion;
- hidden tangent-chain authored expansion.

## 5. Multi-Edge authored meaning

The Edge list in one Feature is a **semantic set**, not a provider execution order.

Required properties:

- at least one input Edge;
- no duplicate semantic Edge reference;
- canonical deterministic ordering for comparison/persistence/evaluation;
- user selection order carries no durable modeling meaning;
- connected and disconnected inputs may coexist in the same Feature;
- all input references consume the same upstream Body stage;
- all selected Edges are submitted as one Feature operation against one upstream Body;
- connected Edge networks are not implemented as an authored sequence of independent one-Edge Features;
- no partial success: the Feature succeeds as a whole or fails as a whole.

If the provider's result depends on input insertion order, the adapter must impose one deterministic semantic order and PM-05A must prove that the chosen policy is stable for the accepted matrix. Provider traversal order is never authored authority.

## 6. Connected-corner behavior is mandatory

PM-05 production acceptance requires connected-corner behavior. At minimum PM-05A must characterize and PM-05C/D must productionize:

- two selected Edges sharing one material corner;
- three selected Edges meeting at a common box/trihedral corner;
- a closed explicit Edge loop where geometrically valid;
- mixed connected + disconnected explicit Edge sets;
- Fillet and Chamfer versions of the above where the provider supports a geometrically valid result.

Corner transition faces/edges are part of the one Feature result, not additional authored Features.

A common connected-corner case may return **Failed** when the requested Radius/Distance is geometrically impossible. It may not be declared unsupported merely to avoid multi-edge/corner implementation.

If PM-05A demonstrates that the production provider cannot supply deterministic connected-corner semantics/topology accounting for these common cases, PM-05 stops for explicit Owner D2 review. It must not silently downgrade to a one-Edge production tool.

## 7. Strict material Edge input

Fillet and Chamfer consume a strict bounded **material Edge**, not a Curve carrier.

The durable input family is conceptually:

```text
MaterialEdgeReference
    BodyStageRef
    semantic material-Edge selector
```

The selector must identify one bounded engineering Edge by semantic provenance. It must not contain or depend on:

- `TopoDS_Edge` / provider handles;
- runtime Edge tokens;
- provider topology ordinals;
- Viewer/presentation tokens;
- mesh indices;
- XYZ/centroid/length as identity;
- nearest/similar geometry;
- provider traversal order.

Current PM-02 runtime semantics already distinguish:

```text
FeatureCurveAddress / Curve meaning
strict_edge_status
BodyEdgeTopologyRecord.referenceability
periodic_seam
representation_partition
```

but there is no current durable strict `MaterialEdgeReference` type. PM-05A must freeze the concrete semantic selector/branch discriminator before PM-05B schema work.

A durable Edge selector must be capable of distinguishing disconnected bounded branches when one Curve/surface-pair meaning is insufficient.

## 8. Edge authoring admission

A viewport Edge is admissible for Fillet/Chamfer authoring only when the current evaluated Body stage proves it is:

- accounted in the complete `BodyStageTopologyCatalog`;
- a material engineering Edge;
- referenceable;
- singularly Resolved as a strict bounded Edge;
- not a periodic seam representation artifact;
- not an ADR-0017 same-Surface representation partition;
- current for the same DocumentRevision/runtime evaluation generation.

Stale runtime topology evidence is rejected.

An Ambiguous, Missing, Unsupported, representation-artifact or integrity-failure Edge cannot be committed as a new PM-05 input.

## 9. Consumed Body stage

Every Fillet/Chamfer consumes the exact Body stage immediately before that Feature in ordered history.

All authored Edge references in the Feature must resolve in that same consumed stage.

Examples:

```text
Extrude 1
Revolve 1
Fillet 1
Chamfer 1
```

`Fillet 1` resolves inputs only in `AfterFeature(Revolve 1)`.

`Chamfer 1` resolves inputs only in `AfterFeature(Fillet 1)`.

Global final-Body searching is forbidden.

## 10. Upstream-change semantics

For each singular authored material Edge input:

- exactly one valid semantic current candidate -> **Resolved**;
- no candidate -> **Missing**;
- more than one singular candidate after split/branching -> **Ambiguous**;
- undeclared/non-referenceable semantic meaning -> **Unsupported**.

Mandatory rules:

- split never means "take the first/longest/nearest fragment";
- split never silently expands one authored Edge input into multiple Edges;
- merge never silently chooses a winner among incompatible prior meanings;
- coordinates, length, radius or proximity may support diagnostics only;
- a later unrelated Edge with identical geometry never revives a Missing reference;
- repair is explicit authored mutation.

## 11. Feature evaluation states

Reference resolution and geometric executability remain separate.

Examples:

```text
input Edge Missing
    -> Feature Blocked

input Edge Ambiguous
    -> Feature Blocked

upstream Feature Failed/Blocked
    -> Feature Blocked

all Edges Resolved
Radius/Distance geometrically impossible
    -> Feature Failed
```

A Failed/Blocked authored Feature retains FeatureId, explicit Edge references, parameter and name.

No stale last-good B-Rep becomes current Body truth or downstream modeling input.

## 12. Kernel operation contract

PM-05 adds provider-neutral kernel operations for Fillet and Chamfer.

Conceptually:

```text
upstream RuntimeSolid
+ current runtime Edge realizations resolved from semantic references
+ one Radius / Distance
        ↓
one exact provider operation
        ↓
one SolidModelingResult
```

Requirements:

- all selected Edges are applied in one provider feature operation;
- no sequential per-Edge authored sub-operation is used to manufacture connected-corner behavior;
- no best-effort partial feature result;
- valid success is exactly one valid solid;
- no fuzzy escalation;
- no "heal until it works";
- display/pick/tessellation tolerances are not modeling inputs;
- runtime provider Edge tokens are transient adapter inputs only.

### Tangent propagation

PM-05A must explicitly characterize whether the provider silently propagates a selected Edge into a tangent contour/chain.

The production contract is explicit-input semantics. Therefore either:

1. the provider path can be bounded to exactly the authored material Edge set; or
2. any unavoidable propagation must be elevated to an explicit separately Owner-accepted authored semantic rule before PM-05B.

Silent provider-driven input expansion is forbidden.

## 13. Refine / unify / healing policy

Default PM-05 direction:

- no generic post-operation healing;
- no global same-domain unification merely to improve appearance;
- no iterative tolerance widening;
- preserve complete provider topology accounting.

PM-05A must compare the accepted provider result with any candidate refine/unify step needed for correctness.

If an explicit refine/unify step is necessary, the completion proposal must state:

- exact operation;
- exact timing;
- effect on generated/inherited topology;
- effect on Edge reference survival;
- whether modeling-semantics version must change.

A generic `UnifySameDomain`-style cleanup is not implicitly authorized by this Work Contract candidate.

## 14. Generated topology semantics

PM-05 must not treat Fillet/Chamfer output as an opaque B-Rep.

Every successful resulting stage must still have complete Face/Edge/Vertex accounting.

PM-05A must characterize semantic provenance for at least:

- a Fillet blend surface associated with an explicit source Edge meaning;
- a Chamfer surface associated with an explicit source Edge meaning;
- connected-corner/transition patches created from multiple selected inputs;
- boundaries between generated Fillet/Chamfer surfaces and inherited surfaces;
- inherited material Edges that survive/trim/split/delete;
- generated vertices/points at transition boundaries;
- provider representation artifacts introduced by the operation.

Production semantic addresses must derive from `FeatureId` plus defensible input/corner provenance, never provider Face/Edge order.

Not every provider patch must automatically become durably referenceable. However ordinary visible engineering Edges produced by PM-05 that are needed for subsequent accepted Fillet/Chamfer chaining must receive defensible semantic reference meaning. A visually ordinary material Edge may not remain permanently unreferenceable merely because it was produced by Fillet/Chamfer.

## 15. PM-05A hard evidence gate

PM-05A is an evidence/topology checkpoint. It may add bounded test/provider instrumentation but does not persist Fillet/Chamfer Features and does not expose a production GUI tool.

Mandatory provider/evaluation matrix includes at least:

### Fillet

- single straight box Edge;
- multiple disconnected box Edges;
- two adjacent Edges sharing one corner;
- three box Edges meeting at one trihedral corner;
- closed box Edge loop;
- mixed connected/disconnected set;
- radius approaching geometric failure;
- upstream dimension change preserving selected meanings;
- selected Edge trimmed;
- selected Edge deleted;
- selected Edge split;
- sequential `Fillet -> later edge-consumer` topology inspection.

### Chamfer

The analogous matrix for equal-distance Chamfer.

### Chaining

- `Fillet -> Chamfer`;
- `Chamfer -> Fillet`;
- selecting ordinary generated engineering Edges from the first Feature as input to the second;
- cold re-evaluation with zero reliance on previous-process provider handles/history.

### Required PM-05A conclusions

Before PM-05B production persistence begins, PM-05A must freeze for Owner review:

1. concrete durable strict `MaterialEdgeReference` selector, including branch discriminator where required;
2. deterministic canonicalization of a multi-Edge authored set;
3. connected-corner semantic provenance for generated topology;
4. provider tangent-chain behavior and explicit-input enforcement strategy;
5. fixed refine/unify policy;
6. generated Face/Surface/Edge/Curve/Vertex/Point role families required for chained edge features;
7. exact supported common-corner matrix.

**No PM-05B production mutation is authorized until the PM-05A conclusions are explicitly Owner-accepted.**

This gate may narrow exotic provider corner cases, but it may not reduce the production feature to single-Edge-only authoring or remove common connected-corner support.

## 16. Durable Feature model

After PM-05A acceptance, PM-05B introduces durable definitions equivalent to:

```text
FilletFeature
    edges: 1..N MaterialEdgeReference
    radius: LengthValue

ChamferFeature
    edges: 1..N MaterialEdgeReference
    distance: LengthValue
```

Rules:

- `radius > 0`;
- `distance > 0`;
- all Edge references valid, unique and in one consumed Body stage;
- canonical Edge-set order;
- FeatureId lifecycle follows ADR-0014;
- default names follow existing ordered Feature naming conventions;
- no separate durable corner Feature;
- no authored runtime/provider topology token.

`PartFeatureDefinition` gains Fillet and Chamfer variants only after the PM-05A hard gate.

## 17. Persistence

PM-05B owns native Part schema **v14**.

Required migration:

- v1..v13 load under their existing meanings;
- migration does not rewrite existing DocumentId/BodyId/FeatureId/SketchId/ProfileId/DatumId/AxisId/EntityId identities;
- old documents acquire no synthetic Fillet/Chamfer Features;
- v14 persists only semantic Edge intent and fixed parameters, never provider topology/runtime handles;
- Save/Reopen and cold reconstruction produce the same semantic status/result or explicit deterministic failure.

A modeling-semantics-version bump is not required merely because new Feature kinds are added. PM-05A must request explicit D2 review if the chosen numerical/refine policy would change previously accepted Extrude/Revolve/reference outcomes.

## 18. Draft / preview / Finish / Cancel

Fillet and Chamfer use revision-bound application drafts analogous to existing Part Feature tools.

Create draft:

- selection-first and command-first are supported;
- one or more admissible Edges may be added/removed while active;
- parameter edits refresh preview;
- no durable FeatureId is consumed before successful Finish.

Preview:

- is an exact transient candidate result of the whole edge Feature;
- uses the exact upstream Body stage;
- applies the complete explicit Edge set together;
- does not substitute a raw tool or approximate visual effect;
- carries no authored mutation or durable topology identity;
- stale preview loses publication authority.

Finish:

- requires all semantic inputs Resolved and exact candidate evaluation successful;
- commits exactly one Feature in one transaction / one Undo step;
- consumes one fresh FeatureId.

Cancel/rejected Finish:

- zero authored mutation;
- zero durable FeatureId consumption.

## 19. Multi-selection interaction

Normal product interaction supports both:

```text
select multiple admissible Edges -> Fillet / Chamfer
```

and:

```text
Fillet / Chamfer -> select/toggle multiple admissible Edges
```

During an active draft:

- eligible Edge hover/preselection is semantic/topology aware;
- clicking an eligible Edge toggles membership in the explicit input set;
- duplicate input is impossible;
- ineligible seam/partition/unsupported/stale topology is rejected visibly;
- the Operations panel exposes selected Edge count and current status;
- a clear-selection action is available;
- no implicit tangent-chain/loop expansion occurs unless a later accepted semantic rule explicitly introduces it.

## 20. Operations UI

Minimum Fillet surface:

```text
FILLET

Selection
  Selected edges: N
  [ Clear ]

Parameters
  Radius: [ value ] unit

Status
  Preview: OK
  / Blocked: ...
  / Failed: ...

[ Finish ] [ Cancel ]
```

Minimum Chamfer surface:

```text
CHAMFER

Selection
  Selected edges: N
  [ Clear ]

Parameters
  Distance: [ value ] unit

Status
  Preview: OK
  / Blocked: ...
  / Failed: ...

[ Finish ] [ Cancel ]
```

GUI and Command Line are adapters over the same semantic draft/validation path.

## 21. Command Line

PM-05 provides keyboard-first commands:

- `FILLET`;
- `CHAMFER`.

They support the same selection-first/command-first explicit Edge set and exact parameter semantics as GUI.

At minimum the command adapter supports:

- parameter entry/change;
- Finish;
- Cancel;
- clear/remove input selection;
- structured rejection of stale/ineligible/ambiguous topology.

Command Line does not create a separate modeling path.

## 22. Edit and repair

Edit Fillet / Edit Chamfer:

- preserves existing FeatureId;
- restores the complete authored semantic Edge set;
- restores Radius/Distance;
- allows adding/removing/replacing Edge references;
- previews from the Feature's exact upstream stage;
- commits one semantic edit transaction / one Undo step;
- Cancel preserves the previous authored Feature exactly.

Repair is explicit.

If one input becomes Missing/Ambiguous/Unsupported, Edit identifies the failing semantic input and allows explicit remove/replace. No nearest/similar automatic rebind is permitted.

## 23. Delete / Suppress / Undo / Redo

Existing ADR-0014 Feature lifecycle applies without exception:

- Suppress preserves FeatureId and inputs but removes the Feature contribution from evaluation;
- Delete removes the authored Feature;
- downstream references resolve/fail against the changed ordered history;
- Undo/Redo restores exact Feature identity and authored input set;
- branching/high-water ID rules remain non-aliasing.

No PM-05 lifecycle obligation is deferred to PM-06.

## 24. Save / reopen / cold reconstruction

PM-05 must prove:

- successful Fillet/Chamfer Save -> Close -> Reopen;
- Failed/Blocked Feature Save -> Close -> Reopen with authored intent preserved;
- exact Edge set and parameter restoration;
- no previous-process runtime topology handles needed;
- fresh topology catalogs and provider solids rebuilt from authored intent;
- chained `Fillet -> Chamfer` and `Chamfer -> Fillet` cold rebuild;
- history prior to close is not required to persist unless already covered by the general history contract.

## 25. Structured diagnostics

Diagnostics identify:

- FeatureId where authored;
- failing input index/semantic Edge reference where applicable;
- reference status: Missing / Ambiguous / Unsupported;
- upstream unavailable state;
- invalid Radius/Distance;
- provider/kernel failure;
- invalid B-Rep / no valid single-solid result;
- topology-integrity failure.

A resolved semantic input followed by a provider geometric failure is reported differently from an unresolved input.

## 26. Checkpoint sequence

### PM-05A — Edge-feature topology/provider evidence

Evidence-only hard gate defined in §15. No persisted Fillet/Chamfer Feature and no production toolbar action.

### PM-05B — durable EdgeReference / Feature model / schema v14

After explicit Owner acceptance of PM-05A conclusions:

- concrete strict MaterialEdgeReference;
- FilletFeature / ChamferFeature;
- structural validation;
- v14 persistence/migration;
- semantic commands/draft foundation;
- non-aliasing Feature identity.

### PM-05C — kernel operations / evaluation / topology lineage

- provider Fillet/Chamfer;
- explicit multi-Edge operation;
- connected-corner production behavior;
- complete output topology accounting;
- generated semantic role families;
- deterministic inherited/split/remove behavior;
- Failed/Blocked distinction.

### PM-05D — Workbench / toolbar / Viewer / preview / Command Line

- Part toolbar `Create:` / `Modify:` grouping;
- Fillet / Chamfer buttons in Modify;
- semantic multi-Edge preselection/picking;
- selection-first/command-first;
- Operations panels;
- exact preview;
- `FILLET` / `CHAMFER` parity.

### PM-05E — edit / repair / lifecycle / persistence

- Edit Edge set/parameter preserving FeatureId;
- explicit repair;
- Delete/Suppress;
- Undo/Redo;
- upstream preserve/split/merge/remove matrix;
- Save/Reopen/cold rebuild;
- Fillet<->Chamfer chaining.

### PM-05F — documentation / automated evidence / Owner Windows acceptance

- current internal docs;
- paired PL/EN Product docs;
- generated Product Browser;
- cumulative PM-05 automated acceptance matrix;
- final exact-head Windows FULL;
- Owner supported-Windows workflow;
- only explicit Owner PASS closes PM-05.

## 27. Acceptance matrix minimums

Before PM-05 closure, automated and Owner evidence must cover at least:

- toolbar Create/Modify grouping;
- selection-first and command-first multi-Edge Fillet;
- selection-first and command-first multi-Edge Chamfer;
- disconnected Edge set;
- two-edge connected corner;
- three-edge trihedral corner;
- closed loop;
- mixed connected/disconnected set;
- invalid excessive Radius/Distance -> Failed, no partial result;
- upstream preserved Edge -> Resolved/recompute;
- upstream deleted Edge -> Missing/Blocked;
- upstream split singular Edge -> Ambiguous/Blocked;
- no first/nearest/longest fallback;
- seam and same-Surface partition not authorable;
- explicit input set unaffected by provider traversal order;
- no silent tangent-chain authored expansion;
- generated ordinary engineering Edge usable by later accepted edge Feature;
- Fillet -> Chamfer;
- Chamfer -> Fillet;
- Edit preserves FeatureId;
- Cancel/rejected edit zero mutation;
- Delete/Suppress/Undo/Redo;
- v13 -> v14 migration;
- Save/Close/Reopen;
- true cold reconstruction;
- current docs + Browser;
- final Windows Owner PASS.

## 28. Explicit exclusions

PM-05 does not include:

- variable-radius Fillet;
- per-Edge parameter values;
- full-round / face fillet;
- setback controls;
- Chamfer distance-angle;
- asymmetric two-distance Chamfer;
- automatic tangent-chain/loop authoring;
- Datum Axis;
- Projection / Project Edge;
- general topology healing framework;
- arbitrary feature reorder/insertion;
- multi-body;
- direct face editing;
- general surface modeling;
- Shell / Draft / Pattern / Mirror;
- Assembly/Drawing implementation.

## Documentation impact

Internal docs: required  
User/Product docs: required  
Reason: PM-05 introduces durable strict material-Edge references, new Fillet/Chamfer Feature kinds and persistence, multi-Edge connected-corner behavior, kernel operations, Part-toolbar organization and new failure/repair lifecycle.

Before completion:

- internal Part/persistence/kernel/viewer docs describe the as-built MaterialEdgeReference, evaluation/topology lineage, accepted refine/unify policy and Fillet/Chamfer lifecycle;
- paired PL/EN Product docs describe the accepted constant-radius Fillet and equal-distance Chamfer workflows, explicit multi-selection and repair behavior;
- Product Browser is regenerated from canonical Markdown;
- excluded advanced variants and implicit tangent-chain authoring must not be documented as implemented.

PM-05A evidence-only implementation does not by itself require Product documentation of a user-facing feature. PM-05F must make all required documentation current before package closure.

## 30. Activation state

This exact Work Contract was explicitly Owner-accepted on 2026-10-06.

PM-05 is ACTIVE only at **PM-05A — Edge-feature topology/provider evidence**.

Authorized now:

- bounded provider/evaluation evidence;
- test-only or evidence-only adapter instrumentation needed to observe Fillet/Chamfer provider history/topology;
- semantic experiments that do not persist a production Fillet/Chamfer Feature and do not expose a production Fillet/Chamfer GUI command.

Still blocked:

- durable `MaterialEdgeReference` schema/API;
- schema v14 persistence;
- `FilletFeature` / `ChamferFeature` in production `PartFeatureDefinition`;
- production kernel/evaluation mutation path;
- production toolbar buttons / Operations / Command Line;
- PM-05B through PM-05F.

PM-05A must produce the conclusions required by §15 and return for explicit Owner acceptance. Only that second Owner decision may authorize PM-05B+ production mutation.
