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

In the active Part Workbench, one top row compositionally aligns the current Document/context tools on the left with `Undo`, `Redo`, `Save` and `Close` on the right. Below it, the left side contains Document Tree, the center contains the 3D Viewport, and the right side contains Properties and contextual Operations. Status/diagnostic information belongs to the active Workbench.

At normal width the side panels start with narrow proportions (about 150 px for Tree and 270 px for Properties/Operations), while additional space primarily goes to the Viewport. Smaller widths pass through a compact layout. In a narrow window the side panels collapse in a controlled way; `Tree` and `Panel` controls in the top row deliberately recover the side you need. Resizing or collapsing a panel does not change geometry, selection, Undo/Redo or the active Sketch/Profile context.

<!-- section-id: product.parts.edit -->
## Editing Document properties

When no model/reference object is the primary selection, Properties shows the Document fields:

- Number;
- Title;
- Description;
- Engineering revision;
- Input/display length unit: mm, cm, m, in or ft.

Changing the Part unit changes how unitless Length input is interpreted and how physical values are displayed. Existing geometry is not rescaled. The unit is authored Document state, participates in Undo/Redo, makes the Part dirty when changed and becomes durable after `Save`. Older native Parts open as mm.

`Apply Properties` changes the other Document fields in the open session. Those changes are not durable on disk until you use `Save`.

`Undo` and `Redo` operate on authored changes in the current open session. On Windows, `Ctrl+Z` invokes the same active-Document Undo path and `Ctrl+Y` invokes Redo. A focused text editor keeps its own local Undo/Redo.

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

<!-- section-id: product.parts.datum-plane -->
## Datum reference plane

The current Part provides one construction-reference object: **Datum Plane** with the **Offset** constructor. Its source can be an Origin XY/XZ/YZ plane, a planar Face of the current Body, or an earlier Datum Plane. A clicked planar Face is persisted as a semantic Body surface at an explicit history stage, not as a provider Face handle.

There is one tool regardless of entry path. You can select an admissible source first and start **Datum Plane**, or start the tool with no source and select one afterwards. The right **Operations** panel shows `Constructor = Offset`, `Source`, signed `Offset`, **Reverse**, **Finish Datum Plane** and **Cancel**. A new draft starts at **10 mm**. Reverse does not persist a second direction flag; it negates the same signed Offset.

After an admissible source is acquired, the active draft shows a finite plane outline/border in the Viewport. If the current Body intersects the plane, preview also shows a virtual intersection line. The current build does not render the neutral translucent plane fill; that presentation-only polish is deferred so Origin and Datum planes can be styled consistently. Draft preview is presentation-only: it has no `DatumId`, is not a pickable reference and clears on invalid input, **Cancel**, **Finish** or context exit. It creates no Undo step and no authored state.

After **Finish**, a durable Datum Plane with stable `DatumId` exists. Its finite footprint and border remain presentation, not authored size for the infinite plane. The virtual Body intersection is also presentation-only. Clicking that line selects the owning Datum Plane; it never becomes Edge/Curve meaning or Projection geometry.

In Document Tree, **Reference Geometry** sits directly below `Origin`. Each Datum Plane has its own authored Show/Hide state, Properties shows identity, source, Offset, status and diagnostics, and **Edit Datum Plane** uses the same draft while preserving `DatumId`. Showing or hiding the complete group bulk-updates child authored visibility in one Undo/Redo operation; the group has no second persisted visibility flag. Delete is rejected atomically while another Datum Plane or Sketch still depends on the target Datum.

GUI and Command Line drive the same draft. `DATUM PLANE` or `DATUMPLANE` starts the tool, while the active context accepts signed Offset/Length input plus `REVERSE`, `FINISH` and `CANCEL`. A Missing, Ambiguous, Unsupported, Blocked or otherwise invalid source publishes no stale last-good frame and cannot authorize downstream modeling.

<!-- section-id: product.parts.sketch-host -->
## Creating and viewing a Sketch

Use `Sketch` in the expanding left side of the active-Document top row, then select either `XY Plane`, `XZ Plane` or `YZ Plane` from Origin, a current planar Body Face, or an existing Datum Plane. A planar semantic Face is accepted as standard Sketch support; a non-planar Face remains selectable but reports **Unsupported** instead of falling through to another target. A Datum-backed Sketch persists only the support `DatumId`; authored local U/V geometry remains independent from the Datum's current world placement. Selecting the support is still a runtime draft: **Create Sketch** performs the single authored Create transaction and **Cancel** changes nothing. After the Sketch exists and Edit is active, the same action area becomes **Finish Sketch**.

During Sketch Edit the tools are grouped as:

```text
Select

Create
  Line
  Circle
  Arc
  Rectangle

Modify
  Move
  Copy
  Rotate
  Scale
  Mirror
  Project Geometry

Inspect
  Measure
```

Command Line is a shared Workspace surface and no longer requires a click before ordinary CAD entry. While the viewport or another normal CAD surface has focus, you can start typing immediately: the characters appear in Command Line while focus remains in the viewport. Enter submits the token to the active context. Clicking Command Line still works and edits the very same live buffer.

Inside an active Sketch, the implemented command words are `SELECT`, `LINE`, `CIRCLE`, `ARC`, `RECTANGLE`, `MEASURE`, `MOVE`, `COPY`, `ROTATE`, `SCALE` and `MIRROR`. Command Line is context-sensitive: when the active stage expects a point, the current semantic PointRequest has precedence over starting a new command. At Direct Distance stages, a bare numeric value is interpreted as a distance. During active grip manipulation, `C` + Enter is a local keyword that enables Grip Copy; outside an active grip it is not a global command alias. Enter consumes the submitted token even when it is invalid: the model and active tool/stage stay unchanged, the editable field is cleared and a diagnostic reports the error instead of forcing you to delete the rejected text manually.

Real text-entry fields — for example property editors — keep keyboard ownership and do not feed Command Line. Application shortcuts using Ctrl/Alt/Meta are also not converted into CAD text. Menus, popups, modal dialogs, another application window and another visible SS2 Workspace keep their own keyboard ownership; they do not feed a background Command Line.

A partially typed CAD token belongs to the semantic request in which typing started. Changing to another request/tool/Sketch/Document clears that partial token before it can execute in the new context. Moving the pointer to adjust direction inside the same PointRequest does not clear it. Switching Documents or navigating to Workspace likewise clears partial input so it can never leak into a hidden Document.

With viewport CAD focus and a non-empty Command Line buffer, Delete is reserved by live input and does not delete selected geometry. Use Backspace to remove the last character. With an empty buffer during active Sketch Edit, Delete belongs to the Sketch semantic context: selected Sketch geometry is deleted, while a stale Profile/Tree selection cannot take over the command. If no Sketch geometry is selected, Delete does nothing rather than deleting a stale Profile. Outside Sketch Edit, normal Part/Profile Delete behavior remains available. Clicking blank space in Document Tree clears the Tree/Part selection only. If the Command Line or another real text field itself has focus, Delete behaves as normal text editing.

The bottom Command Line stays one row high. Its right-side diagnostic area is permanently reserved, so showing an error does not resize the input field or make the Viewer jump. Long diagnostics are shortened visually in that row; the complete text is available as a tooltip.

### Repeat Last Command

During one active Sketch Edit session, SimpleSolid remembers the last successfully activated command from `Line / Circle / Arc / Rectangle / Move / Copy / Rotate / Scale / Mirror`.

While ordinary `Select` is active and the viewport has focus, **Enter** or **Space** starts that command again.

Repeat starts a fresh command invocation. It does not replay earlier Base/Reference/axis/placement points, preview state or a previous selection snapshot. It uses the current selection through the command's normal entry grammar — for example, MOVE with a current selection is selection-first, while MOVE with no selection enters Select objects. Repeated COPY asks for a new Base Point and does not reuse prior placements.

`Select`, ordinary selection changes, Delete, grip manipulation, Undo/Redo and Esc do not replace the remembered command. The memory is cleared when Sketch Edit ends and does not carry into another Sketch, Document or reopened file.

Existing key meaning keeps priority: Enter still commits active manipulation/transforms, Enter/Space still completes Select objects during an active transform, and Space in text-entry focus remains a literal text space.


### Rectangle and creation role

**Rectangle** uses two clicks: **First Corner**, then **Opposite Corner**. The rectangle is axis-aligned to the active Sketch U/V axes. Both U and V extents must be non-zero. A successful placement creates four ordinary Line entities in one atomic operation and one Undo step, then Rectangle stays active for another placement.

Rectangle is not a persistent special primitive. After creation its four perimeter Lines are independent ordinary Lines with independent EntityIds. Editing one edge can open or distort the rectangle; SimpleSolid does not add hidden Coincident, Horizontal/Vertical or rectangularity constraints.

The right **Operations** panel shows a checkable **Construction** option whenever Line, Circle, Arc or Rectangle is active. It sets the role of geometry created **from now on**. Turning it on or off does not modify already selected geometry and creates no Undo step. During Rectangle, the same Operations panel also shows **Draw Diagonals**; that option is hidden for the other creation tools. To change existing geometry, use the separate Regular/Construction controls that Operations shows during ordinary Select. Creation Role starts as Regular for each new Sketch Edit session and is not persisted.

**Draw Diagonals** is a Rectangle-only runtime option and starts OFF for each new Sketch Edit session. When ON, the same Rectangle commit adds both diagonals as ordinary **Construction** Lines. A Regular rectangle therefore creates four Regular perimeter Lines plus two Construction diagonals; a Construction rectangle creates six Construction Lines. The intersection does not create a center point, Rectangle identity, group or constraint.

Rectangle preview follows the same perimeter/diagonal semantics. Construction preview is dashed as a visual cue only. The final commit is bound to the Document revision captured at First Corner; if the Document changes before Opposite Corner, the pending rectangle fails closed instead of being silently rebased.

At Opposite Corner, precision input also accepts `Width;Height`. Width and Height are positive Length values; the current pointer quadrant supplies left/right and up/down orientation. This Rectangle-specific grammar is not reinterpreted as generic `U;V`.

Line/Circle/Arc creation and ordinary selection retain the existing behavior. Selected editable entities show state-based square grips: hollow idle, cyan hollow hover and filled yellow active/captured.

### Grip edit modes

Center grips cycle:

```text
Move → Rotate → Scale → Mirror → Move
```

Supported non-center Line/Circle/Arc grips start in owner-only **Reshape** and cycle:

```text
Reshape → Move → Rotate → Scale → Mirror → Reshape
```

Move/Rotate/Scale/Mirror operate on the complete selection frozen when manipulation starts; Reshape edits only the active-grip owner. Every preview is recomputed from interaction-start geometry. Rotate captures a fresh reference direction, Scale a fresh reference radius, and Mirror uses the active grip as the first axis point.

You can type exact values in the numeric modes: Angle for Rotate, a positive Factor for Scale and Axis Angle for Mirror. Once accepted, the exact value outranks further pointer motion.

Space cycles the mode and creates no authored change. LMB or Enter commits; Esc cancels the uncommitted manipulation. Grip Copy is available only in Reshape and Move and turns OFF when cycling into another mode.

### Grip Copy

While a grip manipulation is active, type `C` and press Enter to enable **Grip Copy**. Copy is a commit modifier available only in Reshape and Move.

In **Reshape + Copy**, each accepted placement creates one fresh-ID copy of only the active-grip owner. In **Move + Copy**, each placement duplicates the complete frozen selection. Originals remain unchanged and selected.

Placement can use LMB or the same precision PointRequest. After a successful copy, the interaction-start source/pivot are retained for another placement, but request-local pointer candidates and numeric locks are cleared. An exact no-change placement is a no-op. Space cycles mode and turns Copy OFF; it must be enabled again after returning to Reshape or Move. Esc ends the transient session while keeping committed copies; Undo then affects the latest committed history step normally.

### Selecting objects for Modify

Move, Copy, Rotate, Scale and Mirror work on any mixed selection of Line, Circle and Arc.

If objects are already selected, activate the Modify tool. The current selection is frozen immediately and the command advances to its first reference-point stage.

If nothing is selected, activate the tool first. The command enters **Select objects**:

- click objects to add them;
- Ctrl+click toggles membership;
- left-to-right drag uses Window selection;
- right-to-left drag uses Crossing selection;
- Ctrl+rectangle toggles hits;
- blank LMB does nothing;
- Enter, Space or RMB finishes object collection when at least one object is selected.

### Move

Specify a **Base Point**, then a destination. The complete frozen selection previews one rigid translation equal to `destination - base`.

You can specify the destination with LMB or Direct Distance: after Base Point, point the cursor in a direction, type a distance in Command Line and press Enter. A zero-distance Move is a clean no-op: it creates no authored change and no Undo step.

### Copy

COPY uses the same object-selection grammar as the other Modify tools. After the source selection is frozen, specify a **Base Point**, then a placement point. The placement can be clicked with LMB or resolved through Direct Distance using the current cursor direction.

Preview shows the original source selection translated by `placement - base`. On each valid non-zero placement:

- the original Line/Circle/Arc entities remain unchanged and remain selected;
- every new copied entity receives a fresh non-aliasing EntityId;
- one placement is one atomic authored change and one Undo step;
- COPY remains active so additional placement points can be accepted without reselecting objects or redefining Base Point.

Every repeated placement is derived from the same original source snapshot and Base Point, never from the previously created copy.

A placement exactly at Base Point does not create an invisible coincident copy. It is a clean no-op: no entities are created, no EntityIds are consumed and no Undo step is added.

Esc ends the active COPY session while preserving the source selection. Copies already committed during that session remain in the Sketch. Undo/Redo acts on individual committed placements; Redo restores the same copied EntityIds, and a new COPY after Undo does not reuse IDs that belonged to a previously committed and undone copy.

Undo does not rewind the session-local identity high-water, so a new COPY in the same session does not reuse IDs from the undone copy. If Undo restores exactly the last saved authored state, the Document remains clean as before. When a later committed copy is saved normally, the existing Part format also persists the current `next_entity_id`.

### Rotate

Specify a **Base Point**, a **Reference Point**, then a destination that defines the final direction.

Reference Point must differ from Base Point. Rotation is the signed angle from the Base→Reference vector to the Base→destination vector in the Sketch plane; positive rotation is counter-clockwise.

A zero-angle Rotate is a valid no-op and creates no authored change or Undo step.

### Scale

Specify a **Base Point**, a **Reference Point**, then a destination.

Scale is derived from distances to Base Point:

```text
factor = |destination - base| / |reference - base|
```

The factor must be finite and strictly greater than zero. Values between `0` and `1` reduce geometry, `1` is a no-op, and values greater than `1` enlarge it. Zero and negative values are not valid Scale factors.

### Mirror

Specify the first and second axis points. Two distinct points define an infinite reflection line.

Mirror reflects the complete frozen selection. Line/Circle/Arc preserve their EntityIds, and Arc direction remains geometrically consistent with the reflected directed arc. If the complete geometry is exactly unchanged by the chosen axis, the operation completes as a no-op.

### Commit and precision input

Move/Copy/Rotate/Scale/Mirror preview is runtime-only. Move/Rotate/Scale/Mirror preserve EntityIds; COPY creates fresh EntityIds only for accepted placements. LMB at the final stage or Enter commits the current valid request.

The shared precision grammar accepts:

```text
Absolute point        U;V
Relative Cartesian    @dU;dV
Relative polar        @Distance<Angle
```

Bare Length uses the current Part unit. Explicit `mm`, `cm`, `m`, `in` or `ft` overrides it. Decimal dot is always accepted and the current decimal comma is accepted where applicable. Bounded arithmetic supports `+`, `-`, `*`, `/` and parentheses with dimensional validation. Bare Angle is degrees; `deg` and `rad` are explicit angle suffixes. Feet/inches quote notation is rejected.

Line uses exact point input for the first and later points. Circle is **Center → Size**; Size defaults to **Diameter** for each Sketch Edit and local `D`/`R` switch Diameter/Radius. Arc is **Start → End → Arc Point / Radius**. Radius must be at least half the chord; pointer side chooses the bulge, Radius creates the minor/semicircle result, and an explicit third Arc Point can create a major arc. Rectangle second stage accepts `Width;Height`.

Rotate accepts exact signed **Angle** and Scale exact positive **Factor**. Grip Rotate/Scale/Mirror expose **Angle**, **Factor** and **Axis Angle**.

### Polar and Dynamic Input

**Polar** is attraction, not snapping. It starts ON at 45° (`360/8`), Reference **Absolute**, with no Additional Angles. Operations configures Step, Absolute/Relative and optional single Additional Angles. Relative requires a valid semantic reference and never silently falls back to Absolute. **F10** toggles Polar. Its settings survive Sketch close/re-entry during the current application session and reset at application restart.

**Dynamic Input (DYN)** starts OFF and is toggled by **F12** or Operations. When ON, the cursor overlay exposes fields of the same active request and uses the same live token as Command Line. Based-point order is **Distance → Angle → dU → dV**; unbased points use **U → V**; Rectangle uses **Width → Height**; Circle exposes Diameter/Radius; Rotate/Scale/grip Mirror expose their single semantic values.

**Tab** locks a valid current field and advances; empty Tab advances without locking; **Shift+Tab** moves backward. **Enter** accepts the request from locks plus remaining pointer/Polar values. **Esc** clears live text first, then request-local locks, then follows ordinary tool cancellation. Locked Angle is absolute from Sketch +U and outranks Polar; locked Distance may combine with captured Polar direction. Conflicting lock families fail closed.

Polar/DYN create no Document revision, dirty state or Undo step and are not saved in the Part.

Space typed while a text-entry field has focus remains text input; it does not trigger a CAD action.

Creation tools preserve pre-existing selection but hide/deactivate grips while active, and newly created geometry is not automatically selected. Use `Finish Sketch` to leave edit.

An existing Sketch can use **Change Sketch Support** from Document Tree or `RESUPPORT` in Command Line. Select a new Origin plane, planar Body Face or Datum Plane, then **Apply Support** to commit exactly one support change. Re-support preserves SketchId, EntityIds and authored local U/V geometry; it changes the host mapping. Missing, Ambiguous, Unsupported, Blocked or cycle-causing targets are rejected without partial mutation. Editing an upstream Datum source/Offset recomputes the Sketch world frame without rewriting its local geometry. Undo/Redo restores the previous semantic support intent.

Copy combined with Rotate/Scale/Mirror, ordinary-Select RMB context, clipboard/cross-Sketch Copy, Grid Snap, constraints/solver and authored dimensions remain later stages. There is no separate Ortho mode; use Polar with a 90° step for orthogonal-only attraction.

<!-- section-id: product.parts.project-geometry -->
## Project Geometry — projecting Body edges into a Sketch

While **Sketch Edit** is active, choose **Project Geometry** from the **Modify** toolbar group or enter `PROJECT` in the global **Command Line**. This is one tool with one shared state; the options appear in the right-hand **Operations** panel. It creates associative projections of **material Edges** from the current Body at an earlier stage of the same Part onto the editing Sketch plane. It does not project a planar Face or automatically capture a whole face boundary.

Operations shows the source stage, selected Edge count, **Regular** or **Construction** (Regular by default), **Remove**, **Clear**, **Finish Project Geometry** and **Cancel**. Click the first real Body edge in the Viewport; use **Ctrl + Left click** to add further edges to the same selection. Valid edges enter a temporary selection, and their current projection must resolve into a live preview before Finish becomes enabled. Preview style reflects the selected role. Unsupported edges, wrong-stage edges and unavailable current sources do not produce a partial committed result.

Choose **Finish Project Geometry**, enter `FINISH`, or press **Enter** with the Viewport focused to create the linked Sketch entities in one Undo step. An empty or invalid Finish does not modify the Part. **Remove** drops the primary staged Edge, **Clear** removes all staged Edges, and **Cancel** exits with no authored changes. When Edge sources are staged, the first **Esc** clears the source selection and the second returns to **Select**. If Command Line contains unfinished text, the first Esc only clears that text. Typed `CANCEL` and the Cancel button always exit immediately.

While the tool is active, Command Line accepts `REGULAR`, `CONSTRUCTION`, `REMOVE`, `CLEAR`, `FINISH` and `CANCEL`. The `PROJECTGEOMETRY` start alias is also supported. Command Line and the right-hand buttons operate on the same staged selection. A rejected keyword leaves the tool active and reports a diagnostic. Switching Documents, ending Sketch Edit, or issuing Undo/Redo during staging clears the selection and preview.

After Finish, a projected **Line, Circle or Arc** has a distinct linked visual style, independent of its Regular/Construction role. **Regular** geometry can participate in a valid Profile; **Construction** geometry remains reference-only. Selecting one linked entity in Sketch Select shows its source Feature and current status in the right panel. **Break Link** preserves the entity's current geometry and Sketch identity while removing its source association; it requires a successfully resolved current source. Break Link and Delete are Undo/Redo-capable model operations. When upstream source geometry changes, the projected display derives from the current Body; an unavailable or unsupported source never revives a previously saved curve as a valid linked source.

**Scope:** Only exact Line, Circle and Arc projections of referenceable material Edges from an earlier Body stage are supported. Representation-partition edges, degenerate projections and curves needing approximation are not silently converted to polylines. This is not a face-boundary capture tool, cross-Part projection or automatic constraint creation.

<!-- section-id: product.parts.structural-editing -->
## Structural editing — Trim and Extend

During Sketch Edit, **Modify** provides **Trim**, **Extend** and **Extend Both**. You can also type `TRIM`, `EXTEND` or `EXTEND BOTH` in Command Line.

### Trim

The default workflow is tool-first: start **Trim**, click one or more finite Line/Arc/Circle entities to collect the cutting boundaries, then press **Enter** or **RMB** to accept that boundary set. After confirmation, click target fragments to remove them; Trim remains active for repeated target clicks.

Preselection remains a shortcut: if valid boundaries are already selected before **Trim** starts, they are accepted immediately and the tool opens directly in the target-fragment stage. Regular and Construction geometry can both be targets and boundaries.

Trim is intentionally one-result only:

- a terminal span of a **Line** may be removed, leaving one Line;
- a terminal span of an **Arc** may be removed, leaving one Arc;
- a **Circle** may be trimmed when the selected finite boundaries provide at least two distinct exact cut locations. The clicked local Circle span is removed and the connected complement becomes one Arc.

A middle Line/Arc removal that would leave two pieces is rejected. Split and Join are not part of this tool set. Tangent Circle Trim with only one cut, coincident/overlapping ambiguity or another result that cannot be represented as one supported primitive is rejected without changing the Sketch.

Circle→Arc is a real identity change: the old Circle is retired and the new Arc receives a fresh identity. Existing Profiles are **not** automatically rebound to that new Arc. A Profile that depended on the retired Circle can become Invalid until Undo restores the original geometry.

### Extend

The default workflow is tool-first: start **Extend**, click one or more finite Line/Arc/Circle entities to collect finite boundaries, then press **Enter** or **RMB** to accept them. After confirmation, click near the end of each target Line or Arc that should continue; Extend remains active for repeated targets.

Preselection remains a shortcut: valid boundaries selected before **Extend** starts are accepted immediately, so the tool opens directly in the target-end stage. The selected target end extends to the nearest valid exact intersection in that continuation direction.

Standard Extend uses only the **finite authored boundary geometry**. It does not use a boundary's invisible virtual continuation. Circle is not an Extend target.

### Extend Both

**Extend Both** is only for exactly two Lines. Start the tool, choose the first Line and then the second. When both finite Lines need extension, their infinite supporting lines are used to find one common virtual intersection and both Lines extend to it atomically.

Parallel or coincident Lines, cases where the intersection is already on either finite Line, and cases where only one Line needs extension are rejected. Use ordinary Extend when only one target needs to reach an existing finite boundary.

### Preview, history and failure behavior

Before commit, the viewport previews the proposed removed span or added continuation. The preview is runtime-only and is not selectable geometry.

Each accepted Trim or Extend creates one Undo step. Extend Both changes both Lines in one atomic Undo step. **Esc** cancels only the transient tool state; edits already committed remain normal history. Undo/Redo preserves the accepted entity identities, and Save/Close/Reopen preserves the resulting geometry and identities.

Structural truth is exact Sketch geometry. Object Snap, Tracking, Polar and Dynamic Input may help acquire input, but they do not create constraints, weld small gaps or make a near miss count as an intersection. Ambiguous, unsupported, stale or non-finite cases fail closed without a partial edit.

<!-- section-id: product.parts.object-snap -->
## Object Snap, Tracking and temporary overrides

During Sketch Edit, **Operations** exposes **Object Snap**, **Modes**, **Temporary** and **Tracking** alongside Polar and Dynamic Input.

Object Snap is exact semantic snapping. It does not move existing geometry and never creates an automatic Coincident, Tangent, Perpendicular or other authored constraint. The current snap is shown by a screen-space marker/label.

Persistent modes are:

- **END** — Line/Arc endpoints;
- **MID** — Line midpoint and Arc midpoint along the authored sweep;
- **CEN** — Circle/Arc center;
- **QUAD** — Circle ±U/±V quadrants and only Arc quadrants on the authored sweep;
- **INT** — exact finite discrete intersections of nearby Line/Circle/Arc geometry;
- **ORG** — intrinsic Sketch Origin `(0,0)`;
- **PER** — request-relative perpendicular result when the active point request has the required base;
- **TAN** — request-relative tangent to Circle/Arc and the deferred/common-tangent Line flow;
- **NEA** — continuous nearest projection on the finite source curve; it is a fallback below more specific eligible snaps;
- **EXT** — explicit positive Line-extension ray beyond an acquired endpoint; it does not apply on the finite Line segment.

Defaults are **END/MID/CEN/QUAD/INT/ORG ON**, **PER/TAN/NEA/EXT OFF**. The Object Snap master, persistent mode set and Tracking master are application/user preferences that survive application restart. They are not saved in the Part and create no Document dirty state or Undo step.

The **Temporary** selector applies exactly one override to the next point request: `END`, `MID`, `CEN`, `QUAD`, `INT`, `PER`, `TAN`, `NEA`, `ORG`, `EXT` or `NONE`. It clears after one accepted point, Esc, request/tool replacement or Sketch exit without changing persistent modes. `NONE` suppresses object-derived OSNAP, OTRACK and EXT/TAN/PER assistance for that point, while explicit numeric input, numeric locks, generic base U/V inference, Polar and the raw pointer remain available. An unavailable requested family fails closed instead of falling back to another snap.

Object Snap uses the same final point-resolution path as Dynamic Input and Polar. Complete explicit numeric input has highest priority. Numeric locks remain authoritative; a snap may assist a partially locked request only when the locked result remains exactly the advertised snap point. Compatible OSNAP then outranks Tracking/inference, which outranks Polar, which outranks the raw pointer.

**Tracking** (OTRACK) starts OFF. When enabled, pause for about **0.4 s** over an eligible exact `END/MID/CEN/QUAD/INT/ORG` snap to acquire a tracking anchor. At most two anchors are retained; a third is not silently substituted. Move away and deliberately dwell over the same acquired snap again to remove that anchor. Accepted points, Esc, request/tool replacement and Sketch exit clear request-local anchors.

Each anchor can expose exact Sketch U/V tracking guides. When Polar is ON, its accepted directions may also participate. A two-anchor guide intersection outranks a single compatible guide projection and Polar. Tracking markers and guides are runtime-only, non-selectable and non-authored. Tracking OFF or `NONE` makes object-derived guides dormant without changing saved preferences.

There is currently no dedicated F3/F11 binding for Object Snap or Tracking; use the visible Operations controls. Polar remains **F10** and Dynamic Input remains **F12**.
<!-- section-id: product.parts.measure -->
## Inspect — Measure

Use **Inspect → Measure** while editing a Sketch to inspect geometry without changing the model. Measure has two read-only modes: default whole-entity inspection and relational **Between**.

### Quick Measure

If exactly one Line, Circle or Arc is selected when Measure starts, it becomes the initial target. With no selection, click an entity. With several selected entities, Measure does not guess. The Measure target is separate from ordinary selection.

Operations shows Line Length/Delta U/Delta V/Angle +U, Circle Radius/Diameter/Circumference/Area and Arc Radius/Start Angle/End Angle/Signed Sweep/Arc Length. Regular and Construction geometry are both measurable. You may also type `MEASURE` in Command Line.

### Measure Between

While Measure is active, click **Between** in Operations or type `BETWEEN`. Between asks for **Target A** and **Target B** while preserving ordinary Sketch selection.

Semantic point markers are normally hidden. Move the pointer near a supported point and its runtime marker appears. Moving away hides an unaccepted marker. Pointer proximity never selects, snaps or moves the CAD pointer: **proximity reveals; it does not acquire**. Click a visible marker explicitly. Accepted point markers remain pinned until the relation is replaced or cleared.

Available point roles are:

- **Line:** Start, Midpoint, End;
- **Circle:** Center and four ±U/±V Quadrants;
- **Arc:** Center, Start, End, Midpoint along the authored sweep.

A **Line body** can also be a target. A whole Circle or Arc body cannot; choose one of its semantic point markers.

Between supports exactly:

- **Point ↔ Point:** Distance, Delta U, Delta V and directed Angle +U from A to B;
- **Point ↔ Line:** perpendicular distance to the Line's **infinite supporting line**;
- **Line ↔ Line:** smaller undirected angle from 0° to 90°.

For Point↔Line, the perpendicular foot is not clamped to the finite segment. When it lies beyond Start/End, the viewport shows a distinct supporting-line continuation. Point↔Point shows one temporary connecting segment. Line↔Line highlights both Lines but does not draw a classical angular dimension arc or text.

Overlapping markers at different semantic coordinates fail closed rather than using nearest-wins or candidate cycling. Same-coordinate markers are measurement-equivalent. This is not OSNAP and creates no constraints.

After a result, the next accepted target starts the next relation. Blank click clears the current relational state but stays in Between. **Esc** once returns to ordinary Measure; **Esc** again returns to Select.

Measure and Between are read-only: no dirty state, revision change, identity allocation or Undo step. Runtime measurement references are not persisted and Measure is not Repeat Last Command.

Linear Measure values are shown in the current Part length unit and Circle area uses the squared current unit; angles remain degrees. Changing the Part unit updates presentation without changing measured geometry. General curve-to-curve minimum distance, OSNAP/tracking/inference, persistent sub-element references, authored dimensions and constraints remain outside the current Measure surface. Broader viewport dimension display is deferred and will be reconsidered with future authored/parametric dimensions and constraints rather than as a separate R8C overlay subsystem.

<!-- section-id: product.parts.profiles -->
## Construction and Profile

Every Sketch Line, Circle and Arc can be **Regular** or **Construction**. Select geometry in ordinary Select and use `Regular` or `Construction` in Operations. Construction remains saved helper geometry, but it does not close or split regions used by Profile. The viewport presents Construction geometry with a dashed line; this is only a presentation cue, while the authored entity role remains semantic truth. Dash/gap cadence is screen-space presentation: short and long Construction lines use the same visual cadence, zoom does not rescale authored geometry, and preview/committed Construction use the same policy. A Regular Rectangle participates in Profile analysis only through its four Regular perimeter Lines; optional Construction diagonals do not split the material region. A Construction Rectangle contributes no material boundary.

The **Profile** tool works inside the active Sketch. Moving the pointer over closed geometry shows a translucent region result; clicking accepts that candidate into the current draft and Status reminds you that **Finish Profile** performs the durable commit. Region truth comes from exact Line/Circle/Arc semantics, not from Viewer tessellation.

Operations provides **Add Area**, **Subtract Area**, **Show Islands**, **Highlight on Hover**, **Show Region Boundaries**, **Show Problems**, **Finish Profile** and **Cancel**. Island detection itself is always active because it is part of the region analysis. **Show Islands** only shows or hides the island diagnostic/presentation; turning it off does not change Profile geometry, validity or picking. The former **Find All Regions** button is no longer part of the normal Profile workflow. During Subtract the resulting draft remains cyan/blue, while the currently hovered region to remove receives a separate red-orange emphasis fill.

An open chain remains open: SimpleSolid does not close a small gap with a hidden tolerance or auto-repair it. Clicking where no bounded region exists reports an open-boundary diagnostic when the analysis detects one. Without snapping/OSNAP, two points that only look coincident on screen are not silently made equal. A disconnected Add result, subtraction that splits material, or ambiguous topology is rejected without changing the Document.

Finish creates a Part-owned Profile with a stable `ProfileId`. The Profile stores semantic references to source-Sketch geometry rather than a copy of the visible fill. Source edits can keep the Profile **Valid**, make it **Invalid**, and later restore it to Valid without changing ProfileId. Moving, rotating, positively scaling or mirroring a complete connected Line/Arc boundary together preserves its existing endpoint topology; this is preservation of an already-established contact, not proximity-based gap healing. Moving only part of the boundary may intentionally open a real gap, which remains open until the authored geometry is actually closed again. SimpleSolid does not automatically rebind an Invalid Profile to similar or nearest replacement geometry.

A Profile appears under its source Sketch in Document Tree. Properties shows Name, ProfileId, Source Sketch, Status, diagnostic, Area, Perimeter, Holes and Visibility. Area/Perimeter/Holes are derived and become unavailable for Invalid instead of retaining stale values. Name and Visibility are authored.

After **Finish Sketch**, a visible Valid Profile remains a flat selectable Part object on the source Sketch plane. Profile visibility is independent from Sketch geometry visibility. Selecting it selects the Profile semantic object, not its source curves. **Delete Profile** removes only the Profile and leaves source Sketch geometry unchanged.

To change an existing Profile region, select exactly one Profile and use **Edit Profile** or enter `EDITPROFILE`. Editing preserves ProfileId. Use `PROFILE` to start a new Profile draft.

Command Line and Operations drive the same Profile state. Context commands are `ADD`, `SUBTRACT`, `FINISH`, `CANCEL`, plus `ISLANDS ON|OFF`, `BOUNDARIES ON|OFF` and `PROBLEMS ON|OFF`. `ISLANDS ON|OFF` controls only Show Islands presentation; it never disables island analysis.

Profile remains Part semantics based on authored 2D geometry and is now a durable source for solid Features. With **Automatic** visibility, an active Feature consuming the Profile hides its normal presentation; after the last active consumer is Suppressed or Deleted, the Profile becomes visible again. Explicit Show/Hide forces shown/hidden presentation without changing modeling evaluation. During Create/Edit Extrude or Revolve, a valid solid preview transiently hides its source Profile to avoid coplanar flicker; when an Edit preview becomes invalid, the source Profile may be revealed temporarily for diagnosis. These runtime overrides never change authored Visibility.

<!-- section-id: product.parts.extrude -->
## Body, Feature and Extrude

Every current Part has exactly one durable **Body**. The Body may be empty or contain an ordered Feature history. The production solid Feature families currently available are **Extrude**, **Revolve**, **Fillet** and **Chamfer**. This section describes Extrude; Axis/Revolve and edge Features are described next.

Extrude supports two equivalent activation paths. **Selection-first:** select exactly one valid Profile and use **Extrude** or enter `EXTRUDE`. **Command-first:** start **Extrude** with no Profile selected; the Workbench enters an explicit selection state and waits for one valid Profile chosen in Tree or viewport. Clicking Extrude again, pressing **Esc**, or entering `CANCEL` leaves that state without authored mutation. The first Feature that creates a solid in an Empty Body must be **Add**. Later Extrudes may be **Add** or **Cut**. Every successful stage must still produce exactly one solid; detached Add, no-effect Add/Cut, a Cut that removes the whole solid, or a multi-solid result is rejected explicitly and is not committed.

Available extent semantics:

- **OneSide** — Length is the full distance from the Profile plane; **Reverse** switches direction relative to the support normal;
- **Midplane** — Length is the total symmetric distance across both sides of the Profile plane; Reverse is not meaningful and is not authored.

A new Create Extrude starts with a default **10 mm** canonical distance, displayed in the current Part unit, so a valid Profile immediately shows preview and direction. Changing a valid parameter updates the dynamic preview. While typing Distance, expensive solid preview evaluation is briefly debounced so text entry remains responsive; Enter/Finish always flushes the current value before commit validation. During preview the already accepted Body stays opaque in its normal presentation. Add shows only material that would actually be added, `tool - upstream Body`, in additive blue; Cut shows only material that would actually be removed, `tool ∩ upstream Body`, in subtractive orange. The complete candidate Body is still evaluated in the background and remains the authority for whether Finish is allowed. A Cut whose tool only touches the Body by a face, edge or point has no positive removable volume and is rejected explicitly as **no effect**. **Finish** performs exactly one durable commit and creates one Undo step. **Cancel**, invalid input or stale preview/context creates no partial authored change and removes the transient delta overlay. Editing an existing Extrude from Properties or Tree uses the same draft, preserves `FeatureId` and again requires one Finish.

Document Tree shows Origin and source Sketch/Profile objects first, with the **Body** and its ordered Features logically below those sources. Invalid Profiles and Failed/Blocked Features keep explicit status text, bold emphasis and a warning icon in Tree; Suppressed remains explicitly labeled and italic. If a higher Feature becomes Failed/Blocked, the final Body remains **Unavailable**, but geometry produced by lower Features that still evaluate UpToDate remains visible as the current-revision history prefix. Later active Features stay Blocked and cannot consume that prefix. If the first active Feature fails, there is no valid lower prefix and no solid is shown. Repairing authored Sketch/Profile geometry refreshes these derived warnings and the full Body presentation immediately rather than waiting for Save. Feature Properties shows `FeatureId`, status/diagnostic, Add/Cut, extent, Length, direction and the source Profile/Sketch. Navigation works Feature → Profile and Profile → consuming Feature without changing ownership: the Profile remains owned by its Sketch.

**Suppress Feature** preserves FeatureId and parameters while removing the Feature contribution from current evaluation. **Unsuppress** reevaluates from current authored state. **Delete Feature** removes that Feature, preserves its source Profile and does not rewrite remaining dependencies; downstream Features may become Failed/Blocked. Suppress, Unsuppress and Delete participate in Undo/Redo.

Command Line and GUI control the same Extrude draft. Supported contextual words include `ADD`, `CUT`, `REVERSE`, `MIDPLANE`, `ONESIDE`, `FINISH` and `CANCEL`; Length uses the shared unit-aware quantity input.

Body/Feature do not have a separate Show/Hide state. Suppress is modeling semantics, not visibility. Current Body Faces, Edges and Vertices are directly selectable for inspection. Durable modeling meaning remains semantic and stage-scoped; Viewer tokens or topology order are never stored as references.

<!-- section-id: product.parts.axis-revolve -->
## Axis and Revolve

A Revolve axis is always an explicit semantic **AxisReference**. You can use the built-in **Origin X/Y/Z Axis** directly; built-in axes keep their Origin identity and never receive a synthetic AxisId. A Part-owned **Axis** is a designation of exactly one non-degenerate Sketch Line. There is no separate GUI Axis creation button: use the Line tool or select one existing Line and work in the contextual **Operations** panel. The resulting Axis has a stable `AxisId`, appears under its source Sketch in Document Tree, and has its own persistent Show/Hide state independent from Sketch visibility.

While **Line** is active, Operations separates two independent choices: **Geometry role — Regular / Construction** and **Part reference — Axis**. Axis starts OFF. Turn it ON when the next Line should also become a Part Axis. The option is one-shot: the first successfully committed Line+Axis creates both objects together in one Undo step, then Axis returns to OFF. A zero-length or rejected Line does not consume the armed option. Regular+Axis and Construction+Axis are both valid.

For exactly one selected Line, the Axis checkbox shows its current Part designation. OFF→ON creates one fresh Axis without changing the Line or its Regular/Construction role. ON→OFF removes that Axis and leaves the Line. If a Revolve references the Axis, SimpleSolid asks for confirmation; after removal the Revolve keeps the missing AxisId intent and reports Missing/Blocked until repaired. Undo restores the same deleted AxisId. If an older file contains several AxisIds pointing to the same Line, the checkbox is indeterminate and disabled until you repair the conflict explicitly in Axis Tree/Properties.

**Edit Axis** re-sources the same AxisId to another admissible Sketch Line. New Create/Re-source authoring rejects a Line already designated by another Axis. Moving or rotating the source Line, or changing the source Sketch support, recomputes the current world axis from the authored Line endpoint order. Deleting the source Line makes the Axis unavailable until it is explicitly repaired. No nearest/similar Edge or Line is substituted automatically.

The `AXIS` Command Line path remains available for keyboard-first authoring. It uses the same semantic Axis draft and the same one-source uniqueness rule; removing the GUI toolbar button does not remove the command.

**Revolve** supports both selection-first and command-first use. A new Revolve requires one valid Profile and one explicit AxisReference; the Profile and Axis may be acquired in either order. Start **Revolve** or enter `REVOLVE`. Origin axes can also be entered as `X`, `Y` or `Z` while the Revolve command is active; an authored Axis is selected from Tree/viewport. There is no inferred/default axis.

New Revolve defaults are **Add**, **One Side**, **360°**, **Reverse off**. The accepted angle range is **0 < Angle ≤ 360°**:

- **One Side** sweeps the authored total angle in one direction; **Reverse** flips that direction;
- **Midplane** uses the authored Angle as the **total** sweep, distributed symmetrically around the Profile plane; Reverse is not authored in Midplane;
- **Add** must produce one connected Body and **Cut** must remove positive volume without deleting the whole Body; detached/no-effect/multi-solid results are rejected.

The Axis must lie in the Profile plane. Profile material must lie wholly on one side of the Axis; touching the Axis, including a boundary segment on the Axis, is allowed, while material crossing through the Axis interior is rejected. Skew/non-coplanar general sweeps and angles above 360° are not part of the current tool.

The viewport keeps the accepted Body in normal presentation and shows only the exact added or removed material as the Revolve preview. The source Profile may be hidden transiently while a valid preview is shown. The selected source Axis is also emphasized while the command is active; a normally hidden Origin/authored Axis may be revealed temporarily for spatial clarity. These are runtime presentation overrides only and never author Profile/Axis visibility. Finish validates the complete candidate Body and commits exactly one transaction; Cancel, invalid input or stale draft/evaluation commits nothing.

Editing an existing Revolve uses the same draft, preserves `FeatureId` and may change Profile, AxisReference, Add/Cut, extent, Angle or One-Side Reverse. Suppress/Unsuppress and Delete use the same generic Feature lifecycle as Extrude. A missing Profile, missing Axis, unavailable Axis source or invalid Profile/Axis relation keeps authored intent and reports structured Failed/Blocked status rather than using last-good geometry.

At a full **360°** sweep, provider periodic seam topology may exist as representation detail, but it is not promoted to a durable engineering Edge solely because the kernel emits a seam. Durable topology meaning remains semantic and stage-scoped.

While Revolve is active, Command Line and Operations drive the same draft. Supported contextual input includes `ADD`, `CUT`, `ONESIDE`, `MIDPLANE`, `REVERSE`, Angle input, `X`/`Y`/`Z`, `FINISH` and `CANCEL`.

<!-- section-id: product.parts.edge-features -->
## Fillet and Chamfer

Part provides two explicit Edge-modifying Features in the Modify group: Fillet and Chamfer.

- Fillet is a constant-radius blend. One Feature uses one common Radius > 0 for every selected Edge.
- Chamfer is an equal-distance chamfer. One Feature uses one common Distance > 0 for every selected Edge.

Both tools accept one or more explicit material Edges. Multi-Edge is normal production behavior: disconnected Edges, connected Edges and common connected corners can be authored in one Feature when the requested geometry is valid. The selected Edge set is the authored intent; SimpleSolid does not silently add tangent neighbors, choose similar geometry or use selection order as modeling meaning.

The resulting local corner topology is allowed to adapt to make that request constructible. Neighboring Edges may shorten or split, vertices may be replaced and adjacent Faces/corner patches may change. Those local result changes are not additional selected inputs. In particular, an unselected tangent Edge is still not silently filleted/chamfered; select the full tangent chain explicitly if you want the operation on the whole chain.

You can work selection-first or command-first. Select admissible Body Edges and press Fillet / Chamfer, or start the tool first and then click Edges in the viewport. While the tool is active, clicking an admissible Edge toggles its membership. Operations shows the selected Edge count and provides Clear; the primary selected input can also be removed without restarting the tool. Only current semantic material Edges are accepted. Periodic seam representation and same-Surface partition Edges are not authorable inputs.

Operations shows the shared Radius or Distance and current preview status. Changing the value or Edge set evaluates the **complete candidate Body**, which remains the authority for Finish. The visible preview shows only the **local material change** relative to the Body immediately before this Feature: removed material in orange (`B0 - B1`) and added material in blue (`B1 - B0`). Unchanged Body material keeps its neutral color; one or both colors may appear, depending on the geometry. If the exact delta preview is unavailable, SimpleSolid does not substitute a blue highlight of the whole Body. Preview is transient, not persistent geometry or an Edge identity source. All explicit Edges are applied together as one Feature operation; there is no partial-success Feature. An excessive Radius/Distance or another geometric provider failure reports Failed and cannot be finished. Finish creates exactly one durable Feature / one Undo step. Cancel or a rejected/stale Finish creates no authored Feature.

The Command Line uses the same modeling path. Enter FILLET or CHAMFER; contextual parameter input, Edge selection, clear/remove, FINISH and CANCEL operate on the same draft as the GUI.

A finished Fillet/Chamfer appears in the ordered Body history with a stable FeatureId. Edit Fillet / Edit Chamfer restores the complete authored Edge set and Radius/Distance and preserves that FeatureId. Edit presents the exact Body stage immediately before the Feature, so replacement Edges are selected from the correct history context rather than from later geometry. Cancelled Edit leaves the previous Feature unchanged; successful Edit is one Undo step.

Edge references are semantic and strict. After an upstream change: one surviving semantic Edge recomputes; removed Edge becomes Missing / Blocked; split or merged meaning with no unique strict winner becomes Ambiguous / Blocked; non-referenceable meaning becomes Unsupported / Blocked.

Repair is explicit. Edit identifies the failing input and lets you remove or replace it; SimpleSolid never chooses the nearest, longest, first or most similar Edge automatically. A Failed/Blocked Feature keeps its authored FeatureId, Edge set, parameter and name.

Suppress preserves the Feature and inputs but removes its modeling contribution. Unsuppress reevaluates it from current authored state. Delete removes the authored Feature; downstream Features then resolve or fail against the changed ordered history. Undo/Redo restores exact authored identities and input sets.

Fillet and Chamfer can consume ordinary material Edges from both Extrude and full or partial Revolve Bodies when the shared Surface/Curve semantic relationship resolves strictly; a periodic representation seam does not thereby become a pickable Edge. They can also consume ordinary generated engineering Edges from earlier edge Features. Analytic curved material Edges produced by ordinary Boolean topology (for example circular plane-cylinder Cut boundaries) are also selectable when their semantic Surface-pair relation resolves strictly. Local transition-spline boundaries created only to close a Fillet/Chamfer corner remain visible/accounted but are not promoted to durable authorable Edge meaning. Both Fillet -> Chamfer and Chamfer -> Fillet are supported where geometry is valid. Save/Close/Reopen reconstructs these chains from semantic intent; previous-process OCCT/Viewer topology handles are not stored.

Current PM-05 variants are intentionally bounded. There is no variable-radius/full-round/face Fillet, per-Edge radius/distance, distance-angle or asymmetric two-distance Chamfer, automatic tangent-chain/loop authoring, generic healing mode or multi-body edge Feature behavior.
<!-- section-id: product.parts.body-topology -->
## Body topology, View Style and semantic inspection

The committed Body presentation exposes current **Face**, **Edge** and **Vertex** topology from the same current evaluation generation as the shaded Body. Direct selection is an inspection/acquisition surface, not persistent provider identity.

The viewport View Style selector offers:

- **Shaded**;
- **Shaded + Edges**;
- **Shaded + Hidden Edges**.

View Style is temporary presentation state. It does not dirty the Part, create Undo/Redo history or change semantic selection. Hidden-edge display does not enable occluded select-through. An Edge that only partitions bounded Face fragments of the same semantic Surface is a representation partition: it stays in topology accounting but is hidden from normal engineering edge display and ordinary picking.

For ordinary all-kind picking, visible candidates use the screen-space priority **Vertex → Edge → Face**. When several candidates overlap under the pointer, **Tab** and **Shift+Tab** cycle the runtime candidate stack before click. The visible preselection you click is the acquisition target; provider return order does not become CAD identity.

Selecting current topology updates Properties without adding Face/Edge/Vertex rows to Document Tree:

- Face shows its semantic Surface, surface class, producer/stage/provenance, referenceability and standard Sketch-support capability;
- Edge shows its Curve meaning, material/representation-artifact state, adjacent Surfaces and referenceability;
- Vertex shows semantic Point/provenance and current XYZ as diagnostic geometry only;
- Body shows current complete topology/accounting counts.

A current topology item can be **Present** while durable singular referenceability is **Ambiguous** or **Unsupported**. SimpleSolid reports that state instead of silently rebinding by proximity or geometry similarity.

Selecting or hovering a Feature in Document Tree highlights its **Current Feature Contribution** on the current Body. This is not a historical-stage replacement and does not create duplicate selectable topology. If a selected topology item disappears after recompute, direct selection clears instead of jumping to similar geometry.

Face-supported Sketches attach durably to the semantic planar **Surface**, not to a provider Face handle. If later modeling splits that Surface into multiple bounded Face fragments, the Sketch may remain Resolved. A coplanar Extrude Add may extend an existing Surface carrier only when Boolean lineage proves unique continuation; visual coplanarity alone never rebinds identity. The artificial partition Edge between continued fragments is not an engineering boundary. Re-entering Sketch Edit resolves presentation against the Sketch support's exact upstream Body stage, even when later Features exist. If the semantic support is deleted it becomes **Missing**; if multiple semantic candidates remain it becomes **Ambiguous**. No stale last-good support frame is used for downstream modeling. Use Change Sketch Support / `RESUPPORT` for explicit repair.

<!-- section-id: product.parts.navigation -->
## 3D navigation and Navigation Cube

The Part Workbench provides middle-button Pan, Shift + middle-button Orbit, mouse-wheel Zoom, Fit and Orthographic/Perspective projection.

A 3D Navigation Cube is shown in the upper-right of the 3D Viewport. It follows the current camera orientation. Click a labeled face for Front, Back, Left, Right, Top or Bottom; click an edge for the corresponding two-axis view; click a corner for the corresponding isometric view. Navigation transitions are animated.

When the view is aligned to a face, the controls around the Cube provide exact 90-degree moves to adjacent views and exact 90-degree clockwise/counterclockwise roll. Home returns to Top-Front-Right isometric orientation and performs Fit All.

Projection is independent from Cube orientation. The `ORTHO/PERSP` control next to the Cube switches only between Orthographic and Perspective. If the model is in Perspective, clicking a Cube face, edge, corner or Home keeps Perspective; the same actions keep Orthographic when Orthographic is active.

Navigation and camera changes are temporary view state. They do not dirty the Part, do not require Save and do not create CAD Undo entries.

<!-- section-id: product.parts.presentation-recovery -->
## 3D presentation failure and retry

If a CAD operation is committed successfully in the authored model but the 3D Viewer cannot refresh its presentation, the model change remains committed. Document revision, Save/dirty state and Undo/Redo history are not rolled back because of a display failure.

The Workbench then shows a diagnostic that the 3D presentation update failed. While that diagnostic is present, the visible scene must not be treated as synchronized with the current model.

The next normal full Viewer refresh automatically retries presentation from the current authored state. After a complete successful refresh, the degraded state and diagnostic clear. Recovery never reconstructs the model from Viewer objects and does not create an extra Undo entry.

<!-- section-id: product.parts.save-close -->
## Save and closing

`Save` writes the current authored Part state: Document properties, Origin visibility, durable Offset Datum Planes with `DatumId`/source/signed Offset/visibility, Sketches with semantic Origin/Body-Surface/Datum support and local geometry, Profiles with RegionIntent/visibility policy, and the Body with ordered Extrude/Revolve Features, stable identities, parameters and Suppressed state. Authored Sketch-Line Axes persist with AxisId, source Sketch/Line identity and visibility; Revolve persists its ProfileId, AxisReference, Add/Cut operation and OneSide/Midplane angle parameters.

The current B-Rep solid, evaluated topology catalog, derived Body-Surface/Datum support frames, transient Datum/Extrude/Revolve/Fillet/Chamfer preview, virtual intersection lines, evaluation status, provider topology tokens and Viewer state are not stored as CAD intent. They are rebuilt by fresh evaluation after open.

Ordinary Save remains conditional on the exact native file version loaded or last saved by the session. A removed, replaced or externally changed target reports a Save conflict rather than being silently overwritten. The in-memory Part, Undo/Redo and local changes stay open.

Closing a dirty Part requires `Save`, `Discard` or `Cancel`. Closing the complete Project/application offers `Save All`, `Discard` and `Cancel`. A failed Save does not close the Document.

<!-- section-id: product.parts.restart -->
## Restart and reopen

After restarting, open the same Project. SimpleSolid scans the Workspace again and opens the saved Part with the same DocumentId, DatumId, SketchId/ProfileId, AxisId, BodyId and FeatureId values. Face- and Datum-supported Sketches keep their semantic support and local U/V geometry even though runtime topology/presentation token values are rebuilt.

Datums, authored Axes, the Body and ordered Extrude/Revolve/Fillet/Chamfer Features are evaluated from scratch from authored state. Datum sources, signed Offsets and authored visibility remain persisted, while world frames and virtual intersections are derived again. Persisted Suppressed Features remain Suppressed, and Automatic Profile presentation is derived again from current active consumers.

Undo/Redo history, active selection, active Sketch/Datum/Axis/Extrude/Revolve/Fillet/Chamfer draft, preview, camera and provider/B-Rep runtime state are not stored and start fresh after reopen.

<!-- section-id: product.parts.conflicts -->
## Document and Save conflicts

If two `.ss2part` files in one Workspace contain the same DocumentId, `Open…` shows an identity-conflict entry and the discovered location information. That entry cannot be opened by DocumentId until the conflict is removed. SimpleSolid does not arbitrarily choose one copy and does not automatically assign a new ID.

A damaged or unsupported native Part is shown as an invalid entry instead of being treated as a valid Document.

A Save conflict is different from a Workspace discovery conflict: it means the already-open session no longer has authority to replace the file version currently at its path. SimpleSolid keeps the in-memory document open and does not silently force-overwrite or recreate a missing target. Save As / Force Overwrite recovery is not part of the current product.

<!-- section-id: product.parts.current-limits -->
## Current Part limits

The current Part provides persistent Sketch support on Origin planes, planar Body Faces and Offset Datum Planes; construction Offset Datum Planes in `Reference Geometry`; Shared-2D authoring with precision input/Polar/Dynamic Input/OSNAP/Tracking/Inference; Trim/Extend/Measure; live-reference Profiles; direct Face/Edge/Vertex inspection; Part-owned Sketch-Line Axes; and one durable Body with ordered Extrude/Revolve/Fillet/Chamfer Features.

Solid modeling currently includes **Extrude Add/Cut** with **OneSide Forward/Reverse** and **Midplane**, **Revolve Add/Cut** with **One Side/Midplane**, explicit Origin/Authored AxisReference, **0 < Angle ≤ 360°** and One-Side Reverse, plus constant-radius **Fillet** and equal-distance **Chamfer** over explicit one-or-more material Edge sets. Edit Extrude/Revolve/Fillet/Chamfer, explicit Missing/Ambiguous/Unsupported Edge repair, Axis create/edit/re-source/show/hide/delete/repair, Feature status, Suppress/Unsuppress, Delete, semantic Sketch re-support, Datum Edit/Show/Hide, Undo/Redo, consumed-Profile automatic visibility and Save/Close/Reopen cold rebuild are supported.

Not yet implemented are Datum Axis, Datum Point, additional Datum Plane constructors, Body-Edge/Curve or other Axis constructors, non-planar standard Sketch mapping, Projection, advanced Fillet/Chamfer variants (variable radius, face/full-round, distance-angle/asymmetric), automatic tangent-chain authoring, other solid operations, arbitrary Feature reorder/insertion, multi-turn (>360°) Revolve, multi-body, Material, Assembly or Drawing tools.

On the Sketch side, authored constraints/solver, authored dimensions, Grid Snap, Rotate/Scale/Mirror+Copy, clipboard/cross-Sketch Copy and final ordinary-Select RMB convergence remain outside the current surface.

Very early test `.ss2part` files from before the current native format are not supported production data.

