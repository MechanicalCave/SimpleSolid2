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

<!-- section-id: product.parts.sketch-host -->
## Creating and viewing a Sketch

Use `Sketch` in the expanding left side of the active-Document top row, then select `XY Plane`, `XZ Plane` or `YZ Plane` from Origin. You may select the plane in Document Tree or, when visible, in the 3D Viewport.

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

Creation tools preserve pre-existing selection but hide/deactivate grips while active, and newly created geometry is not automatically selected. Use `Finish Sketch` to leave edit. Sketch support is currently limited to the three Origin planes.

Copy combined with Rotate/Scale/Mirror, ordinary-Select RMB context, clipboard/cross-Sketch Copy, Grid Snap, constraints/solver, authored dimensions, Datum planes and planar model faces remain later stages. There is no separate Ortho mode; use Polar with a 90° step for orthogonal-only attraction.

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

Profile is 2D/Part semantics. **It does not perform Extrude or create a solid.** Modeled solid operations require separate Part functionality.

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

`Save` writes the current authored Part state, including Origin visibility, Sketches with durable identity/support/placement, Line/Circle/Arc geometry with Regular/Construction roles and stable EntityIds, and Profiles with ProfileId/RegionIntent/name/visibility, to its `.ss2part` file.

Ordinary Save is conditional on the exact native file version that this session loaded or last saved. If that target was removed, replaced, changed on disk, changed to another DocumentId or is currently guarded by another cooperating SS2 Save, SimpleSolid reports a Save conflict instead of silently overwriting the target. The in-memory Part remains open and keeps its local changes/Undo history.

After a successful Save the session adopts the newly published file version, so a later unchanged Save works normally. The strict no-lost-update guarantee is for cooperating SS2 instances; SimpleSolid does not claim a general atomic compare-and-swap against every unrelated external writer.

When closing a Part with unsaved changes, the application requires `Save`, `Discard` or `Cancel`.

When closing the complete Project or application while any Part is dirty, the choices are `Save All`, `Discard` and `Cancel`.

If saving fails or reports a conflict, the Document/Project remains open. Closing the last open Part keeps the Project open and returns to the Project Workspace Dashboard. Using `Workspace` by itself never closes the Part.

<!-- section-id: product.parts.restart -->
## Restart and reopen

After restarting the application, open the same Project.

SimpleSolid scans the Workspace again. The saved Part is rediscovered with the same DocumentId, saved properties, saved Origin visibility, saved Sketch records including geometry roles, and saved Profiles with the same ProfileIds.

Undo/Redo history, active selection, active Sketch edit context and camera state are not stored in the file and start fresh after reopening.

<!-- section-id: product.parts.conflicts -->
## Document and Save conflicts

If two `.ss2part` files in one Workspace contain the same DocumentId, `Open…` shows an identity-conflict entry and the discovered location information. That entry cannot be opened by DocumentId until the conflict is removed. SimpleSolid does not arbitrarily choose one copy and does not automatically assign a new ID.

A damaged or unsupported native Part is shown as an invalid entry instead of being treated as a valid Document.

A Save conflict is different from a Workspace discovery conflict: it means the already-open session no longer has authority to replace the file version currently at its path. SimpleSolid keeps the in-memory document open and does not silently force-overwrite or recreate a missing target. Save As / Force Overwrite recovery is not part of the current product.

<!-- section-id: product.parts.current-limits -->
## Current Part limits

The current Part provides Document identity/properties, a durable display/input length unit, built-in Origin, persistent reference visibility, the 3D Workbench/Viewer foundation, durable Origin-plane Sketches with Line/Circle/Arc authored geometry and Regular/Construction roles, Rectangle authoring that resolves to ordinary Lines, and Part-owned Profiles with live RegionIntent semantics.

The current Sketch UI includes keyboard-first precision input, mm/cm/m/in/ft quantities and bounded expressions, absolute/relative Cartesian and polar point entry, exact Circle/Arc/Rectangle size input, numeric Rotate/Scale and grip transform values, Polar attraction, Dynamic Input with request-local locks, Measure in current physical units, the supported Grip edit cycle, Grip Copy in Reshape/Move, repeated COPY, Repeat Last Command, Undo/Redo and Save/Reopen.

Profile is not a solid operation. The product still lacks Rotate/Scale/Mirror+Copy, ordinary-Select RMB context, clipboard/cross-Sketch Copy, OSNAP/tracking/inference, Grid Snap, authored constraints/solver, authored dimensions, Sketch support on Datum/planar model faces, Bodies, Features/Extrude, modeled solid geometry, Material, Assembly and Drawing tools.

Very early test `.ss2part` files from before the current native format are not supported product data and are not migrated automatically.
