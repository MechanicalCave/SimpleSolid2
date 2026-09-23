# SimpleSolid 2.0 — Internal As-built Overview

<!-- doc-id: internal.overview -->
<!-- document-kind: internal -->

This documentation describes the accepted implementation currently present in SS2.

It is explanatory, not normative. If an as-built document conflicts with the Engineering Constitution, Foundation, Architecture, an accepted ADR or the active Work Contract, the higher-authority source wins and the documentation must be corrected.

<!-- section-id: internal.overview.current-scope -->
## Current implemented scope

The current executable implements the Project platform plus the first persistent CAD Document lifecycle for an empty Part:

```text
App start
→ Project Hub
→ Create / Open Project
→ stable ProjectId + Workspace metadata
→ ProjectSession
→ discover native .ss2part Documents
→ Create / Open Part
→ DocumentSession
→ edit common Document Properties
→ Undo / Redo
→ Save / Close
→ restart
→ rediscover and reopen the same DocumentId
```

The current product does **not** yet implement Part feature modeling, Sketch, Bodies, geometry evaluation, OCCT modeling, Viewer, Assembly, Drawing, BOM, topology selection or persistent naming.

<!-- section-id: internal.overview.layers -->
## Current implementation layers

The implemented dependency direction is:

```text
Qt Project Hub / PartWorkspacePanel
        ↓
ProjectHubController / ProjectSession / DocumentSession
        ↓
PartDocument + PartDocumentTransaction
        ↓
PartDocumentStore / shared atomic-file persistence
        ↓
filesystem
```

Project and Document authored semantics remain below Qt. No OCCT type participates in the current Part lifecycle.

<!-- section-id: internal.overview.identity -->
## Identity and location

A Project is identified by stable `ProjectId`.

A Part Document is independently identified by stable `DocumentId`.

Neither identity is a path or filename. Moving or renaming a Workspace preserves ProjectId. Moving or renaming a native `.ss2part` file inside its Workspace preserves DocumentId.

The current implementations serialize ProjectId and DocumentId as textual UUIDv4 values. That encoding is an implementation fact, not an additional Foundation-level identity rule.

A copied `.ss2part` file preserves its embedded DocumentId. If more than one file in one Workspace declares the same DocumentId, discovery reports `IdentityConflict` and resolution by that ID fails closed.

<!-- section-id: internal.overview.documentation -->
## Documentation system

Canonical current-state sources are:

- `docs/internal/` — internal as-built documentation in English;
- `docs/product/pl/` — Polish user/product documentation;
- `docs/product/en/` — English user/product documentation.

`docs/browser/index.html` is generated from canonical Markdown.

Operational documentation rules are defined in `governance/DOCUMENTATION.md`.
