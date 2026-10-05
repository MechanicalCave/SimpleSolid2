# PM-02J — Owner Manual Acceptance Remediation

**Status:** ACTIVE — AUTOMATED REMEDIATION PASS; OWNER WINDOWS RE-TEST PENDING  
**Parent Work Contract:** `work/PM-02_BODY_SEMANTIC_TOPOLOGY_FACE_SUPPORTED_SKETCH.md`  
**Architecture authority:** ADR-0016 + ADR-0017  
**Trigger:** Owner Windows manual acceptance on 2026-10-05  
**Baseline:** `f605872474e4cc74661a2ad420d93f6a3d6c6664`

## Trigger and observed result

PM-02 automated/runtime evidence through FULL #1451 and documentation evidence through DOCS #1454 remain valid for their exact candidates, but Owner manual acceptance did not pass.

Observed manual findings:

- base Body, topology Properties, View Styles, hidden-edge non-select-through, Feature Contribution, split/trim contribution, planar cap/lateral/Cut-exposed Sketch support, non-planar Unsupported, face-backed Add/Cut, re-support, Undo/Redo and Save/Close/Reopen passed;
- ordinary topology acquisition is unstable on at least one oriented box view/Face; one visible corner Vertex cannot be acquired reliably and adjacent Edges flicker;
- ordinary hover does not expose a stable topology preselection, so Tab/Shift+Tab candidate cycling cannot be verified and is functionally unavailable;
- a face-supported Sketch can be entered for Edit after a downstream Extrude, but Sketch/Profile presentation disappears because Viewer support placement resolution consults only the final Body-stage topology cache rather than the support's declared earlier stage;
- a planar region artificially partitioned by coplanar Extrude Add is visually one engineering surface but cannot provide singular Sketch support;
- the same Add partition Edge is shown as an ordinary engineering Edge, which is misleading for model inspection and would be unsuitable as a future Drawing projection boundary;
- pending Create Sketch support uses a button labeled `Finish Sketch`, even though that action creates the Sketch; the same label later correctly exits Sketch edit;
- activating/finishing Profile/Sketch tooling can cause an unwanted top-level window resize toward the monitor bottom edge;
- whole-Part-Sketch Delete is absent; only Sketch-entity erase exists. This remains a separately bounded lifecycle gap and is not allowed to dilute the blocker fixes below.

Additional Owner investigation established that chained Face -> Sketch -> Extrude itself is healthy: at least seven consecutive operations remained usable on ordinary planar Faces. The Create Sketch blocker is specific to the artificially partitioned coplanar Add surface, not Feature-count depth.

## Accepted D2 remediation

ADR-0017 is accepted for this remediation.

A coplanar Add must **not** merge semantic Surface identity because geometry looks equal.

A newly-created planar Add Surface may continue exactly one inherited planar Surface only when Boolean lineage proves unique material continuity in the current result: either a shared current Face descendant or adjacent created/inherited descendants separated by one current partition Edge that the modeling kernel certifies as planar same-domain. The inherited carrier keeps durable identity; runtime contribution evidence remains separate. Part/Application code must never search by coplanarity/proximity.

A current Edge that only partitions two bounded Face realizations of that same semantic Surface is a representation partition: complete/accounted, but not an ordinary visible/pickable engineering Edge.

## Remediation checkpoints

### J-R1 — governance / D2 acceptance

Gate:

- ADR-0017 accepted and listed in ACTIVE;
- manual result recorded as FAIL/remediation-active rather than PENDING;
- PM-03 and Projection remain inactive.

### J-R2 — Add Surface continuation + representation-partition Edge

**State:** COMPLETED — PASS  
**Final candidate:** `11c61bf9123f04bce55db3e211aa44812599b114`  
**Windows FULL:** #1470 — PASS  
**Merged main:** `bd5226a7d3cddc9743440295f02c9e2e87d6b4df`

Scope:

- production OCCT/Kernel lineage and Part topology only;
- Add only for the accepted first remediation;
- unique lineage-proven inherited continuation through shared descendant or provider-certified same-domain partition adjacency;
- no geometry similarity/proximity search or global same-domain healing;
- inherited Surface expands to current continued Face realizations;
- created claim does not become a competing Sketch-support carrier;
- Feature Contribution remains runtime-truthful without stealing carrier identity;
- same-Surface partition Edge remains accounted but is classified representation-only and excluded from ordinary display/pick.

Required regressions:

- coplanar Add extension whose OCCT Fuse keeps adjacent bounded Faces separated by a result partition Edge yields one Resolved semantic Surface support across those fragments;
- Sketch can be created by selecting either current Face fragment of the continued Surface;
- strict Face may remain split/Ambiguous while Surface support stays Resolved;
- partition Edge is not ordinary visible/pickable;
- equal-plane but lineage-unrelated decoy does not continue;
- two competing inherited continuation candidates remain Ambiguous/fail-closed;
- existing split/delete/alias/similarity matrix stays green;
- Feature Contribution for the Add remains present.

### J-R3 — stage-correct face-Sketch Edit presentation

**State:** COMPLETED — PASS  
**Final candidate:** `77a924eb79578fc084403684471be0bbb4307f32`  
**Windows FULL:** #1471 — PASS  
**Merged main:** `7854302eb97896f88ae6229df52602dc1a70c297`

Scope:

- Viewer/UI derived placement only;
- resolve active face-supported Sketch presentation against the exact support-declared Body stage from the current same-revision evaluation;
- do not use final-stage topology as a substitute;
- no persistence/schema change.

Gate:

- Base -> face Sketch/Profile -> downstream Add/Cut -> re-enter source Sketch Edit;
- Sketch lines, Profile and grid are visible at the correct derived frame;
- upstream support move recomputes the edit presentation;
- Missing/Ambiguous support shows structured unavailable state, never stale last-good frame.

### J-R4 — topology hover/preselection / candidate cycling / ordinary pick

**State:** COMPLETED — PASS  
**Final candidate:** `ed455548cf8ea9d1a1f69c13fcfb369639228eac`  
**Windows FULL:** #1472 — PASS  
**Merged main:** `7a113ac07a568809160a2e956b723d3ffdf1fb1a`

Scope:

- Viewer acquisition only;
- repair hover query delivery, front-visible Vertex/Edge candidate stability and preselection publication;
- preserve Vertex -> Edge -> Face ranking and tool-specific kind filtering;
- no provider-order identity.

Gate:

- stable visible preselection on ordinary box topology;
- near-corner Vertex, near-boundary Edge, Face-interior Face acquisition;
- formerly failing RIGHT-oriented corner/Edges stable;
- Tab/Shift+Tab cycles the current candidate stack;
- hover/cycling creates no authored mutation and does not churn committed Properties before click;
- stale candidate stack cannot commit after scene replacement.

### J-R5 — bounded Sketch UX cleanup

**State:** COMPLETED — PASS  
**Final candidate:** `b9e70c684c2d054da21662ef3a8ae56ac59f60c0`  
**Windows FULL:** #1473 — PASS  
**Merged main:** `7baf13b255f7e5a587a8caeffde19d0ab69434d0`

Scope:

- pending support action label is `Create Sketch`, not `Finish Sketch`;
- after creation/edit entry the normal exit action remains `Finish Sketch`;
- eliminate reproducible top-level window expansion caused by Profile/Sketch tool panel transitions;
- presentation/layout only; no authored semantics.

Gate:

- Create Sketch label changes with state correctly;
- Cancel pending support remains zero mutation;
- entering/leaving Profile/Sketch tools does not resize the application window unexpectedly.

### J-R6 — consolidated automated gate + Owner re-test

**State:** ACTIVE — DOCS/READINESS UPDATE; OWNER RE-TEST PENDING

J-R2..J-R5 are now complete. The post-remediation runtime candidate is `b9e70c684c2d054da21662ef3a8ae56ac59f60c0`, Windows FULL #1473 PASS; it includes the already-passed R2/R3/R4 runtime merges plus R5.

Remaining J-R6 work:

- canonical internal + PL/EN documentation update;
- Product Browser regeneration and docs/closure verification;
- repeat focused Owner Windows checks for manual items 2, 4, 9-15 and the window/label observations;
- then repeat final PM-02 acceptance matrix as needed.

PM-02 must remain ACTIVE until Owner reports PASS.

## Deferred but recorded gaps

### Delete whole Sketch

The current application command surface can erase Sketch entities but does not provide a whole-Part-Sketch Delete command.

This is a real lifecycle gap, but it is not the cause of the failed PM-02 topology/face-support manual gate. Treat it as a separately bounded lifecycle decision after the blocker remediation unless the active PM-02 closure explicitly expands to include it.

### Future authored Split/Divide Face

A future explicit Split/Divide Face tool may intentionally create a design boundary that remains visible/pickable and may project into Drawing.

Do not confuse that future authored operation with representation partitions introduced incidentally by current Boolean Add.

## Completion rule

Do not mark PM-02 COMPLETED and do not activate PM-03 until:

- J-R2..J-R5 pass automated verification;
- canonical docs/Product Browser are current after remediation;
- Owner Windows manual re-test passes;
- final closure records the post-remediation runtime/docs candidates.
