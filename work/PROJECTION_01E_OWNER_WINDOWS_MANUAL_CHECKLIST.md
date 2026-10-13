# PG-01E — Owner Windows practical acceptance checklist (PREPARED / UNRUN)

**Status:** WORKING CHECKLIST, NOT OWNER PASS. **Date:** 2026-10-13. **Active Contract:** `work/PROJECTION_01E_PRODUCT_CLOSEOUT_WORK_CONTRACT.md` (PG-01E E0–E6). **Program:** Part roadmap v1.32. This checklist does not widen the accepted Project Geometry surface or replace exact-head Windows FOCUSED/FAST/FULL.

## 1. Traceable test session (Owner fills before execution)

- Windows OS / build architecture / GPU / OCCT environment: **UNRECORDED**.
- Exact tested executable/PR HEAD SHA (not `main` unless that binary was built from `main`): **UNRECORDED**.
- Build configuration and path: **UNRECORDED**.
- New test Part and `.ss2part` file path (created using current modeling semantics v2): **UNRECORDED**.
- Screenshots, relevant CTest native log/run IDs, and any typed error messages: **UNRECORDED**.
- Owner observed practical result: **NOT TESTED**. Date/time: **UNRECORDED**.

Do not use an old modeling-v1 Part as the positive authoring fixture. Reuse of the original legacy document is only for **read-only typed refusal** after preserving a byte-identical backup. Manual inspection cannot alone prove a fresh OCCT provider generation; match it to separate native automated evidence.

## 2. One reproducible supported Windows scenario

Execute in the native Windows CAD Workbench; note **PASS / FAIL / NOT APPLICABLE** and actual observations for every applicable row. Prepare a **new v2 Part** containing an earlier-stage ordinary Extrude Add body. To exercise supported Face outer/inner boundaries, add a separate current-stage body with a bounded Face with a hole using supported Add/Cut commands. Complex variants may be separate new Parts; never turn unavailable supported input into a fabricated result.

| ID | Action / accepted precondition | Expected observable result | Owner observation |
| --- | --- | --- | --- |
| W01 | Create a new Part, Sketch/Profile and **Extrude Add**, save as native Part; reopen it once | New Part is saved/reopened with native v16 and modeling-v2 semantics; material Body exists | UNRUN |
| W02 | Enter Sketch Edit supported on an Origin plane or current planar Body Face/Datum Plane; open **Project Geometry** from References or `PROJECT` | One Operations panel with modes **Edges / Planar Face / Face Boundary**, Regular/Construction and Finish/Cancel | UNRUN |
| W03 | Choose **Edges**, click a uniquely supported material Edge from a valid earlier Body stage; stage a second via **Ctrl + Left click** | Two current semantic material sources, exact projected Line/Circle/Arc images where applicable; no stale/approximate geometry | UNRUN |
| W04 | Switch between Edges and Face modes without committing; return to Edges, select **Regular**, Finish | Mode switch preserves already staged eligible sources; Finish authors all eligible targets in **one transaction**, with no Face dependency | UNRUN |
| W05 | Undo once, Redo once; select authored linked Sketch entities | Atomic undo/re-creation and stable linked identity/role; no copied seed treated as current geometry | UNRUN |
| W06 | Use existing Regular linked geometry to form a supported closed **Profile** and a downstream **Cut** Feature | Supported chain evaluates from the exact earlier Body stage; the later Feature is valid, not silently rebound to nearest geometry | UNRUN |
| W07 | Edit a supported upstream dimension/line and recompute the linked consumer | Dependent geometry updates from current resolved upstream source; if the Profile becomes nonclosed, later Feature fails visibly rather than consuming a stale seed | UNRUN |
| W08 | Restore a valid contour through supported explicit Sketch edits/repair and reevaluate | Same authored Profile/Feature intent is restored where supported; no automatic contour healing or silent source reassignment | UNRUN |
| W09 | Select currently resolved linked targets, invoke **Break Link**, then Undo/Redo | One atomic detach with preserved current geometry, EntityIds and roles; mixed invalid/unresolved batch does not partially detach | UNRUN |
| W10 | With suitable bounded **planar Face** (prefer outer boundary plus inner hole), enter **Planar Face** and select it | Exactly one bounded planar Face acquisition; outer and inner material boundary Edge uses; no durable persisted Face token | UNRUN |
| W11 | Stage the same material Edge manually in Edges mode and via Planar Face; Finish | Exact material Edge identity deduplicated once, without dedup by merely equal coordinates | UNRUN |
| W12 | Enter **Face Boundary** and manually select more than one bounded Face, including nonplanar only where genuinely available | Only selected Faces' individual outer/inner material Edge members included; no automatic neighboring-Face traversal; certified seam/partition artifacts excluded | UNRUN |
| W13 | For a fixture with an independently unsupported/degenerate **exact projection** member, inspect Operations and Viewer | Per-Face wire member typed `SKIPPED — geometric Unsupported/Degenerate`, local red source overlay; other supported members may remain PARTIAL; no ellipse/polyline approximation | UNRUN |
| W14 | In the same supported fixture trigger **zero supported** projection members, then attempt Finish | Finish disabled / typed `FINISH` rejected; no authored partial Sketch mutation; Clear/Cancel/Esc remain usable | UNRUN |
| W15 | Trigger a rejected stale-stage/provider or ambiguous Face/Edge identity gesture (only when reproducible from actual supported interaction) | Entire invalid gesture/Finish rejects; not converted into geometric skip; previously valid staged sources not silently destroyed | UNRUN |
| W16 | Repeat a valid Finish; Save, Close and Reopen the v2 Part and re-evaluate with actual new runtime provider | Durable links re-evaluate; separate automated evidence verifies provider freshness and stable semantic entity IDs; no saved OCCT/Viewer Face identities | UNRUN |
| W17 | Suppress or delete a referenced earlier source, then inspect later linked consumer and try Break Link | Broken source stays typed unavailable/unresolved; no fallback to saved seed, no silent retarget; unresolved Break Link refuses atomically | UNRUN |
| W18 | Invoke Cancel and check hierarchical Esc/Command Line behavior on a fresh Project Geometry draft | No authored mutation from Cancel; first Esc prioritizes active text/input/staging, later Esc exits according to current tool grammar | UNRUN |
| W19 | Narrow right column, inspect modes/controls; attempt concurrent Feature Edit and Sketch/Profile Tree Edit | Controls stack/wrap without clipped actionable labels, edit exclusivity preserves one valid context (UI-01 accepted behavior) | UNRUN |
| W20 | Observe exact supported v16/modeling-v2 cold Save/Reopen; separately open an **untouched** original modeling-v1 document, when provided | New v2 Part works; original v1 rejected with typed unsupported policy; no auto-migration or original-file rewrite. Record pre/post file hash separately | UNRUN |

For W06–W08, actual closed profile construction is a **prerequisite**; a deliberately open/partial Face Boundary Sketch does **not** automatically form a Profile. If a subcase requires an unsupported production feature or a source beyond the accepted Part stage, mark it **NOT APPLICABLE / CONTRACT GAP**, not as a false PASS or invitation to implement a new mode.

## 3. Resource and performance evidence (E3 — no invented thresholds)

Run the same supported saved v2 Part and edit/reopen scenario for a declared number of repetitions and warm-up cycles. Record machine/build, fixture size/Face/Edge counts, repetitions, elapsed observations and whichever process working-set/native object metrics are **actually measured** with a stated collection method. Capture before/after and any reproducible growth; explicitly mark unavailable metrics **UNAVAILABLE**. This evidence alone does not accept PM-06 performance budgets or authorize optimization.

**Measured setup:** UNRUN. **Cycles/warmup:** UNRECORDED. **Elapsed/resource data:** UNRECORDED. **Trend interpretation:** OPEN.

## 4. Strict final acceptance and STOP

- **No individual Owner observations have been supplied for this PG-01E run.** All rows are UNRUN; previous PG-01C/D practical PASS does not fill them in.
- Cross-check every required E1/E2 negative and positive with **real native OCCT/Qt test evidence**, exact **source SHA/run ID**, and the independent fresh-provider/EntityId checks. The core-only `pm04e2` test double is not real OCCT proof.
- Final gate: required newly authored FOCUSED, applicable FAST and **one exact-head Windows FULL** with Core + native Kernel + Desktop + docs + aggregate; any RED/P0 => STOP and Owner D2 for required out-of-contract mutation.
- **Owner FINAL PASS for PG-01E** and **separate authorization to merge the evidence PR** must be explicit. Neither is granted by this checklist.
- No RG-02 Vertex UX, UI, schema, G1, PM-06 or CI #302 changes under PG-01E.

## Documentation impact

Internal docs: not required — this is an unexecuted Owner test procedure, not current implementation description.
User/Product docs: not required — existing supported commands and caveats are transcribed from the current accepted PL/EN Part guides.
Reason: No product behavior, canonical docs or generated Browser is changed by this work report.
