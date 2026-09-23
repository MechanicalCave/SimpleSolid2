# Project Platform — As-built

<!-- doc-id: internal.project-platform -->
<!-- document-kind: internal -->

<!-- section-id: internal.project-platform.project -->
## Project

A Project is the durable logical project identity.

Current durable Project metadata is stored under the Workspace root at:

```text
.simplesolid/project.json
```

The metadata contains `project_id`, `display_name`, `schema_version` and `created_at`.

The metadata service validates existing initialized Workspaces fail-closed.

<!-- section-id: internal.project-platform.workspace -->
## Workspace

A Project Workspace is exactly the filesystem root in which that Project lives.

The Workspace path is not Project identity. Moving or renaming the Workspace preserves `ProjectId`.

Native Part Documents are ordinary `.ss2part` files inside this Workspace. Their physical location is discovered from the filesystem rather than from a hidden persistent catalog.

<!-- section-id: internal.project-platform.creation -->
## Project creation

The Project Hub creation flow accepts Project name / DisplayName, an existing parent Location and one Project folder name.

Create requires the target child folder to be absent. Existing targets are never silently adopted, initialized or overwritten.

Creation is staged before publication and cleans only SS2-owned artifacts on failure.

<!-- section-id: internal.project-platform.session -->
## ProjectSession

`ProjectSession` is runtime-only.

Opening a Workspace validates Project metadata, resolves a canonical Workspace root and builds a runtime document discovery index.

The ProjectSession coordinates canonical active DocumentSessions for that Project. Within one ProjectSession, one resolved DocumentId has at most one active DocumentSession.

ProjectSession also owns no durable authored Part semantics. Those belong to the PartDocument loaded by its DocumentSession.

Closing a Project destroys the runtime ProjectSession and all contained DocumentSessions.

<!-- section-id: internal.project-platform.documents -->
## Workspace Document discovery

Discovery recursively scans the Workspace for native `.ss2part` files and skips the private `.simplesolid` metadata directory.

A valid file contributes its embedded DocumentId and current path to the runtime index.

One DocumentId at one path is resolved. One DocumentId at multiple paths becomes `IdentityConflict`. Invalid native files remain visible as invalid entries.

Discovery is rebuildable. The current implementation does not require a continuous filesystem watcher.

<!-- section-id: internal.project-platform.recent -->
## Recent Projects

Recent Projects is application/user state, not Project persistence.

The private catalog is versioned and keyed logically by `ProjectId`. Recording an already-known ProjectId at a different location is an identity conflict rather than a silent path update.

Removing a Recent entry never modifies Project or Part files.

<!-- section-id: internal.project-platform.copy-semantics -->
## Filesystem copies

An ordinary filesystem copy of a Project preserves ProjectId.

An ordinary filesystem copy of a `.ss2part` file preserves DocumentId.

Inside one Workspace, two files carrying the same DocumentId are not treated as two independent Parts. They are an explicit identity conflict until the user resolves the filesystem situation outside the current PART-01 feature set.
