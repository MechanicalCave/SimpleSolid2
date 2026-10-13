# RG-02B1/E0 — Native Edge + Point proof, measured evidence (PARTIAL)

**Status:** **PARTIAL / E0 NOT CLOSED; no Owner D2 conclusion or final native FULL claimed.** The first positive fixtures ran on real OCCT, with exact-source Windows FOCUSED PASS. Unsupported general END and provider-neutral 3D MID/tangent remain unclosed.  
**Authority:** Owner-accepted four-test Work Contract `work/RG-02B1_E0_NATIVE_EDGE_POINT_PROOF_WORK_CONTRACT_DRAFT.md`, separately activated by [PR #347](https://github.com/MechanicalCave/SimpleSolid2/pull/347) squash on `main` `b46bbe69b59d660f089967cba9b9f2e9a10ac029`. Accepted Part roadmap v1.33.  
**Current evidence branch:** [Draft PR #348](https://github.com/MechanicalCave/SimpleSolid2/pull/348) `evidence/rg02b1-e0-native-end-incidence-20261013`. Source-only commits used for the initial native checks: `b0719f46c3f59d040a364d5c792c299208871aa2` and `a15389aef684f05f2849d14b46ee33096a7e0c05`.  
**Only source file modified to date:** `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp`, existing registered native target `pm05c2b_part_edge_topology_test`, exact CTest `pm05c2b.part_edge_topology`. All new assertions are additive; no prior G0 or PM-05 checks weakened.  
**Documentation Impact:** This is an evidence-only work report and no production behavior changes. Internal docs: not required; User/Product docs: not required; Browser: not required.

## 1. Exact-head execution ledger — **ACTUAL**

| Source SHA | Windows Actions | Specific job/CTest | Result | Evidence scope |
| --- | --- | --- | --- | --- |
| `b0719f46c3f59d040a364d5c792c299208871aa2` | [#2822](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38391517253) | `windows-msvc-focused` / `pm05c2b.part_edge_topology`, **1/1**, aggregate `windows-msvc` | **PASS** | E0-01 strict 12 material Line Edges, 24 independently Resolved END uses, 8 shared trihedral corners; same semantic Edge→Point pairs through rebuilt real OCCT provider and base height edit |
| `a15389aef684f05f2849d14b46ee33096a7e0c05` | [#2823](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38391789242) | `windows-msvc-focused` / `pm05c2b.part_edge_topology`, **1/1**, aggregate `windows-msvc` | **PASS** | Same E0-01/02/07 assertions plus E0-05 **72** same-stage independently certified straight Edge + noncollinear Point candidate pairs **per fixture** |

`classify` and the aggregate Windows gate returned success on both exact SHAs. FAST, FULL, docs and closure jobs were **SKIPPED**, not PASS, as expected for these focused test-only commits. Neither action run is a final E0 native FULL. CTest's default successful output suppresses the custom stdout counters; **the exact asserted numerical counts are source-checked and the named compiled test passed**, rather than pretending the counters appeared in CI logs.

## 2. Exact positive fixtures and engineering claims

**E0-01, canonical real extruded box (BOUNDED POSITIVE):**
- Existing real Part document creates a 40×30 mm rectangle and extrudes 20 mm. Production `part::evaluatePart()` calls the real `kernel_occt::OcctSolidModelingKernel`.
- At the actual declared after-Extrude `BodyStageRef`, there are **12** current straight material Edge records and **8** current Vertex records. Every Edge is `referenceable` and `Resolved`, `authorMaterialEdgeReference()` yields a valid `MaterialEdgeReference`, and `resolveMaterialEdgeReference()` returns exactly that one current Edge in that same stage.
- Each Edge has **exactly two separately pre-Resolved** `FeaturePointAddress` witnesses, discovered through current real Vertex incidence *only after* checking the Point is individually `Resolved` with a unique one-to-one catalog realization. Each of the 24 endpoint uses is an independently certified three-Surface Point; both END addresses differ. Runtime incidence, provider tokens and provider XYZ are **not** persisted reference identities.
- Each of the 8 individually Resolved corner Points participates in exactly three separately strict semantic Edges. This is one-Point/many-Edge sharing, **not** evidence that two distinct semantic Point addresses overlapping one screen pixel can be chosen unambiguously.

**E0-05, separately certified point off the infinite straight line (BOUNDED GEOMETRIC FEASIBILITY):**
- For each of 12 already-strict straight Edge source meanings, compare with the six other independently certified same-stage Point identities. Only *after* independently locating semantic witnesses, use current diagnostic provider coordinates for cross-product/noncollinearity with that Edge's infinite supporting line.
- Assertions require finite nonzero Edge length and strictly noncollinear result for **72** Edge+off-line Point pairs per fixture, including initial current Body stage, independent OCCT re-evaluation and upstream Extrude height edit. Geometric classification is diagnostic/current-stage-only; **no XYZ or body token is authored or serialized**.
- E0-05 does **not** construct, orient, persist, edit or visually select a Datum Plane, and does not prove any arbitrary free XYZ point.

**E0-07, stage/new-provider/dimension edit (PARTIAL):**
- The same authored Part document/state is independently evaluated by a new OCCT kernel instance; the base Extrude height is separately edited from 20 to 35 mm. The entire set of **(semantic MaterialEdgeReference, sorted two semantic FeaturePointAddress endpoints)** is required equal in all three real evaluated catalogs. Sorting uses strict semantic IDs, never provider iteration order.
- This does **not** test a newly persisted construction Point/Datum schema, completed Save/Close/Reopen of an Edge+Point Datum, UI revision freshness, or every deletion/suppression case. Earlier G0 G0/V07/V09/V11 tests remain background—not newly implemented E0 Datum lifecycle.

## 3. Per-gate status (never infer total E0 PASS)

| E0 | Status on this source | Proven versus still needed |
| --- | --- | --- |
| **01 — straight END** | **BOUNDED POSITIVE / focused PASS** | 12 strict Line Edges, 24 independently Resolved endpoint uses, stage correctness, real OCCT + fresh provider + height edit; **NOT** all shape families and **NOT** an authored Edge-local END selector |
| **02 — shared corner / overlap** | **PARTIAL** | Eight resolved trihedral Points each on 3 strict Edges. Overlapping **distinct** semantic Point meanings and real Viewer point-first candidate-stack filtering **NOT TESTED**. The accepted prior `pm05b2.strict_edge_resolver` negative ambiguity guard is not yet separately re-executed for E0 |
| **03 — bounded Line MID** | **UNRUN / API GAP OPEN** | Current strict Line class evidenced. A stage-bound provider-neutral exact 3D metric half-length MID+oriented tangent query and durable MID role are **NOT PROVEN**. Do not substitute tessellated midpoint or profile coordinates |
| **04 — open Arc END/MID** | **UNRUN / API GAP OPEN** | Existing real native 2D exact Arc projection controls are historical only; E0 has **NOT** proven stage-bound exact 3D current material open-arc half-length and tangent, including negative sweep. Full circle MID remains excluded |
| **05 — off-line Point** | **BOUNDED POSITIVE / focused PASS** | 72 uniquely pre-Resolved independent Point candidates against 12 Line Edges, current same stage, measured noncollinearity. Does **NOT** prove durable Datum source / plane construction |
| **06 — difficult negative topologies** | **UNRUN AS E0** | Historical accepted G0 has real Fillet/Chamfer Unsupported, full Revolve Unsupported and half-cylinder Ambiguous cases. Reuse for future E0 negative characterization; do not promote historical runs to new E0 PASS |
| **07 — history / cold** | **PARTIAL** | Real new provider plus height edit preserve Edge/END semantic pairs; source deletion, full Datum persistence, UI stale and arbitrary native orientation **NOT TESTED** |

## 4. Stop conditions / next D2 decision

The observed canonical positive End/Point identity is encouraging but cannot justify global END support. Existing `FeaturePointAddress` is bounded to three-Surface intersections; G0 already proves valid native Vertices may be Unsupported or intrinsically Ambiguous. A strict material Edge never upgrades an Ambiguous Point through its incident Edge list.

The eventual Datum Plane `Edge + Point` product will require an exact **typed durable point-source role** for eligible END/MID, an evidenced provider-neutral 3D analytic query with a stable plane frame/Reverse rule, versioned compatible Datum serialization and separately approved complete UI. Those are **Owner D2/D3 STOPs**: **no `src/**`, public Kernel API, schema, Viewer/UI, standalone Datum Point or G1 work is authorized by this evidence-only contract.**

**Next E0 evidence actions within existing allowlist only:** characterize negative real Vertex/Edge source interactions using `pm04c2.production_revolve` and `pm05c2b.part_edge_topology` as needed, and retain `pm05b2.strict_edge_resolver` failure invariant. Explicitly record a blocked 3D query where already-registered tests cannot substantiate it. Broader Windows FAST and final Ready-for-Review exact-head FULL and independent Owner evidence acceptance are **PENDING**, not to be inferred from #2822/#2823. No PR merge permission has been requested or granted.
