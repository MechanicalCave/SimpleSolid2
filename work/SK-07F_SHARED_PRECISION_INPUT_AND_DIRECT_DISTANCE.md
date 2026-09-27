# SK-07F — Shared Precision Input and Direct Distance Foundation

**Status:** PROPOSED — OWNER REVIEW  
**Proposed:** 2026-09-27  
**Decision class:** D2 interaction/input architecture + bounded D1 implementation  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Architecture:** ADR-0003, ADR-0008, ADR-0009, ADR-0010  
**Program roadmap:** proposed `work/SKETCH_ROADMAP.md` v1.3  
**Milestone:** R7 — Common transforms, Copy and command grammar — sixth bounded slice

## 1. Goal

Introduce one shared semantic **precision-input path** for Sketch interactions that currently expect a point.

The first production capability is **Direct Distance**:

```text
base point + current pointer direction + numeric distance
→ resolved point
```

This is cross-cutting infrastructure, not a Move/Copy special case.

The first clients are:

- Line second point;
- active grip Reshape;
- active grip Move;
- normal MOVE destination;
- normal COPY placement destination.

Ordinary pointer/click input remains valid and shares the same semantic point-acceptance path.

SK-07F does not implement Grip Copy command grammar, Ortho/Polar, Dynamic Input, Object Snap or tracking. Those later features must consume this same input-resolution seam rather than create another one.

## 2. Architectural model

The target interaction model separates four concerns:

```text
Operation mode
    e.g. Line / Reshape / Move / Copy / Rotate / Scale / Mirror

Mutation policy
    edit original / duplicate
    (Grip Copy policy is later)

Input request
    what semantic value is currently required

Input resolution
    how pointer/text/tracking later satisfy that request
```

SK-07F implements the first shared request/resolution path for point acquisition.

No tool-specific numeric parser is authorized.

No duplicate "numeric Move", "numeric Copy" or "numeric grip" state machine is authorized.

## 3. One active semantic input request

At most one semantic Sketch input request may be active at a time.

The first required request shape is conceptually:

```text
PointRequest
    base: optional Point2
    pointer/current candidate: optional Point2
    direct-distance-enabled: bool
```

Exact C++ representation is D1/D2 within the accepted architecture and need not use these field names.

The semantic owner must remain in the existing Sketch interaction/controller runtime path.

Qt widgets, Viewer tokens and provider objects are adapters only.

## 4. Point sources

A PointRequest may be satisfied by either:

### 4.1 Pointer point

Existing pointer resolution through the active Sketch frame produces a physical U/V point.

Click/accept uses that point exactly as before.

### 4.2 Direct distance

When the request has:

- a valid base point;
- a valid current pointer candidate distinct enough from base to define direction;
- direct-distance enabled;

a positive numeric scalar `d` resolves:

```text
direction = normalize(current_pointer - base)
resolved_point = base + direction * d
```

The resolved point then enters the exact same semantic acceptance/preview/commit path as a pointer point.

The numeric scalar does not directly mutate geometry.

## 5. Direction is pointer-derived in SK-07F

SK-07F deliberately does not add Ortho, Polar, Object Snap Tracking or inference.

Therefore the Direct Distance direction in this slice is the current free pointer direction.

If no non-degenerate direction exists, numeric distance input is rejected without changing the active stage.

This is an explicit extension seam:

```text
SK-07F:
raw pointer direction
        ↓
direct distance

later:
raw pointer
        ↓
Ortho / Polar / tracking / inference
        ↓
resolved direction
        ↓
same direct distance resolver
```

Later direction constraints must not bypass the SK-07F point request.

## 6. Context-first Command Line grammar

The Command Line becomes context-sensitive.

Submission precedence is:

```text
1. active semantic InputRequest
2. active command-local keyword grammar
3. top-level command activation grammar
```

SK-07F productionizes step 1 for bare numeric direct distance.

Examples:

### ordinary Select

```text
MOVE
```

still activates MOVE.

### active MOVE destination PointRequest

```text
50
```

means direct distance 50 in the current pointer direction.

It does not attempt to activate a command named `50`.

### active PointRequest + invalid token

The token is rejected with bounded diagnostic and the request remains active.

SK-07F does not yet add the future grip keyword `C`; it only establishes the routing seam that later command-local keywords will use.

## 7. Minimal scalar text grammar

SK-07F accepts a bare finite scalar distance.

Required user input examples:

```text
50
50.5
50,5
```

The Command Line adapter must accept:

- integer form;
- decimal form using `.`;
- the current UI locale decimal separator where different;
- optional surrounding whitespace.

Thousands/group separators, unit suffixes, coordinate tuples and polar syntax are out of scope and must be rejected rather than guessed.

Semantic Sketch code receives an already parsed physical scalar and remains Qt/locale independent.

Distance must be finite and non-negative.

`0` is a valid resolved distance; downstream tool semantics decide whether the resulting geometry is a no-op or invalid.

## 8. Line integration

After LINE accepts its first point:

- the second-point stage exposes a PointRequest;
- the first Line point is the Direct Distance base;
- current pointer position defines direction;
- entering `50` + Enter resolves a second point exactly 50 physical Sketch units from the first point in that direction;
- the resolved point is accepted through existing Line semantics.

Zero distance remains subject to existing zero-length Line rejection/no-op behavior.

The first Line point cannot be supplied by bare Direct Distance because no base/direction context exists.

## 9. Grip Reshape integration

During active non-center grip Reshape:

- the interaction-start grip position is the Direct Distance base;
- current pointer position defines direction;
- numeric distance resolves the current manipulation point;
- preview is recomputed from interaction-start authored geometry as in SK-07E;
- Enter submission commits the resolved Reshape using the existing direct-manipulation mutation path.

Examples include Line Start/End and the existing Circle/Arc reshape grips.

SK-07F does not redefine primitive-specific reshape geometry.

## 10. Grip Move integration

During active grip Move:

- interaction-start grip pivot is the Direct Distance base;
- current pointer position defines direction;
- numeric distance resolves destination;
- resulting translation is applied to the same frozen selection as SK-07E;
- Enter submission commits one atomic Move.

This applies to Move-only center grips and to non-center grips after Space switches Reshape → Move.

Space CycleEditMode remains authoritative and does not clear the active pointer candidate.

## 11. Normal MOVE integration

After normal MOVE has accepted its explicit Base Point and is awaiting destination:

- Base Point is the Direct Distance base;
- current pointer position defines direction;
- numeric distance resolves the destination point;
- Enter commits through the existing common-transform geometry/update path.

Selection-first and command-first object collection remain unchanged.

No numeric input is added to the object-selection stage or Base Point stage.

## 12. Normal COPY integration

After normal COPY has accepted its explicit Base Point and is awaiting placement:

- Base Point is the Direct Distance base;
- current pointer position defines direction;
- numeric distance resolves one placement point;
- accepted placement uses the existing SK-07C semantic duplication/fresh-identity path;
- one accepted placement remains one Part transaction and one Undo entry;
- COPY remains in repeated-placement mode exactly as before.

The original frozen source snapshot remains authoritative for every placement.

A zero-distance COPY placement remains the existing clean no-op with no ID consumption.

## 13. Preview semantics

Numeric text entry must not create a parallel preview model.

Before Enter, pointer movement continues to produce ordinary pointer preview.

When a valid scalar is submitted:

- the shared resolver produces one resolved point;
- the active operation computes geometry exactly as if that point had been supplied by pointer acceptance;
- the resulting operation commits according to its existing Enter/accept rule.

Future Dynamic Input may show live numeric preview, but SK-07F does not require a floating input HUD.

## 14. Focus and key ownership

Command Line Enter submits the current text to the active InputRequest when one exists.

Viewport Enter with no text keeps its existing operation semantics.

Space remains:

- CycleEditMode during active grip manipulation;
- Repeat Last Command in ordinary Select;
- transform-selection completion where already defined;
- literal space in text-entry focus.

SK-07F must not introduce global digit/key interception while ordinary widgets own focus.

## 15. Runtime lifecycle

InputRequest, pointer candidate, parsed scalar and resolution state are runtime-only.

They are cleared when:

- the local stage completes;
- Esc cancels the stage/session;
- the active tool ends;
- Sketch edit ends;
- active Sketch/Document changes;
- controller/runtime tears down.

No precision-input state is persisted.

## 16. Failure and no-op rules

Invalid text, non-finite scalar, missing direction or stale interaction state fail closed:

- no authored mutation;
- no revision;
- no dirty-state change;
- no history entry;
- current valid interaction stage remains active unless existing stale-state policy requires cancellation.

Resolved geometry still passes the active operation's existing semantic validation.

Numeric input never bypasses geometry validation.

## 17. Automated acceptance coverage

At minimum verify:

1. one active PointRequest owner; no per-tool numeric parser;
2. ordinary Select still treats `MOVE`/`COPY` as commands;
3. active PointRequest receives numeric text before top-level command dispatch;
4. invalid numeric token leaves the active request/stage unchanged;
5. `50`, `50.5` and current-locale decimal form parse as the same physical scalar where applicable;
6. grouping/unit/coordinate/polar syntax is rejected in this slice;
7. Line second point accepts pointer-directed Direct Distance;
8. Line numeric result is exactly the requested distance from first point;
9. zero-length Line result remains rejected/no-op;
10. grip Reshape accepts Direct Distance without changing owner/selection identity;
11. grip Move accepts Direct Distance and moves the frozen selection;
12. Space Reshape↔Move preserves the pointer candidate used for Direct Distance;
13. normal MOVE destination accepts Direct Distance;
14. normal COPY placement accepts Direct Distance;
15. repeated normal COPY remains active after a numeric placement;
16. numeric COPY placement uses fresh IDs and existing high-water semantics;
17. zero-distance COPY consumes no IDs/history;
18. pointer click path remains unchanged for all integrated clients;
19. invalid/missing pointer direction rejects Direct Distance cleanly;
20. numeric input creates no authored state before accepted commit;
21. Esc clears active precision-input state;
22. Sketch/Document switch clears active precision-input state;
23. Repeat Last Command and SK-07E Space CycleEditMode regressions remain green;
24. final runtime candidate passes exact-head Windows FULL;
25. required documentation and Product Browser freshness pass.

## 18. Manual Windows verification

Final candidate requires Owner verification of:

- LINE: click first point, point cursor in a direction, type `50`, Enter → exact 50-unit segment;
- Line endpoint grip Reshape: point cursor, type `50`, Enter → grip endpoint displaced according to the shared resolved point;
- grip → Space → Move: point cursor, type `50`, Enter → frozen selection moves exactly 50;
- center-grip Move behaves the same;
- normal MOVE after Base Point: direction + `50` + Enter gives exact 50 translation;
- normal COPY after Base Point: direction + `50` + Enter creates one exact placement and remains ready for another;
- click placement still works in all those workflows;
- numeric entry with no usable direction is rejected without mutation;
- decimal input works with `.` and current locale decimal separator;
- Esc cancels without leaking numeric state;
- Repeat Last Command, Space CycleEditMode, Rotate/Scale/Mirror, Save/reopen and navigation do not regress.

## Documentation impact

Internal docs: required  
User/Product docs: required  
Reason: SK-07F changes the cross-cutting Command Line/input grammar and establishes the shared precision-input seam for later CAD tools.

## 20. Expected implementation surface

Expected bounded production changes after Owner acceptance:

- `src/sketch/**` for provider-neutral active PointRequest/direct-distance resolution state;
- `src/ui/**` for context-first Command Line dispatch and locale-aware bare-scalar adapter;
- existing Line/direct-manipulation/common-transform paths as clients of the same resolver;
- tests;
- internal/Product docs and generated Browser;
- `work/**`.

No persistence schema change is expected.

Stop for Owner review if implementation requires:

- tool-specific numeric parsers;
- a second interaction/input authority;
- authored precision-input state;
- unit-expression grammar;
- absolute/relative Cartesian coordinate syntax;
- polar syntax;
- Dynamic Input widget/HUD;
- Ortho/Polar direction locking;
- Object Snap/tracking/inference;
- Grip Copy mutation policy;
- changing fresh-identity semantics.

## 21. Explicitly out of scope

SK-07F does not authorize:

- Grip Copy keyword/modifier behavior;
- Reshape+Copy;
- Rotate/Scale/Mirror+Copy;
- ordinary-Select RMB context;
- absolute/relative Cartesian coordinate entry;
- relative polar entry;
- unit suffix/expression parsing;
- Dynamic Input fields or Tab locking;
- Ortho;
- Polar;
- Object Snap;
- Object Snap Tracking;
- inference guides;
- Temporary Snap Override;
- constraints/solver/authored dimensions;
- R8 Measure work.

## 22. Roadmap sequencing consequence

SK-07F intentionally pulls the **minimal shared Input Resolution + Direct Distance foundation** forward from the old R9 sequence because remaining R7 grip/Copy command grammar depends on it.

This does not activate all of R9.

Proposed roadmap v1.3 keeps later R9 responsible for expanding the same foundation with:

- absolute/relative Cartesian input;
- relative polar input;
- unit-aware expression parsing;
- Dynamic Input;
- Ortho/Polar configuration and locking.

R10 remains responsible for Object Snap, tracking and inference.

## 23. Activation and completion boundary

SK-07F is proposal-only.

No production implementation is authorized until explicit Owner acceptance of both:

- this Work Contract;
- proposed Sketcher roadmap v1.3 sequencing.

If accepted as written, completion requires:

- one shared provider-neutral PointRequest/direct-distance path;
- context-first Command Line numeric routing;
- Line, grip Reshape, grip Move, normal MOVE and normal COPY integrated as clients;
- click input preserved;
- no Ortho/Polar/OSNAP/Dynamic Input/Grip Copy implementation;
- required internal and PL/EN Product documentation current;
- final exact-head Windows FULL passes;
- Owner manual Windows verification passes;
- closeout CLOSURE passes;
- later Grip Copy and remaining R7/R9/R10 work stays inactive until separately accepted.
