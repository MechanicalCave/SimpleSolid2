# SK-07G — Grip Copy Modifier

**Status:** ACCEPTED — ACTIVE  
**Proposed:** 2026-09-29  
**Owner acceptance:** 2026-09-29  
**Decision class:** D2 direct-manipulation command grammar already bounded by Sketcher Roadmap v1.4 + D1 implementation  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Architecture:** ADR-0003, ADR-0008, ADR-0009, ADR-0011, ADR-0012  
**Program roadmap:** `work/SKETCH_ROADMAP.md` v1.4  
**Milestone:** R7 — remaining Grip Copy command grammar slice

## 1. Context

R7 already completed the following bounded slices:

- SK-07A — common transform core + MOVE;
- SK-07B — ROTATE / SCALE / MIRROR + tool grouping;
- SK-07C — normal COPY, repeated placements and fresh EntityId lifecycle;
- SK-07D — Repeat Last Command;
- SK-07E — Space CycleEditMode during direct grip manipulation;
- SK-07F — shared PointRequest / Direct Distance;
- WB-02 / ADR-0011 — workspace-global keyboard-first CAD input routing.

AUDIT-01 A-F is complete. CI-03 is complete and provides FOCUSED target/test iteration while preserving FULL as merge evidence.

Sketcher Roadmap v1.4 already fixes the remaining Grip Copy direction:

- Copy is an orthogonal modifier to the current edit/transform mode;
- source/affected set remains frozen;
- each accepted placement creates fresh EntityIds;
- originals remain selected/reference entities;
- repeated placements derive from the original source state and same pivot;
- changing EditMode turns Copy OFF;
- for multi-selection common Move+Copy duplicates the complete frozen affected set;
- for Reshape+Copy only the owner entity of the active reshape grip is copied and reshaped;
- Undo during active Copy cancels transient interaction first, then performs normal global Undo and returns to Select.

SK-07F also deliberately reserved the future grip-local keyword `C`.

This contract activates only that already-planned Grip Copy slice. Ordinary RMB context remains deferred.

## 2. Goal

Add one bounded **Grip Copy modifier** to existing direct grip manipulation.

The user workflow is:

```text
Select entity/entities
→ activate one visible grip
→ existing direct manipulation begins
→ type C
→ Enter
→ Copy modifier becomes ON
→ move pointer or use existing Direct Distance
→ LMB / accepted numeric point creates a copy
→ Copy remains ON for repeated placements
→ Esc ends the transient session
```

Grip Copy must reuse:

- SK-07E direct-manipulation state;
- SK-07C fresh-identity duplication semantics;
- SK-07F PointRequest / Direct Distance;
- WB-02 global CAD input transport;
- the existing semantic Command → validation → DocumentSession → Part transaction path.

No second transform, duplication, keyboard or transaction system is authorized.

## 3. Preserved authority and invariants

The Constitution, Foundation, ADR-0008, ADR-0009, ADR-0011, ADR-0012 and completed R7 contracts remain authoritative.

In particular:

- Sketch authored geometry remains provider-neutral Shared 2D semantic state;
- Part owns hosted Sketch lifecycle and durable mutation;
- EntityId remains opaque, Sketch-local and non-reused according to the existing high-water lifecycle;
- UI, Viewer and OCCT identity never becomes authored identity;
- one Sketch interaction authority owns selection and transient manipulation state;
- exactly one grip is active;
- the direct-manipulation selection snapshot remains frozen from interaction start;
- preview is runtime-only and consumes no EntityIds;
- normal COPY semantics from SK-07C remain unchanged;
- existing Reshape and Move geometry semantics remain unchanged;
- existing Direct Distance resolution remains unchanged;
- durable mutation remains atomic and fail-closed.

## 4. Copy is a modifier, not a third DirectEditMode

Current direct manipulation has:

```text
DirectEditMode
- Reshape
- Move
```

SK-07G shall not add `Copy` as a third edit mode.

Instead the active direct-manipulation session gains one runtime-only mutation policy equivalent to:

```text
copy_modifier = OFF | ON
```

Exact C++ representation is D1.

At direct-manipulation start:

```text
copy_modifier = OFF
```

Space continues to cycle only the accepted DirectEditMode:

```text
Reshape ↔ Move
```

Changing DirectEditMode turns Copy OFF, as already fixed by the roadmap.

No persistent preference is introduced.

## 5. Grip-local CAD input grammar

### 5.1 Keyword

While direct grip manipulation is active, the case-insensitive submitted token:

```text
C
```

means:

```text
enable Grip Copy
```

The user supplies it through the existing WB-02 live CAD input buffer and submits with Enter.

The token is tool-local grammar. It is not:

- a global hotkey;
- an immediate single-key action before Enter;
- a top-level alias for CIRCLE;
- a replacement for the normal COPY command.

Outside active direct manipulation, `C` gains no new meaning.

### 5.2 Context-first precedence

The active PointRequest remains authoritative.

The Sketch semantic CAD-input adapter may explicitly recognize the bounded active-manipulation keyword `C` when a direct manipulation owns the current point request.

Implementation must not fall through from an arbitrary invalid point token to top-level command activation.

Required behavior:

```text
active direct manipulation + "C"
→ tool-local Grip Copy keyword

active direct manipulation + valid numeric scalar
→ existing Direct Distance

active direct manipulation + unknown token
→ reject; remain in same semantic stage; no authored mutation

ordinary Select + "C"
→ existing unknown-command behavior
```

The workspace-global router remains domain-neutral and must not learn Sketch grip semantics.

## 6. Enabling and lifetime

Submitting `C` successfully:

- creates no authored mutation;
- creates no EntityId;
- changes no DocumentRevision;
- changes no dirty state;
- creates no Undo entry;
- preserves active grip identity;
- preserves frozen selection;
- preserves current DirectEditMode;
- preserves interaction-start pivot/base;
- sets Copy ON.

Submitting `C` again while Copy is already ON is an idempotent no-op or bounded "already enabled" success. It must not create a second semantic mode or reset the interaction.

Copy turns OFF when:

- Space changes DirectEditMode;
- Esc ends direct manipulation;
- another tool is activated;
- Undo/Redo boundary cancels the transient session;
- active Sketch/Document context is replaced;
- the grip/direct-manipulation session otherwise terminates.

No "sticky" Copy state survives into the next grip activation.

## 7. Move + Copy semantics

When:

```text
DirectEditMode = Move
Copy = ON
```

the affected set is the complete frozen direct-manipulation selection snapshot.

For accepted placement point P and interaction-start pivot B:

```text
delta = P - B
copy geometry = translate(original frozen selection geometry, delta)
```

Commit behavior:

- originals remain unchanged;
- originals remain the selected/reference set;
- every copied entity gets a fresh EntityId using the existing SK-07C lifecycle;
- created copies are not automatically selected;
- one placement is one atomic semantic command;
- one placement is one Part transaction / revision increment;
- one placement is one Undo entry;
- after success, Grip Copy remains active for another placement;
- the same original frozen source and same pivot remain authoritative.

The second placement must never derive from the first copy.

Exact zero displacement is a clean no-op:

- no copy;
- no EntityId consumption;
- no revision/dirty/history change;
- Copy remains active.

## 8. Reshape + Copy semantics

When:

```text
DirectEditMode = Reshape
Copy = ON
```

only the owner entity of the active reshape grip is duplicated.

Other selected entities remain unchanged and are not duplicated merely because they belong to the frozen selection snapshot.

The copied owner geometry is the existing Reshape preview derived from the interaction-start owner geometry and current accepted point.

Commit behavior:

- original owner remains unchanged;
- all other originals remain unchanged;
- exactly one fresh copied entity is added per accepted placement;
- the copied entity receives the reshaped geometry preview;
- the original selection/primary remains authoritative;
- the new copy does not take over selection;
- each placement is one atomic command / transaction / revision / Undo entry;
- repeated placements remain active;
- every placement derives from the original interaction-start owner geometry, not from a prior copy.

If the accepted reshape result is exactly unchanged from the source owner geometry, the placement is a clean no-op with no fresh ID consumption.

## 9. Existing Direct Distance remains the numeric path

Copy ON does not create a new InputRequest.

The same PointRequest used by current direct manipulation remains active.

Therefore this workflow is valid:

```text
activate grip
→ C + Enter
→ point pointer to define direction
→ type 50
→ Enter
→ copy is committed at existing Direct Distance result
```

No tool-specific numeric parser is authorized.

## 10. Preview and presentation

The existing direct-manipulation preview geometry remains authoritative.

Copy ON changes commit policy, not geometry evaluation.

No new Viewer/provider geometry algorithm is required.

The runtime UI must provide bounded feedback that Copy is ON. Exact wording/prompt formatting is D1, for example:

```text
Grip Copy: ON
```

or an equivalent command prompt/status projection.

This contract does not require new colors, glyphs, overlays or OCCT-native presentation behavior.

## 11. Commit path and identity reuse

Grip Copy must reuse the semantic duplication / fresh-ID lifecycle established by SK-07C.

Implementation may factor existing command logic if needed, but must not create a second allocation policy.

Required identity properties:

- no copied entity reuses a source EntityId;
- repeated placements receive distinct fresh IDs;
- Undo does not make committed copied IDs reusable;
- Redo restores the same copied IDs through existing history semantics;
- new copies after Undo allocate above the durable non-reuse cursor;
- failed or uncommitted preview consumes no durable IDs.

No persistence schema change is expected.

## 12. Undo / Redo / Esc / tool switching

### Esc

During Grip Copy:

- Esc cancels only the current uncommitted preview/session;
- all prior committed copies remain authored;
- the editor returns to ordinary Select;
- original semantic selection is preserved subject to ordinary reconciliation.

### Undo

If Undo is invoked while Grip Copy is active:

1. cancel the active transient direct-manipulation Copy session;
2. return to Select;
3. execute normal global Undo.

If placements A/B/C were previously committed, Undo removes C only.

### Redo

Redo remains ordinary global history behavior and restores the same copied EntityIds.

### Tool switching

Activating another tool cancels only uncommitted direct manipulation. Prior committed copies remain.

## 13. Space CycleEditMode interaction

Space retains SK-07E precedence during active direct manipulation.

If Copy is ON:

```text
Reshape + Copy ON
  --Space-->
Move + Copy OFF
```

or:

```text
Move + Copy ON
  --Space-->
Reshape + Copy OFF
```

The new mode uses the same interaction-start pivot/frozen snapshot rules already accepted by SK-07E.

The user must submit `C` again to re-enable Copy for the new edit mode.

## 14. Repeat Last Command

Grip Copy is a modifier inside direct manipulation, not a top-level repeatable command.

It must not replace the SK-07D last-repeatable-command identity.

Normal top-level COPY remains repeatable exactly as before.

## 15. Failure behavior

Fail closed without partial mutation when:

- direct-manipulation source owner/snapshot is stale or missing;
- active DocumentRevision no longer matches the manipulation revision;
- current geometry preview is invalid;
- fresh identity allocation cannot complete atomically;
- semantic CAD-input context is stale;
- no active direct manipulation owns the submitted `C`;
- any required source entity is missing at commit.

A failure must not create partial copied geometry, partially advance identity allocation, dirty the Document or create a misleading Undo entry.

## 16. Verification footprint

This slice deliberately reuses existing R7 test executables where practical so CI-03 FOCUSED can be used without adding a new CMake test target during ordinary iteration.

### Primary affected

- Sketch direct-manipulation runtime state;
- Part Sketch interaction controller;
- Sketch CAD-input semantic endpoint;
- existing semantic copy command/fresh-ID path.

### First focused state loop

Prefer extending:

```text
Target: sk07e_space_cycle_edit_mode_test
Test:   sk07e.space_cycle_edit_mode
```

Evidence should cover:

- Copy OFF at manipulation start;
- C-equivalent semantic enable path at state/controller boundary;
- Copy remains orthogonal to Reshape/Move;
- Space changes mode and clears Copy;
- original frozen snapshot/pivot remains unchanged;
- repeated placement state does not compound.

### Focused copy lifecycle loop

Reuse:

```text
Target: sk07c_copy_command_test
Test:   sk07c.copy_command
```

when shared fresh-ID/duplication code is changed.

If the existing SK-07C command path is reused without modification, this test is checkpoint evidence rather than mandatory after every state-only edit.

### Focused controller / Direct Distance loop

Prefer extending:

```text
Target: sk07f_precision_input_controller_test
Test:   sk07f.precision_input_controller
```

and, if the Application semantic endpoint itself changes:

```text
Target: d_cad_input_semantics_test
Test:   d.cad_input_semantics
```

Evidence should cover:

- `C` is accepted only in active direct manipulation;
- numeric Direct Distance still wins as numeric point input;
- unknown active-request text does not fall through to top-level command activation;
- Move+Copy duplicates full frozen selection;
- Reshape+Copy duplicates owner only;
- repeated placement remains active;
- zero/no-change placement consumes no IDs;
- Space clears Copy;
- selection and active grip semantics remain coherent.

### Wider checkpoint

Before final candidate, run at least the directly affected R7 set using FOCUSED multi-target or a bounded subsystem checkpoint.

Suggested affected regression set:

- `sk05a.direct_manipulation_state`;
- `sk05a.part_sketch_direct_manipulation`;
- `sk07c.copy_command`;
- `sk07c.copy_controller`;
- `sk07d.repeat_last_command_controller`;
- `sk07e.space_cycle_edit_mode`;
- `sk07f.precision_input_state`;
- `sk07f.precision_input_controller`;
- `d.cad_input_semantics`;
- `wb02.cad_input_session` where the generic router boundary is touched.

### Final

Final runtime/documentation candidate requires exact-head FULL.

FOCUSED, FAST and SUBSYSTEM remain iteration/checkpoint evidence only.

## 17. Automated acceptance coverage

At minimum prove:

1. direct manipulation starts with Copy OFF;
2. active grip `C` enables Copy without authored mutation;
3. `C` outside active direct manipulation does not gain top-level command meaning;
4. repeated `C` while already ON is bounded/idempotent;
5. Move+Copy duplicates the complete frozen mixed selection;
6. Move+Copy leaves originals unchanged and selected;
7. copied Move entities receive fresh non-reused IDs;
8. repeated Move+Copy placements use original frozen geometry and pivot;
9. Reshape+Copy duplicates only active-grip owner;
10. Reshape+Copy leaves all originals unchanged;
11. repeated Reshape+Copy placements derive from original owner geometry;
12. no-change/zero placement creates no copy and consumes no ID/history;
13. every accepted placement creates exactly one atomic transaction/revision/Undo entry;
14. failed placement is all-or-nothing;
15. Space changes DirectEditMode and clears Copy;
16. Esc ends transient Copy but preserves prior committed placements;
17. Undo during active Copy cancels transient state first and then undoes only the last committed placement;
18. Redo restores copied IDs using existing history semantics;
19. new copy after Undo does not reuse prior committed identity;
20. Direct Distance remains usable after Copy is enabled;
21. invalid active-request token does not launch a top-level command;
22. Copy modifier does not replace Repeat Last Command identity;
23. tool/Sketch/Document switch clears Copy runtime state;
24. normal COPY behavior remains unchanged;
25. existing Reshape/Move/CycleEditMode behavior remains unchanged;
26. no persistent schema change occurs;
27. exact-head Windows FULL passes;
28. required documentation and Product Browser freshness pass.

## 18. Manual Windows verification

Final candidate requires bounded Owner verification of:

- single Line endpoint grip → Reshape → `C` + Enter → place copy: original unchanged, copied Line reshaped;
- with multiple entities selected, center/Move-capable grip → `C` + Enter → placement duplicates the complete selection;
- make two repeated placements without restarting grip interaction;
- after Copy ON, Space changes Reshape↔Move and Copy is OFF;
- re-enable `C`, point direction, type `50`, Enter → Direct Distance copy placement;
- Esc after one or more placements returns to Select and keeps committed copies;
- Undo after a completed Grip Copy placement removes only the latest placement;
- normal COPY, Repeat Last Command, ordinary grip Reshape/Move, Command Line keyboard-first entry, Save/reopen and navigation do not regress.

No manual identity-number inspection is required; automated tests own fresh-ID evidence.

## Documentation impact

Internal docs: required  
User/Product docs: required  
Reason: SK-07G adds a user-visible grip command grammar and changes direct-manipulation commit behavior when Copy is explicitly enabled.

## 20. Expected implementation surface after acceptance

Bounded production changes may touch:

- `src/sketch/**` for runtime-only copy-modifier state;
- `src/application/**` only for the bounded Sketch semantic tool-local keyword path;
- `src/ui/part_sketch_interaction_controller.*` for activation/commit orchestration;
- existing semantic Copy/Part command implementation only if needed to reuse SK-07C fresh-ID lifecycle;
- existing R7 test sources;
- internal/Product docs and generated Browser;
- `work/**`.

Avoid adding a new test executable unless existing R7 test targets cannot express required evidence cleanly.

## 21. Stop conditions — Owner review required

Stop before implementation expansion if Grip Copy requires any of:

- a third DirectEditMode;
- changing HandleRole identity;
- changing normal COPY semantics;
- a new persistence schema;
- a new durable Copy preference;
- a new generic CAD-input request hierarchy;
- moving Sketch keyword meaning into the workspace-global router;
- a top-level single-letter command alias;
- changing global selection grammar;
- multiple active grips;
- changing the SK-07C fresh-ID lifecycle;
- new Viewer/OCCT identity or provider-owned semantics;
- RMB context implementation;
- Ortho/Polar/Dynamic Input/Object Snap/tracking;
- solver/constraints;
- generic universal modifier framework not justified by this one concrete use.

Any such expansion is D2/D3 and requires separate Owner approval.

## 22. Explicitly out of scope

SK-07G does not authorize:

- ordinary Select RMB context menu;
- RMB command history/repeat menu;
- normal COPY redesign;
- Rotate/Scale/Mirror Grip Copy variants beyond what naturally follows from the existing direct-manipulation Reshape/Move modes;
- new transform modes;
- new numeric grammar or quantity request types;
- Dynamic Input;
- Ortho/Polar/Grid Snap;
- Object Snap/tracking/inference;
- authored constraints/dimensions;
- Rectangle/Trim/Split/Join;
- region/profile changes;
- solid modeling / Extrude;
- persistence/schema changes;
- UI customization framework.

## 23. Completion boundary

SK-07G may become ACTIVE only after explicit Owner acceptance.

Completion requires:

- Grip Copy is an orthogonal runtime modifier, not a new DirectEditMode;
- `C` is bounded active-grip tool-local grammar through the existing CAD-input semantic path;
- Move+Copy and Reshape+Copy match the roadmap's frozen-source semantics;
- fresh identities reuse the existing SK-07C lifecycle;
- Direct Distance remains shared;
- Copy clears on mode change/session termination;
- no out-of-scope architecture expansion occurs;
- focused/affected evidence passes during implementation;
- required internal and Product documentation is current;
- final exact-head Windows FULL passes;
- Owner manual Windows checklist passes;
- completion bookkeeping passes CLOSURE.

After SK-07G, R7 still has ordinary RMB context behavior as a separately bounded remaining slice. R8+ remains inactive.
