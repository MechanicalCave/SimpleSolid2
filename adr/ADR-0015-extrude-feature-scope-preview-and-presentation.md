# ADR-0015 — Extrude Feature Scope, Dynamic Preview and Presentation Contract

**Status:** ACCEPTED  
**Proposed:** 2026-10-03  
**Owner acceptance:** 2026-10-03  
**Decision class:** D2 Part product/architecture amendment  
**Foundation:** 1.0 (foundation-v1.0)  
**Program authority:** `work/PART_MODELING_V1_ROADMAP.md` v1.5 — PM-01  
**Amends:** ADR-0014 product-scope consequence and the O-06 Extrude matrix  
**Preserves:** ADR-0014 identity, reference, failure, freshness and numerical semantics

## Context

ADR-0014 deliberately froze a minimal first production slice around one-sided Extrude Add. Before PM-01 activation, Owner review concluded that implementing a single-Add special case would create artificial product and code boundaries that would immediately need redesign.

The accepted PM-01 direction is therefore a bounded but complete Extrude Feature family: multiple ordered Extrude Features in one Body, with Add/Cut, OneSide/Midplane, dynamic preview, Command Line parity and explicit Profile-to-Feature discoverability.

This ADR does not authorize multi-body, face-supported Sketches, Through All/Up To extents, topology picking, general dependency infrastructure or provider-native durable identity.

## Decision

### 1. One Extrude Feature family

Part v1 uses one durable Extrude Feature meaning rather than separate Add/Cut feature classes.

Each Extrude records:

- exactly one existing ProfileId;
- operation: Add or Cut;
- extent: OneSide or Midplane;
- finite positive Length;
- Forward/Reverse direction only when OneSide makes direction semantically meaningful.

The first successful solid-producing Feature in an Empty Body must be Extrude Add. A newly created or deliberately edited Feature cannot be accepted as Cut when no valid upstream Body exists.

Already-authored history remains repairable. If upstream edits, Delete or Suppress make a later Cut lose its input Body, that Feature remains authored and becomes Blocked rather than being deleted or rewritten.

Subsequent Extrude Features may be Add or Cut. Every successful stage still produces exactly one valid solid. Detached Add, no-effect Add/Cut, zero-solid Cut and multi-solid results are explicit non-success outcomes.

### 2. Extent semantics

OneSide:

- distance is the full extrusion length from the Profile support plane;
- Forward follows the accepted support-frame normal;
- Reverse uses the opposite direction;
- semantic cap roles are `profile_cap` and `extent_cap`.

Midplane:

- distance is the total extrusion length;
- geometry spans `-distance/2 .. +distance/2` along the accepted support-frame normal;
- Reverse is not a semantic option and is disabled/not authored for Midplane;
- semantic cap roles are `negative_cap` and `positive_cap` relative to the support-frame normal.

Side roles remain bound to exact Profile boundary provenance. Provider topology order never defines cap/side identity.

### 3. Dynamic preview and one Finish

Extrude editing uses one runtime draft shared by GUI and Command Line.

Every valid parameter change may synchronously refresh the preview, including partial numeric editing once the current text parses as a complete legal quantity. Preview is derived runtime state and never authored truth.

The user performs exactly one Finish/Enter action. Finish does not ask for a second confirmation.

At Finish execution, the semantic command revalidates current DocumentId/DocumentRevision, source Profile, active draft generation and the current successful evaluation before one transaction commits authored Feature state. Stale or invalid preview context fails without partial mutation.

Cancel/Escape and rejected Finish create no authored change and no Undo entry.

### 4. Profile visibility and Profile-to-Feature discoverability

PM-01 does not add Body or Feature Show/Hide semantics. Suppress remains the semantic way to remove a Feature contribution while preserving authored identity.

A Profile has an authored visibility policy:

- `automatic`;
- `force_shown`;
- `force_hidden`.

Under `automatic`, a Profile consumed by at least one active/non-suppressed Feature is hidden from normal presentation and becomes visible again when it is no longer consumed. Explicit Show/Hide selects the corresponding force policy and never changes evaluation.

Migration from the current boolean Profile visibility preserves `false` as `force_hidden`; current `true` becomes `automatic`.

Profile remains owned by its Sketch. Tree hierarchy is not required to pretend the Profile is owned by Extrude.

Instead, UI must expose bidirectional discoverability:

- a Feature identifies its source Profile and source Sketch;
- a Profile identifies Features that consume it;
- navigation actions can move selection to the related object;
- entering Feature edit temporarily reveals/highlights the source Profile regardless of normal automatic hiding.

### 5. Command Line parity

GUI and Command Line are adapters over the same Extrude draft and semantic commands.

PM-01 supports the Extrude command vocabulary needed for the accepted workflow, including:

- `EXTRUDE`;
- `ADD`;
- `CUT`;
- `REVERSE`;
- `MIDPLANE`;
- `ONESIDE`;
- `FINISH`;
- `CANCEL`;
- unit-aware Length expressions through the existing quantity grammar.

A selected admissible Profile may seed EXTRUDE directly. Command Line changes immediately update the same Operations panel/draft state and preview; GUI changes update the same active command context.

### 6. Bounded Viewer API expansion

PM-01 is explicitly authorized to add the minimum public provider-neutral Viewer contracts required to present the evaluated Body and transient Extrude preview.

The allowed expansion is presentation-only solid scene/preview support using neutral derived presentation geometry. It must not expose TopoDS/OCAF/provider handles, durable topology ordinals or Part semantic identity as Viewer identity.

PM-01 does not authorize face/edge topology picking or a general model-topology selection API. Those remain owned by later topology-dependent packages.

Tessellation/display quality is derived presentation state and cannot affect modeling, semantic lineage or operation success.

### 7. Numerical policy for chained Add/Cut

ADR-0014 numerical rules remain authoritative.

For PM-01:

- provider fuzzy value is 0 unless this contract is explicitly amended;
- no adaptive/escalating fuzzy retry;
- no silent gap healing;
- Add and Cut must classify no-effect, detached/empty and multi-solid outcomes explicitly;
- refine/unify behavior is an explicit operation policy and must be covered by semantic lineage regression before use.

### 8. PM-02 sequencing amendment

Extrude Cut is no longer deferred to PM-02.

PM-02 becomes the bounded datum/support package needed after PM-01, beginning with accepted Origin-based offset datum plane semantics and Sketch support on that datum. It may exercise the already-delivered Extrude Feature from datum-backed Profiles but does not invent a second Extrude implementation.

Planar model-face support remains PM-03 because it requires the accepted deterministic semantic face-frame/reference path.

## Consequences

Positive:

- the first production feature architecture is not a one-operation special case;
- ordered Features and single-Body Boolean semantics are proven immediately;
- Add/Cut and OneSide/Midplane share one durable Feature meaning;
- dynamic preview remains fast without weakening commit authority;
- source Profile relationships remain understandable without falsifying ownership in the Tree;
- future operations can reuse the same Profile consumption/visibility and preview principles.

Costs:

- PM-01 is larger and requires real Boolean Add/Cut evaluation;
- schema migration must introduce Profile visibility policy as well as Body/Feature state;
- Viewer gains a bounded public presentation contract;
- topology lineage tests must cover OneSide and Midplane cap-role differences.

## Rejected directions

- implementing only one Extrude Add because it is the first Feature;
- separate durable AddFeature/CutFeature types for the same Extrude operation family;
- treating Reverse as meaningful authored state for Midplane;
- committing the last drawn preview without execution-time revalidation;
- Body/Feature Show/Hide as a substitute for Suppress;
- moving Profile ownership under Feature merely to make Tree relationships visible;
- persisting tessellation/provider topology as modeling truth;
- introducing face/edge topology picking into PM-01.
