# PM-02 — View Style / Tree Feature Contribution UX Design

**Status:** ACCEPTED UX DESIGN INPUT — 2026-10-04  
**Production PM-02:** ACTIVE  
**Applies to active contract:** `work/PM-02_BODY_SEMANTIC_TOPOLOGY_FACE_SUPPORTED_SKETCH.md`  
**Architecture basis:** ADR-0014 + ADR-0016 + PM-02P reference-survival evidence

## 1. Purpose

Define the presentation and interaction meaning of:

- viewport visual style;
- direct Face / Edge / Vertex hover and selection;
- Feature hover/selection from Document Tree;
- the relationship between visible topology and semantic Surface / Curve / Point meaning.

This accepted UX/architecture input is normative within the active PM-02 Work Contract.

## 2. Three independent presentation layers

The Viewer must keep these concepts independent:

```text
Base View Style
    ↓
Semantic / selection overlays
    ↓
Feature / command / diagnostic overlays
```

Changing base View Style must not change semantic selection, reference resolution, authored state or command meaning.

### 2.1 Base View Style

PM-02 target set:

1. **Shaded**
2. **Shaded + Edges**
3. **Shaded + Hidden Edges**

Only one base style is active at a time.

### 2.2 Direct topology overlay

Direct viewport interaction highlights what the user physically points at:

- Face;
- Edge;
- Vertex.

The semantic carrier remains internal unless the active command/inspection context needs to visualize it.

### 2.3 Feature/tree overlay

Tree hover/selection visualizes Feature history meaning without changing the base view style.

Feature overlay is not selection authority and is never persisted.

## 3. View Style control

The View Style selector belongs to the viewport navigation/presentation HUD, visually grouped with the existing provider-surface controls such as:

- HOME;
- ORTHO/PERSP;
- Navigation Cube.

Recommended closed form:

```text
HOME    ORTHO/PERSP    Shaded + Edges ▾
```

Recommended popup semantics:

```text
View Style
● Shaded
○ Shaded + Edges
○ Shaded + Hidden Edges
```

The control is single-select.

View Style is presentation state:

- it does not increment DocumentRevision;
- it does not set the Part dirty flag;
- it does not create CAD Undo/Redo history;
- it is not serialized into the Part document.

A later UI preference may remember it outside the Part document, but that is not CAD authored state.

## 4. Base style semantics

### 4.1 Shaded

Display shaded Body surfaces without intentionally exposing the full topological edge network.

This is the lowest-noise modeling view.

### 4.2 Shaded + Edges

Display:

- shaded Body;
- currently visible material/topological edges.

This is the preferred topology-aware everyday view.

### 4.3 Shaded + Hidden Edges

Display:

- shaded Body;
- visible material/topological edges;
- occluded edges using a visually subordinate hidden-line treatment.

Hidden-edge display is presentation only.

Enabling hidden edges must **not** silently make occluded topology directly selectable through the Body. A future explicit Select Through / X-Ray selection mode would be a separate product decision.

## 5. Overlay roles

Exact RGB values are theme/provider policy. Semantic roles are stable.

Recommended roles:

- direct topology hover — cyan/blue family;
- direct topology selection — stronger cyan/blue family;
- Feature Contribution — green family;
- Add operation scope — green translucent volume;
- Cut operation scope — amber/red translucent volume;
- Ambiguous candidates — amber/yellow;
- Missing/broken reference — red;
- Unsupported/representation artifact — neutral/dashed/subordinate.

Overlay meaning must remain distinguishable without relying only on color where practical; line weight, fill, dash and marker shape may also carry state.

## 6. Direct viewport selection: topology first, carrier second

The user directly points at visible topology:

| User gesture target | Runtime topology | Semantic carrier |
| --- | --- | --- |
| wall/patch | Face | Surface |
| material boundary | Edge | Curve |
| corner | Vertex | Point |

Normal direct interaction highlights the bounded topology item.

Carrier visualization appears only when relevant to the active semantic context.

Examples:

- Create Sketch on planar Face -> selected Face + planar Surface support overlay + U/V frame;
- inspect Edge reference -> material Edge + optional Curve extension;
- inspect Vertex reference -> Vertex marker + semantic Point information.

The Viewer never promotes its highlight token to durable CAD identity.

## 7. Feature Contribution — normative definition

For a selected Feature `F`, **Current Feature Contribution** is a set-valued query against the currently authoritative displayed Body stage.

It means:

> which current semantic topology still carries design meaning produced directly by Feature F?

It does **not** mean:

- the historical whole Body immediately after F;
- the complete original sweep/tool volume of F;
- every topology item later affected by F;
- provider-history descendants chosen by raw topology identity.

### 7.1 Face contribution

A current Face belongs to the direct Feature Contribution of F when its current semantic Surface carrier has producer FeatureId = F.

Consequences:

- a Face trimmed by later operations remains contribution of its original Surface producer;
- if one original Face realization splits into several current Face fragments but all retain the same Surface produced by F, **all current fragments are highlighted**;
- this set-valued highlight is not an Ambiguous reference resolution;
- a deleted Surface contributes no current Face highlight.

### 7.2 Edge contribution

A current material Edge belongs to the **direct semantic Edge contribution** of F when its semantic Edge/Curve provenance identifies F as its producing operation.

Examples include operation-owned Boolean intersection Edges introduced by F.

Separately, edges bounding highlighted contribution Faces may be drawn as a **presentation outline envelope** even when those Edges are not semantically owned by F.

The Viewer must not report presentation-outline Edges as direct Feature-owned semantic Edges.

### 7.3 Vertex contribution

A current Vertex belongs to the direct semantic Vertex/Point contribution of F only when its semantic provenance identifies F as producer.

Vertices needed only to outline highlighted Faces/Edges may be presentation envelope markers and are not thereby re-owned by F.

### 7.4 Multi-feature boundaries

An Edge or Vertex can geometrically lie on boundaries involving carriers from more than one Feature.

Feature Contribution must not invent a unique owner merely for coloring.

The semantic catalog remains authoritative. Presentation may:

- highlight direct semantic contribution;
- draw boundary envelope for readability;
- expose contributing/adjacent producers in Properties.

## 8. Tree hover

Hovering a Feature row:

- produces a temporary Feature Contribution overlay;
- does not change primary tree selection;
- does not change Properties authority;
- does not mutate the Part;
- disappears on hover exit.

If a modeling command currently owns viewport target acquisition, command target/preview overlays take priority and tree hover must not compete with that interaction.

## 9. Tree selection

Selecting a Feature row:

- makes that Feature the tree/Properties selection;
- shows a persistent Current Feature Contribution overlay;
- leaves the current Body in its normal base style;
- does not switch the Body to a historical stage.

Recommended Properties may expose:

- FeatureId / name;
- operation;
- evaluation status;
- source Profile/Sketch;
- stage;
- current direct contribution counts;
- Missing/Ambiguous semantic output counts where meaningful.

The overlay is a presentation query. It creates no authored state.

## 10. Split/delete semantics for tree highlight

### Split

If a Surface produced by F is represented by several current Faces:

```text
one historical Face realization
        ↓ later Boolean split
Face A1 + Face A2 + ...
        ↓
tree select F
        ↓
highlight A1 + A2 + ...
```

This is correct because Feature Contribution is set-valued.

It does not resolve a singular strict FaceReference and therefore is not itself Ambiguous.

### Delete

If later modeling removes a semantic Surface/Curve/Point entirely, normal Current Feature Contribution does not ghost the deleted topology.

Properties may report historical outputs that are now Missing.

Historical/diagnostic geometry requires another explicit presentation mode.

## 11. Cut Feature contribution

For a Cut Feature, Current Feature Contribution is the current semantic topology introduced by the Cut, for example:

- Cut-exposed tool Surfaces that remain on the Body boundary;
- Boolean intersection material Edges with direct operation provenance;
- direct generated Points/Vertices where semantic provenance exists.

Existing upstream Surfaces merely trimmed by the Cut remain contribution of their original producer.

This preserves a clean distinction between:

- **who produced this surviving semantic carrier**, and
- **which later operation affected its bounded realization**.

## 12. Operation Scope / Delta — distinct concept

Operation Scope answers:

> what volume did this Feature add/remove at its own upstream stage?

It is separate from Current Feature Contribution.

For Extrude:

- Add -> exact material delta/tool contribution;
- Cut -> exact removed/intersection volume.

Recommended visualization:

- translucent ghost volume;
- outline visible through the current Body;
- Add and Cut use different semantic overlay roles.

Operation Scope is optional presentation and may be delivered after basic Feature Contribution.

It must not become durable identity.

## 13. Historical Stage Preview — distinct concept

Historical Stage Preview answers:

> what did the complete Body look like immediately after this Feature?

This is equivalent in meaning to displaying `BodyStageRef::AfterFeature(F)`.

It is not the default result of selecting F in the tree.

If later productized, it must be an explicit command such as:

- Preview Stage;
- Show Body At This Feature.

Current Body truth and historical diagnostic stage must remain visibly distinguishable.

## 14. Failed / Blocked / Suppressed Feature presentation

### Suppressed

A suppressed Feature has no current contribution from its suppressed evaluation.

Tree selection may show Properties/source intent but must not fabricate current topology.

### Failed / Blocked with resolved prefix

When final Body is unavailable but current evaluation exposes `resolved_prefix_solid`:

- the Viewer may display the already-supported same-revision prefix as diagnostic presentation;
- Feature Contribution for an UpToDate Feature entirely within that prefix may be queried against that prefix stage;
- the UI must label the result as prefix/diagnostic state;
- failed/blocked Feature output is not treated as current successful contribution;
- no command/reference path may consume prefix presentation as final Body truth.

## 15. Overlay priority

Recommended visual priority from lowest to highest:

1. Base View Style;
2. persistent tree Feature Contribution;
3. tree hover Feature Contribution;
4. direct topology hover/selection;
5. active semantic target/support overlay;
6. command preview / operation draft;
7. Ambiguous/Missing/failure diagnostic overlay.

An active modeling command may suppress lower-priority tree hover to avoid competing cues.

Priority controls presentation only. Semantic authority remains with command/domain state.

## 16. Relationship to Surface / Curve / Point visualization

Tree remains design history, not a topology dump.

Do not add default tree branches containing every:

- Surface;
- Curve;
- Point;
- Face;
- Edge;
- Vertex.

Semantic carrier visualization belongs to:

- active command support/target display;
- Properties/inspection;
- explicit future Inspect mode;
- diagnostic overlays.

This keeps the tree centered on authored design intent.

## 17. PM-02 minimum acceptance for this UX

PM-02 should not complete without:

- View Style dropdown in the viewport HUD;
- Shaded;
- Shaded + Edges;
- Shaded + Hidden Edges;
- visual-style changes remaining non-authored;
- hidden-edge display not silently enabling select-through;
- direct Face/Edge/Vertex hover/select overlay independent of base style;
- Feature tree hover showing temporary Current Feature Contribution;
- Feature tree selection showing persistent Current Feature Contribution;
- split Surface contribution highlighting all surviving current Face realizations;
- deleted semantic output not shown as current contribution;
- Cut highlighting direct current Cut-created topology without stealing upstream carrier ownership;
- current Body not replaced by historical stage merely because a Feature is selected;
- clear overlay separation between direct viewport selection and Feature/tree selection.

Operation Scope and Historical Stage Preview may remain later presentation work unless separately promoted into the PM-02 completion gate.
