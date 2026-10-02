# Part Documents — As-built

<!-- doc-id: internal.part-documents -->
<!-- document-kind: internal -->

<!-- section-id: internal.part-documents.model -->
## Current model

The current `PartDocument` is persistent and hosts durable Part Sketch objects with embedded Shared 2D Line/Circle/Arc geometry plus Part-owned Profile objects. It still contains no Body, Feature or modeled solid B-Rep.

Its authored state consists of stable `DocumentId`, common Document Properties, the durable Part display/input `LengthUnit`, persistent presentation state for the seven built-in Origin references, an ordered collection of Part-hosted Sketch records, a monotonic `ProfileId` cursor and an ordered collection of Profiles. Length geometry remains canonical in millimetres; changing the display/input unit changes interpretation and presentation only and never rescales existing geometry.

Each Part Sketch has stable `SketchId`, semantic support restricted to XY/XZ/YZ built-in Origin planes, explicit `SketchPlacement`, persistent visibility and one value-owned `sketch::SketchModel`. Shared 2D owns entity identity, Line/Circle/Arc geometry and Regular/Construction role; Part owns host support/placement/visibility/persistence.

Each Profile has stable `ProfileId`, source `SketchId`, authored name/visibility and durable `ProfileRegionIntent`. RegionIntent references source EntityIds and semantic endpoint/intersection anchors. It does not store Viewer tokens, OCCT topology, sampled fill geometry or derived runtime region indices.

`DocumentRevision` is a technical monotonic counter for successful semantic mutations within the loaded lifecycle.

<!-- section-id: internal.part-documents.origin -->
## Built-in Document Origin

Every Part has seven deterministic semantic references that always exist: Origin Point, X/Y/Z Axis and XY/XZ/YZ Plane.

Their identity comes from their built-in role, not from random DocumentObjectId allocation.

Hiding an Origin reference changes only persistent presentation visibility. It does not delete the reference or change its identity.

The current default is Origin Point plus X/Y/Z axes visible and the three principal planes hidden.

<!-- section-id: internal.part-documents.mutation -->
## Command and transaction boundary

Persistent authored changes follow:

```text
Qt / caller
→ semantic DocumentSession command
→ history / revision validation
→ PartDocumentTransaction staged state
→ atomic domain commit
```

Current commands cover common Document Properties, built-in reference visibility, Sketch creation, mixed Line/Circle/Arc creation/update/deletion, Regular/Construction role changes, common transforms/duplication and Profile create/edit/properties/delete. Durable mutation remains semantic-command driven; UI/Viewer presentation identity is never mutation authority.

Each `PartDocumentTransaction` captures the technical `DocumentRevision` from which its staged full-state snapshot was created. Commit is authorized only when that base revision still equals the owning `PartDocument` revision. A mismatch returns typed `stale_transaction` before validation, no-op comparison or authored mutation, so an older full-state transaction cannot overwrite a newer accepted mutation.

Part transactions are one-shot. The first commit attempt is terminal whether it succeeds, is a no-op, or fails as stale, invalid-state or revision-exhausted; a later commit returns `inactive_transaction`. Rollback is terminal and idempotent. A fresh no-op creates neither a revision increment nor a history entry.

The Part domain validates the complete staged `PartAuthoredState` at commit: hosted Sketch support/placement must be valid and match, SketchId/ProfileId values must be unique and below their cursors, every Profile must reference an existing source Sketch, and RegionIntent structure must be valid. Invalid full-state replacement returns typed `invalid_state` without changing authored state or revision. The same validator protects `PartDocument::restore`, which returns a structured validated reconstruction result rather than constructing an invalid live document.

`DocumentSession::verifyRevision()` remains a second command/history consistency guard; the owning Part transaction is the domain authority for stale-state rejection.

Undo and Redo reapply authored states through `PartDocumentTransaction` and therefore count as new semantic mutations with increasing technical `DocumentRevision`. Undo/Redo preserve accepted EntityId lineage/high-water rules and restore authored identity rather than Viewer/presentation identity.

<!-- section-id: internal.part-documents.session -->
## DocumentSession

`DocumentSession` is runtime-only and contains the current physical path, loaded `PartDocument`, expected technical revision, Undo/Redo history, the saved authored-state checkpoint and — for a native file opened/created through the Project runtime — a `PartFileCheckpoint`.

Undo/Redo still uses a runtime `std::vector` of two-snapshot history entries containing the authored state before and after each accepted command. Adding a new command no longer deep-copies all older history entries. The session prepares one pending history entry, reserves the required vector capacity and prepares the Sketch EntityId high-water map and ProfileId cursor before the Part transaction mutates the live document. History entries are non-copyable and no-throw movable, so vector relocation transfers ownership rather than copying prior authored snapshots.

A Redo suffix remains logically intact while a command is being prepared. It is destroyed only after a successful changed Part commit, then the already-prepared entry is appended inside reserved capacity and the prepared identity high-waters are published without copying older history. A rejected, failed or no-op command therefore preserves the current Undo/Redo branch. C1 intentionally leaves the `before/after` snapshot representation and history depth policy unchanged; deeper representation or budgeting remains subject to AUDIT-01 C2 measurement.

`needsSave()` compares authored state with the saved authored-state checkpoint. It is not defined by numeric `DocumentRevision` equality, which allows Undo back to the saved semantic state to become clean even though `DocumentRevision` increased.

The native-file checkpoint represents the exact file version established by load/create or the last successful Save. It contains the expected DocumentId, exact byte length, SHA-256 digest and platform file identity. Detached/test sessions may exist without a file checkpoint, but ordinary Save then fails closed rather than performing an unconditional overwrite.

Closing and reopening creates fresh runtime history.

<!-- section-id: internal.part-documents.persistence -->
## Native Part persistence

The native extension is `.ss2part`.

The current Part domain writer uses schema **v7**. It stores document properties, the durable display/input length unit, built-in Origin visibility, hosted Sketch records, canonical `next_profile_id` and authored Profiles.

Each Sketch stores stable SketchId, Origin-plane support, explicit placement, visibility and one embedded Shared 2D model. The model stores canonical `next_entity_id` plus mixed Line/Circle/Arc `entities[]`. Every entity stores its authored `regular` or `construction` role.

Each Profile stores canonical ProfileId, source SketchId, authored name/visibility and the semantic RegionIntent loop/anchor structure. Derived region indices, sampled presentation geometry, Viewer tokens and OCCT handles are not serialized.

Schemas v1–v6 remain readable. Older schemas have no length-unit field and therefore migrate in memory to **mm** without rescaling any geometry. Opening an older schema does not rewrite the file; a later successful ordinary Save publishes current schema v7 with the selected length unit.

ProjectId, DocumentSession, Undo/Redo, active Sketch/Profile tool context, Polar/Dynamic Input runtime configuration, request-local numeric locks, region-analysis cache, camera, active selection, Qt objects, Viewer objects and OCCT handles are not serialized as authored Part state.

Ordinary Save remains conditional on the session's native-file checkpoint. Save-conflict rules and whole-file atomic publication are unchanged.

<!-- section-id: internal.part-documents.discovery -->
## Discovery and canonical sessions

ProjectSession owns a rebuildable Workspace index.

A DocumentId resolves only when exactly one valid native file in the Workspace declares it. Duplicate physical files with the same DocumentId form `IdentityConflict` and include all discovered relative paths.

When a resolved Part is already open, a second Open request returns the existing canonical DocumentSession rather than creating a second mutable session for the same DocumentId.

<!-- section-id: internal.part-documents.profiles -->
## Profile semantics and lifecycle

Profile creation/editing is a Part operation over derived Shared-2D regions. The Profile tool caches region analysis for the current Sketch model state, performs hover/pick against that cache and keeps Add/Subtract composition runtime-only. Repeated pointer motion on unchanged geometry does not rebuild the full arrangement.

Nested/disconnected island analysis is part of that complete derived region truth and remains active for every Profile session. The runtime **Show Islands** option controls presentation/diagnostic visibility only; it cannot change RegionCandidate truth, Profile validity, point picking, RegionIntent or authored state. The former public Find All Regions action is no longer part of the normal Profile workflow; internal region enumeration remains derived analysis.

Finish executes one semantic Profile command. Create allocates one fresh ProfileId. Edit preserves the existing ProfileId and atomically replaces RegionIntent. Cancel, hover, diagnostic lookup and rejected drafts do not mutate authored state or consume identity.

Profiles are live references rather than geometry snapshots. `evaluateProfile` resolves durable RegionIntent against current source Sketch geometry. Missing source entities/intersections or ambiguous/unresolved topology make the Profile Invalid without rewriting intent; later source repair can return the same ProfileId to Valid.

Delete Profile removes only the Profile. Source Sketch geometry remains authored. Source Sketch removal cannot silently strand dependent Profiles. UI command targeting is runtime-only: while a Sketch is actively edited, its semantic selection owns Sketch Delete and stale Tree/Profile presentation cannot become mutation authority.

<!-- section-id: internal.part-documents.sketch-presentation -->
## Active Sketch presentation and spatial input

While a Part Sketch is actively edited, `PartViewportController` rebuilds a neutral authored Sketch scene from the current embedded `SketchModel`, maps Line/Circle/Arc geometry from Sketch U/V through `SketchPlacement` into 3D, presents the intrinsic Sketch Origin as a non-authored overlay and keeps runtime presentation-token bindings back to `SketchId + EntityId`.

A separate preview scene is transient and independently replaceable/clearable. Neutral provider rays are intersected with the active Sketch frame to produce Sketch-local U/V. Selection, hover, grips, preview, pointer candidates and Command Line input state remain runtime-only and do not change Part revision, dirty state, history or persistence until a semantic command commits.

Presentation tokens are ephemeral. Scene rebuild/history may allocate different tokens for the same authored EntityId; tests and runtime logic must therefore reacquire presentation bindings after a rebuild rather than treating Viewer tokens as durable identity.

If the active Sketch disappears through Undo/history or the editing context is replaced, presentation/input state fails closed and is cleared.

<!-- section-id: internal.part-documents.current-limits -->
## Current limits

The Part model durably owns hosted Shared 2D Line/Circle/Arc entities, a display/input length unit and Part-owned Profiles. The active Sketch editor supports semantic point/Window/Crossing selection, mixed Delete, Line/Circle/Arc/Rectangle creation, Regular/Construction role changes, Profile Create/Edit with Add/Subtract, Move/Copy/Rotate/Scale/Mirror, the full supported grip edit cycle, Grip Copy in Reshape/Move, Repeat Last Command and the R10 shared precision-input path.

Length input accepts mm/cm/m/in/ft with canonical millimetres, dimensional arithmetic and absolute/relative Cartesian or polar point syntax. Polar and Dynamic Input are application-session runtime aids; request-local numeric locks remain transient. Exact Rotate/Scale and supported grip Rotate/Scale/Mirror numeric input reuse the same semantic request pipeline.

Presentation tokens, preview, pointer input, Command Line/Dynamic Input live token, Polar capture, numeric locks, hover/grip state and camera remain runtime-only and are not Part/Sketch identity.

The product still does not implement authored constraints/dimensions/solver, OSNAP/tracking/inference, Grid Snap, Rotate/Scale/Mirror+Copy, ordinary-Select RMB context, clipboard/cross-Sketch Copy, Datum/Construction Plane support, planar model-face Sketch support, Body/Feature modeled solid geometry, persistent topology naming or Material.

The Viewer is not a second model: no OCCT object or Viewer token is durable Part/Sketch identity, support or authored state.

