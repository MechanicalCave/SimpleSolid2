# AUDIT-01 — Architecture Stabilization and Audit Implementation Program

**Status:** ACCEPTED — PROGRAM CONTRACT  
**Version:** 1.0  
**Owner acceptance:** 2026-09-27  
**Decision class:** D2 program sequencing / architecture stabilization  
**Foundation:** 1.0 (foundation-v1.0)  
**Source audit:** external architecture audit reviewed by Owner on 2026-09-27  
**Audit baseline:** PR #78 at `519f9c54e26ec85be0d216585b59e8a7f158d2fa`, Windows FULL #618 PASS  
**Subordinate feature roadmap:** `work/SKETCH_ROADMAP.md` v1.4

## 1. Purpose

This program makes the external architecture-audit implementation plan durable in the repository so it cannot disappear with chat/session context.

The Owner accepts:

- the obligation to address packages A through F;
- the default execution order in this document;
- the stop-gates and evidence requirements in this document;
- the rule that no finding is silently dropped merely because the active Work Contract changes.

This program is **not** blanket authorization to implement all packages at once.

Only the Work Contract named by `work/ACTIVE.yaml -> active_work` authorizes production mutation. Future package contracts may resolve implementation choices that the audit intentionally left open, but they must not silently remove a mandatory finding or weaken its acceptance evidence.

If a later Owner decision supersedes a finding, this program must be amended explicitly rather than allowing the finding to disappear from context.

## 2. Authority and reconstruction

While this program is referenced by `work/ACTIVE.yaml`, every implementation session must read:

1. `AGENTS.md`;
2. Constitution / Foundation / Architecture;
3. relevant accepted ADRs;
4. `work/ACTIVE.yaml`;
5. this program;
6. `work/SKETCH_ROADMAP.md` v1.4 for preserved Shared 2D / Sketch feature direction;
7. the active bounded Work Contract;
8. `governance/DOCUMENTATION.md`.

The program governs stabilization sequencing. It does not override Foundation, accepted ADRs, durable identity rules, mutation authority or product semantics already frozen elsewhere.

## 3. Preserved architecture

Do not globally rewrite SS2 to satisfy this program.

Preserve:

- authored semantic model separate from Viewer/presentation;
- Shared 2D as the neutral authored 2D mechanism;
- semantic Commands / Validation / Transaction / owning Domain Document for durable mutation;
- durable IDs independent of Qt, OCCT, topology ordinals and presentation tokens;
- provider-neutral global CAD Input transport;
- context-first PointRequest semantics;
- runtime-only prompt/buffer/focus/selection/preview;
- Copy fresh-ID policy and accepted Undo/Redo identity/cursor behavior;
- current clean/dirty semantics, including the accepted technical ID-cursor rule.

No event-sourcing rewrite, ECS, plugin framework, universal command framework or second editing engine is authorized by this program.

## 4. Mandatory execution order

Default order is strict:

| Order | Package | Required outcome | Activation |
| --- | --- | --- | --- |
| 1 | **A — WB-02 closeout hardening** | context lifetime, Delete precedence, input-capture ownership, real integration evidence | **COMPLETED — 2026-09-27** |
| 2 | **B2 — transaction freshness** | stale overlapping domain transaction cannot overwrite newer state | **COMPLETED — 2026-09-27** |
| 3 | **B1 — save/file conflict protection** | ordinary Save cannot silently overwrite externally changed/replaced/missing file state | **COMPLETED — 2026-09-27** |
| 4 | **C1 — history-copy cost removal** | remove deep copy of all previous history while preserving strong consistency | future bounded Work Contract |
| 5 | **C2 — measured history representation/budget** | choose any deeper history redesign only from Release measurements | future bounded Work Contract |
| 6 | **D — semantic input endpoint outside QWidget + core-only build** | current semantic Line/Move/Copy input testable without Qt/OCCT and neutral core independently compilable | future bounded Work Contract |
| 7 | **E1 — geometry numerical stability** | reproduce/fix translation/scale sensitivity with explicit validation evidence | future bounded Work Contract |
| 8 | **E2 — presentation scale/failure behavior** | measure refresh/preview cost; define provider-failure consistency before optimization | future bounded Work Contract |
| 9 | **F — region/profile semantics gate** | accepted exact region/profile semantics required by Part before solid modeling | future bounded Work Contract |

Interleaving feature work with this order requires an explicit Owner scheduling amendment. Context loss is not authorization to skip a package.

No demonstrational/provisional Extrude is allowed before package F passes its gate and a separate Part solid-operation contract is accepted.

## 5. Package A — mandatory WB-02 closeout hardening

Package A was incorporated into WB-02 by Owner acceptance on 2026-09-27 and is **COMPLETED** on final candidate `c66d550280485430ab6ddeb92d998919260e9b00`.

### A1 — bind live buffer to semantic editing-context identity

Confirmed risk at audit baseline:

- `CadInputSession` generation changes on endpoint attach/detach;
- Finish Sketch / sketch replacement can change the semantic owner while the endpoint object remains the same `CadWorkbench`;
- prompt refresh alone is therefore not a sufficient context boundary.

Required contract:

- the document/editor semantic input context has an opaque identity or generation independent of QWidget address;
- Finish Sketch, entering a different Sketch, Document switch, Workspace navigation, Workbench/editor replacement and teardown invalidate old buffer and old diagnostic;
- prompt refresh, hover, pointer movement and repaint inside the same semantic request do **not** clear the buffer;
- an old context callback/token must not execute in a new context;
- staleness is checked before a domain effect, not only after submission;
- the router remains domain-neutral and does not inspect SketchId, SketchTool or Document kind;
- context owner explicitly signals semantic-context replacement;
- do not indiscriminately clear on every generic state/prompt notification.

Accepted request-stage policy for this package:

- if a partially entered token belongs to a concrete semantic request and that request is replaced, invalidate the token;
- pointer-direction movement inside the **same** PointRequest does not replace the request and must not invalidate the token.

Acceptance evidence:

- A1.1 token typed in Sketch A cannot execute after Finish Sketch / entry to Sketch B;
- A1.2 Document switch and return cannot restore old token;
- A1.3 Workspace / teardown detach before runtime destruction;
- A1.4 prompt/pointer updates inside the same request preserve typed text;
- A1.5 stale-context attempt changes neither model, revision, dirty, Undo nor Redo.

### A2 — Delete precedence while editing CAD input

Confirmed risk at audit baseline:

- global input filter handles Backspace/Esc/Enter/Space/printable text but not Delete;
- viewport Workbench treats Delete as semantic deletion of selected Sketch geometry.

Accepted minimal behavior:

- with normal CAD focus and **non-empty** live buffer, Delete is consumed by the CAD input editing layer and does not delete geometry; outside the QLineEdit it need not alter the append-only buffer;
- Backspace removes the final character as today;
- with empty buffer and viewport CAD focus, existing Delete-selection semantics remain;
- with actual QLineEdit/text-editor focus, Delete remains ordinary local text editing;
- Ctrl/Alt/Meta shortcut routing is preserved;
- non-empty-buffer Space does not trigger CycleEditMode;
- rejected Enter still consumes the token, preserves semantic context and shows diagnostic.

Acceptance evidence must inspect model, revision, dirty, Undo/Redo and buffer, not only UI labels.

### A3 — constrain global QApplication input capture to the authorized Workspace

Confirmed risk at audit baseline:

- capture is installed globally at qApp;
- checks exist for visibility, modal window, text editor, endpoint and modifiers;
- capture does not yet prove that the focus/event target belongs to the active authorized Workspace or that a popup/menu owns input.

Required contract:

- capture only from the active authorized CAD surface/window for this Workspace;
- foreign non-modal window must not feed background CAD;
- hidden Document / inactive Workspace must not consume text;
- popup/menu ownership prevents background CAD capture;
- modal ownership prevents background CAD capture;
- ambiguous ownership means do not capture;
- real text editor/property editor and active text-composition ownership remain local;
- do not transfer focus to Command Line per typed character;
- two visible shells in a test must not capture each other's text; this does not authorize multi-project UI as a product feature.

Acceptance evidence:

- A3.1 active CAD surface feeds buffer without focus transfer;
- A3.2 independent text editor owns its text;
- A3.3 foreign non-modal window cannot modify CAD buffer;
- A3.4 popup/menu retains keys;
- A3.5 modal window blocks CAD capture;
- A3.6 two visible shells are isolated;
- A3.7 Ctrl+S/Ctrl+Z routing is not converted into CAD text.

### A4 — close the evidence gap

WB-02 fake-endpoint tests remain useful but are insufficient for integration risks.

Before WB-02 completion, real Workbench integration must re-prove:

- Line Direct Distance;
- grip Reshape / Move;
- normal MOVE;
- repeated COPY and fresh IDs;
- Repeat Last Command;
- CycleEditMode;
- preview cancellation before Undo/Redo;
- Document switching;
- Save/Close/Reopen;
- rejected token is consumed, context remains valid, diagnostic is shown, no authored mutation occurs;
- fixed one-row Command Line geometry and reserved diagnostic region remain stable.

The final Package-A candidate requires exact-head Windows FULL and Owner manual Windows verification.

### Package A implementation checkpoint — 2026-09-27

Current classification after implementation and Windows FULL #622 on `91877f6297a7af13cd874fbfe6a012ac5be4985f`:

- **A1 — CONFIRMED / IMPLEMENTED / automated evidence PASS:** live input is bound to an opaque semantic context generation; same-request pointer movement preserves it; request/Sketch context replacement invalidates it; endpoint validates the expected generation before semantic interpretation/effect.
- **A2 — CONFIRMED / IMPLEMENTED / automated evidence PASS:** non-empty viewport CAD buffer consumes Delete without model/revision/dirty/Undo/Redo mutation; empty-buffer and real text-editor Delete semantics remain separate.
- **A3 — CONFIRMED / IMPLEMENTED / automated evidence PASS:** capture is constrained by active owning window, active Workbench focus/target, popup/modal ownership, text-editor ownership and endpoint presence; foreign windows and a second visible shell are isolated.
- **A4 — PARTIAL PASS:** the real Workbench regression and provider-neutral/session/UI tests pass in the 73-test FULL suite. Final status remains pending current documentation, regenerated Browser, final exact-head FULL and Owner manual Windows verification.

Package A is not complete until those remaining evidence gates pass.

### Package A final completion evidence

**Completed:** 2026-09-27  
**Final candidate:** `c66d550280485430ab6ddeb92d998919260e9b00`  
**Windows FULL:** #626 — PASS — 73/73  
**Owner manual Windows:** PASS — 2026-09-27

Package-A findings are closed as follows:

- **A1 — CONFIRMED / FIXED / VERIFIED:** opaque semantic context generation now owns live-token lifetime; request/context replacement invalidates old input while pointer movement in the same request preserves it; stale generation is rejected before semantic effect.
- **A2 — CONFIRMED / FIXED / VERIFIED:** Delete cannot fall through to geometry deletion while a viewport-entered live token is non-empty; authored state/revision/history remain unchanged.
- **A3 — CONFIRMED / FIXED / VERIFIED:** qApp capture is constrained by active Workspace/window/focus ownership and yields to text editors, foreign windows, popup/menu, modal ownership and modifiers/shortcuts.
- **A4 — EVIDENCE CLOSED:** real Workbench and global-input tests plus the unfiltered FULL suite re-prove accepted Sketch/Direct-Distance/grip/MOVE/COPY/history/document-lifecycle behavior.

Package A completion does not activate B2 automatically. B2 remains the next mandatory AUDIT-01 package and requires a separate bounded Owner-accepted Work Contract.

## 6. Package B2 — stale transaction protection

This package is next after WB-02 merge.

Mandatory problem to resolve:

- overlapping transactions created from the same old document state must not allow a later stale full-state commit to overwrite effects of a previously accepted commit.

The bounded B2 contract must choose and document one enforcement policy. Preferred audit direction is base-revision capture with typed stale-transaction conflict; an alternative exclusive-transaction policy may be proposed only if it proves the same safety.

Required outcomes:

- stale commit is rejected explicitly;
- rejection changes no document state or history;
- commit/rollback end transaction lifecycle according to an explicit rule;
- repeated commit cannot apply moved/partial state;
- restore/replace-state paths preserve domain invariants or are restricted to controlled validated paths;
- preserve accepted no-op, revision increment and revision-exhaustion semantics.

Tests must cover T1/T2 overlap, rollback, repeated commit, no-op, revision exhaustion and invalid state through public domain API.

### B2 activation record

**Owner acceptance:** 2026-09-27  
**ADR:** ADR-0012 — ACCEPTED  
**Work Contract:** `work/B2_STALE_PART_TRANSACTION_PROTECTION.md` — ACCEPTED / ACTIVE  
**Proposal evidence:** Windows PR gate #628 PASS on `b722ee93ec4aac9e2593cc58b5f6c0b7fed4b784`

The accepted B2 policy is optimistic base-revision binding, stale-before-no-op, one-shot transaction lifetime, owning Part-domain complete-state validation and validated reconstruction. B1 and later packages remain inactive.

### B2 implementation checkpoint

Runtime implementation is present and verified by Windows FULL #631 on `e39569c440ca41e24eb9330e3258b259bc7f0dfa` with 73/73 tests. Internal docs and generated Browser passed docs gate #632 on `cd0b68250d32598aa3c930c457aacd354e30d61a`.

B2 remains ACTIVE until a final exact-head FULL is green on the documented candidate and closeout/CLOSURE completes. B1 remains inactive.

### B2 final completion evidence

**Completed:** 2026-09-27  
**Final candidate:** `e23114341973fa857ae4471d2abdd4107b9f79e6`  
**Windows FULL:** #635 — PASS — 73/73  
**Manual UI verification:** not required by accepted B2 contract

B2 is closed as **CONFIRMED / FIXED / VERIFIED**:

- transactions bind to immutable base technical revision;
- stale mismatch rejects before validation/no-op/mutation;
- every commit attempt is terminal and repeated commit is inactive;
- complete Part authored state is validated at the owning domain boundary;
- invalid state and max-revision failures do not mutate authored state or revision;
- reconstruction uses the same Part invariant boundary and returns structured failure;
- DocumentSession and persistence regressions remain green;
- B2 makes no file-level concurrency, stale-state merge or multi-thread mutation claim.

B2 completion does not activate B1 automatically. B1 remains the next mandatory AUDIT-01 package and requires a separate bounded Owner-accepted Work Contract.

## 7. Package B1 — file/save conflict protection

This package follows B2.

Mandatory problem to resolve:

- ordinary Save must not silently overwrite a file that changed identity/content, disappeared or was replaced since load/last successful save.

Future B1 contract must explicitly define:

- file checkpoint held by an open document after load/save;
- typed conflict for changed/replaced content;
- behavior for removed/moved file;
- conflict leaves disk file, dirty state and Undo/Redo unchanged;
- force-overwrite, if ever supported, is a separate explicit user operation;
- successful save updates checkpoint to the actually published version;
- atomic replacement, conflict detection and crash durability are separate guarantees;
- concurrency guarantee among cooperating SS2 instances;
- limitations versus unrelated external processes/sync tools;
- safe create-new / temporary-file reservation semantics on supported Windows platform.

Do not claim mtime/size alone proves unchanged content. Do not claim a pre-rename hash is an atomic compare-and-swap against arbitrary external writers.

Tests include two sessions, replaced DocumentId, changed content with similar metadata, missing file, publication failure, normal repeated save/reopen and controlled competing writes.

### B1 activation record

**Owner acceptance:** 2026-09-27  
**ADR:** ADR-0013 — ACCEPTED  
**Work Contract:** `work/B1_NATIVE_PART_SAVE_CONFLICT_PROTECTION.md` — ACCEPTED / ACTIVE  
**Proposal evidence:** Windows PR gate #637 PASS on `ec254ce4510ba5649bf3db80026eeb261dba5b18` with 73/73 tests

The accepted B1 policy is exact native-file checkpoint (DocumentId + byte length + SHA-256 + platform file identity), cooperative per-target guard across check/publish/new-checkpoint, typed missing/replaced/content/busy conflicts, no implicit recreation and no Force Overwrite. Strict cross-process no-lost-update guarantee applies to cooperating SS2 writers only. C1 and later packages remain inactive.

### B1 final completion evidence

**Completed:** 2026-09-27  
**Final candidate:** `79ef8ed2c59f9209d0695867d4b6fa43979159d3`  
**Windows FULL:** #662 — PASS  
**Owner manual Windows:** PASS — 2026-09-27

B1 is closed as **CONFIRMED / FIXED / VERIFIED**:

- every open native Part session carries an exact runtime checkpoint with DocumentId, byte length, SHA-256 and platform file identity;
- ordinary Save is conditional and detects missing, replaced, identity-changed or content-changed targets before publication;
- cooperating SS2 writers share a deterministic per-target guard so two saves from one checkpoint cannot both publish;
- conflict/failure preserves authored state, technical revision, Undo/Redo, saved-state checkpoint and session file checkpoint;
- successful publication returns and installs the checkpoint of the actually published file, so immediate repeated Save succeeds;
- create-new/temp reservation uses exclusive creation and cleanup leaves no durable temporary artifact;
- required internal and PL/EN Product documentation plus generated Product Browser are current;
- Windows FULL #662 passed on the exact final runtime/docs candidate;
- Owner manual Windows verification passed for external modification/replacement, external removal and repeated Save/reopen, including clean explicit-Save reachability.

The guarantee remains intentionally bounded: unrelated external writers racing after validation are not covered by an atomic filesystem compare-and-swap claim. Force Overwrite, Save As, automatic merge and live reload remain outside B1.

B1 completion does not activate C1 automatically. C1 remains the next mandatory AUDIT-01 package and requires a separate bounded Owner-accepted Work Contract.

## 8. Package C1 — remove deep copy of previous history

Mandatory first history optimization:

- remove the operation that deep-copies all previous history entries when adding a new command;
- do not simultaneously replace the whole Undo model.

Required consistency:

- prepare potentially failing resources before mutating durable document state;
- do not destroy Redo branch before the new accepted command can be represented safely;
- validation/allocation/commit failure cannot leave mutated document without matching history;
- one accepted commit -> one Undo entry;
- preserve authored identity and technical cursor rules.

## 9. Package C2 — choose history representation and budget from measurements

Only after C1 and Release measurements may a deeper history representation be selected.

Required benchmark matrix where feasible:

- 1,000 and 10,000 entities;
- history depths 10, 100 and 1,000;
- add entity;
- edit one object;
- multi-object transform;
- Undo/Redo;
- create a new history branch.

Report median, p95, maximum and peak memory; separate semantic command cost from rendering.

If baseline cannot complete a case safely, record the cutoff rather than forcing failure.

Memory/depth limits are product decisions and must not be silently introduced.

Event sourcing is not authorized merely because Undo exists.

## 10. Package D — semantic input ownership outside QWidget and core-only build

Preserve neutral `CadInputSession`.

Required target responsibility split:

- Qt adapter: events/focus/widgets/local text editing only;
- CadInputSession: live buffer/receiver/lifecycle transport only;
- semantic application endpoint: current request, allowed token interpretation and typed result;
- interaction state: tool stage/base/direction/preview intent;
- Command/DocumentSession: current-model validation, transaction, history;
- domain: durable authored model/invariants.

`CadWorkbench::submitCadInput` and `parseBareSketchDistance` are candidates for extraction, but the refactor must not change accepted grammar, locale behavior, rejection semantics or command effects.

Do not build a speculative complete hierarchy of future InputRequest types.

Acceptance:

- current Line/Move/Copy semantic scenarios are testable through non-QWidget, non-OCCT API;
- GUI adapter reaches the same semantic result/revision/history;
- add an explicit CMake configuration that compiles/tests the neutral core without Qt/OCCT/provider targets;
- full Windows GUI/provider gate remains mandatory.

## 11. Package E1 — numerical stability of geometric construction

Before region/profile consumers depend on these constructions:

- reproduce suspected translated/small-scale instability in actual C++/supported toolchain;
- compare equivalent local figures at origin and after large translation;
- include radius 0.1 with translation 1,000,000 in both axes;
- perform construction in local coordinates and scale-normalize if required;
- distinguish algorithmic conditioning from product tolerance;
- make collinear/nearly-collinear, very large/small scale and overflow behavior deterministic;
- finite output alone is insufficient: verify residual error through requested points.

Coordinate limits, units and tolerance policy require explicit contract before rejecting previously legal files.

## 12. Package E2 — presentation update scale and provider failure

Treat current scene rebuild / sampled curves / per-segment AIS behavior as a scale risk to measure, not as automatic proof that renderer must be rewritten.

Required:

- measure single-mutation refresh and pointer-preview update;
- consider differential updates only after stable semantic identity mapping;
- presentation tokens remain ephemeral;
- preview should not rebuild unchanged scene unnecessarily;
- provider failure must have an explicit consistency/recovery behavior;
- durable command is not rolled back merely because redraw failed;
- authored model remains authority and UI must surface presentation failure;
- renderer tessellation/chords never define region/profile semantics.

Any optimization claim requires measured before/after evidence and preserved picking behavior.

## 13. Package F — region and profile semantics gate

No solid-modeling consumer such as Extrude is authorized before this package is accepted and completed.

The F contract must explicitly answer and test:

- which authored/evaluated 2D curve types participate and which are excluded;
- geometric closure independent of screen zoom/DPI;
- intersection and overlap handling;
- loop/hole orientation, nesting and validity;
- deterministic semantic region selection;
- ownership split: Shared 2D owns reusable geometric analysis, Part owns meaning of consumption;
- exact authored/accepted evaluated geometry is analyzed, not Viewer tessellation;
- whether Part stores snapshot or reference to profile source and consequences of Sketch change;
- behavior when source changes/disappears;
- what semantic identity/intention is persisted and how schema is versioned.

This program does not preselect snapshot vs reference and does not imply full associativity.

Region/profile tests may be implemented before any solid B-Rep feature.

After F passes, the first Part solid operation requires a separate Owner-accepted contract using provider-neutral Kernel API and no durable OCCT topology ordinals.

## 14. Cross-package invariants

Every package must:

- classify each audit finding as CONFIRMED, ALREADY FIXED, NOT REPRODUCED or CONTRACT CHANGED with evidence;
- create the minimal reproducing test before/with the fix where applicable;
- not weaken existing tests;
- preserve Command -> Validation -> Transaction -> Domain mutation path;
- revalidate current context before durable commit;
- not persist UI/viewer/provider identity;
- update internal and Product docs according to `governance/DOCUMENTATION.md`;
- use the canonical docs generator for `docs/browser/index.html`; do not hand-edit generated Browser content;
- run the repository-appropriate exact-head gate before completion;
- record what could not be tested.

## 15. Feature-roadmap interlock

`work/SKETCH_ROADMAP.md` v1.4 remains the accepted feature-direction roadmap.

While AUDIT-01 is active, this stabilization program has scheduling precedence. Default next work is the next package in Section 4.

Grip Copy, RMB context, later precision-input expansion, regions and solid modeling are not activated by AUDIT-01.

Feature work may be interleaved only after explicit Owner scheduling approval and only if it does not bypass a prerequisite audit gate.

## 16. Program completion

AUDIT-01 completes only when:

- A through F have each been closed by accepted bounded contracts or explicitly superseded by an Owner amendment recorded here;
- mandatory evidence for each package exists;
- package F profile/region gate is complete;
- no unresolved finding is silently omitted.

Completion of AUDIT-01 still does not itself authorize Extrude or any later solid operation.

## Documentation impact

Internal docs: not required for this program-plan commit  
User/Product docs: not required  
Reason: this file records accepted future stabilization sequencing and governance; as-built/product behavior changes only when each bounded package is implemented.
