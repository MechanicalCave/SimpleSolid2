# PG-01D — Planar Face boundary Project Geometry: bounded D2 design PROPOSAL

**Status:** **OWNER D2-F1–F6 AND BOUNDED WORK CONTRACT APPROVED 2026-10-09 — ACTIVATION PENDING.**  
**Prepared:** 2026-10-09, following Owner FINAL PASS and squash merge of PG-01C.  
**Verified baseline:** `main` merge commit `d928e8a5d1f5893b73de3e4fcd399d7e0483af83`, [PR #304](https://github.com/MechanicalCave/SimpleSolid2/pull/304), original accepted HEAD `95731b9a4`, Windows FULL #2102 **196/196 PASS** and explicit Owner manual PASS.  
**Program:** accepted Part Modeling v1 roadmap v1.30 §9B; `work/PROJECTION_01_PROJECT_GEOMETRY_DESIGN_PROPOSAL.md` PG-01D.  
**Invariants:** Constitution v1.0, Foundation v1.0 §7.2/7.5, Architecture v1.0, ADR-0014/0016/0017, PG-01A/B/C accepted behavior.  
**Authority boundary:** Owner explicitly accepted D2-F1–F6 in conversation: `akceptuje - kontynuuj` (2026-10-09), after receiving the D2 proposal and request for approval. This accepted **the bounded architectural design and the default visible partial-warning UX** (no modal). The Owner subsequently explicitly accepted the bounded `work/PROJECTION_01D_PLANAR_FACE_BOUNDARY_WORK_CONTRACT.md` on 2026-10-09. No new schema or permission to edit production exists **until a distinct `ACTIVE.yaml` activation commit is merged**.

## 1. User outcome and package boundary

Extend the **same existing Project Geometry tool** with an explicit **planar Face** source-selection mode. The Owner selects one current **bounded material Face** on a legal earlier Body stage; the tool discovers its **outer boundary and hole wires**, creates a disposable exact Line/Circle/Arc projection for every eligible material boundary Edge, visibly lists individually skipped unsupported curve types, and commits the supported set as linked Sketch entities in **one all-or-nothing semantic Finish transaction**. No new Part/Sketch tool, no second right-panel state machine and no automatic gap healing.

One Face may contain more than one wire and may have a partial supported result. A partial Face projection need not form a valid Profile; the user may repair the open contour explicitly. Existing manual material Edge selection remains available and can be staged together with one Face selection, subject to strict deduplication.

**Excluded:** nonplanar Face selection, spline/ellipse approximation, polyline persistence, broad OCCT/B-Rep healing, automatic topological Face membership updates on upstream feature edit, cross-Part/Assembly links, automatic closure, automatic semantic rebinding, new Sketch constraints engine, PG-01E and PM-06.

## 2. Owner D2 choices to freeze before production

### D2-F1 — source identity: bounded Face gesture, durable Edge links

**Proposed choice:** Face is a **transient batch-expansion gesture**, not a new persisted link type. At the current declared BodyStageRef, a Viewer-picked bounded Face must identify **one exact semantic Face realization**, separately from its Surface carrier. Resolve it only in the current provider/Part topology snapshot. Failure to obtain a unique material Face is Missing/Ambiguous/Unsupported and blocks the Face gesture; a Surface that spans multiple Face fragments cannot be used to guess which one the user clicked.

Each admitted output is persisted using the **already accepted** PG-01B `SketchId + EntityId → MaterialEdgeReference` on that *individual* material Edge. The selected Face token, its transient wire/Edge enumeration, OCCT handles, Face ordinals, visual triangle IDs and loop positions are **never durable CAD intent**. This preserves v15 compatibility. If obtaining strict bounded Face meaning would require a new public semantic selector/API, stop and raise a second explicit D2 before altering ADR/Part persistence.

**Tradeoff:** This intentionally does **not** automatically add new edges when the source Face's perimeter changes in future. Previously stored Edge links continue to update or fail closed according to PG-01B/C. Re-run Project Geometry to capture changed membership.

### D2-F2 — authoritative boundary enumeration, not cosmetic outline

**Proposed choice:** the Kernel/OCCT provider owns transient interrogation of the **actual selected TopoDS_Face** and its oriented wires. Return a provider-neutral, revision/stage-bound description of each outer/hole boundary use and its actual Edge occurrences. Determine wire/outer-hole semantics by native topology with a validated planar Face frame and face orientation, never by tessellation, screen-space projection, nearest-point clustering, sorted point coordinates or arbitrary traversal index.

Every provider Edge occurrence must be mapped back through the Part's **current complete semantic topology catalog** as a uniquely identified **material Edge** before authoring; an Edge carrier is not automatically a singular material boundary occurrence. Edges classified as representation-partition artifacts under ADR-0017 cannot be silently promoted to Project Geometry material sources. The system must report noneligible occurrences without inventing identifiers or missing a supported material perimeter.

**First evidence gate:** native OCCT experiment with a planar cap, a planar cut opening/hole, reversed Face orientation, split Face realization, and partition/seam cases. Prove that the reported outer and hole loops really correspond to the picked bounded material Face and every eligible Edge has exactly one strict stage-scoped semantic source. If not, **STOP D2**, do not ship a partial guessed algorithm.

### D2-F3 — exact curves and typed partial skip

**Proposed choice:** for each **uniquely resolved eligible** material Edge call the same exact PG-01A/C projection contract, preserving Line/Circle/Arc and the existing endpoint identity policy. Only the distinct class of *known semantic source + geometrically unsupported curve/projected image* is **skippable per Edge** and must yield a visible reason and highlighted skipped occurrence. Skipped unsupported Edges do not appear in the authored batch.

`Missing`, `Ambiguous`, provider loss, wrong BodyStage, identity mismatch, cycle, stale selection/revision or invalid Face boundary accounting are **integrity failures**, not equivalent to geometrically unsupported curves; these block the affected Face gesture/Finish and preserve previously staged valid manual Edges. Never partially commit a failed semantic transaction. A Face whose accepted supported count is zero cannot enable Finish on its own; report why.

### D2-F4 — deduplication and ordering

**Proposed choice:** staged source deduplication uses the **strict equal semantic MaterialEdgeReference within the same Body stage**, not positions, curve coincidence, TopoDS equality beyond the provider read boundary, Viewer pick token equality or point proximity. The same material Edge seen through manual Edge pick plus Face boundary (or multiple Face wires) appears once in the accepted authoring batch. Preserve deterministic relative ordering of accepted loop descriptions for preview/user diagnostics, but do **not** persist wire order or make EntityIds depend on provider enumeration order.

If two distinct semantic material Edges happen to overlay exactly, do **not** merge them. Explicitly diagnose ambiguous source identity where applicable.

### D2-F5 — one Workbench state and explicit partial-result UX

**Proposed choice:** reuse PG-01C's single transient `Project Geometry` tool state, the Sketch toolbar launcher, right Operations, Command Line, shared Finish/Cancel/Undo, and exact-current Viewport source selection. Add a mode toggle **Edges / Planar Face** in the existing Operations rather than a second tool or independent dialog.

On Face pick, show the selected source stage/Face admission status, outer/hole loops, selected supported material Edges, skipped counts and **per-Edge skip reason** (with visible source highlight). A partial supported result must be explicitly disclosed **before** Finish; do not claim the boundary is closed when unsupported arcs/curves leave gaps. `Clear`, `Remove`, text-buffer-first Esc, second Esc, `CANCEL`, Document switch and revision/provider changes keep the accepted PG-01C focus/generation discipline.

**D3 UX point for Owner:** confirm that skipped geometric Unsupported is **visible warning, Finish still permitted for the supported subset** (program-level accepted direction) while any semantic integrity issue blocks that Face as a whole. No modal Yes/No gate is proposed. Owner may require an explicit partial-Finish acknowledgment; if so, define it before implementation.

### D2-F6 — transaction, freshness, persistence and rollback

**Proposed choice:** Face expansion and preview are **ephemeral**. On Finish, revalidate selected Face at the current revision/stage, re-enumerate the exact source boundary, compare intended admitted material Edge identities and Unsupported classes, and use one PG-01B/C batch `CreateProjectedSketchEdgesCommand` to commit the accepted current `MaterialEdgeReference` set or nothing. A changed Face membership or skip classification since preview triggers a stale draft diagnostic, **not** silent recomputation into a different unseen set.

One Undo/Redo entry, stable targets, construction/regular role parity, and current provider-derived geometry as in PG-01C. Existing v15 Part persistence and runtime source evaluation remain unchanged. Cold Save/Close/Reopen must not require Face token persistence. If this cannot be achieved without broader application/kernel APIs, enumerate the specific contract change as an Owner D2 STOP; do not add a Face token field to native storage by default.

## 3. Proposed authorized scope, only if later accepted/activated

- **Kernel + OCCT:** narrowly scoped typed **read-only bounded Face-wire query** and native provider implementation; no B-Rep mutation or general topology healing. Expected candidate modules: `src/kernel/**`, `src/kernel_occt/**`.
- **Part:** read-only strict current Face/Edge stage mapping and transient topology diagnostics if existing catalog adapters are insufficient; `src/part/**` only with explicit bounded API scope and **no durable schema changes**.
- **Application:** transient Face batch validation and existing atomic Edge authoring Command reuse; exact revision/provider checks; `src/application/**` narrowly bounded.
- **UI + Viewer:** extend the **existing** Workbench/Sketch controller and provider-neutral Face pick/diagnostic presentation; no second selection model; `src/ui/**`, `src/viewer/**` only as necessary for the accepted interface.
- **Tests/docs/work:** synthetic semantic/kernel/real Qt-OCCT regressions; current internal as-built + bilingual PL/EN Product documentation and deterministic Browser once shipping behavior is implemented.
- **No** `work/ACTIVE.yaml` edit or production implementation on this proposal branch. New Owner-accepted Work Contract with explicit per-file boundaries + dedicated `ACTIVE.yaml` activation commit and governance verification must precede a separate production feature branch.

## 4. Mandatory acceptance and RED→GREEN plan

**D0 evidence before D2 implementation choice is finalized:**
1. Read-only current topology inventory: which specific Face catalog records are strictly Face-addressable and material, vs merely Surface-carrier-backed or Ambiguous; verify stage-scoped Face pick without provider ordinal persistence.
2. OCCT-only synthetic planar Cap/Cut/Chamfer Face-wire inventory with outer perimeter and one/multiple hole loops, reversed orientation and surface split/partition. Validate exact counted occurrences and stable semantic mapping, or STOP.
3. Negative cases: nonplanar Face, unsupported geometric curve on a uniquely identified material Edge, ambiguous source semantic Edge, partition artifact and mixed material/manual selection; distinguish source-integrity failure from skippable unsupported image.

**Production test matrix after separate Owner D2 approval + activation:**
- A single selected planar Face with one closed supported perimeter yields one valid linked Profile with the same current-source rules as PG-01C.
- An outer wire with a genuine hole wire projects all supported exact Line/Circle/Arc members with correct hole/region semantics and no fabricated closure.
- One exact Unsupported boundary member is omitted **visibly**; other admitted Edges remain available for one atomic Finish; any open contour remains open until explicit user repair.
- Two staged source paths naming the **same** semantic material Edge deduplicate once; two different overlapping Edges do not collapse.
- Missing/Ambiguous/stale/invalid Face-source identity, malformed native wire accounting, wrong stage, source suppression/provider loss reject safely without mutation or clearing unrelated valid staging.
- Undo/Redo one batch; Break Link one/many; existing ProfileId/RegionIntent and stable EntityIds; upstream edit, rebuild, cold Save/Close/Reopen with fresh OCCT, and no persisted Face source token.
- Full PG-01C toolbar/Operations/Command Line/Esc/Enter/Cancel/focus and ordinary linked Edge work remain regression-green.
- Native Windows **FOCUSED → FAST** (record any independent #302 issue without weakening tests), exact-head **FULL**, docs/Browser verification and **separate Owner practical Windows PASS** before merge.

## 5. Owner-approved D2 design — remaining Work Contract gate

**Owner decision received:** `akceptuje - kontynuuj` (2026-10-09) approves **D2-F1 transient Face gesture + durable per-Edge bindings**, **D2-F2 provider-owned oriented outer/hole wire accounting + strict material mapping**, **D2-F3 only geometric Unsupported skip**, **D2-F4 semantic-reference deduplication**, **D2-F5 existing single Workbench tool with prominent partial warnings and no added modal confirmation**, **D2-F6 exact-current atomic Finish without Face-token persistence**.

**Remaining governance gate:** the Owner accepted `work/PROJECTION_01D_PLANAR_FACE_BOUNDARY_WORK_CONTRACT.md` on 2026-10-09 (`Zatwierdzam Work Contract PG-01D — kontynuuj`); next is a **distinct activation commit** to `work/ACTIVE.yaml` on a governance branch; only then may a dedicated production PG-01D branch mutate code. If the native F2 evidence is insufficient to map strict bounded Face and actual oriented wire occurrences, implementation **STOP / new Owner D2**. PG-01E / PM-06 remain gated.

## Documentation impact

Internal as-built docs: **not required** for this design-only proposal (it does not describe implemented behavior).  
PL/EN Product docs: **not required** for this design-only proposal (no user-facing feature changes).  
Generated Browser: **not required**; canonical as-built/Product sources are unchanged.  
**For an eventual production PG-01D Work Contract:** internal as-built, Product PL and EN, generated Browser and full verification are **required**.
