# RG-02B1 D2 — Datum Plane Edge + Point: source identity and exact geometry (PROPOSAL)

**Status:** **D2 DESIGN DRAFT FOR OWNER DECISION — NOT ACCEPTED, NOT MERGED, NOT ACTIVE, NOT AN EXECUTABLE WORK CONTRACT.**  
**Date:** 2026-10-13  
**Program:** Owner-accepted Part Modeling roadmap v1.33; active implementation authority remains **evidence-only** `work/RG-02B1_E0_NATIVE_EDGE_POINT_PROOF_WORK_CONTRACT_DRAFT.md` after [activation PR #347](https://github.com/MechanicalCave/SimpleSolid2/pull/347), squash `b46bbe69b59d660f089967cba9b9f2e9a10ac029`.  
**Read-only code baseline:** `main` `b46bbe69b59d660f089967cba9b9f2e9a10ac029`. **Separate measured branch:** [PR #348](https://github.com/MechanicalCave/SimpleSolid2/pull/348) Ready for Review / OPEN / UNMERGED, exact evidence HEAD `5d135eb449bdd26f59cacc3ea606f084ac91be08`, Windows FULL [#2828](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38392879099) PASS Core 25/25 + Kernel 57/57 + Desktop 117/117, docs PASS = **199/199**. The Owner accepted **limited E0 findings as D2 input**, **not E0 final/feature PASS or PR #348 merge**.  
**UX input:** accepted *principle* in separate Draft [PR #345](https://github.com/MechanicalCave/SimpleSolid2/pull/345) (§1/§1A); full §1B UX/keyboard/Reverse approval remains **OPEN**.  
**Normative references:** `AGENTS.md`, Constitution / Foundation / Architecture, accepted ADR-0009/0011/0012/0014/0016/0017, `work/ACTIVE.yaml`, roadmap v1.33, `governance/DOCUMENTATION.md`.

## 1. Immutable user intent — not up for implicit re-design

Use **one existing Datum Plane tool**, keep existing Offset untouched, and offer one additional provisional **Edge + Point** constructor. The user indicates an Edge and a Point on the 3D model in **either order**. **No separate END-versus-MID or on-versus-off-edge modes**, no picker menu introduced as an escape hatch. The already-existing END/Vertex marker visual is reused for MID; ordinary Edge/Face hover is retained.

**Owner's explicit clarification to the user interaction (2026-10-13):** At a solid corner, multiple current Edges and their Point hit meanings meet at the same visible location. The first Point click identifies **the endpoint location that the user indicated**, NOT a compulsory prechosen independent Point address. Once the user picks one specific strict material Edge, the algorithm must resolve the **one endpoint of THAT Edge at the clicked location**, **even if the earlier hover/primary click selected another coincident Edge's Point**. The same result holds for Edge-first. Do not ask which overlapped Point the user meant. This clarification is accepted **UX intent**, not proof that a durable endpoint source already exists for every native edge.

After a complete Edge+Point gesture:
- A uniquely indicated bounded **terminal of the selected Edge** (END), or its exact Edge-local MID, gives a **perpendicular plane** at that location with normal parallel to the analytic Edge tangent. Supported straight Line or open trimmed circular Arc, where strict semantic identity and exact geometry are proven.
- An independently certified semantic Point **off the infinite supporting line** of a strict straight Edge yields a **plane containing that line and the Point**.
- Collinear off-source Point, arbitrary point on Edge, off-edge Point with curved Edge, closed circles/periodic seams, stale/unsupported geometry, or two indistinguishable termini of the **same chosen Edge** at one hit location: **typed fail-closed rejection**, not silent guessing.

**Two different kinds of disambiguation MUST remain separate:** (1) **transient gesture disambiguation**: given a specific strict Edge, locate the uniquely clicked *native terminal of that Edge* among its two current ends; the old highlighted Point from another Edge is not source authority; (2) **durable endpoint reference**: prove that the same chosen terminal can be identified by stable feature/curve/edge-end provenance after edit, cold rebuild and save. A globally `Ambiguous` three-Surface `FeaturePointAddress` need not prohibit a **different independently proven Edge-local terminal semantic role**, but selecting an incident Edge by itself does **not** upgrade that global Point address to `Resolved` or prove persistent terminal provenance.

**Never force the user into a second overlapping-Point selector.** A stored generation-scoped candidate/hit region and current native topology may classify which of the chosen Edge's two terminal locations was clicked (screen hit/depth for the *gesture only*). Neither screen proximity nor current native endpoint order becomes the **persisted identity**. If no or both ends of the chosen Edge are compatible, reject; do not choose the nearer/order-first end. The complete concrete focus/copy/Reverse UI still requires its separate Owner D3 approval.

## 2. Verified current contracts and why a new typed source is necessary

| Current source | Source-backed fact | Architectural constraint |
| --- | --- | --- |
| `src/part/include/simplesolid2/part/semantic_topology_reference.hpp` | `MaterialEdgeReference` = explicit `BodyStageRef`, `FeatureCurveAddress`, strict `EdgeBranchDiscriminator` (`SingularAtAuthoredStage`, `BetweenSemanticPoints`, `AtSingleSemanticPoint`). `FeaturePointAddress` is currently **exactly a three-Surface intersection**, not "any 3D picked Point". | Reuse Edge reference; do not extend old Point meaning implicitly or infer a generic endpoint from provider orientation/order. |
| `src/part/include/simplesolid2/part/feature_evaluation.hpp`, `src/part/feature_evaluation.cpp` | `BodyStageTopologyCatalog` is complete and stage-bound. Each runtime Vertex has explicit Resolved/Ambiguous/Missing/Unsupported and diagnostic `provider_point`; incidence is **runtime-only**. `semanticPointForRuntimeVertex` requires unique independently Resolved `FeaturePointAddress` and one-to-one native realization. | An independently certified Point and a visually clickable Vertex are **different facts**. Never persist runtime incidence/XYZ. |
| `src/kernel/include/simplesolid2/kernel/edge_projection.hpp`, `src/kernel_occt/include/simplesolid2/kernel_occt/solid_modeling_kernel.hpp` | Existing provider-neutral `IEdgeProjectionQuery` binds a generation-specific material Edge to a Body and produces **2D projected geometry**; no general stage-bound **3D exact END/MID position + tangent/length** query is exported. | No fake 3D MID result using `Arc2`, tessellation or screen-space position. New query is separate D2. |
| `src/part/include/simplesolid2/part/datum.hpp`, `part_document.hpp`, `datum_evaluation.cpp` | Authored Datum storage is `std::vector<OffsetDatumPlane>`; evaluator and draft/commands assume Offset. `Frame3` derived, not serialized. | New Datum constructor must be a **distinct typed durable variant**, not smuggled into Offset. Existing DatumId and offset behavior must remain stable. |
| `src/part/part_document_store.cpp`, `part_document_store.hpp` | Current native Part schema **v16**; existing Datum kind `offset_plane` has strict **exact five fields** `id, kind, source, offset_mm, visible`. Unknown/malformed Datum kind fails. | Design a versioned compatible new Datum JSON kind only **after D2 acceptance**. Leave v16 Offset loader intact; explicit old-file/new-file refusal rules, no silent fallback/migration. |
| `src/viewer/include/simplesolid2/viewer/solid_presentation.hpp` | `BodyTopologyPickQueryResult` already holds a generation-scoped candidate vector; existing click typically commits a *single* presentation token. | A new **operation-local, nonpersistent** candidate ledger is required for point-first and MID source-local hits. |

**Actual E0 #348 evidence (bounded, Owner-accepted as decision input):** real rectangular Extrude: 12 strict Line Edges / 24 independently Resolved END uses / 8 three-Edge corners; 72 independently certified straight Edge+off-line Point pairs per evaluated fixture, stable source pairs after new native provider and height edit; real half-cylinder: 4 current Ambiguous Vertex realizations from 2 intrinsically Ambiguous semantic Points, plus 2 Unsupported without semantic address. **Unproven**: two distinct Resolved Point meanings overlapping in the Viewer, point-first UI; exact 3D open Arc query/MID tangent; complete persisted Datum frame/edit/rebuild and bad-topology END semantics. **No full E0-feasibility claim.**

## 3. Proposed D2 decisions for independent Owner acceptance

### D2-P1 — Edge-local END/MID plus independent off-line Point (**REVISED PROPOSAL**)

The Owner clarified that the *selected Edge* disambiguates an overlapping endpoint hit. A Datum END must therefore be a **terminal of the selected strict material Edge**, not necessarily an independently authored global `FeaturePointAddress`. Propose a **future tagged Datum point source** at an explicit Body stage, conceptually:

```text
DatumPointSource =
    SelectedMaterialEdgeEnd {
        source_edge: MaterialEdgeReference,
        terminal: StableEdgeLocalTerminalKey  // PROVEN provenance, NOT numeric 0/1
    }
  | SelectedMaterialEdgeMid {
        source_edge: MaterialEdgeReference,
        role: MID
    }
  | IndependentSemanticPoint {
        stage: BodyStageRef,
        address: FeaturePointAddress
    }
  // No global Vertex identity, provider first/last index, raw XYZ or Viewer token.
```

**These are conceptual alternatives, NOT implemented C++ types or accepted v17 fields.**

- `SelectedMaterialEdgeEnd`: once the gesture identifies **which of the chosen Edge's two current termini** was clicked, use a **stable Edge-local endpoint provenance key** as the *durable* source. If that particular terminal has a uniquely `Resolved` existing `FeaturePointAddress`, that semantic Point can be part of its provenance proof. But **an independent global FeaturePointAddress is not required by the UX for an Edge-local END**; it is only one possible certified representation. A new terminal key must be proven against real unsupported/ambiguous global Point cases before use.
- `SelectedMaterialEdgeMid`: the half-length point is intrinsic to the **bounded selected Edge** and needs no native B-Rep Vertex. Store that strict material Edge plus the `MID` role, never a sampled XYZ or fabricated `FeaturePointAddress`.
- `IndependentSemanticPoint`: the off-line construction Point for a straight Edge still needs its **own independently Resolved** semantic identity at the exact common Body stage; a different Edge's visible unsupported Vertex does not automatically become an independently usable free Point.

**Pick order invariant:** a Point-first gesture may initially highlight a point associated with Edge A; if the user next selects Edge B sharing the clicked corner, and precisely one terminal of **Edge B** matches the hit location at the current stage, **Edge B's terminal wins** even if no global point candidate from B was frontmost. Edge-first behaves identically. The transient gesture may use native endpoint geometry/screen hit support to select the *terminal location*, but this geometry is NEVER durable identity. Both termini of the same Edge hit indistinguishably, absent endpoint, stage mismatch or missing semantic provenance: fail closed at commit, not a third UI mode.

### D2-P2 — prove a stable Edge-local terminal key; do NOT block on global Point status alone (**REVISED PROPOSAL**)

| Strategy | What it identifies | D2 recommendation |
| --- | --- | --- |
| A. Edge-local END key based on an independently `Resolved` `FeaturePointAddress` plus strict selected `MaterialEdgeReference` | An already provable terminal on canonical straight Extrude edges. | **Valid first technical witness**, not the definition of the user's click or a general coverage guarantee. |
| B. Provenance-backed **Edge-local terminal discriminator**, independent of intrinsic global Point identity | The particular start/end *semantic terminal* of the selected bounded material Edge even when its global three-Surface Point address is Unsupported/Ambiguous, provided stable feature/curve/branch/terminal provenance can be established. | **Preferred architectural target for END coverage**, but NEEDS separate real native proof (Fillet/Chamfer, Revolve/open Arc, trimmed/split cases, edits, provider restart); no unproven runtime terminal key may be serialized. |
| C. Raw OCCT endpoint `first/last`, `TopoDS_Vertex`, Edge enumeration order, nearest 3D XYZ or screen hit converted into a stored coordinate | A potentially unstable realization of a terminal. | **Reject for persistence**, even when it happens to work at a common corner today. |

**The Owner's UX claim is respected without making an unsupported engineering promise:** on a regular open bounded Edge, one terminal exists at each distinct endpoint location; the selected Edge can disambiguate overlapping hits **during a gesture**. This does **not imply** that two stable persistent endpoint semantics are already available from today's `MaterialEdgeReference` for every curve/split/refined B-Rep. For a closed/degenerate edge, two projected endpoints overlap, more than one terminal fits the gesture, or independent provenance cannot survive edit/cold replay: **reject with a precise status**, preserve intent, and request a new bounded D2 proof—never tell the user to pick a different visually overlapping point.

**Important integrity rule:** edge-local terminal identity is a *new distinct source-kind proposal*. Selecting an Edge may resolve *that Edge-local END* by independently valid provenance, **but does not rewrite** `FeaturePointResolution.status` from Ambiguous/Unsupported to Resolved. The accepted `pm05b2.strict_edge_resolver` negative regression remains intact.

### D2-P3 — a separate strict 3D native geometry query (**RECOMMEND ACCEPT IN PRINCIPLE, API SHAPE OPEN**)

Propose future **provider-neutral, evaluation-local** `IExactBoundedEdgeGeometryQuery` (working name), separate from `IEdgeProjectionQuery`. Input is a **current `RuntimeSolidHandle` and current runtime Edge token scoped to exactly that Body evaluation generation**, handed to Kernel only **after Part has strictly resolved a `MaterialEdgeReference` in its declared stage**. It must reject provider/Body-generation mismatch, invalid edge, seams/partition/nonmaterial edge, unsupported curve, collapsed/zero-length boundary and ambiguous branch.

Finite query roles: `END_AT_CERTIFIED_EDGE_LOCAL_TERMINAL` requires Part's separately proven **selected strict Edge + stable terminal provenance key**, not a global Point address, provider endpoint ordinal or Viewer token. For the canonical already-Resolved subset, a uniquely certified `FeaturePointAddress` may corroborate this terminal. `MID_AT_HALF_BOUNDED_LENGTH` requires an exact bounded, open, nondegenerate material **Line or circular Arc**. Output is **geometry-only**: evaluated world point and tangent **line** (plus explicit orientation validity/status if D2-P4 can make its sign deterministic), material bounded length/analytic classification, typed status. **No semantic ID, transient binding or provider shape is persisted**.

- Straight Line MID = physically half of the **bounded material Edge length**; tangent normal to perpendicular plane is the true Line direction, not display segment vector.
- Open Arc MID = half the **actual bounded arc length along that trimmed Edge**, with analytic derivative/tangent there. Avoid a naive arithmetic average of generic OCCT curve parameters. Positive and negative trimmed sweeps, reversal, retained trims and cold replay need dedicated real 3D native fixtures before use.
- Full closed circle/periodic seam, nonanalytic/ellipse/complex curves, representation partitions and mixed-generation tokens are **unsupported**, not approximated.
- There is **no completed native E0-03/04 proof** of this query. Detailed C++ interface and tolerance/error bounds require a separately scoped Owner-accepted code contract with targeted native RED/GREEN first. Reusing internal OCCT routines from 2D projection is an implementation possibility, **not evidence** that the new public 3D contract exists.

### D2-P4 — plane frame, Reverse and storage must be approved TOGETHER (**RECOMMEND SEPARATE OWNER REVIEW**)

**Plane meaning versus authored orientation:** A geometric perpendicular plane is invariant under normal reversal, but a downstream Sketch uses signed right-handed `Frame3{O,U,V,N}`. Its **sign and U-axis matter** for geometry persistence. Therefore passing a point+tangent alone is not a sufficient Datum contract.

Recommended constraints for the precise next D2 review (not final choices):
- Source-local END/MID perpendicular Datum: `O` = current exact source point; `N` = one deterministic semantic tangent direction. `U` comes from an independently justified stable reference/canonical projection, `V = N × U`, each normalized. The rule must not depend on OCCT parameter orientation, provider iteration, arbitrary first Edge, mesh, camera, cursor or transient hit list. A sign flip on reverse native orientation without authored Reverse is a **RED**.
- For an Edge with two **proven stable Edge-local terminal keys**, a canonical semantic ordering of those keys might help orient `N` if and only if both keys remain attached to the same logical termini after edits and cold provider rebuild. Ordering existing `FeaturePointAddress` values is just one *possible proof path*, not a requirement for the user gesture. Lexical keys or orientation without a geometric/stage proof are **not automatically** a stable frame rule. For single-branch and open Arc cases with insufficient terminal provenance/orientation, STOP rather than use native first/last.
- Through-line-plus-off-line-Point: `O` anchored at a deterministic current semantic source point on the line (or source-driven canonical anchor if proven), `U` aligned with a strictly canonical line direction, `N` from `U × (Point - O)`, `V = N × U`; reject collinearity/short vectors and ambiguous sign. A different valid `O` along the same line gives the same plane but can move downstream Sketch U/V coordinates; thus anchoring is a **semantic persistence decision**.
- `Reverse` must be an **authored, reversible, deterministic orientation choice**, not replacement of END/MID or recomputation from Viewer. Whether Reverse flips only N with a specified basis flip or uses a different accepted frame transform must be decided by Owner with downstream Sketch invariant tests.
- Store a distinct `EdgePointDatumPlane`-style tagged kind in a **new explicitly accepted schema version** (current v16 must not be silently overwritten). Carry one DatumId, one strict material Edge source, typed Point-source choice, user Reverse/orientation field, visibility and nothing provider-derived. Preserve Offset `kind: "offset_plane"` and existing v16 strict five-field decode unchanged. Define older-file read, current-file save, unknown/malformed new kind, backwards refusal, post-edit missing/ambiguous preservation, atomic Undo/Redo and cold provider Save/Close/Reopen **before code**.

**No final U/V/N, sign, length tolerances, Reverse UI, v17 layout or migration policy is accepted by this Draft.** This is the most important remaining D2 blocker to an executable Datum product contract.

### D2-P5 — selected Edge resolves the clicked *terminal location*, not the first highlighted Point (**REVISED PROPOSAL**)

```text
pick(Point first):  // or pick Edge first; result must be the same
    gesture := snapshot(point-hit LOCATION / depth / all native candidates,
                        current Body stage, Part revision, Viewer generation)
    // No persistent Point identity; highlighted candidate is never locked in.

pick(Edge second):
    edge_ref := independently authorStrictMaterialEdge(...) or STOP
    termini := obtain both CURRENT bounded native ends OF edge_ref
               from exact current stage / provider generation
    matches := ends of THIS selected Edge compatible with gesture hit
               (geometry/screen data is GESTURE evidence only)
    if matches.size() == 1:
        selected_terminal := matches.only()
        if stable provenance-backed EDGE-LOCAL terminal key certified:
            source := SelectedMaterialEdgeEnd(edge_ref, key)
            derived := PERPENDICULAR
        else:
            STOP UnsupportedTerminalProvenance // not global Point repair
    else if MID marker of the same certified Edge uniquely hit:
        source := SelectedMaterialEdgeMid(edge_ref)
        derived := PERPENDICULAR only with exact 3D metric evidence
    else if matches.size() > 1:
        STOP AmbiguousGesture // two ends of SAME Edge indistinguishable
    else:
        // Not selected-Edge END/MID: independent off-line case only.
        independent_candidates := certify actual semantic Point addresses
        reject intrinsically Ambiguous/Unsupported/Missing/stale candidates
        if exactly one valid, off-infinite-line Point on straight edge:
            source := IndependentSemanticPoint(stage, address)
            derived := THROUGH_LINE
        else:
            STOP typed Ambiguous/Unsupported/Collinear/NoMatch
Finish:
    strict revalidation of selected Edge, Edge-local terminal provenance,
    independent off-line Point (when used), Body stage/current generation,
    exact 3D geometry, canonical frame and document revision
    one atomic Command/Undo if valid; zero mutation otherwise
```

**Concrete shared-corner acceptance test (still UNRUN):** three source Edges A/B/C terminate at the same visible corner. Hover/first click indicates A's Point. The user then chooses B. If the clicked endpoint location uniquely matches B's current terminal **and B has independently certified durable terminal provenance**, the final source is **B+its own terminal**, not A's Point, not "no matching global Point from B", and not any on-screen chooser. Repeat B→Point, with Edge A and C, plus cold rebuilt model. A Vertex shared natively across multiple Edges may even be one single B-Rep `Vertex`; the user still gets the **chosen Edge's terminal source meaning**, not a fabricated separately persisted vertex per Edge.

**Negative proof obligations:** both termini of chosen Edge projected into one indistinguishable hit, two current branches of the same strict material meaning, wrong stage, source-lost terminal provenance, curved off-line Point, unsupported periodic/degenerate geometry, and a genuinely global Ambiguous independent Point **must not** silently choose a winner. Screen hit tolerance can recognize *what location the user indicated*; it must never be used to fabricate durable terminal provenance, as a unique new `FeaturePointAddress`, or as save/rebuild rebinding identity.

This pseudocode is a D2/D3 design candidate, not executed code and not a verified new Body END picking API. The existing `BodyTopologyPickQueryResult.candidates` supports full transient hit inspection, but operation-owned buffering, native terminal hit semantics, MID marker, focus/Esc and Command Line parity require separate Owner approval of full UX in PR #345.### D2-P6 — split implementation **after** decisions; no overlapping authority (**RECOMMEND ACCEPT AS GATE**)

A proposed order, **not a pre-approved executable contract sequence**:

1. Close any remaining strictly necessary **E0 evidence holes** only in the current exact four-file test-only authority; report STOP if native/query cannot be tested without production API.
2. Owner accepts/rejects **P1/P2/P3** source/query architecture explicitly. Owner separately settles **P4** canonical frame, Reverse and schema compatibility, with cold stage and downstream Sketch tests specified. No "just implement and see" for frame or serialization.
3. Obtain full Owner UX review/approval for **P5** before any product UI/Viewer/Operations/Command Line mutation; product interaction principle does not substitute for that approval.
4. Draft **one smallest executable D2 source/query Work Contract** with exact source/test allowlist, no broad G1/Axis/PM-06. Owner exact-text acceptance, governance merge and dedicated ACTIVE activation are separate gates. Only then code on a fresh branch, native FOCUSED/FAST, exact-head FULL, PL/EN Product and as-built docs as specified, Owner practical acceptance and independent PR merge.
5. Persisted Datum variant + UI parity/Reverse may require distinct narrowly authorized follow-ups because source/query proof is not synonymous with product readiness. Do not silently blend separate contracts.

## 4. Decision matrix and STOP results

| Scenario | Expected future status | Proof obligation |
| --- | --- | --- |
| Canonical Extrude straight Edge + clicked common corner (another Edge's Point happened to be highlighted) | **Desired Edge-local END behavior** | Strict chosen Edge determines one matching current terminal independent of initial hover; existing E0 certified Point addresses can support the initial stable-key proof, but **both pick orders, exact hit correlation, durable terminal key, frame and persistence remain UNRUN/OPEN**. |
| Canonical straight Edge + independently Resolved off-line trihedral Point | **Candidate supported** | E0 positive for source meaning/noncollinearity; plane frame, stage and serialization still open. |
| Strict bounded straight Edge MID | **Architecture proposal, NOT IMPLEMENTED** | Typed Edge-local MID, strict native 3D metric query, orientation, viewport MID marker/pick, schema all absent today. |
| Strict open trimmed Arc MID/END | **UNPROVEN; STOP before code** | Native bounded arc-length midpoint and tangent, certified endpoint provenance, source stage and orientation; full circle remains Unsupported. |
| Half-cylinder globally Ambiguous semantic Point at visible native Vertex | **Do not author global Point**; conditional future **Edge-local END** | Four native realizations/two intrinsic Ambiguous global Point groups proven. A selected strict Edge *could* identify its terminal as a different semantic kind **only after** provenance proof; without stable terminal key reject. Never overwrite global Point's status. |
| Fillet/Chamfer or full Revolve globally Unsupported endpoint | **Conditional Edge-local END, else Unsupported** | Selected Edge disambiguates a current terminal gesture but **not** persistent identity without producer/terminal provenance; no automatic G1, topology-order or XYZ-based repair. |
| Point-first shared corner with other Edge's Point highlighted | **Desired by Owner; NOT TESTED** | Selected Edge's own one matching terminal is chosen, regardless of initial hover; retain click location/candidate set and prove a durable Edge-local terminal key. No separate point chooser. |
| Source deletion, later-stage split/merge, stale token or provider mismatch | **Missing/Ambiguous/Blocked**, no mutation | Exact stage, invalidation, no stale last-good, correct one-transaction Undo/Redo/Save/cold re-evaluation. |

## 5. Rejected shortcuts

Do NOT (i) serialize raw XYZ, OCCT handle, `TopoDS_Vertex`, runtime index or presentation token; (ii) reinterpret MID as a fabricated three-Surface Vertex; (iii) infer durable END by native first/last parameter or Edge order; (iv) upgrade a globally Ambiguous Point to Resolved by Edge incidence **or mistake a current gesture disambiguation for proven persistent Edge-local terminal provenance**; (v) run 2D `projectEdgeToPlane` as substitute for the missing 3D query; (vi) silently mutate `OffsetDatumPlane` or v16 `offset_plane` schema; (vii) choose frame orientation from camera/provider UV; (viii) declare general END/MID ready because Windows FULL of **evidence-only tests** is 199/199 GREEN; (ix) merge #348, #345, this Draft, or activate D2 by a generic "continue."

## 6. Owner decision requested, narrowly

**D2 design consultation (not code authority):** The Owner has **clarified the binding UX rule**: clicking a coincident corner and then Edge B must select **B's own corresponding END**, even if the first visually highlighted point belonged to Edge A. The updated proposed architecture is **P1 three source meanings: Edge-local END, Edge-local MID, independently Resolved off-line semantic Point; P2 provenance-backed stable Edge-terminal key (not global Point status) with fail-closed unproven cases; P3 stage-scoped exact 3D Line/open-Arc query; P5 selected-Edge terminal-hit correlation and both pick orders**. Is this revised source/geometry direction acceptable **in principle** after review? This does not approve an implementation, guess stable endpoint keys, or assume closed/full-circle semantics.

**Separate explicit decisions still needed:** P4 signed deterministic U/V/N frame, Reverse, line anchor and current v16 → new-version schema compatibility; full PR #345 §1B UX interaction specification; independently selected production contract and ACTIVE activation. Do not infer acceptance of these from accepting P1/P2/P3/P5 or from earlier E0 evidence.

**Scope/status close:** E0 tests remain active under existing contract; E0 PR #348 is **OPEN/UNMERGED** and requires PR-specific Owner merge consent separately. This proposal modifies **only one work document**, has **no runtime/test/code/CI, user-visible product or browser effect** and gives NO permission to change production. 

## Documentation impact

Internal docs: not required  
User/Product docs: not required  
Reason: This work-only draft records alternative architecture decisions and existing evidence for Owner consultation, without changing accepted semantics, production code, current-state documentation, runtime behavior, commands, UI or persistence. A future accepted/shipped API/Datum implementation will require current-state Internal documentation, paired PL/EN Product docs and deterministic generated Browser validation.
