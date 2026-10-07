# PM-05F — Fillet Local-Topology Remediation

**Status:** ACTIVE — Owner acceptance blocker
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

> **Strict authored intent, permissive local topology.**

The durable Fillet/Chamfer Feature continues to store exactly the semantic Edge set explicitly selected by the user. That set must remain deterministic, stage-scoped and free of nearest/similar/provider-order rebinding.

However, the provider is allowed to modify local topology required to realize that request. At a selected Edge endpoint or a 3/4/5-edge junction it may trim/split neighboring Edges, replace a vertex with several vertices, split adjacent Faces, create corner patches, or internally use extra contour members when that is a local construction consequence of the requested operation.

Those transient participants do **not** become authored inputs.

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

## Remediation decision rule

- **Provider Build succeeds, semantic publication fails:** fix Part/topology publication; do not change provider construction policy unnecessarily.
- **Current code rejects before Build only because provider contour is not set-equal:** replace exact-equality gating with the Owner-amended seed-vs-local-accommodation policy.
- **OCCT Build itself fails on ordinary cube adjacent inputs:** inspect native contour/corner fault evidence and provider setup before considering any higher-level fallback.
- **Complex case needs local extra contour members but yields valid accounted geometry:** allow them transiently; do not persist them as Feature inputs.

No generic healing, fuzzy escalation, nearest-geometry rebinding or advanced Fillet variant is authorized by this remediation.

## Completion gate

Remediation is complete only after:

- focused diagnostics prove the root cause;
- bounded fix has regression coverage for at least adjacent-pair and trihedral cube Fillet plus Chamfer control;
- complex mixed/generated Edge fixture is covered;
- exact-head Windows FULL passes;
- canonical docs/Product Browser are updated if the as-built provider-contour semantics changed;
- Owner repeats the supported-Windows acceptance workflow and explicitly reports PASS.
