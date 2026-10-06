# PM-04C — Revolve Semantic / Kernel / Topology Completion

**Status:** COMPLETED — PASS  
**Parent Work Contract:** `work/PM-04_AXIS_REVOLVE.md`  
**Checkpoint split:** C1 provider-neutral semantics/evaluator + C2 production OCCT Revolve/topology  
**Date:** 2026-10-06

## Runtime evidence

### PM-04C1 — provider-neutral Revolve semantics / evaluator

- exact candidate: `57cb332f2c6e8e3032a3647df3c8457d4d1583a3`;
- Windows FULL #1547: PASS;
- merged main: `704879e90aa56f8338164d1ccdb918d122d0e03b` (#264).

C1 delivered:

- authored `RevolveFeature` in ordered Part feature history;
- Add/Cut;
- OneSide with Reverse and Midplane total-angle semantics;
- `0 < Angle <= 360°`;
- durable `ProfileId + AxisReference` with Origin/Authored AxisReference;
- Axis high-water / never-allocated-ID validation;
- bounded Axis source Body-stage dependency floor;
- provider-neutral `Axis3 + AngularRevolveInput`;
- coplanarity and closed-half-plane admission;
- distinct MissingAxis / axis-not-in-profile-plane / profile-crosses-axis diagnostics;
- ordered evaluator dispatch through the existing Body-stage pipeline;
- partial start/end and side semantic topology roles;
- full-turn semantic contract with no authored start/end cap;
- fake-provider topology regression including periodic-seam representation semantics.

### PM-04C2 — production OCCT Revolve / topology lineage

- exact candidate: `6e627749b850524c0dc3730ed19c7f9506b37536`;
- Windows FULL #1549: PASS;
- final aggregate `windows-msvc`: PASS;
- merged main: `70d1b5986fa87d736626afbe858a6c096eabd36d` (#265).

C2 delivered:

- production `OcctSolidModelingKernel::revolve()`;
- partial signed angular sweep and canonical full-turn OCCT revolution;
- deterministic rotated Profile frames for partial start/end caps;
- deterministic planar carrier frame for Revolve-generated planar side Surfaces;
- truthful provider Surface kind: Plane/Cylinder/Cone/Sphere/Torus/Other;
- boundary-use provenance for generated side identity;
- explicit axis-contact collapse as Missing side claim rather than invented material topology;
- full-turn periodic seams accounted through the existing PM-02 `periodic_seam` observation;
- seam remains non-referenceable representation artifact at Part level;
- Revolve Add/Cut reuse the accepted Boolean/lineage implementation with fuzzy = 0;
- inherited Face/Surface/Edge/Vertex lineage remains the existing exact OCCT operation-history path;
- ADR-0017 Add Surface continuation remains the single continuation mechanism;
- complete runtime Face/Edge/Vertex inventory and semantic catalog.

## Verification

Windows FULL #1547 on C1 and #1549 on C2 passed the exact candidate heads.

Dedicated regressions:

- `pm04c1.revolve_semantic_kernel` proves authored semantics, admission, AxisReference behavior, ordered evaluator dispatch, topology role meaning, full-turn seam semantics, Axis high-water and stage-cycle guards;
- `pm04c2.production_revolve` proves production OCCT partial/full Revolve, Reverse cap identity, carrier kinds, axis-contact collapse, Add/Cut lineage, complete topology accounting and real Part periodic-seam representation-artifact classification.

PM-00A E06 remains valid supporting evidence for cold full-rotation semantic repeatability and provider seam non-identity.

## Architecture boundary

PM-04C does **not** add:

- Revolve GUI / Operations panel;
- Revolve preview;
- Revolve lifecycle draft;
- Revolve Create/Edit/Delete application commands;
- Revolve persistence schema;
- Datum Axis;
- Body Edge/Curve AxisReference;
- Projection.

Those remain owned by PM-04D/E or later separately activated work.

## Result

PM-04C is **COMPLETED — PASS**.

The next authorized checkpoint is **PM-04D — Revolve draft / Operations / Command Line / preview** under the unchanged Owner-accepted PM-04 Work Contract.
