# Part Documents

<!-- doc-id: product.parts -->
<!-- document-kind: product -->

<!-- section-id: product.parts.create -->
## Creating a new Part

After opening a Project, use `New Part…` in the Workspace.

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

<!-- section-id: product.parts.edit -->
## Editing properties

Select a valid Part in the list and choose `Open`.

The properties panel lets you edit:

- Number;
- Title;
- Description;
- Engineering revision.

`Apply Properties` changes the open Document session. The change is not durable on disk until you use `Save`.

`Undo` and `Redo` operate on property changes in the current open session.

<!-- section-id: product.parts.save-close -->
## Save and closing

`Save` writes the current authored Part state to its `.ss2part` file.

When closing a Part with unsaved changes, the application requires `Save`, `Discard` or `Cancel`.

When closing the complete Project or application while any Part is dirty, the choices are `Save All`, `Discard` and `Cancel`.

If saving fails, the Document/Project remains open.

<!-- section-id: product.parts.restart -->
## Restart and reopen

After restarting the application, open the same Project.

SimpleSolid scans the Workspace again. The saved Part is rediscovered with the same DocumentId and saved properties.

Undo/Redo history is not stored in the file and starts empty after reopening.

<!-- section-id: product.parts.conflicts -->
## DocumentId conflicts and invalid files

If two `.ss2part` files in one Workspace contain the same DocumentId, the list shows `Identity conflict` and every discovered path.

That entry cannot be opened by DocumentId until the conflict is removed. SimpleSolid does not arbitrarily choose one copy and does not automatically assign a new ID.

A damaged or unsupported native Part is shown as `Invalid Part` instead of being treated as a valid Document.

<!-- section-id: product.parts.current-limits -->
## Current Part limits

The current Part stores Document identity and common Document Properties.

It does not yet contain Sketch, Bodies, Features, 3D geometry, Viewer, Material or Assembly/Drawing tools.
