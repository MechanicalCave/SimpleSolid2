# SimpleSolid 2.0

SimpleSolid 2.0 is a clean architectural restart of SimpleSolid focused on a coherent CAD model for human engineers and automation/AI clients.

## Current phase

**Part Modeling v1 — PM-05 Edge Features final acceptance**

The current product includes the Project/Workspace shell, native Part Documents, Shared-2D Sketch/Profile authoring, semantic Body topology, Offset Datum Plane reference geometry, authored Sketch-Line Axis, Extrude, Revolve, and explicit multi-Edge Fillet/Chamfer with edit/repair/lifecycle/persistence.

PM-05A through PM-05E are completed and exact-head gated. PM-05F is the active checkpoint for current documentation, cumulative automated evidence and final supported-Windows Owner acceptance. PM-05 remains active until that explicit Owner PASS; PM-06 remains separately gated.

The repository authority for the exact active scope is always `work/ACTIVE.yaml` and its referenced accepted Work Contract/program roadmap.

## Run locally

Start the normal SS2 work session:

```text
START_SS2.cmd
```

The opened SS2 PowerShell provides:

```text
ss2-status
ss2-run
ss2-docs
ss2-resume
```

`ss2-run` performs an incremental build and starts the current `SimpleSolid2` application.

`ss2-docs` regenerates the self-contained Product Browser at `docs/browser/index.html` from canonical Markdown.

## Documentation

Current-state documentation is maintained alongside the code:

- `docs/product/pl/` — Polish user/product documentation;
- `docs/product/en/` — English user/product documentation;
- `docs/internal/` — internal as-built documentation;
- `docs/browser/index.html` — generated Product Browser, Polish by default with PL/EN switching.

Markdown is canonical. Documentation completion and update rules are defined in `governance/DOCUMENTATION.md`. Future work contracts declare their Documentation Impact and required documentation is updated before completion.

## Architecture entry points

- `governance/FOUNDATION.md`
- `governance/CONSTITUTION.md`
- `governance/ARCHITECTURE.md`
- `AGENTS.md`
- `work/ACTIVE.yaml`

## SS1 relationship

The previous SimpleSolid repository is a donor/reference implementation. SS2 selectively reuses proven concepts, tests and compatible code. SS2 Foundation is authoritative when the two differ.
