# PH-02B — Recent Project Availability UX

**Status:** ACCEPTED  
**Owner acceptance:** 2026-09-23  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Related baseline:** `work/PH-01_PROJECT_HUB.md`, `work/PH-02A_PROJECT_HUB_SELECTION_UX.md`

## Goal

Make Project Hub show when a Recent Project can no longer be opened from its remembered Workspace location, before the user attempts Open.

## Scope IN

- Derive current Recent Project availability when Hub refreshes.
- Availability is runtime/presentation state only; it is never persisted into the Recent catalog.
- Distinguish:
  - `Available`
  - `WorkspaceMissing`
  - `ProjectInvalid`
  - `IdentityMismatch`
- A missing Workspace remains in Recent Projects.
- Problematic entries are visibly marked in the list.
- For a selected problematic entry:
  - `Open` is disabled,
  - `Locate…` remains enabled,
  - `Remove from Recent` remains enabled.
- For an available selected entry, all three actions remain enabled.
- Double-click does not attempt Open for a non-openable entry.
- `Locate…` remains the explicit recovery path and must continue to verify expected `ProjectId` before updating location.

## Scope OUT

- no automatic removal of missing Recent entries,
- no persistence of availability state,
- no global filesystem scan,
- no ProjectId, metadata, relocation or Recent persistence semantic changes,
- no Create Project workflow changes,
- no CAD-domain implementation,
- no Part / Assembly / Drawing / DocumentSession / Viewer / OCCT.

## Acceptance

Automated tests prove:

1. Valid remembered Workspace is `Available`.
2. Moved/deleted remembered Workspace is `WorkspaceMissing`.
3. Existing folder with invalid/missing Project metadata is `ProjectInvalid`.
4. Existing valid Project with a different `ProjectId` at the remembered location is `IdentityMismatch`.
5. Availability inspection does not remove or rewrite the Recent entry.
6. Hub visibly marks a missing Workspace.
7. Selecting a missing/problematic entry leaves `Open` disabled while `Locate…` and `Remove from Recent` are enabled.
8. Selecting an available entry enables `Open`.
9. Existing PH-01 and PH-02A tests continue to pass unchanged.

## Decision level

This is D0/D1 presentation/application behavior. Availability is a derived internal Hub view state and does not alter durable identity, persistence meaning, subsystem ownership or public semantic APIs.
