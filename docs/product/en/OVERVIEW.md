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

- creating and opening Projects;
- stable Project identity and Recent Projects;
- moving a Project and recovering its location with `Locate…`;
- creating and opening native `.ss2part` Part Documents;
- stable `DocumentId` independent of filename and path;
- opening several Parts and switching them with bottom Document Tabs;
- a shared CAD Workbench with Document Tree, Properties, Operations and Status areas;
- an OCCT-backed 3D Viewport with reference grid and ViewCube;
- Pan, Orbit, Zoom, Fit, standard/corner views and Orthographic/Perspective;
- built-in Part Origin point, axes and planes;
- synchronized Tree/Viewport selection with a primary selection context;
- persistent Show/Hide for Origin references with Undo/Redo;
- editing Number, Title, Description and Engineering Revision;
- Save and safe closing with unsaved-change protection;
- rediscovering Parts after restart;
- identity-conflict detection when two files in one Workspace carry the same DocumentId.

The current Part is a durable CAD Document with a shared 3D working environment, but it does not yet contain modeled solid geometry.

Sketch, Bodies/Features, Assembly and Drawing are not yet available.

<!-- section-id: product.overview.browser -->
## Product Browser

The Product Browser is generated from canonical Markdown documents stored in the repository.

Polish and English are maintained as peer sources. The Browser opens product documentation in Polish by default and allows switching to English.
