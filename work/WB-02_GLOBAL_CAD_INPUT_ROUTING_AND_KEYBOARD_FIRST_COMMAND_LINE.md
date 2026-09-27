# WB-02 — Global CAD Input Routing and Keyboard-First Command Line

**Status:** COMPLETED  
**Proposed:** 2026-09-27  
**Owner acceptance:** 2026-09-27  
**Owner manual Windows verification:** PASS — 2026-09-27  
**Final exact-head Windows FULL:** #626 — `c66d550280485430ab6ddeb92d998919260e9b00` — PASS — 73/73  
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
- every non-empty Enter/Return submission consumes the live buffer;
- accepted submission clears any prior diagnostic;
- rejected submission creates no mutation, leaves the active semantic context valid, keeps the submitted text out of the editable buffer and publishes a bounded diagnostic;
- correction of the current token happens before Enter through ordinary editing/Backspace;
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
8. Enter submits and consumes a non-empty buffer whether accepted or rejected;
9. rejected input leaves the semantic tool/stage active, creates no mutation and presents a diagnostic with an empty editable buffer;
10. a long diagnostic remains single-line in permanently reserved width and does not change Command Line height or input width;
11. Esc clears non-empty buffer without prematurely cancelling the underlying CAD tool;
12. after buffer clear, another Esc reaches normal tool cancellation;
13. empty-buffer Enter/Space regressions remain correct;
14. Line Direct Distance works without clicking Command Line;
15. grip Reshape/Move Direct Distance works without clicking Command Line;
16. normal MOVE/COPY Direct Distance works without clicking Command Line;
17. ordinary MOVE command text can be launched keyboard-first from Select;
18. active PointRequest still outranks top-level MOVE text;
19. invalid token creates no revision/dirty/history change;
20. switching Document clears buffer and old endpoint receives nothing further;
21. switching to Workspace dashboard clears/detaches Document input;
22. switching back never restores stale partial token;
23. all completed SK-07A through SK-07F tests remain green;
24. exact-head Windows FULL passes;
25. documentation freshness/verification passes.

## 17. Manual Windows verification

Final candidate requires Owner verification of:

- with Sketch viewport focused, LINE → first point → direction → type 50 → Enter produces exact 50 without clicking Command Line;
- typed characters appear immediately in visible Command Line while viewport focus remains active;
- normal MOVE command can be typed and launched from viewport in ordinary Select;
- grip Reshape and grip Space→Move accept 50 keyboard-first;
- normal MOVE/COPY destination accepts 50 keyboard-first;
- clicking Command Line and typing still produces identical semantic behavior;
- a separate editable UI field receives ordinary typing without feeding Command Line;
- Backspace edits the live token before submission;
- unsupported text followed by Enter clears the editable token, keeps the active CAD context alive and shows a diagnostic;
- a deliberately long diagnostic does not make the Command Line taller, narrow/expand the input field or move the Viewer above it;
- Esc clears a non-empty live buffer, and Esc after an empty buffer still cancels the active CAD interaction;
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
User/Product docs: required  
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


## 21. External architecture-audit closeout amendment

**Owner acceptance:** 2026-09-27  
**Program authority:** `work/AUDIT-01_ARCHITECTURE_STABILIZATION_PROGRAM.md` v1.0

The external architecture audit was reviewed against PR #78 at `519f9c54e26ec85be0d216585b59e8a7f158d2fa`. WB-02 remains ACTIVE and must not close until Package A of AUDIT-01 is complete.

This amendment adds the following bounded closeout scope and no more:

- **A1 semantic context generation:** live buffer/diagnostic must be invalidated when the owning Document/editor/Sketch/request context is replaced, without clearing on ordinary prompt refresh or pointer movement inside the same request; stale context must fail before domain effect;
- **A2 Delete precedence:** non-empty CAD input buffer with normal CAD focus consumes Delete instead of deleting selected geometry; empty-buffer Delete and real QLineEdit editing retain their existing semantics;
- **A3 authorized capture boundary:** qApp-level capture must prove active Workspace/window/focus ownership and yield to foreign windows, menus/popups, modal/text-entry/composition ownership and ambiguous ownership;
- **A4 real integration evidence:** tests using real Workbench interactions must prove A1-A3 and re-run accepted Line/grip/MOVE/COPY/repeat/cycle/history/document/save-reopen behavior.

Accepted request-lifetime rule:

- replacing the semantic request invalidates a partially entered token;
- moving the pointer/direction inside the same PointRequest does not invalidate it.

Previously accepted WB-02 refinements remain authoritative:

- Enter/Return consumes submitted text even on rejection;
- rejected input leaves the semantic tool/stage active and creates no authored mutation;
- Command Line remains a fixed one-row surface with permanently reserved single-line diagnostic width.

This amendment does **not** authorize packages B-F, new input grammar, new quantity request types, Grip Copy, Dynamic Input, Ortho/Polar, snapping, regions or solid modeling.

Completion now requires all original WB-02 acceptance conditions plus Package-A evidence from AUDIT-01, exact-head Windows FULL and Owner manual Windows verification.


## 22. Package-A automated evidence checkpoint

**Runtime candidate:** `91877f6297a7af13cd874fbfe6a012ac5be4985f`  
**Windows FULL:** #622 — PASS — 73/73 CTest

Automated Package-A evidence on that runtime candidate:

- A1: `CadInputSession` binds the live token to the endpoint's opaque semantic context generation; stale-generation submit is rejected before endpoint interpretation/effect; real Workbench tests prove pointer movement inside one PointRequest preserves text while request replacement and Finish Sketch clear it without authored mutation.
- A2: real Workbench test proves non-empty-buffer Delete leaves the token, authored state, DocumentRevision and Undo/Redo depths unchanged instead of deleting selected geometry.
- A3: the Qt integration test proves real text-editor ownership, shortcut preservation, foreign non-modal window isolation, popup ownership and isolation between two visible Workspace shells.
- Existing Line, grip Reshape/Move, MOVE, repeated COPY, Repeat Last Command, CycleEditMode, history and document lifecycle regressions remain in the unfiltered FULL suite.

This checkpoint is not final WB-02 completion evidence. Current documentation/Product Browser, a final exact-head FULL on the documented candidate and Owner manual Windows verification are still required.


## 23. Final completion record

WB-02 and AUDIT-01 Package A are complete on runtime/documentation candidate `c66d550280485430ab6ddeb92d998919260e9b00`.

Final evidence:

- Windows PR gate #626 checked out that exact SHA and passed documentation verification, Build, selector checks, **73/73** unfiltered CTest tests and stable `windows-msvc`;
- `sk01.workbench_sketch_host`, `wb02.cad_input_session`, `wb02.cad_input_boundaries` and `wb02.global_cad_input_ui` all pass;
- A1 semantic-context generation binds a live token to the semantic request/context, invalidates it on real context replacement and rejects stale context before semantic interpretation/domain effect;
- pointer/direction movement inside the same PointRequest preserves typed input;
- A2 non-empty-buffer Delete is consumed by CAD input precedence and leaves authored model, revision and Undo/Redo unchanged; empty-buffer Delete retains existing geometry-delete semantics;
- A3 global Qt capture is constrained to the active owning Workspace/window/focus and yields to real text editors, foreign windows, popup/menu ownership, modal ownership and application shortcuts;
- A4 real Workbench regression evidence covers Line Direct Distance, grips, MOVE, repeated COPY/fresh IDs, Repeat Last Command, CycleEditMode, history interaction, Document lifecycle and prior WB-02 refinements;
- provider-neutral CAD input boundary remains free of Qt/OCCT/Part/Sketch request types;
- Command Line remains keyboard-first, rejected Enter consumes the token without mutating authored state, and the one-row diagnostic geometry remains stable;
- Owner manual Windows verification passed on 2026-09-27 on the final candidate.

No B2/B1/C/D/E/F implementation is authorized by this completion. The next program step is a separately accepted bounded B2 stale-transaction-protection contract.

WB-02 closeout now requires only CLOSURE and merge of PR #78.
