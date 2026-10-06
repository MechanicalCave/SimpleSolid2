# PM-04F — Axis Designation UX Amendment

**Status:** ACCEPTED — OWNER 2026-10-06  
**Decision class:** D2 bounded PM-04 product/authoring amendment  
**Parent Work Contract:** `work/PM-04_AXIS_REVOLVE.md`  
**Program authority:** `work/PART_MODELING_V1_ROADMAP.md` v1.25  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Baseline before amendment:** main `9e52e16c71d192ddb9355ed6eb59862dff36ac5f`  
**Checkpoint:** PM-04F — pre-closeout UX remediation, final exact-head verification, documentation and Owner Windows acceptance

## 1. Trigger and purpose

Owner Windows review before final PM-04 acceptance found that the implemented Axis architecture is sound but the authoring surface exposes the wrong user model.

The existing top-level **Axis** tool presents a Part-owned Sketch-Line Axis as if it were an independent geometric construction tool. In ordinary Part mode the source Sketch geometry is normally hidden, while the only authored Axis constructor in PM-04 is one existing Sketch Line. This makes the toolbar action difficult to discover and conceptually misleading even though the underlying `AxisId -> Sketch Line` architecture is correct.

This amendment changes the authoring UX without replacing the accepted Axis domain model.

## 2. Preserved architecture

The following PM-04 rules are unchanged:

- Part owns authored Axis, AxisReference, AxisId lifecycle, visibility, repair and Revolve consumption;
- Shared 2D owns Line geometry and `Regular | Construction` role;
- `Axis` does **not** become a Shared-2D `EntityRole`;
- an authored Axis stores semantic source identity `{SketchId, EntityId}` and derives world origin/direction from the current Line/support;
- Origin X/Y/Z remain built-in AxisReference values and never receive synthetic AxisId;
- Revolve accepts only an Origin AxisReference or an authored Part Axis; it does not consume an arbitrary raw Sketch Line directly;
- Edit/Re-source Axis preserves AxisId and remains the explicit identity-preserving repair path;
- Viewer/provider identity remains runtime-only;
- durable mutation still follows semantic Command -> Validation -> Transaction -> Part Document -> Evaluation.

## 3. Accepted user model

A Sketch Line has two orthogonal meanings in the authoring UI:

```text
Sketch Line
├─ Geometry role: Regular | Construction
└─ Part designation: Axis | none
```

The Part designation is a user-facing projection of a separate Part-owned Axis object:

```text
Sketch Line EntityId
        ↑
        │ source
Part Axis AxisId
```

Therefore all four combinations are legal:

| Geometry role | Part designation |
| --- | --- |
| Regular | none |
| Construction | none |
| Regular | Axis |
| Construction | Axis |

Changing `Regular <-> Construction` never creates, deletes or re-identifies an Axis.

## 4. GUI authoring surface

### 4.1 Remove top-level Axis tool button

The GUI **Axis** authoring button is removed from the Part Modeling / Sketcher toolbar surface.

This does not delete Axis capability, the Axis domain object, Axis Tree/Properties support, Show/Hide, Delete, Edit/Re-source or the `AXIS` Command Line path.

There is no separate GUI "Axis mode" for creating a new Axis.

### 4.2 Line creation Operations

While the Line tool is active, Operations exposes:

```text
Line

Geometry role
(o) Regular
( ) Construction

Part reference
[ ] Axis
```

Rules:

- `Regular/Construction` retains the existing creation-role behavior;
- `Axis` defaults OFF;
- `Axis` is **one-shot**: after the first successfully committed Line+Axis operation it returns to OFF;
- later continuous Line segments are ordinary Lines unless the user explicitly enables Axis again;
- zero-length/rejected Line input does not consume the one-shot designation.

### 4.3 Existing selected Line

For exactly one selected Sketch Line, Operations exposes the current authored state:

```text
Selected line

Geometry role: Regular | Construction

Part reference
[ ] / [x] Axis
```

The Axis control is not offered as an equivalent third geometry role.

For Circle, Arc or a selection that is not exactly one Line, the Axis designation control is unavailable.

## 5. Atomic Line + Axis creation

When `Axis = ON` during new-Line authoring, one accepted user gesture creates:

1. the new Sketch Line with its normal fresh EntityId; and
2. one fresh Part Axis referencing that exact `{SketchId, EntityId}`.

This is **one semantic transaction and one Undo step**.

Required consistency:

- Line and Axis either both commit or neither commits;
- Axis validation/allocation failure cannot leave a committed Line behind;
- Undo removes both authored objects in one step;
- Redo restores the same Line identity and AxisId under the existing history rules;
- no generic composite-command framework is introduced solely for this case.

A bounded Part-hosted command/transaction may create both objects because the host Part owns both the Sketch container and Axis collection.

## 6. Axis designation on an existing Line

### OFF -> ON

For exactly one selected Line with no authored Axis using that source, enabling Axis:

- creates one fresh Part Axis;
- preserves the Line EntityId and geometry role;
- is one semantic transaction / one Undo step.

### ON -> OFF

For exactly one selected Line used by exactly one authored Axis, disabling Axis:

- deletes that Axis using the existing Axis Delete semantics;
- preserves the source Line;
- is one semantic transaction / one Undo step.

If a Revolve currently references the Axis, the GUI must present an explicit confirmation that removing the Axis will leave the consumer with Missing/Blocked Axis intent. After confirmation, existing PM-04 Delete semantics remain authoritative: the Revolve is retained and is never silently rebound.

Undo restores the same AxisId and repairs the consumer where the rest of the model is valid.

## 7. One-source authoring uniqueness and compatibility

New PM-04 authoring adopts the rule:

> one exact `{SketchId, Line EntityId}` source may be owned by at most one newly-created/re-sourced Part Axis.

Therefore:

- Create Axis rejects a source already used by another authored Axis;
- Edit/Re-source Axis rejects a target Line already used by a different authored Axis;
- the Line designation checkbox has an unambiguous meaning for all newly-authored state.

### Existing duplicate-source state

The pre-amendment implementation permits multiple different AxisIds to reference the same source Line. This amendment must **not** invalidate, merge or silently rewrite such already-persisted authored state.

Compatibility rule:

- existing duplicate-source Axis records remain structurally loadable;
- no schema/modeling-semantics bump is required solely for this UX amendment;
- AxisIds and Revolve references remain unchanged;
- a selected Line with more than one authored Axis source match shows an explicit conflict/indeterminate state and does not permit checkbox mutation;
- repair is performed explicitly through Axis Tree/Properties Delete or Edit/Re-source until the source becomes unique;
- no arbitrary Axis is selected as "the" checkbox owner.

This is a legacy-compatibility exception, not authorization to create new duplicates.

## 8. Identity-preserving re-source remains explicit

Checkbox OFF followed by ON means Delete old Axis + Create new Axis and therefore allocates a new AxisId. It is **not** a re-source operation.

To preserve identity:

```text
Axis 7 -> Line A
        │ Edit / Change Source
        ▼
Axis 7 -> Line B
```

The existing Axis object remains selectable in Tree/Properties and keeps the explicit Edit/Re-source action.

Re-source:

- preserves AxisId and authored visibility;
- preserves downstream AxisReference identity;
- rejects a target Line already designated by another authored Axis;
- remains one semantic transaction / one Undo step.

## 9. Command Line

The `AXIS` Command Line path remains supported as an expert/keyboard-first adapter.

It continues to use the same AxisDraft / semantic command path:

- selection-first may acquire exactly one selected admissible Line;
- command-first may enter source acquisition and then select one Line;
- duplicate-source creation is rejected explicitly;
- Finish creates one Axis / one Undo step;
- Edit/Re-source remains identity-preserving.

Removing the GUI toolbar button does not remove `AXIS` from CAD Input.

## 10. Revolve behavior

Revolve semantics do not change.

In particular:

- raw Line selection does not become a direct AxisReference;
- no Axis is inferred from Profile geometry;
- Origin X/Y/Z remain direct built-in references;
- authored Axis remains a distinct Part object;
- current coplanarity, half-plane, Add/Cut, OneSide/Midplane, Reverse, lifecycle and persistence rules remain unchanged.

## 11. Required automated evidence

At minimum add/adjust regressions for:

- no GUI Axis toolbar action while Axis capability remains available contextually and through Command Line;
- Regular + Axis and Construction + Axis creation;
- Axis one-shot resets only after successful Line+Axis commit;
- Line+Axis atomic commit, one Undo step and one Redo restoration;
- failed Line/Axis validation leaves model/revision/history unchanged;
- existing Line OFF -> ON creates exactly one Axis;
- existing Line ON -> OFF deletes exactly one Axis and Undo restores the same AxisId;
- referenced Axis removal preserves downstream Missing/Blocked intent;
- duplicate-source Create and Re-source reject with zero mutation;
- pre-amendment duplicate-source persisted state still loads without identity rewrite;
- duplicate-source selected-Line UI fails closed/indeterminate;
- Edit/Re-source preserves AxisId;
- Regular/Construction changes never alter Axis designation;
- Save/Close/Reopen preserves the resulting authored Axis intent;
- Revolve continues to accept only Origin or authored AxisReference;
- `AXIS` Command Line parity remains.

Existing PM-04A-E semantic/kernel/lifecycle evidence remains valid unless touched by the implementation, but a new final exact-head FULL is mandatory because runtime/UI behavior changes after #1563.

## 12. PM-04F gate reset

The previous PM-04F automated readiness was valid for the pre-amendment implementation but is no longer final closure authority.

After implementation:

1. focused/subsystem tests appropriate to each slice must pass;
2. final runtime candidate must pass exact-head Windows FULL;
3. internal as-built and PL/EN Product documentation must describe the new authoring workflow;
4. Product Browser must be regenerated from canonical Markdown and pass DOCS validation;
5. the Owner Windows workflow must be updated and executed on the final accepted candidate;
6. only explicit Owner PASS may close PM-04.

PM-05 remains inactive until PM-04 closes.

## Documentation impact

Internal docs: required  
User/Product docs: required  
Reason: the amendment changes the user-facing Axis authoring workflow, interaction ownership, command/UI relationship, duplicate-source authoring rule and final PM-04 acceptance procedure. Product Browser regeneration is required after the canonical Product documentation update.
