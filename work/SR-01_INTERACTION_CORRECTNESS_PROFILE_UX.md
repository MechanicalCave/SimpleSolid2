# SR-01 — Interaction Correctness and Profile UX Stabilization

**Status:** COMPLETED  
**Proposed:** 2026-10-02  
**Owner acceptance:** 2026-10-02  
**Decision class:** bounded D2 interaction/action-target semantics and Package-F runtime-option amendment + bounded D1 implementation  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Architecture:** ADR-0005, ADR-0006, ADR-0008, ADR-0009, ADR-0011, ADR-0012  
**Program roadmap:** `work/SKETCH_ROADMAP.md` v1.9  
**Baseline:** `main` at `0872e8d2864ad1a62bdf436a7e52a6dd8ce0a11e` after completed R12  
**Milestone:** pre-Part stabilization — SR-01 of SR-01 → SR-02 → SR-03 → readiness re-test

## 1. Goal

SR-01 closes three concrete interaction-correctness gaps found during the post-R12 Sketcher profile-authoring readiness review:

1. destructive Delete must always target the active semantic interaction context rather than presentation residue such as a stale Tree/Profile selection or visible Properties page;
2. standard Windows `Ctrl+Z` / `Ctrl+Y` must route to the existing active Document history while preserving accepted focus and transient-interaction arbitration;
3. Profile island analysis must remain continuously active while any user option controls presentation only, and the diagnostic-only `Find All Regions` button must leave the normal Profile workflow.

SR-01 is a stabilization slice. It does not add new authored Sketch geometry, new Part features, new persistence semantics or new solid-modeling capability.

## 2. Authority and preserved architecture

The Engineering Constitution, Foundation and accepted ADRs remain authoritative.

In particular:

- durable mutation remains GUI / AI / Script → Command → Validation → Transaction → owning Document → Evaluation;
- selection, Properties pages, Tree rows and Viewer/provider objects are runtime presentation state, not CAD authority;
- Sketch entities are addressed by semantic `EntityId`;
- Part Profiles remain Part-owned durable `ProfileId + RegionIntent`;
- provider/runtime identity never becomes durable CAD identity;
- ambiguity and stale context fail closed;
- ADR-0011 global CAD-input and text-focus arbitration remains upstream of command targeting;
- ADR-0012 transaction freshness/atomicity remains unchanged.

This contract does not create a second history mechanism or a second selection model.

## 3. Frozen command-target rule

For a destructive CAD command that has already passed global keyboard/input arbitration:

> **The command target is determined by the active semantic interaction context and its semantic selection. Properties presentation, stale Tree selection, hover state and Viewer/provider objects never determine the target.**

While a Sketch is actively being edited:

- Sketch semantic command context owns Sketch editing commands;
- if one or more editable Sketch entities are selected, `Delete` targets only those Sketch entities through the existing semantic command/transaction path;
- a remaining Tree/Part/Profile selection may stay visible as runtime presentation state but cannot steal the Delete command;
- if no qualifying Sketch entity is selected, `Delete` must not fall through to a stale Profile/Tree selection and delete a Part object;
- a no-target/fail-closed Delete creates no authored mutation, revision change or Undo entry.

Outside Sketch edit, existing Part/Profile Delete behavior may operate from the valid active Part semantic selection.

The generic `Delete` key rule above does **not** disable explicitly named semantic Part commands. During active Sketch Edit, a deliberately invoked **Delete Profile** action from the selected Profile's Properties may delete that Profile without deleting its source Sketch geometry. This explicit action is not a fallback target for the generic `Delete` key.

If **Delete Profile** targets the Profile currently open in an Edit Profile session, the transient Profile draft is cancelled first without authored mutation, then one normal `DeleteProfileCommand` performs the deletion. If another Profile Create/Edit session owns a different semantic target, deletion fails closed until that Profile operation is finished or cancelled.

The currently displayed Properties page is never command authority; it may expose an explicitly named command only for the semantic Profile currently selected.

## 4. Tree blank-space selection behavior

LMB on blank space in the Document Tree clears the selection owned by the Tree/Part context.

This is not a global `clear all selection` rule.

In particular:

- clearing Tree/Part selection must not itself delete anything;
- clearing Tree/Part selection must not silently clear an independent active Sketch viewport selection unless an already-accepted context transition explicitly requires that;
- Tree row/index and widget selection remain runtime state only.

Exact Qt event-filter mechanics are D1.

## 5. Undo / Redo keyboard routing

On supported Windows:

- `Ctrl+Z` invokes the same accepted semantic Undo path as the existing Undo action for the active DocumentSession;
- `Ctrl+Y` invokes the same accepted semantic Redo path as the existing Redo action for the active DocumentSession;
- no Sketch-local history stack is introduced;
- an inactive/hidden Document must never receive the shortcut;
- accepted transient-interaction precedence remains unchanged: where current semantics cancel transient manipulation/tool state before document history, the keyboard shortcut follows that same path;
- a real text editor with focus retains its own local Undo/Redo and the CAD shortcut must not steal it.

ADR-0011 focus/input ownership remains authoritative. If a key event is consumed earlier by an accepted text/CAD-input context, SR-01 does not bypass that ownership.

Exact Qt shortcut object placement is D1 provided there is one semantic history endpoint and no duplicate action authority.

## 6. Profile island semantics — bounded amendment to Package F

Package F remains authoritative for region topology, `ProfileId`, `RegionIntent`, live-reference reevaluation, connected-material rules, nesting and fail-closed behavior.

SR-01 amends only the former runtime option semantics described by Package F §13:

> **Island detection/analysis is always active. A user option may control only presentation of the already-detected islands.**

Required behavior:

- region/profile analysis always evaluates complete supported topology, including nested disconnected bounded material candidates;
- island count/diagnostic truth is derived from that complete analysis and is not disabled by a UI checkbox;
- Profile validity, RegionCandidate truth, point picking, RegionIntent construction and reevaluation are independent of island-visibility preference;
- no option may merge islands into one ProfileId;
- no option may silently create additional ProfileIds;
- no option may change authored state or create Undo history.

The visible option should be renamed from `Detect Islands` to wording that expresses presentation only, preferably `Show Islands` or `Highlight Islands`. Exact PL/EN wording is D1.

Default island presentation remains ON.

If implementation proves that correct always-on island truth would require changing region geometry/topology algorithms rather than removing the current presentation/analysis coupling, stop for Owner review.

## 7. Find All Regions

The diagnostic-only `Find All Regions` button is removed from the normal Profile Operations workflow.

Provider-neutral region enumeration/analysis remains available internally where already required.

SR-01 does not add:

- automatic bulk Profile creation;
- a second public Profile-authoring workflow;
- any persistence for diagnostic region lists.

A private/internal command kind may remain if it has legitimate test/internal use and is not exposed as a normal user workflow.

## 8. Scope IN

- deterministic Delete target arbitration in active Sketch edit;
- protection against stale Tree/Profile selection stealing destructive commands;
- explicit **Delete Profile** remains available during ordinary active Sketch Edit without becoming a generic Delete-key fallback;
- deleting the Profile currently open in Edit Profile cancels only that transient draft before the single semantic Profile deletion;
- Tree blank-space clearing of Tree/Part selection;
- Windows `Ctrl+Z` / `Ctrl+Y` routing to existing active Document history;
- preservation of text-editor focus ownership;
- always-on Profile island analysis;
- presentation-only Show/Highlight Islands runtime option, default ON;
- removal of `Find All Regions` from normal Profile UI;
- regression tests for the accepted interaction semantics;
- required internal and PL/EN Product documentation;
- regenerated deterministic Product Browser;
- exact-head Windows FULL and Owner manual Windows verification.

## 9. Scope OUT

- SR-02 performance/latency optimization;
- differential or incremental Viewer architecture;
- SR-03 responsive Workbench shell/layout;
- new region topology algorithms or geometric healing;
- new Product geometric tolerance;
- Split / Join;
- authored constraints, solver or dimensions;
- ordinary Select RMB context redesign;
- projected/reference geometry;
- Axis / Centerline durable semantics;
- Part Body/Feature Tree;
- Extrude or any solid operation;
- persistent selection/UI state.

## 10. Expected implementation surface

Expected bounded production surfaces are:

- `src/ui/cad_workbench.cpp`;
- `src/ui/part_document_tree_controller.*`;
- `src/ui/part_sketch_interaction_controller.*`;
- existing application/UI action routing only as required to connect standard Undo/Redo shortcuts to the existing semantic history path;
- `src/ui/part_viewport_controller.*` only if needed for runtime selection reconciliation or island presentation;
- `tests/**`;
- affected `docs/internal/**`, `docs/product/pl/**`, `docs/product/en/**`, generated Browser;
- `work/**` closeout/evidence.

No change is expected in authored Sketch/Part persistence schema or durable Profile model.

A local helper/controller extraction is D1 if it only removes presentation-state coupling and does not create a new public ownership contract.

## 11. Required automated verification

Tests must prove at minimum:

- Profile selected in Tree → enter/edit Sketch → select Line → Delete removes only the Line and does not delete the Profile;
- the same sequence remains correct even if Profile Properties is still the visible Properties page;
- active Sketch edit + no Sketch entity selection + stale Profile/Tree selection → generic Delete does not delete the Profile;
- active Sketch edit + deliberately selected Profile + explicit **Delete Profile** → only the Profile is deleted, source Sketch geometry is unchanged, and Undo restores the same ProfileId/RegionIntent;
- active Edit Profile session for that same Profile + explicit **Delete Profile** → transient Profile draft is cancelled without its own history entry and the Profile is deleted by one normal semantic command;
- a conflicting active Profile Create/Edit session for another target fails closed rather than deleting through stale presentation;
- outside Sketch edit, valid selected Profile → existing Profile Delete still works;
- blank Tree click clears Tree/Part selection without authored mutation;
- blank Tree click does not accidentally mutate or delete active Sketch geometry;
- Sketch authored mutation → `Ctrl+Z` follows the existing Undo path;
- subsequent `Ctrl+Y` follows the existing Redo path;
- local `Ctrl+Z` / `Ctrl+Y` in a real text editor does not invoke CAD history;
- switching active Documents prevents shortcut delivery to the old/inactive Document;
- accepted transient interaction cancellation/history precedence is identical between toolbar action and shortcut;
- island presentation ON and OFF produce identical semantic region/Profile analysis;
- island presentation ON visibly exposes the existing island diagnostic/highlight;
- island presentation OFF suppresses only that presentation;
- nested-island geometry never auto-merges material islands or auto-creates Profiles;
- normal Profile Operations UI no longer exposes `Find All Regions`;
- no fail/no-target path increments authored revision or creates history.

Existing selection, Profile, history, CAD-input and persistence regressions must remain green.

## 12. Manual Windows verification

Owner manual verification must include at least:

1. reproduce the originally reported stale-selection sequence: select a Profile/Tree item, enter Sketch edit, select Sketch geometry, press Delete, and confirm only intended Sketch geometry is removed;
2. repeat with Profile Properties still visible;
3. active Sketch edit with no Sketch selection and stale Profile/Tree selection: generic Delete must not remove the Profile;
4. while Sketch Edit remains active, deliberately select a Profile and use **Delete Profile** in Properties: only the Profile is removed and Undo restores the same Profile identity/intent;
5. open Edit Profile for that Profile and repeat explicit **Delete Profile**: the draft is cancelled and the Profile is deleted without touching source Sketch geometry;
6. click blank Document Tree space and confirm Tree/Part selection clears predictably;
7. execute several Sketch mutations and verify `Ctrl+Z` / `Ctrl+Y` step through the same history as the existing actions;
8. verify a focused text editor keeps local Undo/Redo;
9. verify a representative nested-island Profile case with island presentation ON and OFF: semantic Profile result remains identical while only presentation changes;
10. verify `Find All Regions` is absent from the normal Profile workflow;
11. representative Save → Close → Reopen regression.

## 13. Persistence, identity and lifecycle

SR-01 introduces no authored schema change.

The following remain runtime-only and are not persisted into CAD documents:

- Tree selection;
- Sketch selection;
- Properties page;
- island presentation toggle;
- keyboard shortcut state;
- Viewer/provider presentation identity.

`EntityId`, `SketchId`, `ProfileId`, `RegionIntent`, identity high-water and existing lifecycle rules remain unchanged.

Application/user settings persistence for a presentation preference is not required by SR-01. If existing generic application settings are reused without changing CAD semantics, that is D1; creating a new cross-domain settings architecture is out of scope.

## Documentation impact

Internal docs: required  
User/Product docs: required  
Reason: SR-01 changes user-visible Delete/selection behavior, standard keyboard Undo/Redo access and Profile island option meaning, and it establishes a maintainers' action-target invariant that must remain explicit.

Internal documentation must describe:

- active semantic context versus presentation-state command targeting;
- Delete fail-closed behavior in active Sketch edit;
- standard shortcut routing to existing Document history;
- always-on island analysis versus presentation-only island visibility.

PL/EN Product documentation must describe:

- `Ctrl+Z` / `Ctrl+Y`;
- predictable Delete behavior while editing Sketch;
- Show/Highlight Islands as display-only behavior;
- removal of `Find All Regions` from the normal Profile workflow where currently documented.

Product Browser must be regenerated and deterministic.

## 15. Delegated D1 tuning

The Owner delegates:

- exact Qt shortcut/action wiring that preserves this contract;
- exact Tree blank-space event handling;
- exact local helper/controller decomposition;
- exact island-option label between equivalent presentation-only wording;
- exact island highlight glyph/color/style;
- exact diagnostics wording;
- exact regression-test file placement.

D1 may not change command-target ownership, history authority, Profile topology truth, durable identity, persistence or scope.

## 16. Stop conditions

Stop for Owner review if implementation requires or attempts:

- a new persisted selection/action state;
- a persistence schema change;
- a new global Selection Manager or new public selection-ownership architecture;
- a second history mechanism or changed Document history semantics;
- RegionIntent rewriting/rebinding;
- changed Profile region/topology truth beyond the bounded always-on island-analysis amendment;
- a new Product geometric tolerance, gap healing or topology repair;
- Viewer/provider identity as command authority;
- a public incremental/differential Viewer mutation protocol;
- SR-02 performance redesign;
- SR-03 responsive-shell work;
- ordinary Select RMB context redesign;
- Axis/Centerline semantics;
- Part Feature Tree or any solid-modeling operation.

## 17. Activation and completion boundary

The Owner explicitly accepted this SR-01 contract and Sketcher Roadmap v1.9 on 2026-10-02.

On 2026-10-02 the Owner additionally accepted the bounded SR-01 interaction refinement that distinguishes generic Sketch-owned `Delete` from the explicitly named **Delete Profile** command during active Sketch Edit. The refinement does not reopen selection architecture, persistence, Profile identity or history semantics.

This acceptance authorizes only the bounded SR-01 scope recorded here.

Implementation may begin only after the synchronized governance activation candidate containing:

- this Work Contract;
- `work/SKETCH_ROADMAP.md` v1.9;
- `work/ACTIVE.yaml` pointing to SR-01;

is recorded on the SR-01 branch and passes the repository's work/governance verification.

Completion requires:

- accepted semantics preserved;
- required automated regression evidence;
- required internal + PL/EN Product documentation and current Browser;
- exact-head Windows FULL;
- Owner manual Windows PASS;
- work-only CLOSURE closeout.

After SR-01 completion, production scope stops again. SR-02 does not activate automatically; it requires its own accepted measurement-first Work Contract.


## 18. Completion evidence

SR-01 completion evidence on 2026-10-02:

- exact runtime/manual candidate `92d096c396ba0d3ba144d0626c8e96aa667b01af` passed Windows FULL #1128, including exact checkout, deterministic documentation verification, bootstrap/dispatcher checks, complete desktop build graph, core-only **16/16 PASS**, full desktop **83/83 PASS**, warm FULL parity, comparative timing evidence and final `windows-msvc` aggregation PASS;
- the Owner manually verified the same exact runtime candidate on Windows and confirmed that SR-01 behavior works as intended, including generic Sketch-owned Delete, explicit **Delete Profile** during Sketch Edit, and the accepted Edit Profile deletion refinement;
- stale Tree/Profile presentation no longer steals generic Delete from active Sketch semantics; no-target Delete fails closed without authored revision/history mutation;
- explicit **Delete Profile** remains a deliberate Part command during Sketch Edit, removes only the Profile, preserves source Sketch geometry, and supports Undo through the existing Document history;
- standard Windows `Ctrl+Z` / `Ctrl+Y` route to the existing active DocumentSession history while real text editors retain local keyboard ownership;
- Profile island analysis remains always active while **Show Islands** controls presentation only; the normal **Find All Regions** workflow is removed;
- required internal documentation, PL/EN Product documentation and generated Product Browser are current on the runtime candidate;
- no persistence/schema, durable identity, RegionIntent, Viewer/provider authority, SR-02 performance architecture, SR-03 shell-layout, Part Feature Tree or solid-modeling scope was introduced.

All SR-01 acceptance conditions are satisfied. SR-01 is complete at this work-only closeout candidate. No production CAD Work Contract is active after SR-01 completion.

The next program action is preparation and explicit Owner review of the separate measurement-first **SR-02 Sketch Interaction & Presentation Latency** Work Contract. SR-02 remains inactive until that contract is accepted and activated.
