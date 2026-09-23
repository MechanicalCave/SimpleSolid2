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

The metadata contains:

- `project_id`;
- `display_name`;
- `schema_version`;
- `created_at`.

The current metadata schema version is 1.

The metadata service validates existing initialized Workspaces fail-closed. Missing metadata, malformed metadata, unsupported schema, invalid ProjectId and invalid timestamps are explicit failures.

<!-- section-id: internal.project-platform.workspace -->
## Workspace

A Project Workspace is exactly the filesystem root in which that Project lives.

The Workspace path is not Project identity.

Moving or renaming the Workspace preserves `ProjectId`. Project relocation therefore updates a known location only after identity has been revalidated.

<!-- section-id: internal.project-platform.creation -->
## Project creation

The Project Hub creation flow accepts:

- Project name / DisplayName;
- an existing parent Location;
- one Project folder name.

Project name and Project folder are independent. Neither is durable identity.

Create requires the target child folder to be absent. Existing targets are never silently adopted, initialized or overwritten.

Creation is staged:

```text
existing parent Location
→ SS2-owned temporary staging directory
→ metadata initialization
→ publish staging directory as requested Workspace
→ ProjectSession::open
→ record Recent
→ enter Workspace Shell
```

If creation fails, SS2 cleans only artifacts it owns. Rollback of a published Workspace verifies that its metadata still carries the ProjectId created by that operation before removing it.

Folder input is restricted to one child folder name. Absolute paths, `.`, `..` and nested paths are rejected.

<!-- section-id: internal.project-platform.session -->
## ProjectSession

`ProjectSession` is runtime-only.

Opening a Workspace validates Project metadata and resolves a canonical Workspace root. The session exposes the current ProjectId, DisplayName and Workspace root.

Closing a Project destroys/resets the active ProjectSession. Reopening creates a new runtime session for the same durable Project identity.

ProjectSession is not persisted and is not Project identity.

<!-- section-id: internal.project-platform.recent -->
## Recent Projects

Recent Projects is application/user state, not Project persistence.

The private catalog is versioned and keyed logically by `ProjectId`. It stores the ProjectId, DisplayName and remembered absolute Workspace path.

The store keeps at most 20 entries.

Recording an already-known ProjectId at a different location is an identity conflict rather than a silent path update. Explicit relocation is the supported operation for changing the remembered location of the same Project.

Removing a Recent entry never modifies Project files.

<!-- section-id: internal.project-platform.copy-semantics -->
## Filesystem copies

An ordinary filesystem copy preserves the embedded ProjectId.

If Hub already knows ProjectId A at one Workspace and another presented Workspace also declares ProjectId A, the second location is not registered as a separate logical Project.

The current product has no Duplicate / Save as New Project operation yet. Creating a new independent identity from a copy is therefore not currently available.
