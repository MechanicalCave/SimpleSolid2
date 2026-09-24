# WS-01 — Project Workspace Navigation & Multi-Document Host

**Status:** ACCEPTED — IMPLEMENTED  
**Owner acceptance:** 2026-09-24  
**Decision class:** D2 Architecture + implementation  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Related ADRs:** ADR-0001, ADR-0002, ADR-0003, ADR-0006  
**Accepted ADR:** ADR-0007

## 1. Goal

Move Project-level document navigation out of Part Workbench and establish the Project Workspace Shell as the owner of:

```text
Project toolbar
Workspace Dashboard
Document Tabs
Workspace | Document(DocumentId) navigation
DocumentKind → Workbench routing
```

The current implementation routes Part only.

## 2. Required user flow

```text
Open Project
→ Workspace Dashboard

New Part… / Open…
→ open Part DocumentSession
→ add/focus Project-level Document Tab
→ show Part Workbench

while Part A is active:
New Part… / Open…
→ Part B can be opened without closing Part A

Workspace
→ return to Project Dashboard
→ Part A / Part B sessions and tabs remain open

click Part A tab
→ return to Part A Workbench

close last Document
→ Workspace Dashboard
→ Project remains open
```

## 3. Scope IN

### Project Workspace Shell

Implement a stable Project-level shell containing:

- Project identity/path header;
- always-visible Project toolbar;
- first-class Workspace Dashboard;
- Project-level Document Tabs;
- content host for active Document Workbench.

Toolbar actions:

- Workspace;
- New Part…;
- Open…;
- Refresh;
- Close Project.

### Runtime navigation

Represent current navigation explicitly as:

```text
Workspace
Document(DocumentId)
```

Navigation state is runtime-only.

### Document tabs

Tabs represent open DocumentSessions.

- one tab per open DocumentId;
- selecting a tab activates that Document;
- an already open Document focuses its existing tab/session;
- tab labels reflect current Part display name and dirty marker;
- tab close uses existing Save / Discard / Cancel semantics;
- closing active Document detaches Workbench before DocumentSession destruction;
- closing last Document navigates to Workspace.

### Part Workbench boundary

Refactor `CadWorkbench` so it no longer owns:

- Project document tabs;
- Create/Open/Refresh actions;
- cross-document close routing.

It receives one active Part Document context from the Project Workspace host and retains Part-specific editing, Sketch lifecycle, camera/runtime state and current Document actions.

## 4. Scope OUT

```text
Assembly Workbench
Drawing Workbench
New Assembly
New Drawing
Project configuration semantics
Workspace Dashboard domain features
recent activity persistence
SK-02 2D entities
constraints / solver
new CAD persistence schema
```

The Dashboard may contain minimal placeholder/project information only.

## 5. Architecture invariants

- ProjectSession remains owner of open DocumentSessions.
- Project Workspace navigation does not become durable Project state.
- DocumentId, not tab index, routes a Document.
- Project Workspace is not a fake Document and has no DocumentId.
- Part Workbench never guesses another DocumentKind.
- switching `Document → Workspace → Document` does not close/save/mutate the Document.
- non-owning Workbench bindings detach before DocumentSession destruction.
- no UI row/tab/widget becomes durable identity.

## 6. Acceptance tests

At minimum prove:

1. opening a Project with zero open Documents shows Workspace Dashboard;
2. Project toolbar remains visible in Workspace and active Part contexts;
3. create Part from Workspace opens Part tab and Workbench;
4. create/open a second Part while first Part remains open creates/focuses a second Project-level tab;
5. both DocumentSessions remain open simultaneously;
6. clicking Workspace while Documents are open keeps all sessions/tabs open;
7. Workspace navigation does not change DocumentRevision or dirty state;
8. clicking an existing Document Tab returns to its Workbench;
9. opening an already open Document reuses its canonical DocumentSession and focuses its tab;
10. tab identity routes by DocumentId, not tab index;
11. dirty marker updates after authored mutation/save;
12. closing inactive tab closes only that DocumentSession;
13. closing active tab detaches Workbench before session destruction;
14. closing last tab returns to Workspace;
15. Close Project handles dirty open Documents from either Workspace or Document context;
16. Part Sketch edit/runtime camera behavior remains correct across Document ↔ Workspace navigation;
17. existing native Workbench stress and SK-01/SK-01A regressions remain PASS;
18. exact-head Windows docs/verify/build/CTest gate is PASS.

## 7. Delivery slices

```text
Slice A
Project Workspace Shell + explicit navigation state

Slice B
move Document Tabs / Create / Open / Refresh ownership to Project level

Slice C
refactor Part Workbench to one-active-Document editor boundary

Slice D
multi-Document navigation/close/dirty tests

Slice E
docs + Product Browser + exact-head gate
```

## Documentation impact

Internal docs: required  
User/Product docs: required  
Reason: WS-01 changes Project Workspace ownership, Document navigation, tab ownership and the way users create/open/switch Documents.

## 8. Completion

WS-01 completes only when Project-level navigation is independent from Part Workbench and exact-head Windows verification passes.

Completion does not activate Assembly/Drawing implementation or SK-02.


## 9. Completion record

WS-01 implementation is complete.

Implemented state:

- Project Workspace Shell owns the persistent Project toolbar, Workspace Dashboard, Project-level Document Tabs and runtime navigation;
- navigation is explicit runtime state: Workspace or Document(DocumentId);
- Workspace Dashboard remains reachable while multiple DocumentSessions stay open;
- Project toolbar remains visible in Workspace and active Part contexts and exposes Workspace, New Part, Open, Refresh and Close Project;
- Project-level tabs represent open DocumentSessions and route by stable DocumentId rather than tab index;
- creating/opening another Part while a Part is active leaves existing sessions open and activates/focuses the requested tab;
- opening an already open Part reuses its canonical DocumentSession/tab;
- Workspace navigation detaches the Part Workbench without Save/Close/authored mutation and keeps all open tabs/sessions;
- returning through a Document tab rebinds the corresponding open DocumentSession;
- closing an inactive tab closes only that DocumentSession;
- closing the active/last tab preserves detach-before-destroy ordering and returns to Workspace when no Documents remain;
- tab labels reflect Part display name and dirty marker;
- Part `CadWorkbench` no longer owns ProjectSession, Project discovery, Project tabs or cross-document navigation;
- Part `CadWorkbench` receives only the active DocumentSession plus Workspace path context required for presentation;
- Workbench camera runtime state remains keyed by DocumentId across Document/Workspace navigation and is reset at the Project boundary;
- current DocumentKind routing supports Part only and does not guess Assembly/Drawing behavior;
- a dedicated architecture boundary regression prevents ProjectSession/QTabBar/discovery ownership from leaking back into Part Workbench and prevents CAD-domain ownership from leaking into the Project Workspace Shell;
- internal and PL/EN product documentation describe the as-built Workspace/Document navigation model;
- the compiled CTest suite contains 36 tests;
- final exact-head Windows gate #230 passed documentation dispatch/verification, bootstrap verification, build and the complete CTest suite on head `817d0fd25b4a234bd9a8aa99e0c6ae46f6df513a`;
- the tested head tree was `b095045ddb32eb7290be3cbecd2226dbad23a97a`;
- WS-01 was squash-merged to `main` as `de78c3993b7e3d5aab561a4f4bc35423f44ea743`, whose tree is the same tested tree `b095045ddb32eb7290be3cbecd2226dbad23a97a`.

All WS-01 completion conditions are satisfied. No further WS-01 implementation work is active.
