# RG-02C/C1-P1-A — exact analytic cylindrical Face-axis native evidence

**Status: BLOCKED — FIRST WINDOWS DRAFT FAST #3335 RED (DOC-01), P1-A NATIVE TESTS NOT EXECUTED.** No P1-A technical PASS or protected-main production merge is claimed. **Owner D2 additional-file decision required** because the accepted six-path implementation allowlist excludes generated `docs/browser/index.html`, but DOC-01 requires it after editing the approved internal as-built document.

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

## First RED and exact bounded-contract blocker (2026-10-16)

At source/test/docs/evidence HEAD `4253cf218a0602932dc7f787d6fd5484a0186a50`, real Windows [PR #433 DRAFT FAST #3335](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38893894889) **FAIL** in official DOC-01 verification, **before** C++ compilation or native test execution. The failing `windows-msvc-fast` log says: **`[BROWSER] docs/browser/index.html is stale. Run '.\\ss2.ps1 docs' and commit the generated file.`** The generated Browser encodes canonical documentation text including `docs/internal/*.md`: `scripts/docs/ss2-docs-core.ps1::Get-CanonicalDocPaths` enumerates `docs/internal`, and `New-BrowserHtml` embeds that markdown; `scripts/docs/ss2-docs-validation.ps1::Test-BrowserFreshness` compares `docs/browser/index.html` to the exact generated result. This is a **real, deterministic DOC-01 failure**, not a C1-P1-A query compile failure or native geometric FAIL.

The accepted exact P1-A D2 six-path allowlist *requires* `docs/internal/PART_EDGE_PROJECTION_KERNEL.md` be updated (`Internal docs: required`) but **does not include** `docs/browser/index.html` and forbids additional docs paths without fresh Owner amendment. Therefore there is currently **no policy-compliant way to PASS Windows**. No script weakening, bypass, exclusion of internal source from Browser, rollback of required internal docs or creation of an out-of-scope generated file is authorized. **Fail-closed STOP** for Owner to decide the minimal D2 addendum: seventh path `docs/browser/index.html`, **generated only** via existing canonical generator, with Browser parity DOC-01 and otherwise unchanged original six files. Such a separate governance addendum requires Owner acceptance, guarded governance merge, and independent ACTIVE-only amendment/activation before actual production branch may edit that seventh path. No P1-A code compile/native test result may be inferred from #3335.

## Actual Windows gates (do not invent PASS)

| Gate | Exact tested SHA / run | Result |
| --- | --- | --- |
| First Windows DRAFT FAST / DOC-01 [#3335](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38893894889) | `4253cf218a0602932dc7f787d6fd5484a0186a50` | **RED**, Browser stale; C++ / native test not executed |\n| Native FOCUSED `pm04c2.production_revolve` | Pending | NOT RUN |
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
