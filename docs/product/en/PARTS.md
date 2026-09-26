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

Use `Sketch` in the tool strip directly above the 3D Viewport, then select `XY Plane`, `XZ Plane` or `YZ Plane` from Origin. You may select the plane in Document Tree or, when visible, in the 3D Viewport.

After a valid selection, Part creates a durable Sketch in the same Workbench/Viewport. The initial camera aligns normal to the Sketch plane and the grid moves to the Sketch local frame, but Pan, Zoom, Orbit and ViewCube remain available and do not dirty the Document.

While Sketch edit is active, the tool strip provides `Select`, `Line`, `Circle` and `Arc`. Operations shows the current tool stage plus contextual actions, and the compact Command Line accepts `SELECT`, `LINE`, `CIRCLE` and `ARC`.

Stored Line, Circle and Arc geometry is shown together with the Sketch-local `(0,0)` Origin marker. Use `Finish Sketch` to leave edit; the authored Sketch remains in the Part and survives Save/Close/Reopen with its SketchId, support, placement and authored entities. Double-click an existing Sketch in Document Tree or use `Edit Sketch` to re-enter edit without changing authored state.

`Select` is additive for all three primitive kinds. Ordinary Left-click adds an unselected entity and makes it primary; clicking an already selected entity keeps membership and makes it primary. Ctrl+Left-click toggles membership. Left-to-right drag is Window selection, right-to-left is Crossing; ordinary rectangles add and Ctrl+rectangle toggles. Blank Left-click or Esc in Select clears selection. `Delete Selection` or Delete removes a mixed selected Line/Circle/Arc set atomically as one Undoable operation.

Every selected editable entity exposes square runtime grips. Idle grips are hollow, hover uses a cyan hollow emphasis, and the active/captured grip is filled yellow and may be slightly larger. Grip size stays screen-space/DPI aware while hit tolerance is independent from visible marker size. A grip wins hit testing over underlying geometry.

Grip behavior is:

- Line: Start/End reshape only that Line; Center moves the complete frozen selection.
- Circle: Center moves the complete frozen selection; four quadrant grips change only the owning Circle radius with center fixed.
- Arc: Center moves the complete frozen selection; Start/End reshape only that Arc while preserving its canonical center/radius and branch rules; Arc/Mid changes only radius while preserving center and start/end directions.

Direct-manipulation preview is runtime-only: it does not dirty the Document, change revision, allocate EntityIds or create Undo history. After grip activation you may release the mouse button, move freely, then use another LMB or Enter to commit the current valid preview. Esc cancels only the preview and keeps selection. A committed mixed Move is one atomic operation and one Undo step; edited entities keep their EntityIds. Undo/Redo first cancels transient interaction, then runs normal Document history.

Creation tools preserve the pre-existing selection but hide/deactivate grips while active, and newly created geometry is not automatically selected.

- `Line`: click a first point and then successive points for continuous segments; each accepted segment is one Undo step.
- `Circle`: click Center, then a Radius point. Zero radius is ignored. Circle remains active for another Circle after a successful commit.
- `Arc`: click Start, Through, then End. The three points define the directed circular path, including CW/CCW and short/long choice. Duplicate/collinear/invalid triples do not commit. Arc remains active for another Arc after success.

Operations exposes Finish/Cancel for the active creation tool. Esc cancels the current incomplete point stage before returning the tool to Select; it does not finish the whole Sketch.

Sketch support is currently limited to the three Origin planes. Snapping/inference, coordinate/dynamic input, constraints/solver, authored dimensions, R7 common transforms/Copy, Construction/Datum planes and planar model faces remain later stages.

<!-- section-id: product.parts.navigation -->
## 3D navigation and Navigation Cube

The Part Workbench provides middle-button Pan, Shift + middle-button Orbit, mouse-wheel Zoom, Fit and Orthographic/Perspective projection.

A 3D Navigation Cube is shown in the upper-right of the 3D Viewport. It follows the current camera orientation. Click a labeled face for Front, Back, Left, Right, Top or Bottom; click an edge for the corresponding two-axis view; click a corner for the corresponding isometric view. Navigation transitions are animated.

When the view is aligned to a face, the controls around the Cube provide exact 90-degree moves to adjacent views and exact 90-degree clockwise/counterclockwise roll. Home returns to Top-Front-Right isometric orientation and performs Fit All.

Projection is independent from Cube orientation. The `ORTHO/PERSP` control next to the Cube switches only between Orthographic and Perspective. If the model is in Perspective, clicking a Cube face, edge, corner or Home keeps Perspective; the same actions keep Orthographic when Orthographic is active.

Navigation and camera changes are temporary view state. They do not dirty the Part, do not require Save and do not create CAD Undo entries.

<!-- section-id: product.parts.save-close -->
## Save and closing

`Save` writes the current authored Part state, including Origin visibility and created Sketches with their durable identity/support/placement plus embedded Line/Circle/Arc geometry and stable EntityIds, to its `.ss2part` file.

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

The current Part provides Document identity/properties, built-in Origin, persistent reference visibility, the stabilized 3D Workbench/Viewer foundation and durable Origin-plane Sketches with Line/Circle/Arc authored geometry.

The current Sketch UI provides Select/Line/Circle/Arc creation, additive point/Window/Crossing selection, semantic primary and hover, state-based square grips, mixed-selection Move, bounded owner-only reshape, atomic mixed Delete, Undo/Redo, Operations and the compact Command Line.

Very early test `.ss2part` files from before the current native format are not supported product data and are not migrated automatically.

The product still lacks snapping/inference, coordinate/Dynamic Input, constraints/solver, authored dimensions, R7 common transforms/Copy, Sketch support on Datum/planar model faces, Bodies, Features, modeled solid geometry, Material, Assembly and Drawing tools.
