# ADR-0006 — Workspace, Document Workbench and contextual tool surfaces

**Status:** ACCEPTED  
**Date:** 2026-09-24  
**Owner acceptance:** 2026-09-24  
**Decision class:** D2 Architecture  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Related:** ADR-0002, ADR-0003, ADR-0005

## Context

Manual SK-01 validation exposed three UI/lifecycle architecture corrections:

- Project Workspace currently looks like an inactive Part editor even when no CAD Document is open;
- the right-side Operations surface was used as a permanent tool launcher, but it should belong to the currently active tool/edit context;
- an existing Sketch needs an explicit edit-entry path from Document Tree.

The same validation also exposed a critical lifetime regression: closing an active Part can destroy its DocumentSession before the Workbench detaches runtime Viewer/Tree bindings.

## Decision

### 1. Project Workspace and Document Workbench are distinct runtime contexts

Opening a Project enters Project Workspace context only.

If no CAD Document is open, no Part/Assembly/Drawing editor is active and the Workspace shows a neutral empty/document-launch surface.

Creating or opening a CAD Document activates the Workbench appropriate to that DocumentKind.

Current implementation supports Part only. Future Assembly/Drawing routing must use DocumentKind and must not make the empty Workspace pretend to be any one CAD domain.

Closing the last open Document returns to the neutral Workspace surface without closing the Project.

### 2. Project/document launch actions do not belong to Part editor semantics

Project Workspace owns document-launch actions such as current `New Part…` and neutral `Open…`.

The active Part Workbench owns document editing actions such as Save/Undo/Redo/Close and Part editing tools.

This ADR does not implement Assembly/Drawing creation; it only fixes ownership of the launch surface.

### 3. Editor toolbar launches tools

The editor surface has a toolbar region above the 3D Viewport.

The Part `Sketch` tool launcher belongs there.

Future Part modeling tools may also contribute launch actions there under later contracts.

### 4. Operations is contextual

The right-side Operations area is not a permanent tool catalog.

It presents controls/instructions belonging to the active operation or edit context.

For SK-01A:

- no active tool/edit context → no Sketch launcher in Operations;
- Sketch support-pick active → contextual support-pick guidance/cancel action may be shown;
- Sketch edit active → `Finish Sketch` is shown there.

Future Sketch tool panels may use this contextual area under later contracts.

### 5. Existing Sketch edit entry

A durable Sketch in Document Tree can enter the same Sketch edit context by:

- double-click on the Sketch item;
- context-menu `Edit Sketch`.

Both paths identify the Sketch by stable SketchId stored as transient Tree item data and revalidate it against the active Part Document before entering edit mode.

Tree row/index is never durable Sketch identity.

### 6. Runtime bindings must detach before owning session destruction

UI/Viewer controllers that hold non-owning pointers into DocumentSession must be detached before ProjectSession erases/destroys that DocumentSession.

The same rule applies before destroying the complete ProjectSession.

No runtime cleanup function may dereference a DocumentSession after its owner has destroyed it.

## Consequences

Workspace remains neutral until a real Document is activated.

The toolbar/Operations distinction can scale to future CAD tools without turning Operations into a second launcher strip.

Sketch edit can be resumed from persisted Tree objects without adding new authored state.

Document close/project close lifetime ordering becomes an explicit correctness invariant.
