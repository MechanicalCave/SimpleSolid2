# RG-02B1/D2-Q1 — stable selected-Edge-local END provenance: evidence-first scope PLAN

**STATUS: PLANNING DRAFT ONLY (2026-10-13), NOT A WORK CONTRACT / NOT OWNER-ACCEPTED EXECUTABLE SCOPE / NOT ACTIVE / NO CODE AUTHORITY.** Q0 exact bounded 3D Edge geometry is now merged to `main` at `596ec70fd10d5931c09a20dd5111f709812f0692` (PR #354); Q0 remains `work/ACTIVE.yaml.active_work`. Owner's Q0 consent did not merge planning PR #351 or authorize Q1 code. This planning-only file is written as a separate Draft to avoid losing implementation momentum while preserving independent scope gates.

## 1. Owner-accepted user semantics held invariant

- There is **one existing Datum Plane tool**, with `Offset` unchanged and future `Edge + Point` beside it. END and MID use the **same existing END marker visual convention**; no on/off-edge selector, separate MID mode, point-overlap chooser, novel hotkey or modified Command Line grammar. Stage-specific, same-source exact meaning drives preview; signed default Frame3 and `Odwróć / Reverse` are the separate future Q2 gate, not Q1.
- At one shared native corner, multiple Edges A/B/C may have physically coincident terminal locations and independent or globally ambiguous Point meanings. If the user ultimately selects **strict Edge B**, a Point-first or Edge-first gesture must identify **the clicked terminal of B** uniquely, even if the initial hover displayed another Edge's Point. This is **transient gesture disambiguation**, not automatically a durable source identity.
- The persisted candidate is `SelectedMaterialEdgeEnd { source_edge: MaterialEdgeReference, terminal: StableEdgeLocalTerminalKey }` attached to the *selected Edge*, NOT merely a global three-Surface `FeaturePointAddress`, OCCT vertex handle, `RuntimeVertexToken`, `current_terminal_positions_world[0/1]`, native trim parameter, world XYZ, screen location or Viewer presentation token.
- A proven independently Resolved global semantic Point may provide a first exact terminal witness **in conjunction with its strict selected Edge**; inability to resolve such a global Point does **not** automatically prove that the chosen Edge-local END is semantically impossible. Conversely, Edge membership or coincident XYZ must **never** turn an intrinsic globally Ambiguous Point into a new global Resolved `FeaturePointAddress`.

## 2. Existing source boundaries verified read-only

| Existing source / evidence | What it really guarantees | What Q1 still must establish |
| --- | --- | --- |
| `src/part/include/simplesolid2/part/feature_evaluation.hpp` | `BodyStageTopologyCatalog` has strict `BodyStageRef`, `BodyEdgeTopologyRecord` / `BodyVertexTopologyRecord`; Vertex records hold `point_candidates`, `incident_material_edges`, diagnostic `provider_point` | Candidate Part-stage **Edge-owned terminal lineage** or typed proof that a terminal cannot be safely authored |
| `part::authorMaterialEdgeReference` + `resolveMaterialEdgeReference` | A strict chosen material Edge must resolve uniquely at the declared stage; stale/multi-branch/representation failures typed fail-closed | A **stable discriminator of which logical terminal** on that resolved material Edge survives edit/cold re-evaluation |
| Merged Q0 `IExactBoundedEdgeGeometryQuery` | Exact current Body-scoped two physical terminal positions, paired unoriented tangent lines, and true bounded 3D MID | Ephemeral array slot and native END positions must NOT be persisted; Q0 is a geometry witness **after** Q1 strict source admission |
| Native E0 evidence + `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp` | Canonical rectangular Extrude: 12 independently material straight Edges, 24 Edge-terminal uses, 8 shared corners; strict source replay and edited height/cold provider evidence | Pair of stable **Edge-local END keys** for every eligible canonical Edge, including shared corners and cross-stage replay |
| Existing G0 partial endpoint families | Fillet/Chamfer / full Revolve and half-cylinder may be globally Unsupported/Ambiguous under three-Surface `FeaturePointAddress` | An independent Edge-local key may be supported **only if** producer-specific terminal provenance is actually demonstrated. Never promise universal END |
| Merged RG-02B1 D2 source proposal and Owner Datum UX document | `work/RG-02B1_D2_EDGE_POINT_SEMANTIC_DESIGN_PROPOSAL.md` §D2-P1/P2/P5 and `work/RG-02B1_DATUM_EDGE_POINT_WORK_CONTRACT_DRAFT.md` §0A/§1B fix the user intent | Q1 must prove semantic identity first; Q2+ implements pick buffer/preview/default signed frame/versioned persistence |

**Source-audit caution:** the D2 document predates merged Q0 and reports its earlier absence; Q0 is now in `main`. Neither prior E0 nor newly merged Q0 proofs establish **durable** END key stability by themselves.

## 3. Q1 narrow technical work and exact STOPs — proposal, NOT executable allowlist

**Q1-A: evidence-first terminal authorability.** From `BodyStageRef` + strict `MaterialEdgeReference` and current complete Part catalog, enumerate the chosen Edge's two *current* native terminal observations, compare only that Edge's incidence to the stage's known terminal-point semantic candidates and producer lineage. Propose a distinct, versionable **Edge-local semantic terminal key** only if each of the two physical ENDs has an independently stable, unique semantic discriminator. The single `MaterialEdgeReference` alone is insufficient to distinguish two ENDs. Do not require both ends to be globally Resolved `FeaturePointAddress` if a stronger source-specific Edge-local provenance can be proved.

**Q1-B: durable same-source replay, not proximity repair.** For a proven Edge and terminal key, resolve the **same logical END** on exactly the authored Body stage after upstream height edit, independent cold provider and re-evaluation. Runtime endpoints may swap indices, trim signs, or presentation order; the semantic key must not flip. If the source/terminal provenance has disappeared, become ambiguous, or changes to a different semantic Edge, resolve `lost/ambiguous/unsupported` **without** silently switching END or falling back to nearest XYZ, selected native Vertex ID or previous geometry.

**Q1-C: typed source boundary only.** Draft a Part-owned value with explicit terminal provenance and typed resolved/ambiguous/lost/unsupported outcomes. The final public signature and any persistent storage layout are **not yet decided**; Q1 does not edit current Part document schema v16, authored Datum kind, UI or Command Line, and does not implement MID (already current-Q0 geometric observation). Versioned Datum persistence and signed orientation belong to Q2+ after a separate exact contract.

**Q1-D: no behavioral expansion by inference.** Shared corner must be authorable for Edge B only where B's exact END key is independently certified. Fillet/Chamfer/Revolve or same-Edge coincident ends can remain unsupported; if stable producer lineage is missing, STOP and return an explicit list of unsupported cases. No global Point-status rewrite; the Edge-local key is a *different* kind of source.

## 4. Finite native evidence matrix — ALL NEW Q1 TESTS UNRUN

| Proposed witness | Scenario | Required measurable result / fail-closed rule |
| --- | --- | --- |
| Q1-01 | Canonical Extrude 12 strict Line Edges, 24 physical Edge-terminal uses | Each admitted Edge has exactly **two distinguishable semantically keyed termini** at one stage; count 24 authorable uses, no dependence on native slot 0/1 |
| Q1-02 | Three Edge uses at each of 8 common corners | Edges A/B/C share XYZ, but selecting Edge B and the clicked corner identifies **B's terminal key** alone; other Edge sources remain distinct; first-hinted Point A must not override B |
| Q1-03 | Independent cold OCCT provider and repeated evaluation | Same source Edge + key resolves to same **semantic** terminal despite altered native tokens/ordering, or fails typed where provenance is not guaranteed |
| Q1-04 | Upstream Extrude height modification and stage replay | The same authored key follows the correct terminus (e.g. top stays top) or returns typed lost, never swaps silently |
| Q1-05 | Wrong BodyStageRef, stale Edge, branch ambiguity, missing key | Typed fail-closed, no current terminal binding and no Q0 result falsely treated as durable |
| Q1-06 | Half-cylinder global ambiguous Vertex + Fillet/Chamfer/Revolve unsupported families | Preserve global Point statuses; allow Edge-local END **only** with independent stable lineage, otherwise explicit not-authorable status |
| Q1-07 | Same selected Edge's ends geometrically indistinguishable / closed or periodic | Reject ambiguous terminal interpretation; no OCCT trim index or nearest XYZ fallback |
| Q1-08 | Correct strict-source terminal selected → Q0 3D evaluation-local query | Bind exact same current Body and Edge, verify terminal geometry/tangent is paired with semantic key *without persisting Q0 runtime slot*. Do **not** author a plane or Frame3 here |

**Evidence must be truly native**, not derived solely from Q0's successful Line/Arc and END-array results. The Q1-02 expectation for choosing Edge B is a **semantic geometry** test, while actual in-viewport point-first/Edge-first click-buffer and Command Line equivalence are **Q2+/UI integration** and remain unimplemented.

## 5. Exact future scope still to audit — no source edits authorized in this Draft

Candidate areas to inspect before creating an executable Q1 Work Contract: `src/part/include/simplesolid2/part/semantic_topology_reference.hpp`, `src/part/include/simplesolid2/part/feature_evaluation.hpp`, existing `src/part/feature_evaluation.cpp`, existing registered native `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp` and `tests/pm05b2_strict_edge_resolver_test.cpp`. These are **read-only investigation candidates**, NOT an approved writable allowlist. Confirm exact target registration, whether OCCT private Vertex incidence/producer endpoint lineage is exposed enough at the Part stage, and whether proving the key would require changing `src/kernel_occt/**` or further files. If yes, STOP for exact-path Owner contract review; do not silently append Q1 edits to the already merged Q0 contract.

**Do not** alter existing active Q0 contract, add new Q1 ACTIVE pointer, mutate persistent Part v16, change the single Datum Plane UI, or merge this planning Draft by inference. Proposed gate order: source-backed Q1 proof design + exact file/CTest allowlist → distinct complete executable Q1 Work Contract Owner approval → exact-head contract merge consent → separate approved/merged ACTIVE switch → Q1 source/native implementation and FOCUSED/FAST/Ready-FULL. No impossible demand to run unimplemented native Q1 tests before activation.

## Documentation impact

Internal docs: not required
User/Product docs: not required
Reason: Planning-only `work/**` note for future Part-native semantic END provenance; no Q1 API, product UI, storage, current as-built behavior or code changes in this Draft.
