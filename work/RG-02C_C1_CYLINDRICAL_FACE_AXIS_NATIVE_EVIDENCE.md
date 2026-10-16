# RG-02C/C1-P0 — native cylindrical Face evidence

**Status:** IMPLEMENTATION CANDIDATE; Windows tests NOT YET RUN. No technical P0 PASS is claimed.

## Scope

Read-only, additive assertions in existing registered native OCCT test `tests/kernel_native/pm04c2_production_revolve_test.cpp`, per Owner-accepted C1-P0 contract and activated `work/ACTIVE.yaml`. Reuse actual resulting single-Body circular Extrude, full and partial Revolve, half-cylinder Boolean Cut and circular-rim Fillet/Chamfer fixtures. Observe actual stage-scoped current Face-to-semantic Surface candidates, `SurfaceKind::cylinder`, other Face kinds, carrier partitions and unsupported/nonunique candidates. Never derive an Axis from operation type or runtime tokens.

**Not proven:** exact cylinder infinite-axis origin/direction/radius, cross-topology source survival, native provider query, persistence, Axis construction, UI, or a positive cylindrical *straight-edge* Fillet if not represented by the reused fixture. Classifying a cylinder does not extract its axis. Circular-edge Fillet may be toroidal; report actual observed type, never infer from operation name.

## Validation

- Native Windows FOCUSED: PENDING.
- Windows FAST: PENDING.
- Windows Ready FULL Core+Kernel+Desktop: PENDING.
- Exact-head evidence CLOSURE: PENDING.
- Owner P0 technical FINAL PASS: NOT REQUESTED.

Preserve first RED and GAP evidence. Only promote any status after checking exact CI logs on the corresponding SHA.

## Documentation impact

Internal docs: not required
User/Product docs: not required
Reason: This P0 test and work-only evidence report do not change shipping product semantics or UI. A later C1 product feature requires new DOC-01 work under an independent Owner contract.
