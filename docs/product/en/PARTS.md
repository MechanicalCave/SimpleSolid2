# Part Documents

<!-- doc-id: product.parts -->
<!-- document-kind: product -->

<!-- section-id: product.parts.create -->
## Creating a new Part

After opening a Project, you see the Project Workspace Dashboard with no active Part editor. Use `New Part…` on the persistent Project Workspace toolbar to create a Document and open its Part Workbench. That toolbar remains visible while a Part is being edited.

The dialog shows the folder hierarchy of the current Workspace. Choose the target folder, enter the Part filename and confirm creation.

A native Part is one portable file and uses the extension:

```text
.ss2part
```

All durable Document content belongs to that one file. Moving or renaming the file changes its location, not its DocumentId.

The application proposes `Part001.ss2part`, `Part002.ss2part`, and so on when those names are available.

Use `Create Folder` if you want to add a new folder inside the Workspace before creating the Part.

The location picker is constrained to the current Workspace. It does not expose private `.simplesolid` metadata as a normal target and does not allow creating the Document outside the Workspace.

An existing target file is never silently overwritten.

<!-- section-id: product.parts.identity -->
## DocumentId, filename and properties

Every new Part receives a stable `DocumentId`.

Filename and path are not Document identity. You may rename or move the `.ss2part` file inside the Workspace; after `Refresh`, the same DocumentId is discovered at the new path.

`Number`, `Title`, `Description` and `Engineering revision` are durable Document properties and are independent of the filename.

<!-- section-id: product.parts.workbench -->
## Opening Parts and using the Workbench

Use `Open…` on the persistent Project Workspace toolbar to choose a discovered Document. `Open…`, `New Part…` and `Refresh` remain available while another Part is already being edited. Opening a Project by itself does not activate the Part Workbench; an editor appears only after a specific Document is created or opened.

The dialog shows Document kind, name, location and status. You can resize the columns by dragging the header separators; by default, Location receives more space than Name. The current product supplies Part Documents only, but the Open UI is not Part-specific.

Invalid native files and DocumentId conflicts remain visible but cannot be opened as resolved Documents.

Several Parts can remain open at once. The bottom Document Tabs belong to Project Workspace and switch the active Document. Opening a Part that is already open activates the existing tab/session instead of creating a second mutable session.

Use `Workspace` on the top toolbar to return to the Project Workspace Dashboard without closing any open Documents. Their tabs remain available, and clicking a tab returns to that Part. Navigating to Workspace does not Save or change authored state. Closing the last Document also returns to Workspace without closing the Project.

In the active Part Workbench, the left side contains Document Tree. The center contains the 3D Viewport with a tool-launch strip above it. The right side contains Properties and contextual Operations. Status/diagnostic information belongs to the active Workbench.

<!-- section-id: product.parts.edit -->
## Editing Document properties

When no model/reference object is the primary selection, Properties shows the Document fields:

- Number;
- Title;
- Description;
- Engineering revision.

`Apply Properties` changes the open Document session. The change is not durable on disk until you use `Save`.

`Undo` and `Redo` operate on authored changes in the current open session.

<!-- section-id: product.parts.origin -->
## Origin, selection and visibility

Every Part has an `Origin` group in Document Tree containing:

- Origin Point;
- X Axis, Y Axis, Z Axis;
- XY Plane, XZ Plane, YZ Plane.

These references always exist. Hiding one does not delete it.

You can select one or several Origin references in the Tree. Use the context menu `Show` or `Hide` to set visibility for the complete supported selection.

A group visibility change is one Undo/Redo operation.

The Tree and 3D Viewport share one selection. The primary selected Origin reference is shown in Properties with its role, type, semantic identity and current visibility.

Viewport selection behavior is:

- Left click — replace selection;
- Ctrl + Left click — toggle an item in the selection;
- Left click on empty space — clear selection;
- Right click — reserved for context menus; where no menu exists it currently does nothing.

After `Save`, Origin visibility survives closing and restarting the application.

<!-- section-id: product.parts.sketch-host -->
## Creating and viewing a Sketch

Use `Sketch` in the tool strip directly above the 3D Viewport, then select `XY Plane`, `XZ Plane` or `YZ Plane` from Origin. You may select the plane in Document Tree or, when it is visible, in the 3D Viewport.

Operations is not a permanent tool list. It shows controls for the current operation: Sketch support-pick context/Cancel while choosing the plane, then contextual Sketch actions while editing. Outside Sketch edit, the tool strip and Operations return to the Part-modeling context.

After a valid selection, Part creates a durable Sketch and stays in the same Workbench and the same 3D Viewport. The camera automatically aligns normal to the Sketch plane and the grid moves to the Sketch local plane. While Sketch edit is active, the tool strip switches to `Select` and `Line`; Operations shows the current Sketch/tool state and actions; and a compact Command Line appears below the Viewport. The Command Line accepts `SELECT` and `LINE` in this stage.

Any authored Line geometry already stored in that Sketch is presented in the viewport together with an Origin marker for the Sketch's local `(0,0)`.

Automatic alignment does not lock the camera. While the Sketch is active you can still use Pan, Zoom, Orbit and ViewCube. Navigation does not change Sketch placement and does not dirty the Document by itself.

Use `Finish Sketch` to leave the current edit context. The Sketch remains an authored Part object, stays in Document Tree and after `Save` survives Close/Reopen with the same SketchId, support and placement.

To edit an existing Sketch again, double-click it in Document Tree or use its `Edit Sketch` context-menu action. Entering edit by itself does not change authored state and does not require Save.

In Sketch edit, `Select` is the default tool. Left-click a Line to replace the semantic selection, Ctrl+Left-click a Line to toggle it, and drag a rectangle for Window selection left-to-right or Crossing selection right-to-left. Rectangle selection replaces the current selection. `Delete Selection` in Operations, or Delete while the Viewport owns Sketch Select focus, removes the selected Lines as one Undoable operation.

Activate `Line`, click the first point, then click successive points to create continuous segments. The rubber-band preview is temporary; each committed segment is one Undo step. `Finish Line` or `Cancel Line` returns to Select without rolling back already committed segments. Esc cancels the pending Line stage first and then returns Line to Select; it does not finish the Sketch. Undo/Redo first abandons any pending Line stage, then applies normal document history.

Support is currently limited to the three Origin planes. Arc/Circle, grips/direct manipulation, snapping/inference, constraints, dimensions, solver, coordinate entry, Construction/Datum planes and planar model faces remain separate later stages.

<!-- section-id: product.parts.navigation -->
## 3D navigation and Navigation Cube

The Part Workbench provides middle-button Pan, Shift + middle-button Orbit, mouse-wheel Zoom, Fit and Orthographic/Perspective projection.

A 3D Navigation Cube is shown in the upper-right of the 3D Viewport. It follows the current camera orientation. Click a labeled face for Front, Back, Left, Right, Top or Bottom; click an edge for the corresponding two-axis view; click a corner for the corresponding isometric view. Navigation transitions are animated.

When the view is aligned to a face, the controls around the Cube provide exact 90-degree moves to adjacent views and exact 90-degree clockwise/counterclockwise roll. Home returns to Top-Front-Right isometric orientation and performs Fit All.

Projection is independent from Cube orientation. The `ORTHO/PERSP` control next to the Cube switches only between Orthographic and Perspective. If the model is in Perspective, clicking a Cube face, edge, corner or Home keeps Perspective; the same actions keep Orthographic when Orthographic is active.

Navigation and camera changes are temporary view state. They do not dirty the Part, do not require Save and do not create CAD Undo entries.

<!-- section-id: product.parts.save-close -->
## Save and closing

`Save` writes the current authored Part state, including Origin visibility and created Sketches with their durable identity/support/placement and embedded authored Line geometry, to its `.ss2part` file.

When closing a Part with unsaved changes, the application requires `Save`, `Discard` or `Cancel`.

When closing the complete Project or application while any Part is dirty, the choices are `Save All`, `Discard` and `Cancel`.

If saving fails, the Document/Project remains open. Closing the last open Part keeps the Project open and returns to the Project Workspace Dashboard. Using `Workspace` by itself never closes the Part.

<!-- section-id: product.parts.restart -->
## Restart and reopen

After restarting the application, open the same Project.

SimpleSolid scans the Workspace again. The saved Part is rediscovered with the same DocumentId, saved properties, saved Origin visibility and saved Sketch records.

Undo/Redo history, active selection, active Sketch edit context and camera state are not stored in the file and start fresh after reopening.

<!-- section-id: product.parts.conflicts -->
## DocumentId conflicts and invalid files

If two `.ss2part` files in one Workspace contain the same DocumentId, `Open…` shows an identity-conflict entry and the discovered location information.

That entry cannot be opened by DocumentId until the conflict is removed. SimpleSolid does not arbitrarily choose one copy and does not automatically assign a new ID.

A damaged or unsupported native Part is shown as an invalid entry instead of being treated as a valid Document.

<!-- section-id: product.parts.current-limits -->
## Current Part limits

The current Part provides Document identity/properties, built-in Origin, persistent reference visibility, the shared stabilized 3D Workbench/Viewer foundation, durable Sketches with embedded authored Line data and active-Sketch Line/Origin presentation on Origin planes.

Very early test `.ss2part` files created before the current native format are not supported product data and are not migrated automatically.

The current UI provides the first complete authored Sketch Line workflow: contextual Select/Line tools, continuous Line creation with preview, point and Window/Crossing selection, atomic Delete, Undo/Redo integration, Operations and a compact SELECT/LINE Command Line. It still lacks grips/direct manipulation, snapping/inference, coordinate entry, constraints/solver, Sketch support on Datum/planar model faces, Bodies, Features, modeled solid geometry, Material, Assembly or Drawing tools.
