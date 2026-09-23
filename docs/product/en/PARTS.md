# Part Documents

<!-- doc-id: product.parts -->
<!-- document-kind: product -->

<!-- section-id: product.parts.create -->
## Creating a new Part

After opening a Project, use `New Part…` in the Workbench.

Enter a path relative to the current Workspace. A native Part file uses the extension:

```text
.ss2part
```

The application proposes `Part001.ss2part`, `Part002.ss2part`, and so on when those names are available.

You may use an existing subfolder, for example `Parts/Shaft.ss2part`. The application does not silently overwrite an existing file.

<!-- section-id: product.parts.identity -->
## DocumentId, filename and properties

Every new Part receives a stable `DocumentId`.

Filename and path are not Document identity. You may rename or move the `.ss2part` file inside the Workspace; after `Refresh`, the same DocumentId is discovered at the new path.

`Number`, `Title`, `Description` and `Engineering revision` are durable Document properties and are independent of the filename.

<!-- section-id: product.parts.workbench -->
## Opening Parts and using the Workbench

Use `Open Part…` to choose a resolved native Part.

Several Parts can remain open at once. Use the bottom Document Tabs to switch the active Part. The same Workbench changes its Tree, Properties and 3D Viewport context to the selected tab.

The left side contains Document Tree. The center contains the 3D Viewport. The right side contains Properties and Operations. Status/diagnostic information is below the tabs.

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

After `Save`, Origin visibility survives closing and restarting the application.

<!-- section-id: product.parts.navigation -->
## 3D navigation and ViewCube

The Part Workbench provides:

- middle-button Pan;
- Shift + middle-button Orbit;
- mouse-wheel Zoom;
- Fit;
- Front, Back, Left, Right, Top and Bottom views;
- Isometric and corner orientations;
- Orthographic / Perspective switching.

Use the ViewCube overlay in the upper-right of the 3D Viewport for standard orientations, Fit and projection switching.

Navigation and camera changes are temporary view state. They do not dirty the Part, do not require Save and do not create CAD Undo entries.

<!-- section-id: product.parts.save-close -->
## Save and closing

`Save` writes the current authored Part state, including Origin visibility, to its `.ss2part` file.

When closing a Part with unsaved changes, the application requires `Save`, `Discard` or `Cancel`.

When closing the complete Project or application while any Part is dirty, the choices are `Save All`, `Discard` and `Cancel`.

If saving fails, the Document/Project remains open.

<!-- section-id: product.parts.restart -->
## Restart and reopen

After restarting the application, open the same Project.

SimpleSolid scans the Workspace again. The saved Part is rediscovered with the same DocumentId, saved properties and saved Origin visibility.

Undo/Redo history, active selection and camera state are not stored in the file and start fresh after reopening.

<!-- section-id: product.parts.conflicts -->
## DocumentId conflicts and invalid files

If two `.ss2part` files in one Workspace contain the same DocumentId, `Open Part…` shows an identity-conflict entry and every discovered path.

That entry cannot be opened by DocumentId until the conflict is removed. SimpleSolid does not arbitrarily choose one copy and does not automatically assign a new ID.

A damaged or unsupported native Part is shown as `Invalid Part` instead of being treated as a valid Document.

<!-- section-id: product.parts.current-limits -->
## Current Part limits

The current Part provides Document identity/properties, built-in Origin, persistent reference visibility and the shared 3D Workbench/Viewer foundation.

It does not yet contain Sketch, Bodies, Features, modeled solid geometry, Material, Assembly or Drawing tools.
