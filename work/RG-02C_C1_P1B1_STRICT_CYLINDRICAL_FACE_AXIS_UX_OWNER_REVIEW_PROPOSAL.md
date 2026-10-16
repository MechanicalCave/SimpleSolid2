# RG-02C/C1-P1-B1 — strict singleton cylindrical Face Axis: existing Work Features UX consultation

**Status: OWNER DIRECTION A ACCEPTED (2026-10-16); NEW CYLINDRICAL-FACE UX DESIGN IS PRESENTED FOR SEPARATE EXPRESS OWNER SIGNOFF — NOT YET UX-ACCEPTED / NOT CODE ACTIVE.**
**Baseline:** protected `main` `b14c6821c8707e57f7b9a8ad8cacf348a6a54dba`. Owner expressly said `akceptuje i akceptuje implementację i merge - kontynuuj pracę i nie przerywaj` following the Agent's recommendation of **architecture A**. This establishes Owner direction and delivery intent, **not** prior review of the concrete additional UX below or approval to bypass exact D2, `ACTIVE.yaml`, Windows and protected-main gates.
**Authority:** Constitution/Foundation/Architecture 1.0, accepted ADR-0014/-0016/-0017, roadmap v1.33 RG-02, [RG-02 product direction](RG-02_REFERENCE_GEOMETRY_EXTENSION_WORK_CONTRACT_DRAFT.md) §2.3 and §3 mandatory **separate prior Owner approval of ANY new RG-02 UI behavior**, [existing Owner-approved Edge UX](RG-02C_B1_SINGLE_STRAIGHT_BODY_EDGE_AXIS_EXISTING_TOOL_UX_CONSULTATION_PROPOSAL.md) (one `Edge` constructor at that stage), accepted [P1-B0 scope](RG-02C_C1_P1B0_FACE_SURFACE_ADMISSION_NATIVE_EVIDENCE_D2_PROPOSAL.md) and [merged P1-B0 native evidence](RG-02C_C1_P1B0_FACE_SURFACE_ADMISSION_NATIVE_EVIDENCE.md) / #438.
**Status of B0:** technical FINAL PASS with explicit GAPs and separately Owner-authorized guarded squash, not a demonstrated multi-Face same-cylinder Surface partition-positive. This A proposal **rejects** such a partition rather than presuming B's missing proof.

## 1. Exact new UX review decision — separate from architecture/implementation approval

Introduce a **second** entry `Cylindrical Face` in the **existing** `Work Features → Axis → Operations → Constructor` dropdown, beside current `Edge`; keep `Edge` the default to preserve prior UX. This does **not** add an Axis button, a second Axis panel or a second draft authority. `Sketch Line` is **NOT** a Work Features constructor — its Axis is still created inside Sketch and nested under that Sketch. Existing Edge-sourced Axes and new Face-sourced Axes both appear under `Reference Geometry`, each once, with unchanged `AxisId` identity/visibility/Tree semantics. Built-in Origin axes remain under Origin.

**Source:** pick exactly one **current Body bounded Face** that the native OCCT query independently certifies as an analytic cylinder. The authored source at the declared `BodyStageRef` is the Face's **one** semantically certified, uniquely Resolved cylindrical `SurfaceReference`, and the **strict bounded Face resolution must also be Resolved with exactly one current Face realization** at the source stage. User-visible pick remains `Face`; durable data is the stage-qualified semantic Surface (not `TopoDS_Face`, runtime token, Face ordinal, JSON coordinates or an operation-origin heuristic). After a Boolean partition into several Faces, the Axis must show **Unresolved / Ambiguous — repair required**, even if all members share an exact cylinder. No automatic carrier-wide fallback or geometry-nearest rebinding.

**Geometry:** display one finite neutral cue of the exact **infinite unoriented** world-space axis, after the source passes strict identity and native `IExactCylindricalFaceAxisQuery`. Keep the actual Face highlighted using the existing Body Face selection grammar. No Axis-level `Reverse`, segment endpoints, length, side-of-cylinder, role conversion or sign checkbox. Revolve owns Reverse/direction exactly as it does with existing Axis sources. A partial/trimmed cylinder is eligible if and **only if** its strict Face is singular. Planar, conical, toroidal, nonanalytic or ambiguous/stale sources fail closed, with no last-good preview.

### W1 — create, no source (reviewable mockup, NOT implemented)

```text
Work Features: [Datum Plane] [Axis]         [Extrude] [Revolve] ...
Tree                 3D Viewport            Operations — Axis
 Origin                [current Body]        Constructor [Edge | Cylindrical Face ▼]
 Reference Geometry                         Source Face [Select Face…]
   DatumPlane001                             Body stage  [current stage]
 Sketches                                    Status      Select one analytic cylindrical Face
   Sketch 3                                  Preview     None
     Axis001 (Sketch Line)
                                             [Finish Axis — disabled] [Cancel]
```

When `Cylindrical Face` is chosen, the existing `Axis` tool enters **Face-only** pick mode, not Edge mode. The current source is cleared; switching constructor must not carry an Edge token/preview into Face meaning. The source slot is **one** (no multiple picks).

### W2 — actual strict cylindrical Face selected (mockup, NOT implemented)

```text
Tree                      3D Viewport         Operations — Axis
 Reference Geometry       [selected Face]     Constructor  Cylindrical Face
   DatumPlane001          [Axis line cue]     Source Face  Selected (Replace)
 Sketches                                     Body stage   After Feature …
   Sketch 3                                  Strict Face  Resolved (1 of 1)
     Axis001                                  Axis status  Resolved — infinite, unoriented
                                             [Finish Axis] [Cancel]
```

All preview, source text and Finish readiness derive from **the same current draft evaluation**, checked against owning DocumentId, current revision, Body stage and provider-generation. Hover is nonauthoring; a click applies a new candidate but never performs implicit `Finish`. When a selected Face's carrier has `strict_face_status=ambiguous` or `current_faces.size()>1`, display: `Ambiguous cylindrical Face — select/repair a strict singleton; Finish disabled` and clear preview. For `SurfaceKind::cylinder` without positive exact native Face query, show `Exact cylinder geometry unavailable`, not an inferred axis.

### W3 — committed / Edit / Repair (mockup, NOT implemented)

```text
Tree                       Properties — Axis         Operations — Edit Axis
 Reference Geometry         Axis002                   Constructor Cylindrical Face
   DatumPlane001            Source: Cylindrical Face  Source Face [Replace Face…]
   Axis002 (Face source)     Status: Resolved / ...    Stage: After Feature …
 Sketches                   Visibility: [Shown]       [Finish Edit] [Cancel]
   Sketch 3
     Axis001 (Sketch Line — unchanged)
```

`Edit Axis` preserves its `AxisId` and downstream `AxisReference`. A `Replace Face` operation may explicitly acquire **one different certified semantic source** at a legal stage; never silently choose a Face due to geometric similarity or current provider enumeration. For a deleted/suppressed source or partitioned current Surface, show a typed repairable diagnostic and disable `Finish` until a legal replacement is chosen. Deleting an Axis leaves dependent Revolve authored but fail-closed; Undo restores; no persisted last-good native geometry.

## 2. Exact UX acceptance matrix (proposed; nothing newly witnessed)

| UX | Manual / native acceptance condition |
| --- | --- |
| UX-01 | Exactly existing one `Axis` toolbar entry by `Datum Plane`; `Constructor` has `Edge` (default), then `Cylindrical Face`; NO `Sketch Line` item |
| UX-02 | Switching Edge ↔ Cylindrical Face drops transient incompatible source/preview; zero authored mutation; no unapproved extra constructors |
| UX-03 | 3D current Body Face click, unique stage semantic owner + strict singleton + exact cylinder required; same current Body generation |
| UX-04 | Full/partial genuine cylindrical Faces recognized by actual native geometry; full/partial circular Extrude/Revolve and Boolean trim when strict singleton |
| UX-05 | Noncylinder, multi-Face owner ambiguity, missing/stale/foreign Body/stage, wrong current generation — typed refusal, no false source or last-good preview |
| UX-06 | Existing panel/viewport line cue uses infinite *unoriented* line, finite display only; no Axis Reverse; Revolve Reverse unchanged |
| UX-07 | Finishing creates ONE Part-owned semantic AxisId transaction; Cancel/Esc/hover have zero authored/Undo changes |
| UX-08 | Edit/Replace Face retains AxisId, visibility and downstream Revolve links; Cancel preserves preedit state; failing repair does not silently rebind |
| UX-09 | New Face Axis in Reference Geometry, Sketch-Line Axis inside source Sketch, built-in Origin axes unchanged; mixed group visibility |
| UX-10 | Save/Close/Reopen/cold provider rebuild uses only durable `SurfaceReference`+stage; no persisted Face token/line or geometry-nearest recovery |
| UX-11 | GUI and existing owning CAD Command Line `SOURCE`, `FINISH`, `CANCEL`, Enter/Esc converge on one draft authority; no new alias token without separate UX review |
| UX-12 | `Edge`, `Datum Plane`, Sketch Axis, Revolve profiles, one-row Work Features and responsive 40/60→tab layout remain intact |
| UX-13 | No phantom Axis preview on source suppression, Body regeneration, partition, Undo/Redo, Delete; repair stays explicit |
| UX-14 | Full real Windows native FOCUSED/FAST/FULL, DOC-01, manual Windows owner review on committed UX with explicit GAP accounting |

## 3. Design alternatives deliberately excluded

No new `Axis from Entire Surface` option (B), no provider-face ordinal/patch discriminator (C), no default to first Face of partition, no `Sketch Line` constructor entry, no new `Axis` toolbar button, no signed native direction or auto-Reverse. Multi-Face cylinder `strict_face_status=ambiguous` is **rejected by direction A**, not treated as a B0 PASS. The Owner acceptance of architecture A and a wish to implement/merge it do **not** constitute having reviewed or expressly accepted W1–W3 and UX-01…14 before they were presented. This separate UX signoff is mandatory under RG-02 §3; the existing Owner acceptance for `Edge` does not silently extend to the new constructor.

## 4. Required sequencing — the Draft PR is not an implementation or merge

1. Owner review of **this exact new UX** (Constructor `Edge` + `Cylindrical Face`, stage-scoped strict singleton, preview/repair/Tree/CLI behavior) and separate explicit signoff or correction.
2. Separate acceptance and governance squash of a narrowly bounded P1-B1-H1 technical Work Contract with exact implementation allowlist, native v20 persistence/legacy v19 plan and native tests; then independent `ACTIVE.yaml` activation.
3. Headless semantics/schema and native gates before UI mutations; product GUI only **after** UX signoff; exact docs/PL+EN/Browser DOC-01 and owner manual Windows review.
4. Technical FINAL PASS with honest native gap decisions, then **independent** guarded squash approval on verified exact production HEAD. No auto-merge because Owner previously said `merge` in general.

**This PR is one new `work/**` UX review file only. No `src/**`, tests, schema, product docs, `ACTIVE.yaml` or generated Browser mutations. No product native tests are claimed from governance CLOSURE.**

## Documentation impact

Internal docs: not required
User/Product docs: not required
Reason: this is a work-only **unapproved UX design review**. A later activated shipping implementation must explicitly update internal as-built, bilingual Axis documentation PL/EN and generated-only DOC-01 Browser, never hand-edit `docs/browser/index.html`.
