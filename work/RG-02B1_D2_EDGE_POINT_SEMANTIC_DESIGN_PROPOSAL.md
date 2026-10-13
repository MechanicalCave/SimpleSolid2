# RG-02B1 D2 — Datum Plane Edge + Point: source identity and exact geometry (PROPOSAL)

**Status:** **D2 DESIGN DRAFT FOR OWNER DECISION — NOT ACCEPTED, NOT MERGED, NOT ACTIVE, NOT AN EXECUTABLE WORK CONTRACT.**  
**Date:** 2026-10-13  
**Program:** Owner-accepted Part Modeling roadmap v1.33; active implementation authority remains **evidence-only** \`work/RG-02B1_E0_NATIVE_EDGE_POINT_PROOF_WORK_CONTRACT_DRAFT.md\` after [activation PR #347](https://github.com/MechanicalCave/SimpleSolid2/pull/347), squash \`b46bbe69b59d660f089967cba9b9f2e9a10ac029\`.  
**Read-only code baseline:** \`main\` \`b46bbe69b59d660f089967cba9b9f2e9a10ac029\`. **Separate measured branch:** [PR #348](https://github.com/MechanicalCave/SimpleSolid2/pull/348) Ready for Review / OPEN / UNMERGED, exact evidence HEAD \`5d135eb449bdd26f59cacc3ea606f084ac91be08\`, Windows FULL [#2828](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38392879099) PASS Core 25/25 + Kernel 57/57 + Desktop 117/117, docs PASS = **199/199**. The Owner accepted **limited E0 findings as D2 input**, **not E0 final/feature PASS or PR #348 merge**.  
**UX input:** accepted *principle* in separate Draft [PR #345](https://github.com/MechanicalCave/SimpleSolid2/pull/345) (§1/§1A); full §1B UX/keyboard/Reverse approval remains **OPEN**.  
**Normative references:** \`AGENTS.md\`, Constitution / Foundation / Architecture, accepted ADR-0009/0011/0012/0014/0016/0017, \`work/ACTIVE.yaml\`, roadmap v1.33, \`governance/DOCUMENTATION.md\`.

## 1. Immutable user intent — not up for implicit re-design

Use **one existing Datum Plane tool**, keep existing Offset untouched, and offer one additional provisional **Edge + Point** constructor. The user indicates an Edge and a Point on the 3D model in **either order**. **No separate END-versus-MID or on-versus-off-edge modes**, no picker menu introduced as an escape hatch. The already-existing END/Vertex marker visual is reused for MID; ordinary Edge/Face hover is retained.

After both source meanings are independently certified:
- A strict, source-Edge-local **END** or exact **MID** yields a **perpendicular plane through that point**, with normal parallel to the material Edge's analytic tangent there. Supported Line or open trimmed circular Arc only.
- An independently Resolved semantic Point **off the infinite supporting line** of a strict straight Edge yields the unique plane containing that line and Point.
- Collinear off-source Point, arbitrary point on Edge, off-edge Point with a curved Edge, full circle MID, seams/partitions, ambiguous or stale inputs: **typed fail-closed rejection**, not reinterpretation.
- A Point-first viewport click must retain a generation-scoped **transient set of all eligible semantic hit meanings**, because the source Edge can be selected second. A displayed/cycled first \`PresentationToken\` alone is not sufficient. Filtering may choose one of **several independently Resolved** meanings by exact Edge membership; it may **never** turn one intrinsically Ambiguous Point into Resolved via connectivity.

Any expansion to "every visible native Vertex can be END" requires an independent architectural proof and approval. The full UI design remains a separate D3 Owner review, not decided by this technical document.

## 2. Verified current contracts and why a new typed source is necessary

| Current source | Source-backed fact | Architectural constraint |
| --- | --- | --- |
| \`src/part/include/simplesolid2/part/semantic_topology_reference.hpp\` | \`MaterialEdgeReference\` = explicit \`BodyStageRef\`, \`FeatureCurveAddress\`, strict \`EdgeBranchDiscriminator\` (\`SingularAtAuthoredStage\`, \`BetweenSemanticPoints\`, \`AtSingleSemanticPoint\`). \`FeaturePointAddress\` is currently **exactly a three-Surface intersection**, not "any 3D picked Point". | Reuse Edge reference; do not extend old Point meaning implicitly or infer a generic endpoint from provider orientation/order. |
| \`src/part/include/simplesolid2/part/feature_evaluation.hpp\`, \`src/part/feature_evaluation.cpp\` | \`BodyStageTopologyCatalog\` is complete and stage-bound. Each runtime Vertex has explicit Resolved/Ambiguous/Missing/Unsupported and diagnostic \`provider_point\`; incidence is **runtime-only**. \`semanticPointForRuntimeVertex\` requires unique independently Resolved \`FeaturePointAddress\` and one-to-one native realization. | An independently certified Point and a visually clickable Vertex are **different facts**. Never persist runtime incidence/XYZ. |
| \`src/kernel/include/simplesolid2/kernel/edge_projection.hpp\`, \`src/kernel_occt/include/simplesolid2/kernel_occt/solid_modeling_kernel.hpp\` | Existing provider-neutral \`IEdgeProjectionQuery\` binds a generation-specific material Edge to a Body and produces **2D projected geometry**; no general stage-bound **3D exact END/MID position + tangent/length** query is exported. | No fake 3D MID result using \`Arc2\`, tessellation or screen-space position. New query is separate D2. |
| \`src/part/include/simplesolid2/part/datum.hpp\`, \`part_document.hpp\`, \`datum_evaluation.cpp\` | Authored Datum storage is \`std::vector<OffsetDatumPlane>\`; evaluator and draft/commands assume Offset. \`Frame3\` derived, not serialized. | New Datum constructor must be a **distinct typed durable variant**, not smuggled into Offset. Existing DatumId and offset behavior must remain stable. |
| \`src/part/part_document_store.cpp\`, \`part_document_store.hpp\` | Current native Part schema **v16**; existing Datum kind \`offset_plane\` has strict **exact five fields** \`id, kind, source, offset_mm, visible\`. Unknown/malformed Datum kind fails. | Design a versioned compatible new Datum JSON kind only **after D2 acceptance**. Leave v16 Offset loader intact; explicit old-file/new-file refusal rules, no silent fallback/migration. |
| \`src/viewer/include/simplesolid2/viewer/solid_presentation.hpp\` | \`BodyTopologyPickQueryResult\` already holds a generation-scoped candidate vector; existing click typically commits a *single* presentation token. | A new **operation-local, nonpersistent** candidate ledger is required for point-first and MID source-local hits. |

**Actual E0 #348 evidence (bounded, Owner-accepted as decision input):** real rectangular Extrude: 12 strict Line Edges / 24 independently Resolved END uses / 8 three-Edge corners; 72 independently certified straight Edge+off-line Point pairs per evaluated fixture, stable source pairs after new native provider and height edit; real half-cylinder: 4 current Ambiguous Vertex realizations from 2 intrinsically Ambiguous semantic Points, plus 2 Unsupported without semantic address. **Unproven**: two distinct Resolved Point meanings overlapping in the Viewer, point-first UI; exact 3D open Arc query/MID tangent; complete persisted Datum frame/edit/rebuild and bad-topology END semantics. **No full E0-feasibility claim.**

## 3. Proposed D2 decisions for independent Owner acceptance

### D2-P1 — distinguish a semantic Point from an Edge-local role (**RECOMMEND ACCEPT**)

Propose a **future tagged Datum Point source** at one explicit Body stage, conceptually:

\`\`\`text
DatumPointSource =
    ExistingResolvedSemanticPoint {
        stage: BodyStageRef,
        address: FeaturePointAddress
    }
  | ExactBoundedMaterialEdgeMid {
        source_edge: MaterialEdgeReference,
        role: MID
    }
  // No generic Vertex/XYZ or EDGE_END_0/1 based on provider order.
\`\`\`

These are **conceptual semantic alternatives**, not code types or approved v17 fields. With \`ExistingResolvedSemanticPoint\`, the Part evaluator must resolve **exactly one** already independently certified \`FeaturePointAddress\` to exactly one current Vertex at the same declared stage. On-edge END is recognized **only if** that specific resolved point is strictly the selected material Edge's native bounded endpoint at the same stage. The same semantic Point may serve as a valid independent off-line Point for a different straight Edge. No separate long-lived \`END\` tag is required for this proven trihedral subset; the relation is **derived from certified incidence, not identity chosen by incidence**.

\`ExactBoundedMaterialEdgeMid\` has **no independent \`FeaturePointAddress\` or \`RuntimeVertexToken\`** because a midpoint need not be a B-Rep Vertex. Persist the **strict selected \`MaterialEdgeReference\`** and its intrinsic role \`MID\`, never source xyz or midpoint parameter. A subsequent re-evaluation may move its physical position with the authored Edge, without losing intent. Do not create a standalone general 3D Point entity as a prerequisite.

**Hard subtlety:** user gesture is still \`Edge + Point\`; MID is a **point candidate on an Edge**, not a second independently authored Vertex. Point-first MID is possible only when its transient hit candidate is already certified as a *specific bounded Edge's* MID role. The second Edge pick must resolve against the same strict authored material Edge and current Body stage, not compare XYZ. Ambiguous displayed MID candidates remain operation-local until exactly one compatible source is provable.

### D2-P2 — endpoint gap is a separate provenance decision (**RECOMMEND FAIL CLOSED, DEFER UNIVERSAL END**)

Candidate strategies:

| Strategy | Effect | Recommendation |
| --- | --- | --- |
| A. Existing independently Resolved \`FeaturePointAddress\` | Works only for witnessed three-Surface semantic END; preserves current identity and cold re-resolution. | **Preferred initial admissibility**, without silently claiming every native END. |
| B. Stage-bound, provenance-backed **edge-local terminal semantic discriminator** | Could cover a genuinely unsupported native END if independently authored source meaning can distinguish the two termini under edit/split/merge and cold replay, without provider index/coordinate. | **D2 conditional future proposal only**. Must FIRST demonstrate non-heuristic provenance in real Fillet/Chamfer, Revolve/open Arc and split cases. If not proven, return Unsupported. |
| C. Provider \`first/last\`, \`TopoDS_Vertex\`, \`End0/End1\` by iteration, nearest XYZ, pixel or incidence-inferred Ambiguous Point | Appears easy but aliases on parameter reversal/split/merge and violates ADR-0014/16. | **REJECT**. |

Acceptance of option A is **not** permission to quietly cut back the agreed UX to "box corners only" or ship universal-END marketing; unresolved unsupported cases are visible, typed and remain pending. A truly generic END requires another Owner-approved bounded provenance evidence gate or a separate G1 decision—not automatic G1 activation.

### D2-P3 — a separate strict 3D native geometry query (**RECOMMEND ACCEPT IN PRINCIPLE, API SHAPE OPEN**)

Propose future **provider-neutral, evaluation-local** \`IExactBoundedEdgeGeometryQuery\` (working name), separate from \`IEdgeProjectionQuery\`. Input is a **current \`RuntimeSolidHandle\` and current runtime Edge token scoped to exactly that Body evaluation generation**, handed to Kernel only **after Part has strictly resolved a \`MaterialEdgeReference\` in its declared stage**. It must reject provider/Body-generation mismatch, invalid edge, seams/partition/nonmaterial edge, unsupported curve, collapsed/zero-length boundary and ambiguous branch.

Finite query roles: \`END_AT_CERTIFIED_SEMANTIC_POINT\` requires the Part-proven endpoint relation (not a provider endpoint ordinal); \`MID_AT_HALF_BOUNDED_LENGTH\` requires an exact bounded, open, nondegenerate material **Line or circular Arc**. Output is **geometry-only**: evaluated world point and tangent **line** (plus explicit orientation validity/status if D2-P4 can make its sign deterministic), material bounded length/analytic classification, typed status. **No semantic ID, transient binding or provider shape is persisted**.

- Straight Line MID = physically half of the **bounded material Edge length**; tangent normal to perpendicular plane is the true Line direction, not display segment vector.
- Open Arc MID = half the **actual bounded arc length along that trimmed Edge**, with analytic derivative/tangent there. Avoid a naive arithmetic average of generic OCCT curve parameters. Positive and negative trimmed sweeps, reversal, retained trims and cold replay need dedicated real 3D native fixtures before use.
- Full closed circle/periodic seam, nonanalytic/ellipse/complex curves, representation partitions and mixed-generation tokens are **unsupported**, not approximated.
- There is **no completed native E0-03/04 proof** of this query. Detailed C++ interface and tolerance/error bounds require a separately scoped Owner-accepted code contract with targeted native RED/GREEN first. Reusing internal OCCT routines from 2D projection is an implementation possibility, **not evidence** that the new public 3D contract exists.

### D2-P4 — plane frame, Reverse and storage must be approved TOGETHER (**RECOMMEND SEPARATE OWNER REVIEW**)

**Plane meaning versus authored orientation:** A geometric perpendicular plane is invariant under normal reversal, but a downstream Sketch uses signed right-handed \`Frame3{O,U,V,N}\`. Its **sign and U-axis matter** for geometry persistence. Therefore passing a point+tangent alone is not a sufficient Datum contract.

Recommended constraints for the precise next D2 review (not final choices):
- Source-local END/MID perpendicular Datum: \`O\` = current exact source point; \`N\` = one deterministic semantic tangent direction. \`U\` comes from an independently justified stable reference/canonical projection, \`V = N × U\`, each normalized. The rule must not depend on OCCT parameter orientation, provider iteration, arbitrary first Edge, mesh, camera, cursor or transient hit list. A sign flip on reverse native orientation without authored Reverse is a **RED**.
- For certified two-END Edges, source-semantic endpoint ordering may help orient \`N\` if and only if the ordering is durable and stays tied to those SAME two resolved endpoint addresses after edits; lexical ordering without a geometric proof is **not automatically** a full stable tangent/frame rule. For single-branch and open Arc cases with insufficient semantic orientation, STOP rather than use native first/last.
- Through-line-plus-off-line-Point: \`O\` anchored at a deterministic current semantic source point on the line (or source-driven canonical anchor if proven), \`U\` aligned with a strictly canonical line direction, \`N\` from \`U × (Point - O)\`, \`V = N × U\`; reject collinearity/short vectors and ambiguous sign. A different valid \`O\` along the same line gives the same plane but can move downstream Sketch U/V coordinates; thus anchoring is a **semantic persistence decision**.
- \`Reverse\` must be an **authored, reversible, deterministic orientation choice**, not replacement of END/MID or recomputation from Viewer. Whether Reverse flips only N with a specified basis flip or uses a different accepted frame transform must be decided by Owner with downstream Sketch invariant tests.
- Store a distinct \`EdgePointDatumPlane\`-style tagged kind in a **new explicitly accepted schema version** (current v16 must not be silently overwritten). Carry one DatumId, one strict material Edge source, typed Point-source choice, user Reverse/orientation field, visibility and nothing provider-derived. Preserve Offset \`kind: "offset_plane"\` and existing v16 strict five-field decode unchanged. Define older-file read, current-file save, unknown/malformed new kind, backwards refusal, post-edit missing/ambiguous preservation, atomic Undo/Redo and cold provider Save/Close/Reopen **before code**.

**No final U/V/N, sign, length tolerances, Reverse UI, v17 layout or migration policy is accepted by this Draft.** This is the most important remaining D2 blocker to an executable Datum product contract.

### D2-P5 — point-first semantics: candidate filtering is NOT Point repair (**RECOMMEND ACCEPT AS CONSTRAINT**)

\`\`\`text
pick(Point first):
    transient_hits := snapshot(all eligible Point/MID candidates,
                               Part revision, Body stage, Viewer generation)
pick(Edge second):
    edge_ref := independently authorStrictMaterialEdge(...) or STOP
    for each hit:
        independently certify semantic point or Edge-local MID role
        reject intrinsically Ambiguous/Unsupported/Missing/stale hits
        require matching explicit stage and proven endpoint/incidence
    if exactly one certified on-source END/MID -> perpendicular meaning
    else if exactly one independently certified off-line Point
            and strict Edge is straight -> through-line meaning
    else -> typed Ambiguous/Unsupported/Invalid STOP
Finish:
    re-resolve all current semantic sources, stage, geometry and revision
    one Command/Transaction if valid; zero mutation otherwise
\`\`\`

This pseudocode intentionally does NOT turn a Point hit into a durable token; all hit-ledger state is discarded at Finish/Cancel/stale generation. It also does NOT give automatic precedence to a *globally Ambiguous* Point simply because one incident Edge is chosen.

The existing \`BodyTopologyPickQueryResult.candidates\` supports a full hit query, but implementing operation-owned buffering, marker placement for current exact MID, UI focus/Esc and Command Line parity requires **separate full Owner UX acceptance** of PR #345 §1B; this D2 source proposal does not authorize any Viewport/UI change.

### D2-P6 — split implementation **after** decisions; no overlapping authority (**RECOMMEND ACCEPT AS GATE**)

A proposed order, **not a pre-approved executable contract sequence**:

1. Close any remaining strictly necessary **E0 evidence holes** only in the current exact four-file test-only authority; report STOP if native/query cannot be tested without production API.
2. Owner accepts/rejects **P1/P2/P3** source/query architecture explicitly. Owner separately settles **P4** canonical frame, Reverse and schema compatibility, with cold stage and downstream Sketch tests specified. No "just implement and see" for frame or serialization.
3. Obtain full Owner UX review/approval for **P5** before any product UI/Viewer/Operations/Command Line mutation; product interaction principle does not substitute for that approval.
4. Draft **one smallest executable D2 source/query Work Contract** with exact source/test allowlist, no broad G1/Axis/PM-06. Owner exact-text acceptance, governance merge and dedicated ACTIVE activation are separate gates. Only then code on a fresh branch, native FOCUSED/FAST, exact-head FULL, PL/EN Product and as-built docs as specified, Owner practical acceptance and independent PR merge.
5. Persisted Datum variant + UI parity/Reverse may require distinct narrowly authorized follow-ups because source/query proof is not synonymous with product readiness. Do not silently blend separate contracts.

## 4. Decision matrix and STOP results

| Scenario | Expected future status | Proof obligation |
| --- | --- | --- |
| Canonical Extrude straight Edge + one of its independently Resolved trihedral END Points | **Candidate supported** | Real E0 positive; product must additionally prove exact endpoint geometry, source-stage/cold persistence, frame/Undo before shipping. |
| Canonical straight Edge + independently Resolved off-line trihedral Point | **Candidate supported** | E0 positive for source meaning/noncollinearity; plane frame, stage and serialization still open. |
| Strict bounded straight Edge MID | **Architecture proposal, NOT IMPLEMENTED** | Typed Edge-local MID, strict native 3D metric query, orientation, viewport MID marker/pick, schema all absent today. |
| Strict open trimmed Arc MID/END | **UNPROVEN; STOP before code** | Native bounded arc-length midpoint and tangent, certified endpoint provenance, source stage and orientation; full circle remains Unsupported. |
| Half-cylinder ambiguous semantic Point at visible native Vertex | **Ambiguous, reject** | Four native realizations/two intrinsically Ambiguous groups proven; don't change Point status by Edge adjacency. |
| Fillet/Chamfer or full Revolve native unsupported endpoint | **Unsupported, reject** until separate provenance proof | No topology/XYZ guessing or automatic deferred G1. |
| Point-first two distinct Resolved overlapping meanings | **NOT TESTED** | All hit candidates retained, strict membership uniqueness; must differ from intrinsically Ambiguous *single* Point. |
| Source deletion, later-stage split/merge, stale token or provider mismatch | **Missing/Ambiguous/Blocked**, no mutation | Exact stage, invalidation, no stale last-good, correct one-transaction Undo/Redo/Save/cold re-evaluation. |

## 5. Rejected shortcuts

Do NOT (i) serialize raw XYZ, OCCT handle, \`TopoDS_Vertex\`, runtime index or presentation token; (ii) reinterpret MID as a fabricated three-Surface Vertex; (iii) infer END by native first/last parameter or Edge order; (iv) treat ambiguous Point + selected Edge as independent Resolved; (v) run 2D \`projectEdgeToPlane\` as substitute for the missing 3D query; (vi) silently mutate \`OffsetDatumPlane\` or v16 \`offset_plane\` schema; (vii) choose frame orientation from camera/provider UV; (viii) declare general END/MID ready because Windows FULL of **evidence-only tests** is 199/199 GREEN; (ix) merge #348, #345, this Draft, or activate D2 by a generic "continue."

## 6. Owner decision requested, narrowly

**D2 design consultation (not code authority):** Are the proposed separations **P1 existing independent Point vs Edge-local MID**, **P2 unsupported END remains fail-closed pending own provenance proof**, **P3 scoped provider-neutral exact 3D Line/open-Arc metric query**, and **P5 candidate-ledger semantic filtering** acceptable **in principle**, subject to later exact C++ signatures and evidence?

**Separate explicit decisions still needed:** P4 signed deterministic U/V/N frame, Reverse, line anchor and current v16 → new-version schema compatibility; full PR #345 §1B UX interaction specification; independently selected production contract and ACTIVE activation. Do not infer acceptance of these from accepting P1/P2/P3/P5 or from earlier E0 evidence.

**Scope/status close:** E0 tests remain active under existing contract; E0 PR #348 is **OPEN/UNMERGED** and requires PR-specific Owner merge consent separately. This proposal modifies **only one work document**, has **no runtime/test/code/CI, user-visible product or browser effect** and gives NO permission to change production. 

## Documentation impact

Internal docs: not required  
User/Product docs: not required  
Reason: This work-only draft records alternative architecture decisions and existing evidence for Owner consultation, without changing accepted semantics, production code, current-state documentation, runtime behavior, commands, UI or persistence. A future accepted/shipped API/Datum implementation will require current-state Internal documentation, paired PL/EN Product docs and deterministic generated Browser validation.
