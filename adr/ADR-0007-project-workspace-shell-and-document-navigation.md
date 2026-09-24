# ADR-0007 — Project Workspace Shell and multi-Document navigation

**Status:** ACCEPTED  
**Date:** 2026-09-24  
**Owner acceptance:** 2026-09-24  
**Decision class:** D2 Architecture  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Related:** ADR-0001, ADR-0002, ADR-0003, ADR-0006

## Context

SK-01A established a neutral Project Workspace when no CAD Document is open, but Document Tabs and most open-document navigation still remain owned by the Part `CadWorkbench`.

That creates a domain ownership problem:

```text
Part Workbench
→ manages all open Project Documents
```

This does not scale to future Assembly/Drawing editors and prevents opening/creating another Document while a Part Workbench is active without routing through Part-owned UI.

The Owner accepted a Project-level navigation architecture before continuing Sketcher work.

## Decision

### 1. Project Workspace Shell owns Project-level navigation

A persistent runtime Project Workspace Shell exists whenever a Project is open.

It owns:

- Project identity/path presentation;
- Project-level toolbar;
- Workspace Dashboard;
- Document Tabs;
- runtime navigation context;
- DocumentKind → Workbench routing.

It does not own CAD authored semantics.

### 2. Navigation context is explicit and runtime-only

The Project Workspace Shell has one navigation context:

```text
Workspace
or
Document(DocumentId)
```

This state is runtime UI/navigation state and is not persisted.

Open DocumentSessions and current navigation are independent concepts.

The following is valid:

```text
open DocumentSessions:
  Part A
  Part B

navigation:
  Workspace
```

Returning to Workspace does not close, save or mutate any Document.

### 3. Workspace Dashboard is a first-class Project surface

Workspace Dashboard remains available even when Documents are open.

It is a Project-level surface reserved for future Project information, configuration, diagnostics and Project tools.

WS-01 may keep its contents minimal, but it must not be treated as an empty-state placeholder that disappears permanently after opening a Document.

### 4. Project toolbar is always visible

The Project Workspace Shell provides a top Project toolbar visible in both Workspace and Document contexts.

Initial actions:

- `Workspace`;
- `New Part…`;
- neutral `Open…`;
- `Refresh`;
- `Close Project`.

Future `New Assembly…` / `New Drawing…` actions may be added under later contracts.

These actions are Project/document-lifecycle actions, not Part editing tools.

### 5. Document Tabs belong to Project Workspace Shell

Document Tabs represent open DocumentSessions in the active Project.

They do not belong to Part, Assembly or Drawing Workbench.

Selecting a Document Tab changes navigation to `Document(DocumentId)` and routes to the appropriate Workbench for its DocumentKind.

Closing a tab closes that DocumentSession after the normal dirty-state decision.

Closing the last Document leaves the Project open and navigates to Workspace.

### 6. Document Workbench owns only active Document editing

Part Workbench owns Part editing/presentation for one active PartDocument.

It does not own:

- Project document discovery;
- Project-level Create/Open/Refresh;
- Project Document Tabs;
- Workspace Dashboard;
- cross-kind navigation.

Future Assembly/Drawing Workbenches follow the same boundary.

### 7. Current DocumentKind support

WS-01 routes only Part Documents because Part is the only implemented CAD Document editor.

Unknown/unimplemented DocumentKind fails closed with a diagnostic rather than routing to Part.

No Assembly or Drawing implementation is authorized.

### 8. Runtime view/edit preservation

Navigating from a Document to Workspace:

- captures runtime camera/view state as applicable;
- clears active Workbench runtime bindings;
- keeps the DocumentSession open;
- does not mutate authored state or dirty state.

Navigating back to that Document restores its Workbench context using the existing runtime view-state rules.

## Consequences

Project-level navigation no longer depends on Part Workbench.

Multiple Documents can remain open while the user visits Workspace.

Project toolbar stays available while editing a Document, allowing additional Documents to be created/opened without closing the current one.

Future Assembly/Drawing Workbenches can be added behind the same Project-level routing boundary.
