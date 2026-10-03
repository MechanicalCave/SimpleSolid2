# PM-02P.B — Face / Surface Lineage and Frame Evidence

**Status:** COMPLETED — PASS  
**Date:** 2026-10-03  
**Sub-gates:** B1 #1378 PASS; B2 #1379 PASS; B3 #1380 PASS  
**Merged through main:** `a2db078d01cbb1dccc7112fd22a6083e0266e3e8`

## Closed oracle rows

PM-02P.B closes E01-E10 and E17-E19.

Proven invariants:

- every pristine Extrude Face has exactly one semantic Surface carrier claim;
- all planar cap and lateral carriers have deterministic provenance-derived frames;
- Circle/Arc sides remain resolved non-planar carriers rather than disappearing;
- Add/Cut trim preserves semantic carrier identity/frame where declared stable;
- bounded Face split becomes Ambiguous while one semantic Surface may remain Resolved;
- deleted Surface becomes Missing and geometrically identical replacement cannot steal identity;
- alias/merge is fail-closed without an independent semantic winner;
- Cut-exposed tool Surface is a first-class semantic carrier with deterministic frame;
- OneSide Forward/Reverse and Midplane cap roles/frames are exact on XY/XZ/YZ;
- all successful Boolean evidence stages retain complete Face/Edge/Vertex topology accounting;
- false Resolved count remains zero in the accepted Face/Surface matrix.

## Architecture consequence

The evidence supports ADR-0016's deliberate separation:

```text
bounded Face topology != semantic Surface carrier
```

and supports deriving future Sketch world placement from a resolved semantic planar Surface rather than persisting a second independent world-space authority.

PM-02P now advances to C — Edge/Curve lineage. Production PM-02 remains inactive.
