# CAD Workbench and Viewer — As-built

<!-- doc-id: internal.cad-workbench-viewer -->
<!-- document-kind: internal -->

<!-- section-id: internal.cad-workbench-viewer.shell -->
## Shared Workbench shell

WB-01 implements one reusable `CadWorkbenchShell` containing Document Tree, Editor Surface, Properties, Operations, bottom Document Tabs and Status/Diagnostics.

The shell owns layout only. The current Part composition is implemented by `CadWorkbench` plus narrow adapters.

WB-01A stabilized the existing shell/Viewer interaction without adding a new CAD domain.

<!-- section-id: internal.cad-workbench-viewer.active-document -->
## Active document context

ProjectSession may keep several canonical DocumentSessions open.

`CadWorkbench` owns one active DocumentId. Bottom tab changes switch the Tree, Properties, Viewer scene, selection and camera context while leaving other DocumentSessions open.

Camera state is stored in a Workbench runtime map keyed by DocumentId. Closing a tab drops that runtime view state.

<!-- section-id: internal.cad-workbench-viewer.origin -->
## Origin and reference scene

PartViewportController maps the Part's seven deterministic built-in Origin roles to provider-neutral `ReferencePresentation` objects and runtime presentation tokens.

The reference grid is a Viewer presentation primitive. It is not a Part semantic object and is not selectable.

Persistent Origin visibility comes from PartDocument authored presentation state. Hidden references still exist semantically and remain present in the Tree.

WB-01A scene replacement clears native detected/selected state before removing provider presentation objects. A failed native replacement is contained at the provider boundary and leaves provider presentation state coherent instead of propagating a process-level failure.

<!-- section-id: internal.cad-workbench-viewer.selection -->
## Selection authority and input grammar

The current Part viewport controller owns document-scoped runtime selected set plus primary selection.

Tree selection updates that authority. Qt/OCCT picking emits only neutral `SelectionIntent` values carrying `PresentationToken`; the controller maps tokens back to semantic built-in roles.

The controller pushes one coherent selection to Tree and Viewer. Primary selection drives Properties.

Current mouse grammar is:

```text
Left click                 → replace semantic selection
Ctrl + Left click          → toggle semantic selection member
Left click on empty space  → clear semantic selection
Middle drag                → Pan
Shift + Middle drag        → Orbit
Mouse wheel                → Zoom
Right click                → reserved; currently controlled no-op where no menu exists
```

Transient OCCT detection/highlight is not semantic selection.

<!-- section-id: internal.cad-workbench-viewer.viewer-boundary -->
## Viewer provider boundary

`IDocumentViewport` is provider-neutral. It exposes camera/navigation, reference scene, presentation selection and selection-intent transport.

`QtOcctViewerWidget` implements that contract behind the `viewer_qt_occt` module.

The production executable creates the concrete provider only in the application composition root and injects it as a neutral `ViewportSurface`.

The concrete provider contains recoverable OCCT/standard exceptions at its public/event boundary, logs the failure and fails closed instead of allowing those exceptions to unwind through Workbench UI.

Boundary tests reject OCCT/provider tokens from domain/application APIs and Workbench UI.

<!-- section-id: internal.cad-workbench-viewer.navigation -->
## Navigation and responsive ViewCube

The shared viewport supports middle-button Pan, Shift + middle-button Orbit, wheel Zoom, Fit All, six orthogonal standard views, Isometric and eight corner orientations, plus Orthographic/Perspective.

`ViewCubeWidget` is an overlay in the shared Editor Surface and invokes only `IDocumentViewport`.

WB-01A replaced the fixed button-grid presentation with a compact responsive CAD navigation control. It tracks its host Editor Surface and switches to a compact mode when space is constrained, so narrow splitter layouts do not force a large Viewer minimum width or overlap the right-side panels.

Because the production Qt/OCCT viewport is a native `WA_PaintOnScreen` child, overlay movement/resize explicitly schedules a coalesced repaint of the underlying Viewer surface. This prevents stale ViewCube pixels from remaining after splitter or right-panel resize without changing the provider-neutral Viewer API.

Navigation changes are runtime-only and do not increment DocumentRevision, set needsSave or create CAD Undo entries.

<!-- section-id: internal.cad-workbench-viewer.provider-presentation -->
## Current OCCT presentation

The provider currently presents Origin point, X/Y/Z axes, principal Origin planes when visible, a non-selectable reference grid and selected/primary visual emphasis.

Principal reference planes use finite provider presentation geometry; no native OCCT presentation object is durable semantic identity.

There is no modeled Part B-Rep in WB-01/WB-01A. The OCCT provider is presentation/navigation infrastructure, not Part evaluation.

<!-- section-id: internal.cad-workbench-viewer.runtime -->
## Runtime lifetime and stress coverage

Selection, primary selection, camera, projection, transient detection and grid presentation are runtime-only.

Persistent Origin visibility is authored state and is intentionally separate.

Closing/reopening the application recreates runtime view and selection state while preserving saved visibility in the Part file.

The native WB-01A stress regression runs the real Workbench and Qt/OCCT Viewer through 100 iterations combining clicks, empty-space clear, Ctrl-selection, right-click no-op, Show/Hide, Undo/Redo, navigation and repeated narrow/wide resizing.
