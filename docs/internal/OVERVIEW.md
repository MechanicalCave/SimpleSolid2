# SimpleSolid 2.0 — Internal As-built Overview

<!-- doc-id: internal.overview -->
<!-- document-kind: internal -->

This documentation describes the accepted implementation currently present in SS2.

It is explanatory, not normative. If an as-built document conflicts with the Engineering Constitution, Foundation, Architecture, an accepted ADR or the active Work Contract, the higher-authority source wins and the documentation must be corrected.

<!-- section-id: internal.overview.current-scope -->
## Current implemented scope

The current executable implements the Project platform, durable Part/Sketch/Profile authoring and the first single-Body solid-modeling vertical slice:

```text
App start
→ Project Hub / ProjectSession
→ discover native .ss2part Documents
→ canonical DocumentSessions + Document Tabs
→ shared CAD Workbench
→ Part Origin + durable Offset Datum reference geometry
→ persistent Origin-plane / planar Body-Surface / Datum-backed Sketches
→ Shared 2D Line / Circle / Arc / Rectangle authoring
→ precision input + Polar + Dynamic Input + OSNAP/Tracking/Inference
→ Trim / Extend / Measure
→ Part-owned live-reference Profiles + Sketch-Line Axes
→ one durable Body with ordered Extrude / Revolve Features
→ Extrude Add / Cut + Revolve Add / Cut
→ derived Kernel evaluation + solid presentation
→ Feature edit / Suppress / Delete + Undo / Redo
→ Save / Close / Reopen
→ cold rebuild from authored Part state
```

The Part Feature model currently supports one Body and two solid Feature families: Extrude and Revolve. The first successful solid-producing Feature is Add; later ordered Features may be Add or Cut. Extrude supports OneSide Forward/Reverse and Midplane distance; Revolve supports explicit Origin/Authored AxisReference, OneSide/Midplane angle, One-Side Reverse and 0 < Angle <= 360 degrees. Failed, Blocked and Suppressed Feature states remain explicit; derived B-Rep is never persisted as authored truth.

Assembly and Drawing remain unimplemented. Part now provides Offset Datum Planes, Part-owned Sketch-Line Axes, Sketch support on Origin planes, planar Body Faces and Datum Planes, direct current Face/Edge/Vertex inspection, View Styles, Extrude/Revolve and explicit semantic re-support/repair for Sketch/Axis dependencies. It does not yet provide Datum Axis/Point, additional Datum Plane constructors, Body-Edge/Curve Axis constructors, Projection, Fillet/Chamfer, multi-turn Revolve, multi-body modeling or general authored topology-reference repair for arbitrary future Feature inputs.

<!-- section-id: internal.overview.layers -->
## Current implementation layers

The implemented dependency direction keeps authored meaning separate from evaluation and presentation:

```text
Qt ProjectHubWindow / CadWorkbench
        ↓
ProjectSession / DocumentSession
        ↓
semantic Commands + PartDocumentTransaction
        ↓
PartDocument authored state
  ├── Datum / Sketch / Profile semantics
  └── Body / ordered Feature semantics
        ↓
provider-neutral Part + Datum evaluation / Kernel API
        ↓
OCCT solid-modeling provider
        ↓
provider-neutral Viewer scene contracts
        ↓
Qt/OCCT Viewer provider
```

`CadWorkbenchShell` owns only fixed application UI regions. Tree, Properties, Operations, Command Line and Viewer adapters project or invoke semantic state; they are not additional CAD models.

OCCT handles, topology ordinals, Datum presentation objects/intersection segments and Viewer presentation tokens stay runtime/provider-local. They do not participate in Part authored identity, persistence, DatumId, Body/Feature identity or semantic reference meaning.

<!-- section-id: internal.overview.identity -->
## Identity and location

A Project is identified by stable `ProjectId`.

A Part Document is independently identified by stable `DocumentId`.

Neither identity is a path or filename. Moving or renaming a Workspace preserves ProjectId. Moving or renaming a native `.ss2part` file inside its Workspace preserves DocumentId.

The current implementations serialize ProjectId and DocumentId as textual UUIDv4 values. That encoding is an implementation fact, not an additional Foundation-level identity rule.

A copied `.ss2part` file preserves its embedded DocumentId. If more than one file in one Workspace declares the same DocumentId, discovery reports `IdentityConflict` and resolution by that ID fails closed.

<!-- section-id: internal.overview.runtime-presentation -->
## Runtime and persistent presentation state

Camera, projection, pan/orbit/zoom, active selection, Extrude/Revolve/Datum preview geometry, transient Revolve Axis emphasis, virtual Datum/Body intersection segments, evaluated B-Rep handles, derived Datum/Axis frames and presentation tokens are runtime-only. They do not become authored Part identity.

User-authored visibility of built-in Origin references, authored Sketch-Line Axes, individual Datum Planes and Profile visibility policy are persistent presentation semantics changed through semantic commands and covered by Undo/Redo. `Reference Geometry` group Show/Hide bulk-updates child Datum visibility and owns no second persisted group flag. Profile policy is `automatic`, `force_shown` or `force_hidden`; automatic presentation reacts to active Feature consumption without changing evaluation.

Feature Suppress is not visibility. Suppress is authored modeling state that preserves FeatureId and parameters while removing the Feature contribution from evaluation.

The native Viewer clears provider-native detection/selection state before presentation objects are removed, and recoverable provider exceptions are contained at the concrete Viewer boundary. A valid authored commit is not rolled back merely because presentation refresh fails.

<!-- section-id: internal.overview.documentation -->
## Documentation system

Canonical current-state sources are:

- `docs/internal/` — internal as-built documentation in English;
- `docs/product/pl/` — Polish user/product documentation;
- `docs/product/en/` — English user/product documentation.

`docs/browser/index.html` is generated from canonical Markdown.

Operational documentation rules are defined in `governance/DOCUMENTATION.md`.
