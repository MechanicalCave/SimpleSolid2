# SK-07C — COPY, Repeated Copy and Fresh Entity Identity

**Status:** ACCEPTED — ACTIVE  
**Proposed:** 2026-09-27  
**Owner acceptance:** 2026-09-27  
**Decision class:** D2 fresh-identity lifecycle + transform/interaction semantics + bounded D1 implementation  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Architecture:** ADR-0003, ADR-0008, ADR-0009, ADR-0010  
**Program roadmap:** `work/SKETCH_ROADMAP.md` v1.2  
**Milestone:** R7 — Common transforms, Copy and command grammar — third bounded slice

## 1. Goal

Add normal Sketch `COPY` for mixed Line/Circle/Arc selections and establish the durable fresh-`EntityId` lifecycle required by semantic duplication.

SK-07C is deliberately narrower than the complete roadmap Copy model.

This slice proposes:

- toolbar + Command Line `COPY`;
- selection-first and command-first object collection matching the completed common-transform grammar;
- explicit Base Point and pointer-derived placement point;
- repeated placements while COPY remains active;
- one fresh EntityId per duplicated entity per accepted placement;
- original entities remain selected/reference entities;
- each accepted placement is one atomic command, one transaction and one Undo entry;
- every placement derives from the original frozen source snapshot and the same Base Point;
- no persistence schema migration.

This slice does **not** activate Copy as a modifier for grip manipulation, Rotate, Scale, Mirror or owner-only Reshape.

## 2. Preserved authority and invariants

The Constitution, Foundation 1.0, ADR-0008, ADR-0009 and Sketcher roadmap v1.2 remain authoritative.

SK-07C preserves the completed SK-07A/SK-07B architecture:

- authored Sketch geometry is Shared 2D semantic state;
- Part owns hosted Sketch lifecycle and persistence;
- one `SketchInteractionState` remains the semantic runtime interaction authority;
- semantic selection contains EntityIds, not provider objects;
- provider tokens, Qt/OCCT objects and screen geometry never become CAD identity;
- durable mutation flows through semantic Command → validation → DocumentSession → Part transaction/history;
- common transform preview remains runtime-only;
- command-first object collection reuses the existing semantic selection/query bridge;
- source geometry is captured from a frozen semantic selection snapshot;
- partial mutation is forbidden;
- Move/Rotate/Scale/Mirror behavior from SK-07A/SK-07B must not regress.

ADR-0009 already requires the distinction:

```text
state copy / Undo state replication
→ preserve identity

semantic duplication / Copy
→ create fresh identity
```

SK-07C proposes the concrete non-aliasing lifecycle for the first semantic duplication command.

## 3. COPY is translation + semantic duplication

Normal `COPY` uses the existing provider-independent translation transform geometry.

For source snapshot geometry S, Base Point B and placement point P:

```text
delta = P - B
preview geometry = translate(S, delta)
```

COPY differs from MOVE at commit time:

- MOVE updates existing entities and preserves their EntityIds;
- COPY leaves every source entity unchanged;
- COPY adds new semantic entities with the preview geometry;
- every added entity receives a fresh EntityId.

The geometry algorithm must reuse the same translation semantics already used by MOVE. UI/controller code must not implement a second translation algorithm.

COPY may duplicate any mixed frozen set of Line/Circle/Arc.

## 4. Fresh EntityId lifecycle

Each successful placement creates one new semantic entity for each entity in the frozen source set.

Required identity semantics:

- source EntityIds are never changed;
- copied entities never reuse source EntityIds;
- copied entities from different placements never share EntityIds;
- an EntityId that was committed once in the continuing Sketch lineage must not later alias a different entity merely because the copy was undone or deleted;
- Undo of a COPY placement removes the copied entities but must not rewind the durable high-water identity cursor to make their IDs reusable;
- Redo restores the same copied entities with the same copied EntityIds from the historical state;
- a new COPY performed after Undo allocates identities above the preserved high-water cursor rather than reusing the undone copy IDs;
- Save → Close → Reopen preserves both committed copied EntityIds and the non-reuse allocator state;
- a failed/uncommitted preview does not consume durable EntityIds.

The existing schema-v4 Sketch model already carries `next_entity_id`. SK-07C expects to preserve schema v4 and use that state to satisfy non-aliasing.

Exact internal allocation order across a mixed Line/Circle/Arc batch is D1 and is not a public CAD contract. Uniqueness, non-aliasing and historical restoration are the contract.

EntityId exhaustion or invalid allocator state fails the entire placement with no partial additions and no durable cursor mutation.

## 5. Affected set and source snapshot

COPY uses the same affected-set rule as the current common transforms.

The source set:

- is the current semantic selection snapshot when COPY leaves object collection;
- may mix Line/Circle/Arc;
- contains source EntityIds only;
- is frozen for the complete COPY session;
- remains the reference set across all repeated placements.

The source authored geometry snapshot is also frozen at COPY-session start.

Every repeated placement is derived from:

```text
same original source snapshot
+ same Base Point
+ current placement point
```

A later placement must never be derived from a previously created copy.

Created copies do not become the source set.

## 6. Selection-first COPY

If `COPY` is activated with non-empty semantic selection:

- existing selection is used immediately;
- there is no extra selection-complete confirmation;
- source selection and geometry are frozen;
- the command enters **Specify Base Point**.

The Base Point may be any finite resolved Sketch-local point.

Accepting Base Point creates no authored mutation, no EntityId allocation, no revision increment and no history entry.

## 7. Command-first COPY

If `COPY` is activated with empty semantic selection, it enters **Select objects**.

Object collection reuses the completed common-transform grammar:

- ordinary click adds an entity and makes it primary;
- re-picking a selected entity preserves membership and makes it primary;
- Ctrl+click toggles membership;
- left-to-right rectangle uses Window;
- right-to-left rectangle uses Crossing;
- Ctrl+rectangle toggles returned semantic EntityIds;
- blank LMB is a no-op;
- grips are hidden/inactive;
- Enter, Space or RMB completes collection only when non-empty;
- completion freezes the source set and enters **Specify Base Point**;
- Esc cancels COPY and returns to Select while preserving the objects collected so far as normal selection.

No second command-selection owner is introduced.

## 8. Base Point, placement preview and repeated placement

Normal COPY stages are:

```text
Select objects (command-first only)
→ Specify Base Point
→ Specify placement point / preview
→ accept placement
→ remain in Specify placement point for another copy
→ Esc/tool switch/Undo boundary ends the COPY session
```

Pointer motion at the placement stage previews one translated duplicate of the complete frozen source set.

Preview:

- uses the existing runtime-only preview channel;
- creates no EntityIds;
- changes no authored state;
- does not increment revision;
- does not dirty the Document;
- is recalculated from the original source snapshot each frame.

### Proposed zero-displacement rule

SK-07C proposes that exact zero displacement is **not an accepted COPY placement**.

If `P == B`:

- no copy is created;
- no EntityId is allocated;
- no revision/dirty/history change occurs;
- COPY remains at the placement stage.

This avoids silently creating coincident duplicate geometry that is visually indistinguishable from its source.

This zero-displacement behavior is part of the D2 proposal and becomes authoritative only if the Owner accepts SK-07C as written.

## 9. Placement commit and atomicity

Each non-zero accepted placement is exactly one semantic operation:

- one duplication command;
- one staged Part state;
- one transaction;
- one DocumentRevision increment;
- one Undo entry.

The placement command must validate before commit:

- active Part/Sketch identity;
- current expected DocumentRevision;
- every frozen source EntityId still exists and denotes the expected primitive kind;
- source snapshot remains valid;
- translated output geometry is canonical and finite;
- enough fresh EntityIds can be allocated atomically.

Any failure aborts the complete placement.

No subset of a mixed copy may be committed.

After a successful placement:

- originals remain unchanged;
- original source selection/primary remains selected;
- created copies are not automatically selected;
- the COPY session remains active at **Specify placement point**;
- expected revision is advanced to the revision created by that successful COPY placement;
- the same original source snapshot and Base Point remain in use for the next placement.

## 10. Repeated Copy history semantics

Each accepted repeated placement has an independent history boundary.

For placements A, B and C:

```text
COPY placement A → Undo entry A
COPY placement B → Undo entry B
COPY placement C → Undo entry C
```

There is no one giant transaction for the whole repeated-COPY session.

Undo while COPY is active:

1. cancels the current transient COPY preview/session;
2. returns interaction to Select with original source selection preserved;
3. performs ordinary global Undo.

Therefore Undo after placements A/B/C removes C only.

Redo restores C with the same copied EntityIds C originally received.

A new COPY after Undo must allocate newer IDs rather than reuse IDs from the undone placement.

## 11. Revision changes and fail-closed behavior

A successful placement changes DocumentRevision by one. The active COPY interaction may explicitly advance its expected revision after its own successful commit so repeated placement can continue.

Any unrelated/stale revision change during an uncommitted COPY stage invalidates the pending interaction.

On stale revision, missing source or identity mismatch:

- the pending placement commits nothing;
- no new EntityIds become durable;
- already committed earlier placements remain intact;
- interaction returns safely to Select;
- original source selection is reconciled against surviving semantic entities.

No retry may silently reinterpret stale source geometry.

## 12. Toolbar and Command Line adapters

Sketch Modify becomes:

```text
Modify
  Move
  Copy
  Rotate
  Scale
  Mirror
```

The compact Command Line adds:

```text
COPY
```

Toolbar Copy and Command Line COPY are adapters to the same semantic COPY interaction.

This slice must not introduce a second UI-specific Copy state machine.

Architecturally, roadmap Copy remains an orthogonal duplication modifier that can later combine with other edit modes. SK-07C exposes only the bounded normal translation COPY command and must not encode COPY in a way that prevents later Rotate/Scale/Mirror+Copy or grip Copy modifier work.

Numeric distance/coordinate input is not introduced here. As in SK-07B, placement is pointer/resolved-input driven.

## 13. Cancellation and tool switching

Esc behavior:

- during Select objects: cancel COPY and preserve collected normal selection;
- during Base Point: cancel COPY and preserve source selection;
- during placement preview, before or after prior placements: cancel only the uncommitted preview/session; all earlier committed copies remain;
- after return to Select, a subsequent Esc follows ordinary Select clearing semantics.

Switching to another tool:

- cancels only the current uncommitted COPY preview/session;
- preserves committed copies;
- preserves the original semantic selection subject to ordinary reconciliation;
- activates the requested tool.

Finish Sketch / Document switch / close must clear transient COPY state without leaking preview or provider identity.

## 14. Automated acceptance coverage

At minimum verify:

1. mixed Line/Circle/Arc COPY uses existing translation math;
2. selection-first COPY skips object collection;
3. command-first COPY matches common-transform object collection;
4. blank LMB is no-op during command-first collection;
5. Enter/Space/RMB requires non-empty collection;
6. Base Point acceptance creates no authored/history/identity mutation;
7. preview allocates no EntityIds;
8. one placement duplicates the full mixed source set atomically;
9. originals and source EntityIds remain unchanged;
10. every copied entity receives a fresh EntityId;
11. copies are not automatically selected;
12. original selection/primary remains the selected reference set;
13. second and later placements derive from the original source snapshot, not prior copies;
14. each placement creates exactly one revision and one Undo entry;
15. exact zero displacement creates no copy, no revision, no dirty-state change and no allocator advance;
16. stale revision or missing source causes no partial placement;
17. allocation/validation failure leaves no durable high-water cursor change;
18. Undo of a copy does not make copied IDs reusable;
19. Redo restores the same copied EntityIds;
20. new COPY after Undo allocates identities above the preserved high-water cursor;
21. Save → Close → Reopen preserves copied geometry, copied EntityIds and non-reuse cursor state under schema v4;
22. Undo during active repeated COPY cancels transient interaction first, then undoes only the last committed placement;
23. toolbar exposes Copy inside Modify;
24. toolbar and Command Line COPY route to the same semantic state;
25. MOVE/ROTATE/SCALE/MIRROR and grip/reshape regressions remain green;
26. Viewer/navigation regressions remain green;
27. final runtime candidate passes exact-head Windows FULL;
28. required documentation and generated Browser freshness pass.

## 15. Manual Windows verification

Final candidate requires Owner verification of:

- Copy appears in the Modify group;
- selection-first COPY on Line, Circle, Arc and mixed selection;
- command-first COPY on Line, Circle, Arc and mixed selection;
- Base Point → placement preview behaves like translational duplication;
- first placement leaves originals unchanged;
- repeated second/third placements are based on the same original source and Base Point;
- original objects remain selected; copies do not steal selection;
- Esc after one or more placements ends COPY without removing committed copies;
- zero-displacement attempt creates no invisible coincident copy;
- Undo after repeated placements removes only the last placement;
- Redo restores the same placement;
- a new placement after Undo does not produce identity/persistence anomalies;
- Save → Close → Reopen preserves all committed copies;
- MOVE/ROTATE/SCALE/MIRROR, grips/reshape, Pan/Orbit/Zoom/ViewCube remain stable.

## Documentation impact

Internal docs: required  
User/Product docs: required  
Reason: SK-07C introduces user-visible COPY/repeated-COPY behavior and establishes durable fresh-identity/non-reuse semantics across Undo/Redo and persistence.

## 17. Expected implementation surface

Expected bounded production changes after Owner acceptance:

- `src/sketch/**`;
- `src/application/**` for the semantic batch-duplication command if required;
- `src/part/**` only if existing transaction/state restore plumbing requires bounded non-reuse handling;
- `src/persistence/**` only to verify/use existing schema-v4 allocator state; no schema migration is expected;
- `src/ui/**`;
- `src/viewer/**` only if bounded existing preview routing requires it;
- tests;
- affected internal/Product docs and generated Browser;
- `work/**`.

Stop for Owner review if implementation requires:

- changing the persisted Part schema;
- changing EntityId textual/public representation;
- reusing an undone/deleted semantic EntityId for a different entity;
- generic cross-document Copy/Paste or clipboard semantics;
- Copy across Sketches/Documents;
- provider identity in duplication semantics;
- a second semantic selection/interaction authority;
- a generic new overlay framework;
- numeric parser/Dynamic Input work;
- snapping/inference;
- changing existing Move/Rotate/Scale/Mirror semantics.

## 18. Explicitly out of scope

SK-07C does not authorize:

- Copy modifier for grip-started manipulation;
- Rotate+Copy;
- Scale+Copy;
- Mirror+Copy;
- Reshape+Copy;
- semantic Space CycleEditMode;
- ordinary-Select RMB context menu;
- Repeat Last Command;
- clipboard Copy/Paste;
- cross-Sketch or cross-Document duplication;
- numeric distances/coordinates;
- Dynamic Input;
- Object Snap/tracking/inference;
- Ortho/Polar/Grid Snap;
- constraints/solver/authored dimensions;
- Rectangle/Polyline or other primitive breadth;
- planar-face Sketch support;
- regions/profiles;
- R8+ work.

These remain later separately accepted slices.

## 19. Activation and completion boundary

SK-07C was explicitly Owner-accepted on 2026-09-27 and is active within this bounded scope.

Completion requires:

- normal translation COPY and repeated placement use the existing common transform geometry path;
- each placement creates fresh non-aliasing EntityIds;
- Undo/Redo/new-copy/save-reopen satisfy the accepted high-water identity lifecycle;
- selection-first and command-first COPY are implemented;
- repeated placements remain independent atomic history entries;
- Copy is added to Modify and Command Line;
- no out-of-scope Copy modifier/general command grammar enters the slice;
- persistence remains schema v4;
- required internal and PL/EN Product documentation is current;
- final candidate passes exact-head Windows FULL;
- Owner manual Windows verification passes;
- closeout CLOSURE passes;
- remaining R7 and R8+ work stays inactive until separately accepted.
