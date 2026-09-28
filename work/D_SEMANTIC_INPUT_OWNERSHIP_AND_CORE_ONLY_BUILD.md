# D — Semantic CAD Input Ownership and Core-only Build

**Status:** PROPOSED — INACTIVE  
**Proposed:** 2026-09-28  
**Decision class:** D2 Architecture implementation under accepted ADR-0011  
**Foundation:** 1.0 (foundation-v1.0)  
**Program authority:** `work/AUDIT-01_ARCHITECTURE_STABILIZATION_PROGRAM.md` v1.0 — Package D  
**Architecture authority:** ADR-0011 — Workspace-global CAD Input routing and semantic request ownership  
**Baseline:** `main` at `0a0f6a71ae59109a99bcc44e40a6ff2520431e19` after completed C2 KEEP

## 1. Goal

Move CAD text-token meaning out of QWidget/UI code and make the current Line/Move/Copy semantic input path executable and testable through a non-QWidget, non-OCCT application API.

At the same time add an explicit CMake configuration that compiles and tests the neutral SS2 semantic core without locating or building Qt, OpenCASCADE or provider/UI targets.

D is a boundary correction. It is not a new input language and not a redesign of Sketch interaction.

## 2. Confirmed baseline findings

### D.1 CadInputSession is already neutral

`application::CadInputSession` owns:

- live text buffer;
- endpoint attachment/detachment;
- semantic-context generation binding;
- stale-token rejection;
- submission lifecycle;
- runtime diagnostic retention.

Its current boundary test explicitly forbids Qt, OCCT, PartDocument, SketchTool and PointRequest ownership in `cad_input.hpp/.cpp`.

That transport boundary is correct and must be preserved.

### D.2 Semantic token interpretation still lives in QWidget

`ui::CadWorkbench` currently implements `application::ICadInputEndpoint`.

Its `submitCadInput()` currently owns:

- stale semantic-generation validation;
- active Sketch context validation;
- token trimming;
- PointRequest precedence;
- bare Direct Distance parsing;
- command keyword interpretation for
  `SELECT/LINE/CIRCLE/ARC/MOVE/COPY/ROTATE/SCALE/MIRROR`;
- command activation success checks;
- semantic rejection strings.

The file-local `parseBareSketchDistance()` is implemented with `QString` and `QLocale`.

Therefore current token meaning is still coupled to a QWidget implementation even though ADR-0011 says the UI must be an adapter and the active semantic layer must own meaning.

This AUDIT-01 D finding is **CONFIRMED**.

### D.3 Core-only build does not currently exist

`src/CMakeLists.txt` unconditionally calls:

```cmake
find_package(Qt6 6 REQUIRED COMPONENTS Widgets)
find_package(OpenCASCADE CONFIG REQUIRED)
```

and `tests/CMakeLists.txt` unconditionally requires Qt Widgets/Test.

Consequently the neutral Core/Sketch/Part/Application semantic dependency set cannot currently be configured and tested as an explicit supported build mode without Qt/OCCT discovery.

This AUDIT-01 D finding is **CONFIRMED**.

## 3. D2 decision boundary

D changes subsystem ownership and introduces a new application-facing semantic input contract. That is D2.

No new ADR is required **if** implementation follows ADR-0011 exactly:

```text
Qt/focus/widgets
    ↓
CadInputSession transport
    ↓
semantic application endpoint
    ↓
Sketch interaction semantics / DocumentSession
```

Owner acceptance of this Work Contract authorizes that bounded ADR-0011 implementation.

Stop and propose an ADR amendment if implementation would instead:

- move semantic meaning back into the workspace router;
- make CadInputSession aware of Sketch/Part tools;
- make domain code depend on Qt/Viewer/provider types;
- change which layer owns the current semantic InputRequest;
- introduce a materially different generic input architecture.

## 4. Scope IN

- extract current Sketch CAD token interpretation from `CadWorkbench` into neutral Application code;
- extract current bare Direct Distance numeric parsing from Qt/QLocale code into neutral Application code;
- provide only the minimum semantic target/port needed by the existing Sketch interaction controller;
- keep `CadInputSession` as generic transport;
- make `CadWorkbench` a thin UI/presentation adapter for CAD input semantics;
- preserve existing context-generation stale-token protection;
- preserve existing command keywords and precedence;
- preserve current Direct Distance behavior;
- non-QWidget/non-OCCT semantic tests for current Line/Move/Copy paths;
- GUI parity/regression tests;
- an explicit core-only CMake configuration with no Qt/OCCT/provider discovery/build;
- core-only CI/build verification plus the normal full Windows GUI/provider verification;
- required internal documentation and generated Browser;
- lifecycle/governance records.

## 5. Scope OUT

- adding new CAD command keywords;
- aliases or abbreviations;
- coordinate entry;
- polar entry;
- unit expressions;
- signed Direct Distance;
- exponent notation;
- numeric Rotate angle;
- numeric Scale factor;
- Dynamic Input;
- command history/autocomplete;
- Ortho/Polar/Snap;
- changing pointer grammar;
- changing Sketch selection grammar;
- changing Line/Move/Copy/Circle/Arc/Rotate/Scale/Mirror semantics;
- changing Undo/Redo, dirty state or history representation;
- persistence/schema changes;
- Viewer/provider refactor beyond build gating;
- moving all Sketch interaction/controller logic into Application;
- speculative generic hierarchies for future Part/Assembly/Drawing requests;
- Linux product support.

## 6. Required responsibility split

After D the authority must read:

```text
Qt adapter
  events / focus / QWidget presentation / local text editing
        ↓
CadInputSession
  buffer / endpoint lifecycle / context-generation transport
        ↓
Application semantic CAD endpoint
  active request precedence
  current accepted token grammar
  neutral numeric parsing
  typed semantic action/result
        ↓
minimal Sketch semantic target
  current tool/request state
  activate existing tool
  submit existing Direct Distance
        ↓
SketchInteractionState + DocumentSession
  stage/base/direction/preview intent
  command validation / transaction / history
        ↓
authored domain state
```

No Qt, OCCT, presentation token or QWidget identity may cross into the semantic endpoint contract.

## 7. CadInputSession preservation

`CadInputSession` remains the workspace-global generic transport defined by ADR-0011.

It must not learn:

- `SketchTool`;
- `PointRequest`;
- Part/Assembly/Drawing types;
- numeric unit semantics;
- Qt locale types;
- command keywords.

Existing endpoint generation behavior remains authoritative:

- a live token belongs to the semantic context generation that created it;
- context change clears stale buffer/diagnostic state;
- submission validates the generation before semantic interpretation;
- Enter consumes the submitted token whether accepted or rejected.

The existing `wb02.cad_input_session` and CAD-input boundary test remain regression authority.

## 8. Bounded semantic endpoint

D introduces one bounded Application semantic endpoint for the **current production Sketch input surface**.

Exact file/class names are implementation-local, but its public semantic contract must be no broader than:

- current semantic context generation;
- current Sketch command/request snapshot needed for token interpretation;
- current decimal-separator input policy;
- token interpretation;
- typed activation/direct-distance result;
- existing rejection diagnostic.

The endpoint may use current Sketch semantic types such as `SketchTool`, `PointRequest` and `DirectEditMode` because it is a semantic Application adapter. Those types remain forbidden in the generic `CadInputSession` transport.

Do not introduce a generic future `LengthRequest/AngleRequest/ScalarRequest/IntegerRequest` hierarchy in D.

## 9. Typed semantic action boundary

Raw submitted text must not be switched on inside QWidget after D.

The neutral semantic layer must resolve a token to one of the current bounded semantic outcomes, conceptually:

```text
Activate existing Sketch tool
Submit Direct Distance scalar
Reject with diagnostic
```

The UI adapter may call the existing controller operation selected by that typed outcome, but it must not reinterpret the original text.

It is acceptable for a minimal neutral semantic target/port to expose current operations such as:

- query active semantic request/tool state;
- activate one existing `SketchTool`;
- submit one parsed Direct Distance scalar;
- verify the expected resulting tool/context.

It must not expose QWidget, QLocale, viewport/provider or presentation state.

## 10. Current request precedence

Existing precedence is preserved exactly.

When an active `PointRequest` exists:

1. the submitted token belongs to that request;
2. it is **not** interpreted as a top-level command keyword;
3. Direct Distance availability is checked;
4. malformed/unavailable/unresolvable input rejects with no authored mutation.

Example: while a PointRequest owns input, text `LINE` is a malformed distance token; it must not switch tools.

When no semantic request owns the token, the existing top-level Sketch command keywords may be interpreted.

## 11. Bare Direct Distance parser

Move the parser out of `cad_workbench.cpp` and remove its dependency on `QString/QLocale`.

The neutral parser accepts:

- digits;
- at most one decimal separator;
- `.` always;
- the current UI-locale decimal separator supplied as neutral UTF-8 formatting context;
- a finite value `>= 0`.

It continues to reject:

- sign characters;
- grouping separators;
- units;
- tuples;
- polar syntax;
- exponent notation;
- mixed `.` plus locale decimal separator when they differ;
- empty/no-digit input;
- non-finite/overflow input.

The Qt adapter may read the current `QLocale` and translate only the decimal separator into a neutral string/value. QLocale itself must not enter Application code.

No locale expansion beyond current behavior is authorized.

## 12. Current command grammar

With no active semantic request, preserve these exact case-insensitive keywords:

```text
SELECT
LINE
CIRCLE
ARC
MOVE
COPY
ROTATE
SCALE
MIRROR
```

No alias or abbreviation is added.

Unknown tokens preserve the current rejection behavior and produce no authored mutation.

Existing command activation remains routed through the current Sketch interaction controller/state and therefore retains existing selection-first/command-first semantics.

## 13. CadWorkbench after D

`CadWorkbench` may remain the lifetime/composition owner for the Sketch interaction controller and the UI adapter object, but it must stop owning text-token meaning.

Specifically:

- remove the file-local semantic numeric parser;
- remove the raw-string command keyword switch from QWidget code;
- `submitCadInput()`, if retained for interface compatibility, becomes a thin delegation/adaptation boundary;
- no QWidget method decides whether text means LINE versus Direct Distance;
- semantic diagnostic text originates from the neutral semantic endpoint;
- QWidget may present/mirror that diagnostic in status surfaces without changing its meaning.

Prompt formatting may remain presentation code if it is derived from semantic state and does not interpret submitted token syntax.

## 14. Non-QWidget semantic evidence

Add a neutral test target that does **not** include or link:

- QWidget/Qt Widgets;
- OCCT;
- `simplesolid2_ui`;
- `simplesolid2_viewer_qt_occt`;
- provider-native types.

It must exercise at least:

### D.1 Command interpretation

- LINE token activates existing Line semantic intent;
- MOVE token activates existing Move intent using current selection rules;
- COPY token activates existing Copy intent using current selection rules;
- unknown command rejects with no state/revision/history change.

### D.2 Direct Distance precedence

For an active PointRequest:

- a valid `.` decimal scalar resolves through the existing request path;
- a supplied locale decimal separator resolves equivalently;
- mixed separators reject;
- LINE text rejects as request input rather than changing tool;
- unavailable Direct Distance rejects;
- missing pointer direction rejects with no commit.

### D.3 Line semantic result

For current Line PointRequest behavior:

- pointer establishes direction;
- submitted scalar produces the same resolved point as current implementation;
- accepted commit produces exactly the current authored geometry/revision/history effect.

### D.4 Move semantic result

For current Move destination behavior:

- selected entities and Base remain authoritative;
- pointer establishes direction;
- scalar resolves destination through the existing interaction semantics;
- accepted Move preserves EntityIds;
- one accepted commit -> one Undo entry;
- failure/rejection changes no authored state/revision/history.

### D.5 Copy semantic result

For current COPY placement behavior:

- frozen original source selection remains authoritative;
- scalar placement uses the existing direct-distance resolver;
- accepted copy creates fresh EntityIds;
- originals remain unchanged/selected as currently defined;
- each accepted placement creates one transaction/Undo entry;
- rejected/zero/cancelled placement consumes no fresh identity beyond existing rules.

The test may use a small neutral semantic harness, but it must not duplicate an alternative production command implementation merely to make the test pass.

## 15. GUI parity evidence

Existing GUI behavior remains required.

Automated Qt tests must prove that the UI adapter reaches the same semantic endpoint/result rather than retaining a second parser/command switch.

Retain/regress:

- workspace-global viewport keyboard capture;
- Command Line click/focus equivalence;
- context-generation stale-buffer clearing;
- text-editor ownership;
- popup/modal/foreign-window exclusion;
- Delete/Backspace/Esc/Enter precedence;
- existing Line/Move/Copy precision-input behavior;
- current diagnostic/rejection flow.

A boundary/static check should fail if raw command grammar or the bare-distance parser is reintroduced into `cad_workbench.cpp`.

## 16. Core-only CMake configuration

Introduce one explicit option:

```cmake
SS2_BUILD_DESKTOP=ON
```

Default remains `ON`.

With:

```cmake
-DSS2_BUILD_DESKTOP=OFF
```

configuration must not require or build:

- Qt6;
- OpenCASCADE;
- `simplesolid2_ui`;
- `simplesolid2_viewer_qt_occt`;
- desktop executable/provider targets;
- Qt/provider tests.

The core-only configuration must build and test the neutral dependency set required for current authored semantics, including at least:

- Core;
- Sketch;
- Persistence;
- Part;
- generic CAD input transport;
- Application semantic services including the new CAD semantic endpoint.

Provider-neutral libraries not required by this semantic dependency set need not be built in core-only mode.

This is a build/test boundary, not a new product distribution or Linux support promise.

## 17. Core-only verification

Required automated evidence includes an explicit configure/build/test path equivalent to:

```powershell
cmake -S . -B build/core-only \
  -DSS2_BUILD_DESKTOP=OFF \
  -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=TRUE \
  -DCMAKE_DISABLE_FIND_PACKAGE_OpenCASCADE=TRUE

cmake --build build/core-only --config Release
ctest --test-dir build/core-only -C Release --output-on-failure
```

The exact repository command/script may differ, but evidence must prove that the core-only graph does not discover Qt/OCCT accidentally.

A dedicated CMake/CI self-test must fail closed if desktop/provider targets enter the core-only graph.

Normal Windows FULL with desktop/provider targets remains mandatory and cannot be replaced by the core-only gate.

## 18. Dependency/boundary checks

Extend boundary verification so:

- generic `cad_input.hpp/.cpp` remains free of semantic domain types;
- new semantic input Application files are free of Qt/OCCT/provider/UI includes;
- `cad_workbench.cpp` no longer contains `parseBareSketchDistance` or the raw keyword grammar switch;
- core-only CMake does not call required Qt/OCCT discovery;
- no source under Core/Sketch/Part/Application includes QWidget or OCCT provider headers through D.

Do not enforce this by fragile broad substring rules that reject legitimate documentation/comments; checks should target actual boundary files/includes/contracts.

## 19. Failure and consistency

Token rejection must remain side-effect free.

Before an accepted semantic action:

- context generation is validated;
- request precedence is established;
- token parsing/typing succeeds.

Only then may an existing controller/DocumentSession action execute.

Existing downstream invariants remain authoritative:

- validation failures do not mutate authored state;
- one accepted command commit -> one Undo entry;
- Move preserves identity;
- Copy allocates fresh identity only at accepted commit;
- B2 transaction consistency remains unchanged;
- C1/C2 history behavior remains unchanged;
- B1 persistence/save behavior remains unchanged.

D does not introduce a second transaction or history path.

## 20. Expected implementation surface

Expected bounded files may include:

- `src/application/include/simplesolid2/application/cad_input_semantics.hpp`;
- `src/application/cad_input_semantics.cpp`;
- minimal neutral semantic target/port declarations;
- `src/ui/cad_workbench.hpp/.cpp` delegation cleanup;
- minimal `PartSketchInteractionController` adapter changes;
- `src/CMakeLists.txt`;
- root `CMakeLists.txt`;
- `tests/CMakeLists.txt`;
- new non-Qt semantic input tests;
- existing CAD-input/precision-input GUI regressions;
- focused boundary/CMake verification scripts;
- `docs/internal/CAD_WORKBENCH_VIEWER.md`;
- `docs/internal/BUILD_AND_TEST.md`;
- generated Product Browser;
- work lifecycle records.

If implementation requires a broad move/rewrite of the whole Sketch interaction controller, stop for scope review.

## 21. Documentation impact

Internal docs: required  
User/Product docs: not required

Reason: D corrects semantic ownership and build/test boundaries while preserving the current visible command grammar, precision-input behavior and CAD workflow. If implementation changes any visible grammar, prompt/rejection semantics or supported workflow, reclassify Product docs to required before proceeding.

## 22. Manual Windows verification

Because the GUI adapter/input-routing path changes, Owner manual verification is required before closeout.

Minimum smoke:

1. with viewport CAD focus, type `LINE` without clicking Command Line and confirm Line activates;
2. create/continue Line with pointer direction + bare Direct Distance and confirm expected geometry;
3. activate MOVE and commit one Direct Distance move;
4. activate COPY and commit one Direct Distance placement;
5. verify clicking Command Line remains behaviorally equivalent;
6. verify an ordinary text/property editor retains keyboard ownership;
7. verify rejected token creates no authored change and leaves the active semantic tool/request authoritative.

No exhaustive UI retest is required beyond this bounded D surface unless automated evidence exposes a regression.

## 23. Stop conditions

Stop for Owner review if implementation requires:

- changing ADR-0011 ownership;
- changing current accepted command grammar;
- changing Direct Distance syntax/locale behavior;
- moving semantic meaning into CadInputSession;
- moving Qt/OCCT/provider types into Application/domain;
- adding a generic future InputRequest hierarchy;
- adding a third-party parser/dependency;
- changing Sketch pointer/selection grammar;
- changing DocumentSession transaction/history semantics;
- persistence/schema changes;
- broad PartSketchInteractionController rewrite;
- Linux desktop support work;
- weakening full GUI/provider verification.

## 24. Activation gate

This proposal does **not** activate Package D.

Activation requires explicit Owner acceptance of this D2 Work Contract.

No new ADR is required for the proposed implementation because ADR-0011 already owns the architectural decision. If implementation discovers that ADR-0011 must change, stop and obtain a separately accepted ADR amendment.

Only after Owner acceptance may:

- `work/ACTIVE.yaml` switch from completed C2 to active D;
- production semantic-input ownership refactoring begin;
- core-only build/test configuration be added.

## 25. Completion gate

D completes only after:

- raw submitted-token grammar is no longer owned by QWidget code;
- bare Direct Distance parsing is neutral and Qt-free;
- CadInputSession remains generic transport;
- current request precedence and command grammar are preserved;
- neutral Line/Move/Copy semantic tests pass without QWidget/OCCT;
- GUI adapter parity/regressions pass;
- `SS2_BUILD_DESKTOP=OFF` configures/builds/tests without Qt/OCCT discovery;
- default desktop Windows FULL remains green;
- required internal docs and generated Browser are current;
- exact-head Windows FULL passes;
- Owner bounded manual Windows verification passes;
- governance CLOSURE passes;
- merge to main.

After D completion, AUDIT-01 schedules E1 next. D does not activate E1 automatically.
