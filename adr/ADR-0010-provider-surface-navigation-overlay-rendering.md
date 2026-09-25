# ADR-0010 — Provider-surface navigation overlay rendering

**Status:** PROPOSED  
**Date:** 2026-09-25  
**Owner acceptance:** pending  
**Decision class:** D2 Architecture  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Related:** ADR-0003, ADR-0006, WB-01B

## Context

The current Windows Qt/OCCT Workbench renders the CAD scene in a native `WA_PaintOnScreen` viewport while ViewCube is implemented as a QWidget overlay.

Manual WB-01B verification established that this composition is not reliably artifact-free. Two bounded D1 alternatives were tested and rejected:

1. ViewCube as a native child of the native OCCT viewport;
2. ViewCube and OCCT viewport as native siblings under a native Editor Surface host.

Both retained stale/overdrawn ViewCube artifacts in manual Windows testing.

By contrast, Sketch rectangle selection became visually stable after moving its rectangle presentation into the OCCT render surface with `AIS_RubberBand`.

This evidence indicates that the remaining ViewCube defect is architectural rather than another missing repaint call.

## Decision

Introduce a provider-neutral navigation-overlay contract owned by the common Viewer boundary, with provider-local rendering inside the native graphics surface.

The common Viewer contract owns navigation meaning and interaction intent. The Qt/OCCT provider owns the pixels and hit-testing implementation needed to present that navigation overlay in the same native render surface as the CAD view.

Conceptually:

```text
Workbench / ViewCube navigation intent
        ↓
provider-neutral Viewer navigation overlay contract
        ↓
Qt/OCCT provider
        ↓
native in-surface overlay presentation + hit testing
```

The contract must remain semantic/provider-neutral. It must not expose Qt widgets, OCCT handles, raw window handles or provider-native identity.

## Ownership

The common Viewer layer may define:

- finite ViewCube/navigation overlay visibility state;
- standard-view targets;
- Fit intent;
- projection toggle intent;
- provider-neutral hit/action identifiers;
- neutral callbacks for user activation.

The Qt/OCCT provider owns:

- in-surface visual rendering;
- pixel geometry and DPI handling;
- provider-local hit testing;
- provider-local z-order/composition;
- native redraw integration.

The Workbench remains responsible for invoking document/view navigation behavior through existing Viewer semantics. The provider does not gain CAD model ownership.

## Non-goals

This ADR does not authorize:

- durable ViewCube state;
- CAD model mutation;
- Qt/OCCT types in public Viewer contracts;
- a general arbitrary-widget overlay framework;
- moving Properties/Operations/Command Line into the Viewer;
- changing Sketch selection semantics;
- changing camera persistence policy.

## Consequences

ViewCube will no longer depend on QWidget-over-`WA_PaintOnScreen` composition for its visible interactive surface.

A small provider-neutral overlay contract becomes part of the common Viewer API, so this is a D2 public boundary change.

The existing QWidget ViewCube implementation may remain temporarily as donor/reference code during migration, but the completed implementation should not retain two competing visible ViewCube authorities.

Tests must separate:

- provider-neutral navigation semantics;
- provider-local in-surface rendering/hit-test mechanics;
- manual Windows visual artifact verification.

## Alternatives rejected

### Keep adding repaint/invalidation hooks

Rejected because manual WB-01B evidence shows the defect persists despite explicit underlay refresh and native-window composition variants.

### Native QWidget child/sibling composition

Rejected by manual Windows verification under WB-01B.

### Move all Workbench overlays into the provider

Rejected as speculative and too broad. Only the demonstrated ViewCube/native-surface problem is addressed.

## Verification

An implementing Work Contract must prove:

- standard views, Fit and projection toggle remain correct;
- no CAD authored state/history changes;
- correct DPI/resize behavior;
- no stale ViewCube artifacts during resize, compact/regular transitions and navigation;
- no regression in pointer→ray mapping or Sketch selection;
- exact-head FULL CI;
- explicit manual Windows verification of the final visual result.

## Documentation impact

Internal docs: required  
User/Product docs: not required  
Reason: this changes Viewer/UI ownership and provider implementation boundaries without changing intended user workflow.

## Completion boundary

Acceptance of this ADR authorizes a bounded WB-01B implementation amendment for provider-surface ViewCube presentation.

It does not activate R5.
