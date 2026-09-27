# ADR-0011 — Workspace-global CAD Input routing and semantic request ownership

**Status:** ACCEPTED  
**Proposed:** 2026-09-27  
**Owner acceptance:** 2026-09-27  
**Decision class:** D2 Architecture  
**Foundation:** 1.0 (foundation-v1.0)  
**Related:** ADR-0003, ADR-0006, ADR-0007, ADR-0008, ADR-0009, SK-07F

## Context

SK-07F established a shared Sketch PointRequest and Direct Distance resolver. The semantic result is correct, but final manual Windows verification exposed an interaction boundary that SK-07F deliberately did not authorize: numeric text works only after the current Sketch Command Line widget receives text-entry focus.

That is not sufficient for classical CAD interaction. A user working in the viewport must be able to point with the mouse, type a value or command immediately, and press Enter without first finding and clicking a text field.

The same mechanism must later serve more than Sketch. Examples include Part modeling values such as Extrude distance, Assembly offsets or mate values, Drawing commands and values, and future Dynamic Input. At the same time, future ordinary text editors such as object-name fields, search boxes and property editors must retain normal keyboard ownership.

If this is solved inside Part Sketch, or by globally forwarding every key to one QLineEdit, SS2 would create exactly the domain/UI coupling forbidden by the Foundation.

## Decision

SS2 will have one workspace-global CAD input mechanism for an opened Project runtime.

The mechanism separates four responsibilities:

    physical keyboard / text events
            ↓
    focus and shortcut arbitration
            ↓
    workspace CAD input router + runtime text buffer
            ↓
    active generic CAD input context
            ↓
    active tool / semantic InputRequest / command grammar

The global router owns reusable transport and arbitration mechanism. It does not own Part, Sketch, Assembly, Drawing or tool meaning.

The active tool/domain owns semantic meaning. A value such as 120 is not intrinsically an Extrude distance, Move distance, angle, scale factor or count until the active semantic context interprets it.

The visible Command Line is a presentation/input adapter to this mechanism. It is not the semantic owner and is not required to hold Qt keyboard focus for keyboard-first CAD entry.

## 1. Workspace-global Command Line surface

The common Project Workspace Shell owns one visible Command Line surface for the opened Project runtime.

It is not owned by Part Workbench, Sketch edit mode, Assembly Workbench, Drawing Workbench or any particular tool.

The surface may display:

- current prompt/context text;
- the live input buffer;
- bounded runtime diagnostics;
- later command history.

Its state is runtime-only.

The first implementation may have no executable Project-dashboard commands. If no active CAD input context exists, submission fails closed and must never route to an inactive or hidden Document.

The surface being global does not make Project Workspace a CAD Document and does not give the shell CAD domain semantics.

## 2. One generic active CAD input context

At most one generic CAD input context is active for keyboard/text routing.

The active context follows the current runtime navigation and active workbench/editor context.

Conceptually:

    Project Workspace Shell
            ↓
    active Document / Workbench / Editor context
            ↓
    generic CAD input endpoint
            ↓
    tool/domain semantic state

The endpoint is provider-neutral. Its exact C++ representation is an implementation detail, but it must allow the global mechanism to obtain presentation state such as the current prompt and to offer a submitted text token or semantic key action to the active context.

The generic endpoint must not expose Qt widgets, OCCT handles, Viewer provider tokens or filesystem identity.

The generic router must not switch on Part, Assembly, Drawing, SketchTool, PointRequest, Extrude or future tool enums.

## 3. Semantic InputRequest remains owned by the active tool/domain

InputRequest means: the semantic value currently required by an active interaction.

Existing Sketch PointRequest is the first concrete production request.

Future concrete needs may justify typed requests such as a physical length, angle, scalar, integer/count or bounded choice. This ADR does not freeze an exhaustive universal request hierarchy and does not authorize speculative implementations of those future request types.

The important invariant is:

    tool declares WHAT semantic input is required
    shared/input adapters provide HOW the user supplies it

A tool must not find or read the Command Line widget directly.

The workspace router does not parse a request into domain meaning. Parsing/resolution belongs to the active semantic input layer, with reusable parsers/services factored into shared infrastructure only when concrete multi-client reuse justifies them.

This preserves:

    Domain owns meaning.
    Platform owns reusable mechanism.
    UI owns neither.

## 4. Keyboard-first capture without focus transfer

When a normal CAD interaction surface owns focus and the user types printable unmodified text, that text is routed into the workspace CAD input buffer.

The implementation must not move Qt focus to the Command Line widget merely to make typing work.

The Command Line visual surface mirrors the same live buffer immediately.

Therefore a normal flow can be:

    LINE
    click first point
    point the cursor
    type 50
    Enter

without any Command Line click.

The same transport must also allow ordinary command text such as MOVE when the current semantic context permits top-level command activation.

Clicking/focusing the visible Command Line remains a valid input adapter and edits the same buffer rather than creating a second input state.

## 5. Focus arbitration

CAD keyboard capture is conditional, not unconditional global interception.

A real text-editing surface has priority.

Examples include:

- QLineEdit-style text editors;
- text areas;
- editable property cells;
- search/filter fields;
- rename editors;
- dialog text fields;
- future quantity/property editors while they are actively editing text;
- active input-method/composition ownership where applicable.

When such a surface owns text entry, printable text and normal editing keys remain with that surface. The CAD input buffer must not change.

Modal/dialog ownership also takes precedence over background CAD routing.

Application/window shortcuts using Control, Alt, Meta or equivalent shortcut mechanisms remain application/UI shortcuts unless an explicit later contract changes a specific binding. The CAD text router must not turn Ctrl+S, Ctrl+Z or similar shortcuts into Command Line text.

Exact Qt widget classification and event-filter mechanics are D1 implementation details, but behavior must be capability/focus based rather than a hard-coded list of one current Command Line widget.

## 6. Buffer and semantic-key ownership

The workspace mechanism owns one live text buffer for the currently active CAD context.

When that buffer is non-empty:

- printable text extends the buffer;
- Backspace edits it;
- Enter submits it to the active generic input context;
- Esc clears the live buffer before delegating any later Esc to tool cancellation;
- semantic CAD actions such as grip Space cycling must not accidentally fire because text is currently being edited.

On successful submission, the buffer clears.

On rejected submission, authored state does not change. The active semantic context remains authoritative, and the implementation should keep the rejected text available for correction unless the semantic context explicitly ends or changes.

When the buffer is empty, existing CAD key semantics remain authoritative, including SK-07D Repeat Last Command, SK-07E Space CycleEditMode, transform-selection completion, Delete and hierarchical Esc.

This ADR does not redefine broad AutoCAD-style Space-as-Enter behavior. Existing accepted Space semantics must not regress.

## 7. Context-first submission precedence

Text submission is interpreted by the active semantic context in this order:

    1. active semantic InputRequest
    2. active tool-local keyword grammar
    3. active Document/Workbench command grammar
    4. future Project/Workspace command grammar
    5. future application command grammar

Only levels implemented by an accepted contract participate.

A lower level must not steal a token from an active higher-level context.

For example, if a Sketch point stage owns an active PointRequest, typing MOVE is not allowed to silently abandon that request and launch top-level MOVE unless the active tool grammar explicitly defines such behavior.

SK-07F context-first Direct Distance semantics remain authoritative.

## 8. Active Document and navigation lifecycle

Only the currently active input context may receive routed CAD input.

An inactive or hidden Document must never consume keyboard text.

On Document switch, navigation to the Workspace dashboard, active-workbench replacement, editor-context replacement or runtime teardown:

- the old generic CAD input endpoint is detached before its owning runtime can be destroyed;
- the live input buffer associated with the old context is cleared;
- stale submissions fail closed;
- no authored mutation, dirty state or Undo entry is created by buffer cleanup.

This follows the existing detach-before-destroy lifecycle rule.

The system does not silently resume a partially typed token in another Document.

## 9. Multiple presentation/input adapters, one semantic request

Command Line is the first global adapter, not the only future adapter.

Future adapters may include:

- Dynamic Input near the cursor;
- an Operations task field;
- a Properties/quantity editor;
- scripting/automation text submission where appropriate.

They must converge on the same active semantic request/tool state.

A focused dedicated field may own keyboard entry locally, but acceptance must still feed the same semantic operation rather than create a parallel tool state machine.

No adapter may become an authored model authority.

## 10. Prompt, diagnostics and history

Prompt and diagnostic presentation are runtime projections of the active semantic context.

Structured failure should originate from the subsystem that understands the input; the global UI presents that result.

A future Command Line history may be retained for the opened Project/application session as runtime presentation state. This ADR does not authorize persistence of command history into Project or CAD Document files.

Live buffer, prompts, focus state, router state, active endpoint and history do not dirty a Document and do not create CAD Undo history.

## 11. Dependency boundary

Provider-neutral CAD input contracts belong in shared Application/Platform-level code that does not depend on Qt, OCCT, Part, Assembly, Drawing or Sketch.

Qt event capture, widget focus inspection and Command Line widgets are UI adapters above that boundary.

Domain/workbench adapters depend on the generic contract and translate it to their current semantic tool state.

Dependency direction remains:

    Qt/UI keyboard and fields
            ↓
    generic CAD input mechanism
            ↓
    active application/workbench semantic endpoint
            ↓
    domain/tool request and command semantics

No lower semantic layer depends upward on the global Command Line widget.

## 12. Failure behavior

Ambiguous ownership fails closed.

Examples:

- no active CAD input context;
- endpoint detached during a context switch;
- stale request;
- rejected token;
- invalid numeric text;
- focus owned by a real text editor;
- input-method composition not safely routable.

Failure must not mutate authored CAD state, revision, dirty state or Undo history.

The router must never guess another Document, tool or request owner.

## Alternatives rejected

### Auto-focus the current QLineEdit

Rejected because focus is presentation state, it competes with viewport interaction and other future editors, and it does not scale to Dynamic Input or multiple domains.

### Forward only digit keys from the viewport

Rejected because the same transport must support command names, keywords, signs/syntax introduced later and other semantic text inputs. Tool-specific key hacks would fragment the grammar.

### Put all command parsing in Project Workspace Shell

Rejected because the shell would acquire Part/Sketch/Assembly/Drawing semantics and grow a domain switchboard.

### One universal NumericRequest owned by the router

Rejected because equal text can mean physical length, angle, dimensionless scalar, count or domain-specific choice. Meaning stays typed and semantic.

### Give every Workbench its own Command Line stack

Rejected because behavior, focus arbitration and future input adapters would diverge across Part, Assembly and Drawing.

## Consequences

The user gets one consistent keyboard-first CAD input channel across the application.

Part/Sketch becomes the first adapter rather than the owner of Command Line infrastructure.

Future Extrude, Assembly and Drawing tools can participate by exposing an active semantic input context/request; they do not require a new keyboard-routing architecture.

Future Dynamic Input can mirror/feed the same semantic state instead of duplicating parsers or tool state.

The design adds a D2 shared application boundary but does not change persistent schemas, durable identity or mutation authority.

## Verification requirements

An implementing Work Contract must prove at minimum:

- normal CAD viewport typing updates the global buffer without moving Qt focus;
- direct Command Line focus edits the same buffer;
- a separate ordinary text editor retains keyboard ownership and does not change CAD input state;
- application shortcuts are not swallowed by text capture;
- active semantic requests outrank lower command activation;
- current Sketch Direct Distance works keyboard-first;
- ordinary Sketch command activation works keyboard-first when no request owns the token;
- rejected tokens create no authored mutation;
- non-empty-buffer Enter/Esc/Backspace behavior is deterministic;
- existing empty-buffer Enter/Space/Esc/Delete semantics do not regress;
- switching Documents/Workspace clears live input and prevents hidden-document consumption;
- provider-neutral input contracts contain no Qt/OCCT/domain types;
- no persistent schema or dirty/Undo effect is introduced;
- exact-head Windows FULL and explicit Owner manual Windows verification pass.

## Documentation impact

Internal docs: required  
User/Product docs: required in PL and EN  
Reason: this changes global Workbench/input ownership and the primary keyboard workflow.

## Completion boundary

Owner acceptance of this ADR permits only a separately bounded Work Contract implementing the global routing foundation and migrating existing Sketch input as its first client.

It does not authorize Extrude, Assembly, Drawing commands, new semantic quantity grammars, Dynamic Input, Ortho/Polar, Object Snap/tracking, Grip Copy or other later CAD features.
