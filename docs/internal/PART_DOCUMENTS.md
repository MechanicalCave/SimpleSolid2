# Part Documents — As-built

<!-- doc-id: internal.part-documents -->
<!-- document-kind: internal -->

<!-- section-id: internal.part-documents.model -->
## Current model

The current `PartDocument` is persistent and hosts durable Part Sketch objects with embedded Shared 2D authored Line geometry. It still contains no Body, Feature or modeled solid B-Rep.

Its authored state consists of stable `DocumentId`, common Document Properties, persistent presentation state for the seven built-in Origin references and an ordered collection of Part-hosted Sketch records.

Each Part Sketch has stable `SketchId`, semantic support restricted to XY/XZ/YZ built-in Origin planes, explicit `SketchPlacement`, persistent visibility and one value-owned `sketch::SketchModel`. Part owns host support/placement/visibility/persistence semantics; Shared 2D owns the embedded entity identity and authored 2D geometry.

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

Current commands cover common Document Properties, built-in reference visibility, Sketch creation, mixed Line/Circle/Arc creation and update, mixed deletion, common transforms and duplication. Durable mutation remains semantic-command driven; UI/Viewer presentation identity is never mutation authority.

Each `PartDocumentTransaction` captures the technical `DocumentRevision` from which its staged full-state snapshot was created. Commit is authorized only when that base revision still equals the owning `PartDocument` revision. A mismatch returns typed `stale_transaction` before validation, no-op comparison or authored mutation, so an older full-state transaction cannot overwrite a newer accepted mutation.

Part transactions are one-shot. The first commit attempt is terminal whether it succeeds, is a no-op, or fails as stale, invalid-state or revision-exhausted; a later commit returns `inactive_transaction`. Rollback is terminal and idempotent. A fresh no-op creates neither a revision increment nor a history entry.

The Part domain validates the complete staged `PartAuthoredState` at commit: hosted Sketch support and placement must be valid and match, and hosted SketchId values must be unique. Invalid full-state replacement returns typed `invalid_state` without changing authored state or revision. The same validator protects `PartDocument::restore`, which returns a structured validated reconstruction result rather than constructing an invalid live document.

`DocumentSession::verifyRevision()` remains a second command/history consistency guard; the owning Part transaction is the domain authority for stale-state rejection.

Undo and Redo reapply authored states through `PartDocumentTransaction` and therefore count as new semantic mutations with increasing technical `DocumentRevision`. Undo/Redo preserve accepted EntityId lineage/high-water rules and restore authored identity rather than Viewer/presentation identity.

<!-- section-id: internal.part-documents.session -->
## DocumentSession

`DocumentSession` is runtime-only and contains the current physical path, loaded `PartDocument`, expected technical revision, Undo/Redo history, the saved authored-state checkpoint and — for a native file opened/created through the Project runtime — a `PartFileCheckpoint`.

Undo/Redo still uses a runtime `std::vector` of two-snapshot history entries containing the authored state before and after each accepted command. Adding a new command no longer deep-copies all older history entries. The session prepares one pending history entry, reserves the required vector capacity and prepares the Sketch EntityId high-water map before the Part transaction mutates the live document. History entries are non-copyable and no-throw movable, so vector relocation transfers ownership rather than copying prior authored snapshots.

A Redo suffix remains logically intact while a command is being prepared. It is destroyed only after a successful changed Part commit, then the already-prepared entry is appended inside reserved capacity and the prepared EntityId cursor map is published by no-throw swap. A rejected, failed or no-op command therefore preserves the current Undo/Redo branch. C1 intentionally leaves the `before/after` snapshot representation and history depth policy unchanged; deeper representation or budgeting remains subject to AUDIT-01 C2 measurement.

`needsSave()` compares authored state with the saved authored-state checkpoint. It is not defined by numeric `DocumentRevision` equality, which allows Undo back to the saved semantic state to become clean even though `DocumentRevision` increased.

The native-file checkpoint represents the exact file version established by load/create or the last successful Save. It contains the expected DocumentId, exact byte length, SHA-256 digest and platform file identity. Detached/test sessions may exist without a file checkpoint, but ordinary Save then fails closed rather than performing an unconditional overwrite.

Closing and reopening creates fresh runtime history.

<!-- section-id: internal.part-documents.persistence -->
## Native Part persistence

The native extension is `.ss2part`.

The current Part domain writer uses schema **v4**. It stores document properties, built-in Origin visibility and the ordered hosted Sketch collection. Each Sketch record stores stable SketchId, Origin-plane support, explicit placement, visibility and one embedded Shared 2D model.

Schema v4 stores canonical decimal `next_entity_id` plus mixed `entities[]` records. Supported authored kinds are Line, Circle and Arc. EntityId is model-local and shared across primitive kinds; malformed/non-canonical or duplicate IDs, IDs outside the cursor range and invalid primitive geometry fail closed.

Part schema versions v1, v2 and v3 remain readable. V1 restores no Sketches, v2 restores hosted Sketch records with empty Shared 2D models, and v3 reads the former Line-only `next_entity_id + lines[]` model. Opening an older schema does not rewrite the file; a later successful ordinary Save publishes current schema v4.

ProjectId, DocumentSession, Undo/Redo, active Sketch edit context, camera, active selection, Qt objects, Viewer objects and OCCT handles are not serialized as Part authored state.

Ordinary Save is conditional on the session's native-file checkpoint. A cooperative per-target Save guard serializes cooperating SS2 writers across inspect → compare → publish → new-checkpoint capture. Missing target, changed DocumentId, replaced file object, changed exact content or an already-owned Save guard fail closed with typed Save-conflict diagnostics. A failed/conflicted Save does not advance the saved authored-state checkpoint or file checkpoint.

Successful publication uses whole-file atomic replacement and returns the checkpoint of the newly published target. The strict no-lost-update guarantee is for cooperating SS2 writers that use this guard; the implementation does not claim an atomic filesystem compare-and-swap against arbitrary unrelated writers.

<!-- section-id: internal.part-documents.discovery -->
## Discovery and canonical sessions

ProjectSession owns a rebuildable Workspace index.

A DocumentId resolves only when exactly one valid native file in the Workspace declares it. Duplicate physical files with the same DocumentId form `IdentityConflict` and include all discovered relative paths.

When a resolved Part is already open, a second Open request returns the existing canonical DocumentSession rather than creating a second mutable session for the same DocumentId.

<!-- section-id: internal.part-documents.sketch-presentation -->
## Active Sketch presentation and spatial input

While a Part Sketch is actively edited, `PartViewportController` rebuilds a neutral authored Sketch scene from the current embedded `SketchModel`, maps Line/Circle/Arc geometry from Sketch U/V through `SketchPlacement` into 3D, presents the intrinsic Sketch Origin as a non-authored overlay and keeps runtime presentation-token bindings back to `SketchId + EntityId`.

A separate preview scene is transient and independently replaceable/clearable. Neutral provider rays are intersected with the active Sketch frame to produce Sketch-local U/V. Selection, hover, grips, preview, pointer candidates and Command Line input state remain runtime-only and do not change Part revision, dirty state, history or persistence until a semantic command commits.

Presentation tokens are ephemeral. Scene rebuild/history may allocate different tokens for the same authored EntityId; tests and runtime logic must therefore reacquire presentation bindings after a rebuild rather than treating Viewer tokens as durable identity.

If the active Sketch disappears through Undo/history or the editing context is replaced, presentation/input state fails closed and is cleared.

<!-- section-id: internal.part-documents.current-limits -->
## Current limits

The Part model durably owns Shared 2D Line/Circle/Arc entities. The active Sketch editor supports semantic point/Window/Crossing selection, mixed Delete, Line/Circle/Arc creation, Move/Copy/Rotate/Scale/Mirror, state-based grips, owner-only Reshape, grip Move, Space CycleEditMode, Repeat Last Command and Direct Distance at the currently supported point requests.

Presentation tokens, preview, pointer input, Command Line buffer, hover/grip state and camera remain runtime-only and are not Part/Sketch identity.

The product still does not implement authored constraints/dimensions/solver, snapping/inference, Ortho/Polar/Grid Snap, Dynamic Input, numeric Rotate angle or Scale factor, absolute/relative/polar coordinate entry, unit expressions, grip Copy modifier, ordinary-Select RMB context, Datum/Construction Plane support, planar model-face Sketch support, Body/Feature modeled solid geometry, persistent topology naming or Material.

The Viewer is not a second model: no OCCT object or Viewer token is durable Part/Sketch identity, support or authored state.

