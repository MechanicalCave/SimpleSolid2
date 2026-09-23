# PH-02C — Project Creation UX

**Status:** ACCEPTED  
**Owner acceptance:** 2026-09-23  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Related baseline:** `work/PH-01_PROJECT_HUB.md`, `work/PH-02A_PROJECT_HUB_SELECTION_UX.md`, `work/PH-02B_RECENT_PROJECT_AVAILABILITY_UX.md`

## Goal

Make Create Project operate on a parent projects location and let SimpleSolid create the new Project Workspace folder and its private configuration.

## Scope IN

- `Create Project…` opens one dialog containing:
  - Project name,
  - Location (parent folder),
  - Project folder.
- Project folder is proposed from Project name and remains editable independently.
- Project name / DisplayName and filesystem folder name are not identity.
- User selects an existing parent Location; SS2 creates a new child Workspace folder.
- The created Workspace contains the existing private `.simplesolid/project.json` metadata.
- The target Project folder must not preexist. Existing target folders fail closed with no adoption, overwrite or initialization.
- Folder input must denote one child folder only; no absolute path, `.`, `..` or nested path is accepted.
- Creation uses an SS2-owned staging folder and publishes the completed Workspace to the requested target only after metadata initialization succeeds.
- On failure, SS2 cleans only its own staging/new Project artifacts and never deletes preexisting user data.
- After successful creation:
  - a validated `ProjectSession` is active,
  - Workspace Shell opens,
  - Recent Projects records the new Workspace.
- `Open Project…`, Recent, Locate and PH-02A/PH-02B behavior remain unchanged.

## Scope OUT

- no adoption of existing folders during Create,
- no automatic project-root scanning,
- no durable identity changes,
- no ProjectId format/API expansion,
- no CAD domains,
- no Part / Assembly / Drawing / DocumentSession / Viewer / OCCT.

## Acceptance

Automated tests prove:

1. Create under an existing parent creates exactly one requested Workspace child.
2. Workspace contains valid `.simplesolid/project.json`.
3. DisplayName may differ from folder name while ProjectId remains the identity.
4. Successful Create enters `ProjectSession` and records Recent.
5. Existing target folder fails closed and its contents are preserved.
6. Invalid child-folder inputs fail closed without creating nested/outside paths.
7. Missing/non-directory parent Location fails closed and is not silently created.
8. Metadata/creation failure removes SS2-owned staging artifacts.
9. Qt dialog exposes Project name / Location / Project folder and keeps Project folder editable independently.
10. Existing PH-01 / PH-02A / PH-02B tests continue to pass unchanged.

## Decision level

D0/D1 application/UI workflow only. ProjectId, metadata ownership, Workspace identity semantics and persistence meaning remain unchanged.
