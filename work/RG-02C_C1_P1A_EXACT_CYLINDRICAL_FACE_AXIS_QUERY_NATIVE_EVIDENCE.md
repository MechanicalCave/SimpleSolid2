# RG-02C/C1-P1-A — exact analytic cylindrical Face-axis native evidence

**Status:** implementation candidate, **NOT YET RUN ON REAL WINDOWS**. No P1-A technical PASS or protected-main production merge is claimed.

## Bounded D2 and actual implementation

Owner accepted the exact P1-A technical D2 Work Contract in `work/RG-02C_C1_P1A_EXACT_NATIVE_CYLINDRICAL_FACE_AXIS_QUERY_D2_WORK_CONTRACT_PROPOSAL.md`; it was guarded-squash merged [PR #431](https://github.com/MechanicalCave/SimpleSolid2/pull/431) as `3ec660ae`. The separate [ACTIVE-only PR #432](https://github.com/MechanicalCave/SimpleSolid2/pull/432) was independently Owner-authorized, exact-head Windows CLOSURE #3334 SUCCESS, and guarded-squash merged as `5215817969d1eb83ab9e17a9324a429c9034d637`.

Implementation is strictly six paths: new provider-neutral `src/kernel/include/simplesolid2/kernel/exact_cylindrical_face_axis.hpp`; existing `src/kernel_occt/include/simplesolid2/kernel_occt/solid_modeling_kernel.hpp` and `src/kernel_occt/solid_modeling_kernel.cpp`; additive already-registered `tests/kernel_native/pm04c2_production_revolve_test.cpp`; current-as-built internal `docs/internal/PART_EDGE_PROJECTION_KERNEL.md`; this evidence file. No CMake target, build workflow, persisted Axis, Part reference source, user-facing UI or native schema changes.

The native provider binds an opaque `ScopedExactCylindricalFace` to one current `RuntimeSolidHandle` and real Face inventory token, then queries the **actual mapped** `TopoDS_Face` through `BRepAdaptor_Surface(face,true)`. Only `GeomAbs_Cylinder` returns the world-mm infinite **unoriented** line (axis point + unit representative vector) and true analytic radius. Mismatched Body pointer, absent Face or non-cylinder and invalid numerical geometry refuse with typed empty results, never a cached approximation.

## P1-A native witness plan and truthful limits

- Reuse existing actual Part full Revolve, partial Revolve (90 degrees), circular-profile Extrude and half-cylinder Boolean Cut; check Face-derived native cylinder axis **parallel modulo sign**, shortest distance from expected axis and native analytic radius within fixture tolerance. All should include a refusal for actual noncylindrical faces. Face input is current Body inventory, not any Feature generator parameters.
- Independently re-evaluate an identical Part to create a different current Body object; a Face bound to first Body MUST fail `provider_mismatch` with the second, even if their numeric tokens coincide. Check null Body / null token / absent token binder failures and default unbound Face query failure; fake `RuntimeSolid` from another provider type cannot bind.
- Observe actual circular-rim Fillet and Chamfer Faces without promoting Fillet's operation name to `GeomAbs_Cylinder`. **Straight-edge Fillet positive** is a desired native witness but remains **GAP/NOT TESTED** unless an actual suitable fixture is constructed and succeeds in the approved single test file. A circular-rim torus is never evidence of a fillet-generated cylinder.
- **Not independently executed until proven:** native `TopoDS_Face` topological orientation reversal, OCCT exception/fault-injection, degenerate OCCT cylinder, in-inventory deletion of an already bound Face within immutable runtime, separate toroidal/conical negative fixture if not present. No production test-only backdoors are permitted. Report any gaps truthfully, not as successes.
- No Part-level authored source, persistent `SurfaceReference`, feature Axis, Revolve Reverse semantics, UI or P1-B partitioned-carrier authoring is implemented or tested. Provider scope is evaluation-local geometry only.

## Actual Windows gates (do not invent PASS)

| Gate | Exact tested SHA / run | Result |
| --- | --- | --- |
| Native FOCUSED `pm04c2.production_revolve` | Pending | NOT RUN |
| Independent Draft FAST | Pending | NOT RUN |
| Ready FULL Core, native Kernel/OCCT and Qt/OCCT Desktop | Pending | NOT RUN |
| Official DOC-01 internal as-built | Pending | NOT RUN |
| Evidence-only final exact-head CLOSURE | Pending | NOT RUN |
| Owner P1-A technical FINAL PASS | Not yet requested | PENDING |
| Independently Owner-authorized production squash | Not requested | NOT AUTHORIZED |

Current proposal/activation governance CI checks are **not** native P1-A execution evidence. Failed real runs must remain visible in the acceptance ledger, including first RED and gaps.

## Documentation impact

Internal docs: required
User/Product docs: not required
Reason: This change adds a provider-neutral native Kernel API with evaluation-local provenance/typed failure semantics; the approved internal as-built kernel document is updated in the same bounded branch. No user-facing feature, Part Axis authoring or persistent schema changes in P1-A.
