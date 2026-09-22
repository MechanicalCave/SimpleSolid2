# SimpleSolid 2.0 — Architecture Baseline

**Status:** FROZEN BASELINE  
**Version:** 1.0  
**Date:** 2026-09-22

This document records the initial dependency shape for SS2. Detailed subsystem APIs are defined later by ADRs and work contracts.

## Dependency direction

```text
Application Shell / UI / Automation
                ↓
Application Services / Commands
                ↓
CAD Domains
Part / Assembly / Drawing
                ↓
Shared semantic/platform contracts
                ↓
Core / Geometry / Kernel API / Persistence primitives
                ↓
Qt / OCCT / OS / format adapters
```

Lower layers do not depend upward on CAD domains or UI.

## Initial subsystem map

```text
Application
├── Project Hub
├── ProjectSession
├── DocumentSession
└── Command routing / presentation binding

Domains
├── Part
│   └── Sheet Metal specialization
├── Assembly
└── Drawing

Shared CAD mechanisms
├── Shared 2D Authoring
├── Projection Engine
├── Selection binding
├── Semantic reference infrastructure
├── Units / spatial primitives
└── Viewer scene/presentation mechanisms

Platform
├── Core identity/revision primitives
├── Persistence infrastructure
├── Kernel API
├── OCCT provider
├── Import / Export
└── Diagnostics
```

## Runtime ownership

`ProjectSession` coordinates one opened Project and its open `DocumentSession` objects.

`DocumentSession` coordinates runtime state for one opened persistent CAD Document: save checkpoint, Undo/Redo, edit/transaction state, derived evaluation/cache handles and presentation bindings.

Neither session is durable design intent.

## Technology baseline

- C++20
- CMake
- Qt 6 Widgets
- OCCT behind provider-neutral Kernel API
- Windows-first
- GitHub repository

Exact compiler and dependency patch versions belong to repository/toolchain configuration.

## Architecture change rule

Changes to public APIs, subsystem ownership, dependency direction, durable identity/reference semantics or persistence meaning are D2 Architecture decisions and require Owner approval before implementation.
