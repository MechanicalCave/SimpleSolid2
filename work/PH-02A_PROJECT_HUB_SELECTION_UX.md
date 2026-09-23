# PH-02A — Project Hub Selection UX

**Status:** ACCEPTED  
**Owner acceptance:** 2026-09-23  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Related baseline:** `work/PH-01_PROJECT_HUB.md`

## Goal

Make the Recent Projects actions in Project Hub communicate their selection dependency clearly and behave only on a visibly selected Recent entry.

## Scope IN

- Project Hub starts with no Recent entry selected.
- `Open`, `Locate…`, and `Remove from Recent` are disabled when no Recent entry is selected.
- Selecting one Recent entry enables those actions.
- Clearing selection or refreshing Recent restores the disabled state.
- `selectedProjectId()` returns an ID only for a genuinely selected item, not merely the current item.
- Add automated Qt regression coverage for these states.

## Scope OUT

- no changes to Create Project workflow,
- no changes to ProjectId semantics,
- no changes to project metadata or Recent Projects persistence semantics,
- no new CAD domain behavior,
- no Part / Assembly / Drawing / DocumentSession / Viewer / OCCT,
- no redesign of Project Hub layout beyond the selection-state behavior above.

## Acceptance

The work item is complete when automated tests prove:

1. Hub with populated Recent Projects starts with no visible selection.
2. Recent action buttons are disabled initially.
3. Selecting a Recent entry enables all three actions.
4. Clearing selection disables all three actions again.
5. The selected ProjectId is absent without a real selection and present for the selected item.
6. Existing PH-01 tests continue to pass unchanged.

## Decision level

This is D0/D1 UI behavior inside the accepted scope. It does not change architecture, ownership, durable identity, or persistence meaning.
