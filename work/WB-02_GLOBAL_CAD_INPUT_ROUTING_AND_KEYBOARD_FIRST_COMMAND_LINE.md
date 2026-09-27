# WB-02 — Global CAD Input Routing and Keyboard-First Command Line

**Status:** ACCEPTED — ACTIVE  
**Proposed:** 2026-09-27  
**Owner acceptance:** 2026-09-27  
**Decision class:** D2 shared input/workspace architecture + bounded D1 implementation  
**Foundation:** 1.0 (foundation-v1.0)  
**Architecture:** ADR-0003, ADR-0006, ADR-0007, ADR-0008, ADR-0009, ADR-0011  
**Baseline:** main at 6141c9306f0ed977130b688b8b2a1573493c5186 after completed SK-07F

## 1. Goal

Implement the first bounded production slice of workspace-global CAD input routing.

The user-visible goal is simple:

    viewport remains focused
    → user starts typing
    → global Command Line mirrors the text immediately
    → Enter submits to the active semantic context

No click into Command Line is required.

The architecture goal is broader: establish one reusable keyboard/text transport and focus-arbitration mechanism that future Part, Assembly, Drawing and shared 2D tools can consume without moving their semantic meaning into Project Workspace UI.

Existing SK-07F PointRequest/Direct Distance behavior is the first production client.

## 2. Scope IN

WB-02 includes:

- one provider-neutral generic CAD input routing/session contract;
- one active generic input endpoint selected by current active Workbench/editor context;
- one workspace-owned live text buffer;
- one workspace-global Command Line presentation bound to that buffer;
- keyboard-first printable-text capture from normal CAD surfaces without focus transfer;
- focus arbitration that yields to real text-entry widgets and modal/text-edit contexts;
- preservation of normal application shortcuts;
- generic prompt/submission/diagnostic transport without Part/Sketch tool enums in the router;
- migration of current Part/Sketch Command Line submission to the generic endpoint;
- migration of current SK-07F context-first Direct Distance submission through that endpoint;
- ordinary top-level Sketch command text entry through the same buffer when no higher semantic request owns the token;
- lifecycle detach/clear on Document switch, Workspace navigation, Workbench/editor replacement and teardown;
- automated semantic and Qt integration coverage;
- required internal and PL/EN Product documentation and regenerated Product Browser.

## 3. Scope OUT

WB-02 does not authorize:

- Extrude or any other Part modeling feature;
- AssemblyDocument or Assembly tools;
- DrawingDocument or Drawing tools;
- implementation of generic LengthRequest, AngleRequest, ScalarRequest, CountRequest or other speculative future request kinds;
- new unit suffix/expression grammar;
- absolute or relative Cartesian coordinate grammar;
- polar coordinate grammar;
- Dynamic Input HUD/fields;
- Ortho or Polar;
- Object Snap, Object Snap Tracking or inference;
- Grip Copy / Reshape+Copy / Rotate+Copy / Scale+Copy / Mirror+Copy;
- ordinary-Select RMB context;
- new command aliases;
- broad Space-as-Enter grammar changes;
- command-history persistence;
- CAD persistence schema changes.

These remain separately contracted work.

## 4. Public architecture contract

The production design must preserve three distinct layers.

### 4.1 Workspace input mechanism

Owns:

- live runtime text buffer;
- currently attached generic input endpoint;
- submission transport;
- runtime prompt/diagnostic projection;
- context generation/lifetime protection as needed;
- no CAD authored state.

It does not know specific CAD tools or request kinds.

### 4.2 Qt input/presentation adapter

Owns:

- keyboard event capture at the appropriate common UI boundary;
- inspection of current focus/text-edit ownership;
- Command Line widget/presentation;
- mapping of Qt text/key events into provider-neutral input intents;
- no CAD semantic meaning.

### 4.3 Active semantic endpoint

Owned/adapted by the current Workbench/editor/tool context.

It decides:

- whether an active semantic InputRequest owns submitted text;
- tool-local keyword handling;
- current Document/Workbench command handling;
- semantic validation and diagnostics;
- whether submission is accepted/rejected;
- actual command/tool state transitions.

Existing Part/Sketch is the only concrete production endpoint required in WB-02.

## 5. Global Command Line placement

The visible Command Line moves out of Sketch-only ownership and is hosted by the common Project Workspace Shell.

It remains one surface while the user changes active Part Documents.

The Command Line does not disappear merely because Sketch edit ends.

When a Project Workspace dashboard has no active CAD input endpoint, the surface may show a neutral prompt. WB-02 does not add Project-level command execution; Enter with unsupported text fails closed and must not route to a hidden Document.

Current Part/Sketch prompts are adapted into the global surface.

## 6. Keyboard-first behavior

With a normal CAD viewport/workbench surface focused:

- typing 5 immediately puts 5 in the global buffer;
- typing 0 next produces 50;
- Command Line presentation shows 50;
- viewport focus remains on the viewport;
- Enter submits 50;
- the active semantic context decides its meaning.

The same applies to command text such as MOVE when current context permits command activation.

The implementation must not implement this by calling setFocus on the Command Line for every printable key.

## 7. Focus arbitration

At minimum verify these ownership classes:

### Normal CAD surface

Printable unmodified text goes to CAD input buffer.

### Visible Command Line editor

Typing edits the same shared buffer directly.

### Other editable field

Typing remains local to that field and CAD input buffer is unchanged.

The test surface must include at least one independent text-edit widget that is not Command Line so this rule is proven, not assumed.

### Modal/dialog/text composition context

Background CAD auto-capture is suppressed.

### Application shortcuts

Existing Control/Alt/Meta shortcuts continue through their established shortcut/action path and are not inserted into CAD input text.

Exact Qt detection logic may use focus widget capabilities/types and event semantics as D1, but must not hard-code only the current Command Line as the sole exception.

## 8. Buffer editing/submission grammar

WB-02 supports the minimum global buffer grammar:

- printable text append/ordinary editing needed for current command/scalar input;
- Backspace edits current buffer;
- Enter/Return submits non-empty buffer;
- Esc clears a non-empty buffer first;
- accepted submission clears buffer;
- rejected submission creates no mutation and leaves the active context valid; text remains available for correction unless the context ended;
- click/focus editing of Command Line and keyboard-first viewport editing share one buffer.

When the buffer is empty, existing accepted CAD key behavior remains unchanged.

WB-02 must specifically preserve:

- SK-07D Repeat Last Command;
- SK-07E Space CycleEditMode;
- transform-selection Enter/Space completion;
- direct-manipulation Enter commit;
- Delete selection;
- hierarchical Esc.

No new broad Space-as-Enter behavior is introduced.

## 9. Submission precedence

The Part/Sketch endpoint must retain SK-07F context-first precedence:

    active semantic InputRequest
    → tool-local keyword grammar
    → top-level active Workbench command grammar

The generic router does not implement these branches. It submits text to the active endpoint.

Future Project/application grammar levels are extension seams only and are not implemented here.

## 10. SK-07F migration

Current production Direct Distance remains semantically unchanged.

Required keyboard-first scenarios include:

### LINE

    activate LINE
    click first point
    move pointer to establish direction
    type 50 with viewport focused
    Enter
    → exact 50-unit next point

### Grip Reshape / Grip Move

    activate grip
    establish direction
    type 50
    Enter
    → same resolved-point path as completed SK-07F

### Normal MOVE / COPY

    choose Base Point
    establish direction
    type 50
    Enter
    → same SK-07F destination/placement semantics

Existing dot/current-locale bare scalar grammar remains unchanged in WB-02.

WB-02 does not expand numeric grammar.

## 11. Ordinary command text migration

With Sketch ordinary Select active and viewport focus:

    type MOVE
    Enter

must feed the same existing command activation grammar currently reachable through direct Command Line editing.

The workspace router must not own the list of Sketch commands.

While an active PointRequest owns text, the same MOVE token must retain current context-first rejection/ownership behavior rather than silently launching MOVE.

## 12. Document/workspace lifecycle

The live buffer is associated with the current active input context, not with a hidden Document.

Required behavior:

- Part A has partially typed input;
- switch to Part B;
- Part A endpoint detaches;
- live buffer clears;
- subsequent typing/submission can only reach Part B;
- switching to Workspace dashboard detaches the Document endpoint;
- no hidden Part receives text;
- returning to Part A does not restore stale partially typed text.

Prompt presentation updates to the newly active context.

Endpoint detach occurs before owning Workbench/Document runtime destruction.

## 13. Runtime-only state

The following are runtime-only:

- CAD input router/session;
- active endpoint;
- live text buffer;
- focus arbitration state;
- current prompt;
- current input diagnostic;
- any session-only Command Line history added incidentally for existing presentation compatibility.

They create no:

- DocumentRevision;
- needsSave;
- CAD Undo entry;
- persistence data.

No schema change is authorized.

## 14. Failure rules

Fail closed on:

- no active endpoint;
- stale/detached endpoint;
- endpoint/context changed during submission;
- rejected semantic token;
- unsupported text while Workspace dashboard has no grammar;
- ambiguous keyboard ownership;
- input that belongs to another editable widget.

Failure must not guess a Document, command or request.

Structured/typed result at the semantic endpoint is preferred; UI text remains presentation.

## 15. Architecture boundary tests

Add automated coverage that protects at least:

1. generic CAD input contract contains no Qt, OCCT, Part, Drawing, Assembly or Sketch types;
2. router can be tested with fake generic endpoints without CAD domains;
3. only one endpoint is active at a time;
4. attaching a new endpoint invalidates/detaches old live routing;
5. clearing/detaching buffer has no domain mutation side effect;
6. router does not parse PointRequest/Direct Distance itself;
7. Project Workspace Shell does not switch on SketchTool/Part commands;
8. Part/Sketch endpoint is an adapter into existing semantic controller state rather than a second tool state machine.

## 16. Qt / integration acceptance coverage

At minimum verify:

1. global Command Line exists at common workspace level;
2. it remains available outside Sketch edit while a Project shell is active;
3. viewport printable typing changes the shared buffer without changing viewport focus;
4. directly focused Command Line changes the same buffer;
5. independent text editor typing does not change CAD input buffer;
6. Ctrl+S / Ctrl+Z style shortcuts are not captured as text;
7. Backspace edits buffer;
8. Enter submits non-empty buffer;
9. Esc clears non-empty buffer without prematurely cancelling the underlying CAD tool;
10. after buffer clear, another Esc reaches normal tool cancellation;
11. empty-buffer Enter/Space regressions remain correct;
12. Line Direct Distance works without clicking Command Line;
13. grip Reshape/Move Direct Distance works without clicking Command Line;
14. normal MOVE/COPY Direct Distance works without clicking Command Line;
15. ordinary MOVE command text can be launched keyboard-first from Select;
16. active PointRequest still outranks top-level MOVE text;
17. invalid token creates no revision/dirty/history change;
18. switching Document clears buffer and old endpoint receives nothing further;
19. switching to Workspace dashboard clears/detaches Document input;
20. switching back never restores stale partial token;
21. all completed SK-07A through SK-07F tests remain green;
22. exact-head Windows FULL passes;
23. documentation freshness/verification passes.

## 17. Manual Windows verification

Final candidate requires Owner verification of:

- with Sketch viewport focused, LINE → first point → direction → type 50 → Enter produces exact 50 without clicking Command Line;
- typed characters appear immediately in visible Command Line while viewport focus remains active;
- normal MOVE command can be typed and launched from viewport in ordinary Select;
- grip Reshape and grip Space→Move accept 50 keyboard-first;
- normal MOVE/COPY destination accepts 50 keyboard-first;
- clicking Command Line and typing still produces identical semantic behavior;
- a separate editable UI field receives ordinary typing without feeding Command Line;
- Backspace and Esc edit/clear the Command Line buffer intuitively;
- Esc after an empty buffer still cancels the active CAD interaction;
- Space CycleEditMode and Repeat Last Command do not regress;
- Ctrl+S/Ctrl+Z and ordinary application shortcuts do not become Command Line text;
- changing active Document or navigating to Workspace cannot leak partial input to the old Document;
- Save/reopen/navigation and completed Sketch workflows remain functional.

## 18. Expected implementation surface

Expected bounded changes after Owner acceptance may include:

- shared Application/Platform input contract/runtime source;
- Project Workspace Shell for global Command Line hosting;
- common Qt input adapter/event routing;
- CadWorkbench/Part Sketch adapter migration;
- tests/CMake;
- internal and PL/EN Product docs plus Product Browser;
- work lifecycle metadata.

Exact filenames/classes are D1.

No persistence source should need a semantic schema change.

## 19. Stop conditions

Stop for Owner review if implementation appears to require:

- Project Workspace Shell knowing concrete Part/Sketch/Assembly/Drawing tools;
- a second semantic tool/request state authority;
- tool-specific global key interceptors;
- duplicated numeric parsers per tool;
- Qt types in provider-neutral CAD input contracts;
- persistent Command Line/input buffer state;
- hidden/inactive Document receiving routed input;
- changing SK-07F Direct Distance meaning;
- implementing a future request type solely to justify the abstraction;
- expanding to Dynamic Input, units/coordinates, Ortho/Polar, snapping, Grip Copy or RMB.

## Documentation impact

Internal docs: required  
User/Product docs: required in PL and EN  
Reason: WB-02 changes the global keyboard workflow and Command Line ownership across the common workspace.

## 20. Activation and completion boundary

ADR-0011, WB-02 and roadmap v1.4 were explicitly Owner-accepted on 2026-09-27. Production implementation is authorized only within this bounded scope.

Completion requires:

- workspace-global keyboard-first input mechanism;
- global Command Line presentation;
- robust focus/text-editor arbitration;
- generic active endpoint with no domain knowledge in the router;
- existing Sketch Command Line and SK-07F Direct Distance migrated as first client;
- no later precision/CAD feature expansion;
- automated boundary/integration coverage;
- current docs/Product Browser;
- exact-head Windows FULL;
- Owner manual Windows PASS;
- closeout CLOSURE.
