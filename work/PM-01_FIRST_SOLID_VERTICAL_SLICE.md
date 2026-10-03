# PM-01 — Extrude Feature Vertical Slice: Body / Feature / Add / Cut

**Status:** COMPLETED — PASS; OWNER ACCEPTED 2026-10-03, FINAL MANUAL PASS 2026-10-03  
**Decision class:** D2 production Work Contract with bounded D0/D1 implementation inside ADR-0014 and ADR-0015  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Program authority:** `work/PART_MODELING_V1_ROADMAP.md` v1.5  
**Architecture authority:** `adr/ADR-0014-part-feature-architecture-references-and-modeling-semantics.md`, `adr/ADR-0015-extrude-feature-scope-preview-and-presentation.md`  
**Entry gate:** PM-00B COMPLETED — PASS  
**Owner acceptance:** 2026-10-03 — expanded PM-01 plan accepted before production mutation

## 1. Goal

Deliver the first production solid-modeling vertical slice as a development-ready Extrude Feature family:

`existing valid Profile -> Extrude Add -> single Body -> additional Extrude Add/Cut -> edit/recompute -> Undo/Redo -> Save -> Close -> Reopen -> cold rebuild`.

PM-01 proves durable Body/Feature identity, ordered evaluation, real Boolean composition, dynamic preview, GUI/Command Line parity and lifecycle completeness without introducing multi-body or topology-dependent face picking.

## 2. Activation rule

This Work Contract was activated by explicit Owner acceptance on 2026-10-03 and is now COMPLETED — PASS.

PM-01 production mutation is closed. Any future change to delivered PM-01 semantics requires a separately authorized Work Contract or an explicitly accepted amendment under the normal governance rules.

## 3. Scope IN

PM-01 may implement only what is required for this Extrude vertical slice:

- exactly one durable Part-v1 Body with typed Part-local BodyId distinct from DocumentId;
- ordered typed Feature records with typed Part-local FeatureId and non-aliasing/high-water identity allocation;
- one durable Extrude Feature family with Add/Cut operation and OneSide/Midplane extent;
- first successful solid-producing Feature must be Add; later Features may be Add or Cut;
- finite positive unit-aware Length;
- Forward/Reverse only for OneSide; Midplane uses total distance split equally about the support plane;
- minimum persistent schema migration preserving existing SketchId/ProfileId/EntityId meaning;
- durable modeling-semantics version distinct from container/schema/EngineeringRevision/DocumentRevision;
- Profile visibility policy `automatic / force_shown / force_hidden` with migration of current boolean visibility;
- exact Profile boundary/provenance transfer to provider-neutral Kernel input, including holes;
- production provider-neutral Kernel Extrude/Boolean boundary plus OCCT implementation;
- every successful stage yields exactly one valid solid;
- explicit non-success for no-effect, detached Add, empty Cut, multi-solid or invalid geometry;
- semantic cap/side lineage using ADR-0015 roles and exact boundary provenance;
- deterministic feature evaluation/status using UpToDate / Failed / Blocked / Suppressed;
- edit, reevaluate, Delete and Suppress semantics required by ADR-0014;
- dynamic transient preview driven by a single shared Extrude draft;
- live preview refresh from GUI and Command Line when the current input is valid;
- one Finish/Enter user action with execution-time revision/profile/draft/result revalidation;
- bounded provider-neutral Viewer solid scene and solid preview scene support, presentation-only and without topology picking;
- Part Feature Tree / Properties sufficient to inspect Body, ordered Features, statuses and Profile/Feature relationships;
- bidirectional Profile <-> Feature navigation/inspection without changing semantic ownership;
- automatic hiding of consumed Profiles under automatic visibility policy and temporary source-Profile reveal during Feature edit;
- semantic Command Line support for EXTRUDE / ADD / CUT / REVERSE / MIDPLANE / ONESIDE / FINISH / CANCEL and unit-aware distance input;
- one semantic command/transaction meaning shared by GUI, Command Line and non-GUI callers;
- Undo/Redo, Save/Close/Reopen and true cold rebuild from authored state;
- structured diagnostics for invalid/missing Profile, missing upstream Body, failed Boolean/geometry, Blocked downstream state and stale context;
- tests and Windows manual acceptance for the delivered workflow;
- required as-built and PL/EN product documentation plus regenerated Product Browser.

## 4. Frozen Extrude variants

### 4.1 Input

Exactly one existing valid Part Profile.

The Feature references ProfileId only. It does not duplicate SketchId, support plane, frame or boundary geometry as authored truth.

### 4.2 Operation

- Add;
- Cut.

An Empty Body accepts only Add as a newly finished Feature. Cut requires a valid upstream Body.

### 4.3 Extent

OneSide:

- total distance from the Profile support plane;
- Forward or Reverse.

Midplane:

- total distance centered on the Profile support plane;
- half-distance in each support-normal direction;
- Reverse is disabled/not authored because it would not change geometry.

Excluded extents: symmetric-with-independent-sides, two independent distances, Through All, Up To Face, Up To Next, draft/taper and thin-wall.

### 4.4 Result invariant

After every successful Feature stage the Body result is exactly one valid solid.

Detached Add, no-effect Add/Cut, zero-solid Cut and multi-solid output are explicit Failed outcomes. They never silently create another Body or silently pass through the previous shape.

## 5. Authored state versus evaluation

The durable model stores Body/Feature identity and Extrude intent. UpToDate/Failed/Blocked are derived evaluation states; Suppressed is authored intent.

Structural document validity does not require every Feature dependency to resolve at load time. An authored Feature may remain valid persistent intent while evaluation is Blocked because its Profile or upstream Body is unavailable.

No stale last-good geometry is current truth or a valid downstream modeling input.

## 6. Preview and Finish

One runtime Extrude draft is shared by Operations UI and Command Line.

Draft changes are non-authoring. A valid distance/operation/extent/direction change may trigger immediate synchronous preview evaluation. Invalid/incomplete text keeps the prior valid preview or clears it according to the UI state but never mutates authored state.

Finish/Enter is exactly one user confirmation. Execution must revalidate:

- current DocumentId and DocumentRevision;
- source Profile identity/resolution;
- legal operation against the current upstream Body;
- active draft generation;
- current successful evaluation corresponding to that draft.

If any condition is stale/invalid, Finish fails closed with no partial mutation and no Undo entry.

Cancel/Escape is non-authoring.

## 7. Profile visibility and relationship UX

PM-01 does not introduce Body/Feature Show/Hide.

Profile visibility becomes:

- automatic;
- force_shown;
- force_hidden.

Under automatic policy, Profiles consumed by active/non-suppressed Features are hidden from normal presentation. When no active Feature consumes a Profile, it is visible.

Existing persisted `visible=false` migrates to `force_hidden`; `visible=true` migrates to `automatic`.

Feature Properties/Tree presentation identifies Source Profile and Source Sketch. Profile Properties identify consuming Features. Navigation between related objects is supported. Editing a Feature temporarily reveals/highlights the source Profile without changing its authored visibility policy.

Suppress remains distinct from presentation visibility.

## 8. Kernel and numerical boundary

Kernel API remains provider-neutral and production code outside the OCCT provider cannot depend on TopoDS/OCAF handles, topology ordinals or provider identity.

PM-01 may add the minimum runtime evaluated-solid and neutral presentation-mesh contracts required by Extrude Add/Cut and Viewer presentation.

Modeling policy:

- fuzzy = 0;
- no fuzzy escalation;
- no silent healing;
- exact semantic Profile geometry enters Kernel;
- refine/unify must be explicit and reference-regression-covered before enabled;
- display tessellation is derived and never an operation input.

## 9. Viewer boundary

PM-01 may add only the public Viewer API necessary for:

- final evaluated solid presentation;
- transient Extrude preview presentation.

The API is provider-neutral and consumes derived presentation data, not durable topology identity.

Face/edge topology picking, semantic face/edge selection and public provider topology access remain outside PM-01.

## 10. Command Line behavior

Command Line and GUI manipulate the same Extrude draft.

Required command semantics include:

- EXTRUDE activates creation from an admissible selected Profile or enters Profile selection;
- ADD/CUT switch legal operation and refresh preview;
- REVERSE flips OneSide direction;
- MIDPLANE/ONESIDE switch extent mode;
- a legal Length expression updates distance and preview;
- FINISH/Enter commits when the current draft is valid;
- CANCEL/Escape exits without authored mutation.

Existing unit grammar remains authoritative.

## 11. Persistence and migration

The first Body/Feature schema change is explicit and versioned.

Migration preserves:

- Document properties and DocumentId;
- Origin presentation state;
- SketchId/EntityId and Sketch geometry;
- ProfileId/RegionIntent;
- existing ID high-water semantics;
- user-selected Length unit;
- existing explicit Profile hidden state.

Older valid Parts migrate to one valid Empty Body without inventing a Feature.

No B-Rep, TopoDS/OCAF handle, topology ordinal, Viewer token, tree row, tessellation or runtime session/request generation is serialized as authored CAD identity.

Save/Close/Reopen reconstructs authored Body/Feature intent and reevaluates from durable inputs without previous-process provider/cache state.

## 12. Scope OUT

PM-01 does not authorize:

- multi-body;
- Revolve;
- Fillet or Chamfer;
- Datum/Construction Plane implementation;
- Sketch support on model faces;
- projection/reprojection;
- deterministic planar-face frame derivation;
- Through All / Up To Face / Up To Next Extrude;
- draft/taper or thin Extrude;
- multiple Profiles in one Feature;
- edge/face repair UI beyond diagnostics/navigation required by PM-01;
- face/edge topology picking;
- Assembly, Drawing or occurrence infrastructure;
- Published References UI or final Assembly read interface;
- arbitrary Feature reorder/insertion;
- Loft, Sweep, Shell, Draft, Pattern or solid Mirror;
- general surface/direct-face editing;
- a global dependency framework;
- a mandatory Sketch constraint solver;
- generalized numerical healing or adaptive fuzzy retries;
- Viewer API beyond the bounded solid/preview presentation contract accepted by ADR-0015.

## 13. Acceptance evidence

Completion requires exact-head automated and manual evidence covering at least:

- migrate/open pre-PM-01 Parts with Sketch/Profile identities preserved, boolean Profile visibility migrated correctly and Empty Body created;
- valid Profile -> first Add -> exactly one valid solid;
- Profile with hole -> correct solid with hole;
- OneSide Forward/Reverse with unit-aware live distance preview;
- Midplane total-distance semantics with symmetric result and no authored Reverse;
- second attached Add -> one Body;
- valid Cut -> one Body;
- Cut cannot be newly authored without upstream Body;
- detached Add, no-effect Add, no-effect Cut, zero-solid Cut and multi-solid results fail explicitly;
- edit upstream Profile/Feature -> deterministic recompute or explicit Failed/Blocked without stale last-good truth;
- Feature edit preserves FeatureId and one Finish creates one Undo entry;
- Delete and Suppress are distinct, Undoable and reconstruct correctly;
- consumed Profile automatic hiding, explicit Show/Hide override and reappearance when no longer consumed;
- Feature -> Profile and Profile -> consuming Features navigation/inspection;
- Command Line and GUI manipulate the same draft/meaning;
- dynamic preview updates for legal parameter changes and stale draft/revision cannot commit;
- Cancel/rejected Finish causes no partial authored mutation;
- Undo/Redo restores authored Feature identity and accepted state;
- Save -> Close -> Reopen and true cold rebuild reproduce authored intent and semantic cap/side outcomes;
- OneSide profile_cap/extent_cap and Midplane negative_cap/positive_cap lineage plus side provenance;
- zero false Resolved topology-reference outcomes in the PM-01 matrix;
- camera/display/pick/tessellation state cannot alter modeling result;
- no fuzzy escalation or silent gap healing;
- Viewer solid/preview contract carries no provider/durable topology identity;
- semantic/core, kernel-native and desktop verification remain green on the same final source candidate;
- supported Windows manual workflow passes.

## 14. STOP conditions

STOP and return to the Owner if implementation would require:

- changing ADR-0014 or ADR-0015 semantics;
- persisting provider topology identity or geometry similarity as authored identity;
- adding multi-body;
- adding Extrude variants outside Section 4;
- introducing planar-face support/frame semantics;
- adding topology face/edge picking;
- Viewer public API beyond bounded solid/preview presentation;
- introducing Assembly/global dependency infrastructure;
- changing Foundation ownership;
- an unplanned schema meaning that cannot be expressed within this contract.

## Implementation closeout checkpoint — 2026-10-03

PM-01 runtime implementation is complete through checkpoint PM-01G and merged to `main` as `84a5be5a229f881120d838609ddfdc80c2431557`.

Bounded checkpoint evidence:

- PM-01A / PR #144 — schema/semantic foundation — Windows FULL #1326 PASS;
- PM-01B / PR #145 — production Extrude Boolean Kernel — Windows FULL #1327 PASS;
- PM-01C / PR #146 — evaluator + semantic commands — Windows FULL #1330 PASS;
- PM-01D / PR #147/#148 — final solid presentation + transient preview — Windows FULL #1331/#1332 PASS;
- PM-01E / PR #149/#150 — revision-bound draft/Finish + Operations/Command Line — Windows FULL #1333/#1334 PASS;
- PM-01F / PR #151 — Body/Feature Tree, Properties, relationships and Edit Extrude — Windows FULL #1336 PASS;
- PM-01G / PR #153 — Suppress/Delete lifecycle and cold persistence — exact-head Windows FULL #1339 PASS on `7c4783525c589c9e626ba260680e3e3026827a44`.

PR #152 was superseded by #153 and closed without merge after its runner-side LNK1103 failure; #153 contains the lifecycle functionality and supplies the accepted green evidence.

Runtime implementation completed through PM-01G and manual-remediation checkpoints H1-H7. Final runtime evidence includes Windows FULL #1369 PASS on `aaab5610f8f7c6fa3036d8123849523d06dd66e1`; H7 docs/Product Browser passed Windows DOCS #1370. Owner manual Windows acceptance PASS was reported on `afecfa9884bfeb53d5d0bb84ff110b3b280cf8d2`.

PM-01 completion obligations are satisfied. This final work/governance status synchronization must pass its exact-head closure gate before merge; that gate validates repository invariants only and does not reopen runtime acceptance.

PM-02 and later packages remain inactive.

## Documentation impact

Internal docs: required
User/Product docs: required
Reason: PM-01 introduces the first durable Body/Feature schema and user-visible Extrude Add/Cut workflow, dynamic preview, Command Line semantics, Profile consumption visibility and solid presentation.

## 15. Completion boundary

PM-01 completion authorizes only the delivered Extrude Feature vertical slice. PM-02 and later Part-v1 packages remain separately gated by their own Work Contracts and any still-open roadmap decisions.
