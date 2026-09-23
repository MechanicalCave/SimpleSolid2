# SimpleSolid 2.0

SimpleSolid 2.0 is a clean architectural restart of SimpleSolid focused on a coherent CAD model for human engineers and automation/AI clients.

## Current phase

**MAIN v0.1 — Testable Project Hub: complete**

Implemented and verified lifecycle:

```text
App start
→ Project Hub
→ Create Project under a selected parent Location / Open existing Project
→ stable ProjectId + metadata
→ ProjectSession
→ empty Workspace Shell
→ Close
→ restart
→ Recent Projects
→ reopen the same ProjectId
```

Create Project now creates a new child Workspace folder under a selected existing parent Location; the Project name and folder name remain separate from ProjectId identity. Existing target folders fail closed and are never adopted by Create.

No subsequent product implementation work is active until the Owner accepts the next explicit work contract. In particular, PH-01 does not authorize Part, Assembly, Drawing, DocumentSession, Viewer, OCCT modeling, or other CAD-domain implementation.

## Run locally

Start the normal SS2 work session:

```text
START_SS2.cmd
```

The opened SS2 PowerShell provides:

```text
ss2-status
ss2-run
ss2-resume
```

`ss2-run` performs an incremental build and starts the current `SimpleSolid2` application.

## Architecture entry points

- `governance/FOUNDATION.md`
- `governance/CONSTITUTION.md`
- `governance/ARCHITECTURE.md`
- `AGENTS.md`
- `work/ACTIVE.yaml`

## SS1 relationship

The previous SimpleSolid repository is a donor/reference implementation. SS2 selectively reuses proven concepts, tests and compatible code. SS2 Foundation is authoritative when the two differ.
