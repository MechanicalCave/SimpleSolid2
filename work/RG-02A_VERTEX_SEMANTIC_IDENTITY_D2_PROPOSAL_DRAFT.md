# RG-02A — Semantic Body VERTEX identity after native G0: D2 proposal (OWNER REVIEW DRAFT)

**Date:** 2026-10-12  
**Status:** **OWNER-ACCEPTED BOUNDED G0 EVIDENCE (WITH TWO EXPLICIT GAPS); UNACCEPTED D2 ARCHITECTURE PROPOSAL / NO IMPLEMENTATION AUTHORITY**. In response to the exact question whether to accept G0 findings and both gaps as closure of the evidence-only phase, Owner said **`akceptuję - kontynuuj` on 2026-10-12**. This accepts the measured G0 characterization *as bounded evidence*, **not** a universal durable Vertex identity, adoption of D2 Option B, a G1 contract/activation, production/schema code or RG-02 UI/UX. The Owner subsequently granted **separate merge approval** for evidence PR #332 (`wyrażam - kontynuuj`), and the exact-tested PR was squash-merged as `5b495c4bbed8754d7a1f01e87a30716429db786d`; this is evidence consolidation only, not D2 acceptance. A prior generic "kontynuuj" had authorized preparation of review material only.  
**Source of measured claims:** [G0 evidence PR #332](https://github.com/MechanicalCave/SimpleSolid2/pull/332), source HEAD `da195b486f385bf9c260a542e4624c1923d55b7a`. Its report is [`work/RG-02A_G0_NATIVE_VERTEX_EVIDENCE.md`](https://github.com/MechanicalCave/SimpleSolid2/blob/da195b486f385bf9c260a542e4624c1923d55b7a/work/RG-02A_G0_NATIVE_VERTEX_EVIDENCE.md) and detailed executed matrix is [`work/RG-02A_VERTEX_NATIVE_G0_MATRIX_DRAFT.md`](https://github.com/MechanicalCave/SimpleSolid2/blob/da195b486f385bf9c260a542e4624c1923d55b7a/work/RG-02A_VERTEX_NATIVE_G0_MATRIX_DRAFT.md). This evidence is **MERGED on `main`** via the Owner-authorized [PR #332](https://github.com/MechanicalCave/SimpleSolid2/pull/332) squash commit `5b495c4bbed8754d7a1f01e87a30716429db786d`. The source SHA above remains the immutable exact-head CI reference. The original report still contains historical in-progress wording; the Owner acceptance and verified merge described here supersede that historical governance checkpoint **without expanding any of the documented evidence boundaries**.  
**Windows verification:** exact HEAD FAST [#38310478851](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38310478851) 93/93 PASS; exact HEAD FULL [#38310694444](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38310694444) PASS (core-only 25/25, kernel-native 57/57, desktop 117/117; 199/199); docs / aggregate PASS. Measurement diagnostic RED [#38309695097](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38309695097), numerical native GREEN [#38310021876](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38310021876) 2/2 PASS. No new execution is claimed in this D2 document.  
**Governing boundaries:** `governance/CONSTITUTION.md`, `work/ACTIVE.yaml` (still **RG-02A/G0 evidence ONLY**), roadmap v1.32; parent `work/RG-02_REFERENCE_GEOMETRY_EXTENSION_WORK_CONTRACT_DRAFT.md`, `work/RG-02A_REFERENCE_FOUNDATION_AUDIT_DRAFT.md`; accepted stage-aware Part semantics.

## 1. Exact native evidence motivating D2

The current `FeaturePointAddress` is a semantic set of **exactly three distinct `FeatureSurfaceAddress` values**, with a producer Feature. It is **not** a general persistent Body Vertex identity. Its native `FeaturePointResolution` can still be **Ambiguous** when more than one current material Vertex realizes the same triple.

| Real OCCT Part fixture from G0 | Current native Vertices | Resolved | Ambiguous | Unsupported | Consequence |
| --- | ---: | ---: | ---: | ---: | --- |
| Rectangular Extrude | 8 | 8 | 0 | 0 | Existing triple-Surface identity works |
| Extrude + interior Cut | 16 | 16 | 0 | 0 | Works on this Cut: 8 inherited + 8 new semantic Points |
| Fillet corner | 13 | 10 | 0 | 3 | Three material Vertices lack certifiable Point identity |
| Chamfer corner | 13 | 10 | 0 | 3 | Same gap, independently observed |
| 90-degree Revolve | 6 | 6 | 0 | 0 | Some curved-feature Vertices already work |
| Full 360-degree Revolve | 2 | 0 | 0 | 2 | Both current Part Vertices unsupported; native seams may be representation artifacts |
| Circular Extrude cylinder | 2 | 0 | 0 | 2 | Both current Part Vertices unsupported, though ordinary category pick policy may admit them |
| Analytic half-cylinder Cut | 6 | 0 | 4 | 2 | Two three-Surface Point relations each have multiple current Vertex realizations: **Ambiguous**, not a unique source |

All omitted current-Vertex statuses in this table (Missing) are zero for these fixtures. **Important distinction:** in a separate removed-corner Cut, all **12 current Vertices** are Resolved but **2 old semantic Point source addresses become Missing**. Do not collapse a lineage-reference status into current native Vertex accounting. Provider-side raw Revolve inventory is also distinct from Part-stage inventory (90 degrees: 8 native provider Vertices, all 3 raw adjacent Surfaces, versus 6 Part Resolved; 360 degrees: 4 raw Vertices each with 2 Surfaces versus 2 Part Unsupported).

**Evidence-quality STOP:** the raw native-vs-*resolved semantic* Surface adjacency/producer source for the three Unsupported Fillet/Chamfer Vertices is not yet measured. G0 V11 shows in-memory Part reconstruction and independent kernel regeneration preserve eight semantic addresses, but does not prove stale UI selection rejection or future saved RG-02 point-source persistence. Do not invent either fact.

## 2. Proposed D2 principles for explicit Owner review

### D2-V1 — Preserve existing canonical three-Surface Point references (proposed)

Keep the existing `FeaturePointAddress` meaning, producer provenance, after-Feature `BodyStageRef`, strict unique `Resolved` requirement and typed `Missing/Ambiguous/Unsupported` failure. No change of meaning, reserialization of old data, or silent migration. It remains the preferred certified identity whenever it resolves to **exactly one** current material Vertex. A unique present coordinate alone is not a semantic witness.

### D2-V2 — Permit a future bounded Vertex discriminator ONLY after evidence (proposed conditional investigation, not a schema choice)

For a real native Vertex not certified by D2-V1, investigate whether one can construct **unique, persistent, provider-neutral semantic provenance** from already authored stage data. Potential witness inputs to audit, **not** approved field definitions:

- existing semantic producing Feature + **specific source Sketch entity / cap / side / transition role**;
- bounded *strictly authored* `MaterialEdgeReference` and its semantic incident curve/surface lineage, when the Edge can itself be resolved without using the Vertex candidate being identified;
- explicit source point or transition provenance already carried by a Fillet/Chamfer Feature, when demonstrated native uniqueness and stability across edits/trim/splits;
- oriented bounded branch semantics only if an independent semantic ordering/orientation exists; **never** provider iteration order, first/last `TopoDS_Vertex`, parametric seam side or screen-space pick order.

A candidate discriminator is admissible only if it has an **acyclic provenance proof**: it cannot define its own Vertex in terms of an Edge that needs that same unsupported Vertex to become referenceable. No fallback to `BRep_Tool::Pnt`, radii, length, nearest XYZ, display polyline, cached runtime token or a new arbitrary ordinal masquerading as stable ID.

If these evidence paths do not distinguish a pair of native Vertices, return **`Ambiguous`** and STOP; if insufficient evidence exists, return **`Unsupported`**. No promise that *every* ordinary-pickable Vertex becomes authorable.

### D2-V3 — Selection-to-authorship firewall, stage and lifecycle (proposed)

Future construction-point authoring must require the chosen **specific after-Feature stage** and a successful strict semantic witness at that exact stage. A runtime vertex hit may propose a candidate but may not become persisted intent. Recompute on edit / generation change / clean rebuild from owned semantics; no reuse of stale-last-good Body, wrong-stage picking, replacement Feature rebind or coordinate-based "repair." A missing old source remains authored but unresolved, allowing an explicit future Edit/Repair transaction without silently changing DatumId/AxisId.

The existing ordinary Viewer inspection pickability is **not** a promise of durable VERTEX source authorability. Whether and how a nonauthorable candidate is signaled to the user is entirely subject to the separate Owner UX design gate below.

### D2-V4 — Separate later typed derived-point and consumer design (NOT included in proposed Vertex D2 approval)

The previously accepted RG-02 product expectations remain: `VERTEX` from real certified Body Vertex; `MID` along exactly half the bounded straight/open circular material Edge length; `CENTER` analytic open/closed circular Edge center (possibly off the Edge); no full closed-circle `MID`. Edge-derived semantics would reuse strict `MaterialEdgeReference` and an exact provider-neutral 3D query, not an interpolated mesh segment or XYZ persistence. Datum Plane and Axis consumers share correct stage scopes. **This draft does not select a source schema, kernel API, migration version or implementation contract** for any of these features.

## 3. Candidate choices and consequences (NOT decided)

| Option | Architecture direction | Upside | Disadvantage / verdict |
| --- | --- | --- | --- |
| A | Retain current certified triple-Surface references; mark remaining Vertices Unsupported/Ambiguous | Zero new identity mechanism; safe existing documents | Leaves standard Fillet/Chamfer/cylinder cases unavailable. Safe fallback when no additional exact semantics can be proven |
| B (**recommended for bounded *investigation*, not implementation**) | Keep A for certified Vertices, and study per-family semantic source/incident-edge branch witnesses, each conditional on native G1 proof | Minimal targeted possible coverage; preserves legacy meaning and fail-closed behavior | Requires concrete source provenance/acyclicity/cold-rebuild evidence, compatibility and typed diagnostics before the Owner can approve a persisted schema |
| C | General universal VertexId, provider order, XYZ/nearest or cached token as persistent identity | Appears convenient | **REJECT** under constitutional semantic identity, rebuild and fail-closed requirements; ambiguous aliases/splits/removals would be silently misbound |

**Provisional recommendation:** accept Option **B only as a D2 investigation direction**, with A retained as safe fallback; **do not** preauthorize adding any selector kind, production code or persisted schema. The minimum observable gap is already proved. The exact answer for Unsupported Fillet/Chamfer needs further evidence; the half-cylinder Ambiguous case must remain explicitly unresolved until independently distinguishable by semantic proof.

## 4. Proposed next, separately gated evidence contract (NOT active)

Before any production design commitment, draft a **separate** bounded G1 Work Contract only after the now-recorded Owner G0 findings acceptance **and still-pending** D2 investigation authorization. It would define exact allowed paths, real OCCT fixtures and independent case-wise status assertions to test:

1. For each Unsupported Fillet/Chamfer native Vertex: resolved semantic Surface address cardinality, native/raw incidence, producing Feature/corner/edge provenance and actual bounded incident Edge relations; distinguish no carrier vs >3 vs unresolved/aliased carrier, without inference from `unsupported` status alone.
2. For two three-Surface Ambiguous groups in the analytic half-cylinder: test whether any independently resolvable semantic Edge/curve/branch witness splits the group **across upstream dimension edits, reversed model traversal and cold rebuild**, without provider order or XYZ.
3. Cylindrical seam / full-circle: distinguish actual user-meaningful material endpoints from representation partition/seam artifacts before treating them as construction-point candidates.
4. Positive regressions (rectangular Extrude/Cut, partial Revolve), removed-source `Missing`, same-XYZ replacement and earlier-stage binding. A certified old Point must not regress to ambiguity or transfer producers.
5. Lifecycle guard control: existing `tests/pm02p_e_core_lifecycle_test.cpp` is *nonnative* freshness evidence, not test authority for UI changes or saved RG-02 sources. Any future production `VERTEX` persistence and stale selection integration must have their own Owner-accepted scope and exact tests.

If a G1 fixture requires `src/**`, new CMake targets, schema, UI or files outside a future accepted bounded Work Contract, **STOP and request its explicit amendment**. No self-activation. G0 evidence PR #332 is **not** authority to begin G1.

## 5. Decisions requiring separate Owner action

| Gate | Owner decision | Current status |
| --- | --- | --- |
| G0 findings | Accept measured G0 evidence with two explicitly documented limits: (1) raw/semantic Fillet/Chamfer Unsupported Vertex Surface-incidence/provenance; (2) V11 stale Viewer selection and future saved RG-02 source lifecycle not proved by the in-memory cold rebuild | **OWNER-ACCEPTED 2026-10-12** (`akceptuję - kontynuuj`) as bounded G0 evidence closure; those limitations remain open for future separately scoped work |
| G0 evidence PR #332 | Separate approval to merge exact-HEAD FULL-tested evidence PR after findings accepted | **OWNER-APPROVED AND MERGED** `5b495c4bbed8754d7a1f01e87a30716429db786d` on 2026-10-12; G0 test/evidence integration only |
| D2 direction | Approve an **evidence-first investigation of Option B, preserving A fail-closed**, or instruct another direction; **not** approval of a concrete schema or implementation | **PENDING** |
| G1 bounded Work Contract | Inspect/accept specific allowed code/test files and next proof matrix; separate activation on main | **NOT PROPOSED / NOT ACTIVE** |
| Future product identity/schema | Explicitly accept an evidence-backed semantic selector, version and migration/failure policy | **NOT DESIGNED / NOT ACCEPTED** |
| RG-02 UI/UX | Reviewable complete UX design presented and consulted on; **explicit separate approval before any UI code including 3D VERTEX/MID/CENTER snap markers** | **HARD STOP — PENDING** |

**Mandatory order:** Owner acceptance of measured G0 findings **[DONE, WITH GAPS]** → distinct approval and merge of evidence PR #332 **[DONE]** → separately chosen D2 investigation direction and bounded G1 contract **[PENDING]** → approval/activation if authorized → later production semantic source D2 and contract → **independent UX approval before any UI mutation**. Preparing or merging this *D2 draft* must never short-circuit a prerequisite.

## Documentation impact

Internal docs: not required  
User/Product docs: not required  
Reason: this is an explicitly nonexecuting draft architecture discussion supported by Owner-accepted, Windows-tested and merged G0 evidence PR #332. No product behavior, schema, API or user-visible interface was altered. Future production work requires separate Internal and PL/EN Product documentation and generated Browser verification.
