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

While Sketch edit is active, the tool strip provides `Select`, `Line`, `Circle`, `Arc` and `Move`. The compact Command Line accepts `SELECT`, `LINE`, `CIRCLE`, `ARC` and `MOVE`.

Line/Circle/Arc creation and selection retain the existing behavior. Selected editable entities show state-based square grips: hollow idle, cyan hollow hover and filled yellow active/captured. Center grips move the complete frozen mixed selection; owner-only reshape grips keep their existing primitive-specific behavior.

### Move

`Move` works on any mixed selection of Line, Circle and Arc.

With objects already selected, activate `Move` and immediately specify a **Base Point**, then specify a destination point. The whole frozen selection previews one rigid translation equal to `destination - base`.

With nothing selected, activate `Move` first. The command enters **Select objects**:

- click objects to add them;
- Ctrl+click toggles membership;
- left-to-right drag uses Window selection;
- right-to-left drag uses Crossing selection;
- Ctrl+rectangle toggles hits;
- blank LMB does nothing;
- Enter, Space or RMB finishes object collection when at least one object is selected.

Then choose Base Point and destination. LMB or Enter accepts a valid destination. Esc cancels the current MOVE and preserves the affected selection. A zero-distance Move is a clean no-op: it creates no authored change and no Undo step.

MOVE preview is runtime-only. A non-zero commit is one atomic operation and one Undo step, preserves EntityIds, and works through normal Undo/Redo. Save/Close/Reopen preserves the moved geometry through the existing Part format.

Space typed while a text-entry field has focus remains text input; it does not trigger a CAD action.

Creation tools preserve pre-existing selection but hide/deactivate grips while active, and newly created geometry is not automatically selected. Use `Finish Sketch` to leave edit. Sketch support is currently limited to the three Origin planes.

Rotate, Scale, Mirror, Copy/repeated Copy, Repeat Last Command, snapping/inference, coordinate/Dynamic Input, constraints/solver, authored dimensions, Construction/Datum planes and planar model faces remain later stages.

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

The current Sketch UI provides Select/Line/Circle/Arc creation, additive point/Window/Crossing selection, semantic primary and hover, state-based square grips, normal mixed-entity MOVE with selection-first or command-first Base Point workflow, Center-grip Move, bounded owner-only reshape, atomic mixed Delete, Undo/Redo, Operations and the compact Command Line.

Very early test `.ss2part` files from before the current native format are not supported product data and are not migrated automatically.

The product still lacks Rotate/Scale/Mirror/Copy, repeated Copy, Repeat Last Command, snapping/inference, coordinate/Dynamic Input, constraints/solver, authored dimensions, Sketch support on Datum/planar model faces, Bodies, Features, modeled solid geometry, Material, Assembly and Drawing tools.
