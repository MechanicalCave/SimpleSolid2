# SimpleSolid 2.0 Product Documentation

<!-- doc-id: product.overview -->
<!-- document-kind: product -->

<!-- section-id: product.overview.purpose -->
## About this documentation

This documentation describes the **current accepted product state** of SimpleSolid 2.0.

It is written for understanding and using the application. It does not describe implementation history, active development work, roadmap intent, C++ class structure or CI evidence.

<!-- section-id: product.overview.current -->
## What is currently available

The current product provides:

- Project create/open, Recent Projects and location recovery;
- native `.ss2part` Part Documents with stable `DocumentId`;
- multiple open Parts with Document Tabs;
- a shared CAD Workbench with Tree, Properties, Operations, Status and keyboard-first Command Line;
- an OCCT-backed 3D Viewport, ViewCube, Pan/Orbit/Zoom/Fit and Ortho/Perspective;
- persistent Origin references, planar Body Faces and Offset Datum Planes as semantic Sketch supports;
- Reference Geometry with durable Offset Datum Planes, Edit/Show/Hide and Viewport preview/intersection cues;
- Shared-2D Line/Circle/Arc/Rectangle, Regular/Construction, grip editing and Move/Copy/Rotate/Scale/Mirror;
- unit-aware precision input, Polar, Dynamic Input and OSNAP/Tracking/Inference;
- Trim/Extend and read-only Measure;
- Part-owned live-reference Profiles with Add/Subtract composition;
- exactly one durable Body with ordered Extrude Features;
- Extrude Add/Cut with OneSide Forward/Reverse and Midplane;
- dynamic preview, Edit Extrude, Feature status, Suppress/Unsuppress/Delete and Undo/Redo;
- conflict-protected Save plus Save/Close/Reopen cold rebuild of Body, Datums and their semantic dependencies.

Part is currently the only implemented top-level CAD Document kind. Assembly and Drawing are not yet available.

Current solid modeling remains intentionally limited to Extrude Add/Cut. Datum Axis, Datum Point, additional Datum Plane constructors, Projection, Revolve, Fillet/Chamfer and multi-body remain later scope.

<!-- section-id: product.overview.browser -->
## Product Browser

The Product Browser is generated from canonical Markdown documents stored in the repository.

Polish and English are maintained as peer sources. The Browser opens product documentation in Polish by default and allows switching to English.
