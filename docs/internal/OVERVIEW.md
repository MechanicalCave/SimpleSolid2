# SimpleSolid 2.0 — Internal As-built Overview

<!-- doc-id: internal.overview -->
<!-- document-kind: internal -->

This documentation describes the accepted implementation currently present in SS2.

It is explanatory, not normative. If an as-built document conflicts with the Engineering Constitution, Foundation, Architecture, an accepted ADR or the active Work Contract, the higher-authority source wins and the documentation must be corrected.

<!-- section-id: internal.overview.current-scope -->
## Current implemented scope

The current executable implements the Project platform, persistent empty Part Documents and the shared CAD Workbench / Viewer foundation:

```text
App start
→ Project Hub
→ Create / Open Project
→ ProjectSession
→ discover native .ss2part Documents
→ Create / Open several Parts
→ canonical DocumentSessions
→ shared CAD Workbench
→ bottom Document Tabs + one ActiveDocumentSession
→ Document Tree + built-in Origin
→ shared Properties / Operations surfaces
→ OCCT-backed 3D Viewport + reference grid + ViewCube
→ Tree / Viewport selection synchronization
→ persistent Show / Hide of Origin references
→ Undo / Redo
→ Save / Close
→ restart
→ rediscover the same DocumentId and saved Origin visibility
```

The product still does **not** implement Sketch, Body/Feature modeling, solid geometry evaluation, Assembly, Drawing, BOM, modeled-topology selection or persistent topology naming.

<!-- section-id: internal.overview.layers -->
## Current implementation layers

The implemented dependency direction has two coordinated branches:

```text
Qt ProjectHubWindow
        ↓
CadWorkbenchShell + CadWorkbench
        ├── ProjectSession / DocumentSession
        │       ↓
        │   PartDocument + PartDocumentTransaction
        │       ↓
        │   PartDocumentStore / atomic persistence
        │
        └── PartViewportController / Tree adapter
                ↓
            provider-neutral Viewer API
                ↓
            Qt/OCCT Viewer provider
                ↓
                OCCT
```

`CadWorkbenchShell` owns only the fixed UI regions. Part-specific lifecycle, Tree and Viewer adapters sit outside that neutral shell.

OCCT types stay inside the concrete Viewer provider. They do not participate in Part authored semantics, persistence, Document identity or command/transaction ownership.

<!-- section-id: internal.overview.identity -->
## Identity and location

A Project is identified by stable `ProjectId`.

A Part Document is independently identified by stable `DocumentId`.

Neither identity is a path or filename. Moving or renaming a Workspace preserves ProjectId. Moving or renaming a native `.ss2part` file inside its Workspace preserves DocumentId.

The current implementations serialize ProjectId and DocumentId as textual UUIDv4 values. That encoding is an implementation fact, not an additional Foundation-level identity rule.

A copied `.ss2part` file preserves its embedded DocumentId. If more than one file in one Workspace declares the same DocumentId, discovery reports `IdentityConflict` and resolution by that ID fails closed.

<!-- section-id: internal.overview.runtime-presentation -->
## Runtime and persistent presentation state

Camera, projection, pan/orbit/zoom, active selection and primary selection are runtime-only. They do not increment DocumentRevision, dirty the Part or create CAD Undo entries.

User-authored visibility of built-in Origin references is different: it is persistent presentation semantics stored by PartDocument, changed through DocumentSession commands, Undo/Redo-able and saved in the native Part file.

<!-- section-id: internal.overview.documentation -->
## Documentation system

Canonical current-state sources are:

- `docs/internal/` — internal as-built documentation in English;
- `docs/product/pl/` — Polish user/product documentation;
- `docs/product/en/` — English user/product documentation.

`docs/browser/index.html` is generated from canonical Markdown.

Operational documentation rules are defined in `governance/DOCUMENTATION.md`.
