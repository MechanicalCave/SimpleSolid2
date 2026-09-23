# SimpleSolid 2.0 — Internal As-built Overview

<!-- doc-id: internal.overview -->
<!-- document-kind: internal -->

This documentation describes the accepted implementation currently present in SS2.

It is explanatory, not normative. If an as-built document conflicts with the Engineering Constitution, Foundation, Architecture, an accepted ADR or the active Work Contract, the higher-authority source wins and the documentation must be corrected.

<!-- section-id: internal.overview.current-scope -->
## Current implemented scope

The current executable implements the Project platform and a minimal Qt application shell:

```text
App start
→ Project Hub
→ Create new Project / Open existing Project
→ stable ProjectId + Workspace metadata
→ ProjectSession
→ empty Workspace Shell
→ Close
→ Recent Projects
→ reopen / relocate
```

The current product does **not** yet implement Part, Assembly, Drawing, DocumentSession, CAD geometry, OCCT modeling, Viewer or topology/reference semantics.

<!-- section-id: internal.overview.layers -->
## Current implementation layers

The implemented Project surface follows the frozen dependency direction:

```text
Qt Project Hub / Workspace Shell
        ↓
internal ProjectHubController
        ↓
ProjectSession / RecentProjectStore / ProjectWorkspaceMetadataService
        ↓
filesystem + Qt application-state location adapter
```

Qt selects presentation and user-state locations. Durable Project identity and Project metadata are owned below the UI.

<!-- section-id: internal.overview.identity -->
## Identity and location

A Project is identified by stable `ProjectId`.

A Workspace path is a location, not identity. Moving or renaming the Workspace preserves Project identity. An ordinary filesystem copy also preserves the embedded ProjectId and therefore does not automatically create a new logical Project.

The current metadata implementation generates and validates textual UUIDv4 ProjectIds. That textual format is an implementation fact, not an additional Foundation-level identity rule.

<!-- section-id: internal.overview.documentation -->
## Documentation system

Canonical current-state sources are:

- `docs/internal/` — internal as-built documentation in English;
- `docs/product/pl/` — Polish user/product documentation;
- `docs/product/en/` — English user/product documentation.

`docs/browser/index.html` is generated from canonical Markdown.

Operational documentation rules are defined in `governance/DOCUMENTATION.md`.
