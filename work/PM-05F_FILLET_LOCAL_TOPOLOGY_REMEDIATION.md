# PM-05F — Fillet Local-Topology Remediation

**Status:** AUTOMATED REMEDIATION COMPLETED — PASS; Owner retest pending
**Parent Work Contract:** `work/PM-05_EDGE_FEATURES.md`
**Checkpoint:** PM-05F
**Date activated:** 2026-10-07

## Trigger

Owner manual Windows testing found that Chamfer behaves acceptably on a simple cube while Fillet is unstable for ordinary multi-Edge selections:

- two adjacent cube Edges can fail;
- adding the third Edge incident to the same vertex can make the Fillet valid;
- adding further Edges can make it fail again;
- on a more complex mixed straight/curved/generated shape, only simple single straight Edges were reliably accepted.

This is a PM-05 acceptance blocker. PM-05 remains ACTIVE and PM-06 remains inactive.

## Owner semantic amendment

The Owner explicitly accepted the following engineering rule on 2026-10-07:

> **Strict authored intent, permissive local result topology.**

The durable Fillet/Chamfer Feature continues to store exactly the semantic Edge set explicitly selected by the user. That set must remain deterministic, stage-scoped and free of nearest/similar/provider-order rebinding.

After that exact input set is accepted, the provider may modify local result topology required to realize the request. At a selected Edge endpoint or a 3/4/5-edge junction it may trim/split neighboring Edges, replace a vertex with several vertices, split adjacent Faces and create local corner/transition patches. Those result entities do **not** become authored inputs.

PM-05A tangent evidence is an important boundary: on the current OCCT path, one tangent-chain seed can expand the provider input contour and generate the same result as explicitly selecting the complete chain. That is operation expansion, not mere local corner accommodation. Current production therefore keeps exact provider input-contour membership and does **not** silently Fillet/Chamfer un-authored tangent neighbors.

Automatic tangent-chain/loop authoring remains excluded. Unrelated disconnected expansion remains invalid. Exact whole-candidate preview and complete semantic result accounting remain mandatory.

## Reference review

Two reference implementations informed the amendment:

1. the Owner's earlier SimpleSolid/RepliCAD project passes the explicitly selected Edge set/parameter into a comparatively thin Fillet/Chamfer provider layer and lets the underlying modeling kernel solve blend/chamfer corners; its durable topology resolver is intentionally **not** adopted because it is more geometry-heuristic than current SimpleSolid2 semantics;
2. FreeCAD PartDesign registers each user-selected Edge into one `BRepFilletAPI_MakeFillet` / `BRepFilletAPI_MakeChamfer` operation and does not require the native provider contour to be set-equal to the UI selection. OCCT itself exposes contour construction as internal operation state.

The useful reference principle is separation of authored seeds from provider execution contour, not copying another project's topology identity model.

## Existing evidence gap

PM-05A direct OCCT evidence already proves successful native Fillet Build for:

- single box Edge;
- disconnected pair;
- adjacent pair;
- trihedral three-Edge corner;
- closed loop;
- mixed connected/disconnected set.

PM-05C2b production Part topology evidence proves the trihedral three-Edge Fillet and trihedral Chamfer, but does **not** prove the production adjacent two-Edge Fillet. This matches the Owner symptom where two adjacent Edges fail while adding the third incident Edge can recover.

Therefore the first remediation target is the production pipeline, not the raw OCCT capability.

## Diagnostic plan

Focused evidence must classify the same cube selections through all stages:

1. one Edge;
2. two adjacent Edges;
3. three Edges at one trihedral vertex;
4. a four-Edge loop/mixed set where geometrically valid.

For Fillet and control Chamfer, capture:

- strict semantic MaterialEdgeReference resolution;
- resolved runtime seed tokens;
- provider contour count/membership before Build;
- whether provider contour equals/contains/exceeds the authored seeds;
- OCCT Build success/failure and native fault diagnostics where available;
- single-solid / BRep validity;
- generated Surface/corner history;
- semantic topology publication success/failure and diagnostic code.

A second fixture should represent the Owner's complex-part class: mixed straight/curved/generated engineering Edges, including a Chamfer -> multi-Edge Fillet sequence analogous to the earlier project.

## Root cause A — adjacent multi-Edge Fillet publication

Focused cube matrix evidence on candidate `588e523b80593490208ece2425822d8f71064c58` reproduces the Owner symptom deterministically:

- all 66 two-Edge Chamfer sets: product PASS;
- 42/66 two-Edge Fillet sets: PASS;
- all 24 geometrically adjacent two-Edge Fillet sets: provider PASS but product FAIL;
- all 8 trihedral three-Edge Fillet sets: product PASS;
- every failing adjacent pair has exact provider input membership and one generated edge-feature boundary with provider `CurveKind::other`;
- failure occurs after successful provider execution as Part `topology_integrity_failure`.

The Part curve-stage classifier previously treated `CurveKind::other` adjacent to a Fillet/Chamfer generated Surface as a provider contradiction. That policy is too strong. A local transition spline is a legitimate result boundary even when PM-05 v1 has no durable Curve vocabulary for authoring it later.

Bounded remediation: keep the Edge in complete topology accounting as `semantically_unsupported` / non-authorable, while allowing the valid Body/Feature to remain UpToDate. No heuristic identity is invented.

## Root cause B — curved Boolean Edge authoring and untouched-Edge continuation

The mixed Line/Circle capsule-Cut fixture reproduced the Owner's complex-geometry symptom before Fillet execution: the Cut Body contained the expected curved material boundaries, but the Part Curve classifier published only the line Boolean intersections as authorable while four plane-cylinder circular boundaries were fully accounted yet semantically unsupported.

Bounded remediation admits analytic cross-producer Boolean intersections as durable Curve meaning when identity is still the exact semantic Surface-pair relation and the provider classifies the current realization as line or circle. No XYZ, length, nearest candidate or traversal ordinal is introduced. Multiple bounded realizations of the same semantic relation remain Ambiguous; CurveKind::other remains non-authorable.

The old-project analogue then exposed a second lineage gap after an unrelated exterior Chamfer: OCCT omitted Edge history for untouched distant capsule boundaries, so the same durable Cut Curve meanings could not be re-authored at the adjacent result stage. The remediation recovers continuation only when provider lineage is completely empty and the exact already-published CurveRelation (role + analytic CurveKind + canonical semantic Surface pair) exists in the adjacent stage. If OCCT publishes any descendant, provider history remains authoritative.

Final remediation evidence on exact candidate `bc0b002f4e45172aec24c61913cd28ac8ca43138` / Windows FULL #1739:

- core-only: 25/25 PASS;
- kernel-native: 57/57 PASS;
- desktop: 112/112 PASS;
- `pm05f.fillet_corner_diagnostics`: PASS;
- `pm05f.fillet_complex_fixture`: PASS;
- `pm05f.fillet_curved_edge`: PASS;
- final `windows-msvc` aggregate: PASS.

The complex fixture covers a mixed 2-line + 2-circle capsule Cut loop, direct four-Edge Fillet, unrelated exterior Chamfer, exact semantic re-authoring of the same Cut Curve meanings at the next stage and the chained four-Edge Fillet. The curved-edge fixture separately proves a single circular Boolean Edge for both Fillet and Chamfer with exact provider input membership.

Documentation/Product Browser synchronization passed Windows DOCS #1741. Candidate `219e49e6a94dae5d43d807a147261c2c040084ab` then hardened the complex fixture to prove that, after the unrelated exterior Chamfer, the exact four continued capsule Curve meanings remain **2 lines + 2 circles**, each Resolved with one strict current Edge; Windows FAST #1742 PASS.

Final governance-synchronized candidate `e1c358cf54fd051f15252ea198d116de0b6a9cdc` passed Windows FULL #1744 with core-only 25/25, kernel-native 57/57 and desktop 112/112; final aggregate PASS. PR #296 merged that exact tree to `main` as `f5570b3bcde458670943718e78cdf4a7a77667ea`. No further automated/runtime remediation gate remains; repeat Owner Windows acceptance is pending.

## Remediation decision result

The Owner blocker did **not** require weakening the tangent-contour input guard and did **not** require generic healing/fuzzy fallback.

- ordinary adjacent cube Fillet: provider Build already succeeded; semantic publication was fixed;
- local transition-spline result boundary: fully accounted but intentionally non-authorable;
- curved plane-cylinder Boolean boundary: admitted through semantic Surface-pair + analytic line/circle relation;
- untouched distant Edge after local Chamfer: adjacent-stage continuation recovered only under empty provider lineage plus exact semantic CurveRelation;
- tangent-chain provider input propagation remains rejected unless the full affected chain is explicitly authored.

No generic healing, fuzzy escalation, nearest-geometry rebinding or advanced Fillet variant is authorized by this remediation.

## Completion gate

Automated remediation completion criteria are satisfied:

- focused diagnostics proved both root causes;
- bounded regressions cover adjacent-pair/trihedral cube Fillet with Chamfer control;
- mixed/generated and isolated curved-Edge fixtures pass;
- canonical docs/Product Browser are synchronized;
- exact-head Windows FULL #1744 passed and the candidate merged as #296.

The remaining PM-05F gate is Owner repetition of the supported-Windows acceptance workflow with an explicit PASS.
