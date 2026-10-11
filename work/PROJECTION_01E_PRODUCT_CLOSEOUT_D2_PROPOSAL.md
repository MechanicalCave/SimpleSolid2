# PG-01E — Project Geometry product-wide closeout, D2 proposal (NOT ACCEPTED)

**Status:** PROPOSAL ONLY — awaiting explicit Owner D2 scope decision and a separately accepted bounded PG-01E Work Contract. **NOT ACTIVE; NO PRODUCTION AUTHORIZATION.**  
**Prepared:** 2026-10-11  
**Baseline:** `main` `5bec84370dbfdba44c5cdda677aaaaf028456ed2` after PG-01D implementation [PR #309](https://github.com/MechanicalCave/SimpleSolid2/pull/309) and historical closure [PR #310](https://github.com/MechanicalCave/SimpleSolid2/pull/310) were both squash merged.  
**Program:** accepted `work/PART_MODELING_V1_ROADMAP.md` v1.30, milestone **9B Projection** before **PM-06**; source staging proposal `work/PROJECTION_01_PROJECT_GEOMETRY_DESIGN_PROPOSAL.md` labels **PG-01E product-wide closeout**.  
**Authority:** Constitution and Foundation v1.0, Architecture, ADR-0014/0016/0017, completed PG-01A–D, documentation governance. No change to frozen authority in this proposal.

## 1. Existing accepted facts — do not reopen as design questions

- PG-01A exact Line/Circle/Arc geometric projection, PG-01B associative Part/Sketch derived evaluation and Part persistence, PG-01C integrated Edges tool/lifecycle, and PG-01D Planar Face + manual Face Boundary are **completed** under their accepted scopes.
- PG-01D Owner practical FINAL PASS plus exact feature HEAD Windows [FULL #2440](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38136149906) **199/199 PASS** (Core 25, Kernel 57, Desktop 117). Implementation PR #309 merged as `5adc94f1463ad1c5b0785b3027dc5d93f830a112`; closure PR #310 merged as `5bec84370dbfdba44c5cdda677aaaaf028456ed2`. This is accepted **prior-package** evidence, not evidence for a future PG-01E implementation.
- One Project Geometry tool, **Edges / Planar Face / Face Boundary**, right Operations/Command Line grammar and current-stage source validation; Regular/Construction and linked Break Link; one atomic Finish and Undo; native Face-wire read and manual multi-Face partial support. The selected bounded Face remains a **transient gesture**, never an authored persistent Face link. Unsupported exact projection may be skipped **per independently certified Edge**; invalid semantics/provider, stale stage/revision, or identity ambiguity must **fail closed**.
- Production default same-domain Refine after **Extrude Add / Revolve Add only**; exact material equivalence and strict semantic lineage. **ModelingSemanticsVersion=2** is required independently of native Part container schema **v16**. Old persisted modeling v1 files are **typed rejected**. Owner accepted **no automatic/bulk/offline migration** and no silent retargeting. The early staging proposal's phrase "migration" cannot overrule this later explicit D2-R1–R4 decision.
- Existing CI issue [#302](https://github.com/MechanicalCave/SimpleSolid2/issues/302) (FAST build/CTest registry mismatch) is **independent**, not PG-01E implementation scope and not a PASS waiver.
- `work/ACTIVE.yaml` still names the **completed** PG-01D contract for historical traceability. This provides **no PG-01E mutation authority**.

## 2. Proposed PG-01E purpose and bounded deliverables

**D2-E1 — Final capability/requirements traceability and gap inventory.** Build a versioned evidence matrix for all committed PG-01A–D requirements to exact existing tests/CI runs, current PL/EN product behavior and one reproducible manual Windows procedure. Identify genuinely untested cross-package interactions without making those interactions new features. Distinguish already proven tests from pending PG-01E evidence. Every unresolved gap is tracked as **open**, never implicitly waived.

**D2-E2 — End-to-end native regression / repair.** On **newly created modeling-v2 Part documents**, drive supported ordered chains: earlier-source `Extrude Add` / `Cut`, `Revolve Add` / `Cut`, Fillet/Chamfer and supported Sketch-on-Face/Datum, followed by mixed Edges + Planar Face + Face Boundary projection. Exercise Regular linked Profile feeding a later Feature, supported upstream dimension edits, explicit broken-link diagnostics and repair or Break Link when supported, one atomic Finish, Undo/Redo, Delete, explicit Save/Close/Reopen with a **new native OCCT provider**. Keep stable source/target entity identity; do not rebind by nearest geometry or persisted OCCT token. Negative cases: ambiguity, stale revision/provider, invalid Face wire, true material boundary vs representation partition/seam, unsupported exact curve, no eligible projected Edge, linked Profile after source loss, and unsupported lifecycle operations. No arbitrary "all CAD geometries" guarantee.

**D2-E3 — Modeling-v2 compatibility matrix, not migration.** Record separate axes: native Part schema v1–v16 reader guarantees **where semantics permit**, modeling semantics v1 vs v2, native new v16/v2 Save/Reopen, imported/persisted legacy refusal. Verify typed `unsupported_modeling_semantics` for old v1 documents, including immutable Owner v16/v1 fixtures, and explicit unsupported future versions; never rewrite, repackage or migrate originals in the product. Test-only v2 reconstructions remain clearly identified as test fixtures. No user-facing migration wizard, data backfill, broad reader compatibility rewrite, or resave-on-open.

**D2-E4 — Resource/performance evidence, not premature budgets.** Capture a deterministic Windows native reference baseline for Project Geometry execution, source-stage recompute/linked consumer evaluation, repeated edit/undo/reopen, and any obvious retained native/UI resource growth. Record machine/build/geometry/test shape, number of runs and elapsed/resource methodology; characterize regressions with reproducible evidence. Proposed PG-01E does **not** freeze general Part-v1 performance budgets or optimize unrelated Viewer/Kernel paths; that remains separately scoped PM-06. When a concrete P0 regression is found, STOP and present a bounded remediation decision rather than silently expand scope.

**D2-E5 — Current documentation parity and Owner acceptance.** Audit existing `docs/product/pl/PARTS.md`, `docs/product/en/PARTS.md`, relevant `docs/internal/**`, and deterministic `docs/browser/index.html` for accepted Edges/Planar Face/Face Boundary, partial unsupported vs integrity failure, strict v2 Save/Reopen/refusal and Refine Add scope. Update canonical Markdown and regenerate Browser **only if discrepancies are actually demonstrated**. Produce one reproducible supported-Windows manual acceptance checklist, aggregate evidence table, typed exclusions and an explicit Owner FINAL PASS gate.

**D2-E6 — Exact-head release verification.** Bounded FOCUSED native tests for newly found test gaps, FAST for the suite targeted by any changed code and **one exact-head Windows FULL** including all Core, native Kernel, Desktop, docs and aggregate required jobs. Historical PG-01D FULL #2440 remains immutable prior acceptance; it is **not** substituted for PG-01E exact-head verification. Explicit Owner final manual PASS and **separate merge approval** are still required.

## 3. Proposed phased execution, after explicit Owner acceptance and activation

| Gate | Work | Exit / STOP |
| --- | --- | --- |
| E0 | Collect final PG-01A–D evidence map, inventory exact unsupported variants and affected as-built docs | Missing acceptance obligation is OPEN; no fabricated PASS |
| E1 | Add strictly scoped cross-feature native replay tests to existing suited targets; baseline negatives before changes | Failure characterized by owner module, no arbitrary provider fallback |
| E2 | Resolve *only* expressly accepted blocking lifecycle/integration defects with bounded amendment when necessary | Architecture/API/persistence/semantics delta => separate Owner D2 |
| E3 | Capture repeatable Windows resource/performance evidence and docs/Browser parity | No unapproved performance policy or user-visible promise |
| E4 | Run FOCUSED, FAST if applicable, exact immutable Windows FULL; execute Owner practical test; publish final matrix | Any RED or unresolved P0 => STOP; FINAL PASS and merge separately |

**Expected no-op case:** If E0 establishes the existing implementation and native coverage already satisfy the accepted matrix, PG-01E may be **evidence/documentation-only**. Do not manufacture a code change to justify the package.

## 4. Explicit exclusions / escalation

No new Project Geometry source mode, no automatic multi-Face adjacency/perimeter stitching, no nonanalytic spline/ellipse support, no topology healing or loose tolerance, no new durable Face identity, no global Cut/Fillet/Chamfer Refine policy, no automatic migration of modeling v1, no general provider public-contract expansion, no unrelated #302 repair, no Assembly/PM-06 work, and no changes to frozen Constitution/Foundation without separately approved authority.

Any required new native schema/modeling semantics, Part/Kernel public API, identity, forward/backward-compatibility promise, test infrastructure change, or serious unknown must be reported with exact failing evidence and a **separate bounded Owner D2 amendment BEFORE implementation**.

## 5. Governance and acceptance questions for Owner

1. **Approve scope D2-E1–E6?** In particular: PG-01E primarily closes proof and cross-package lifecycle, not an open-ended feature package; demonstrated defects need a bounded remediation decision.
2. **Approve legacy compatibility interpretation?** "Migration matrix" in the historical PG-01E staging line means a **compatibility/refusal test matrix only**. No automatic migration and no reopening modeling v1 are authorized by PG-01E.
3. **Approve performance scope?** Gather reproducible measurements and resource-growth evidence, without freezing broad Part-v1 budgets (PM-06 ownership).
4. **Approve stage boundary?** PG-01E remains before PM-06; no package activation or default-branch merge is implicit in approving this design proposal.

**Next action after D2 proposal approval:** draft a separate bounded `work/PROJECTION_01E_PRODUCT_CLOSEOUT_WORK_CONTRACT.md`, including detailed exact test matrix and allowed files; obtain explicit Owner Work Contract approval; then perform a **separate governance activation PR on main** changing `work/ACTIVE.yaml -> active_work`. Only after activation merges may a PG-01E implementation/evidence feature branch be opened.

## Documentation impact

Internal docs: **not required** — this document is a design-only proposal and does not alter implemented behavior or ownership.  
User/Product docs: **not required** — this document adds no supported product capability or user workflow.  
Generated Browser: **not required** — canonical Product/as-built content is unchanged.  
Any later approved PG-01E implementation must reassess documentation impact and preserve PL/EN parity and deterministic Browser generation.
