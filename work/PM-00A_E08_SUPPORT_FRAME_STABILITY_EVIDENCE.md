# PM-00A — E08 Support-Frame Stability Evidence

**Status:** COMPLETED — PASS  
**Date:** 2026-10-03  
**Exact source candidate:** `51a223dc7f380c8545de2238ef5d3a6ca9e7345d`  
**Windows FULL:** #1311 — PASS  
**Core-only:** 18/18 PASS  
**Kernel-native aggregate:** 25/25 PASS  
**Desktop FULL:** 84/84 PASS  
**Regression:** `pm00a.e08_support_frame_stability`  
**Matrix:** `work/PM-00A_E01_E10_EVIDENCE_MATRIX.md` v1.0

## Result

E08-01 through E08-04 PASS.

False-Resolved count: **0**.

The accepted Origin-plane Sketch support contract reconstructs one deterministic right-handed O/U/V/N frame from Part-owned support + explicit SketchPlacement. The frame is independent of source Profile geometry, evaluation traversal order, camera/Viewer state and OCCT/provider topology because none of those inputs participate in the accepted frame construction.

## Evidence summary

| Row | Expected | Actual | Cold | Result |
| --- | --- | --- | --- | --- |
| E08-01 | unchanged Origin-plane support produces identical O/U/V/N | XY, XZ and YZ each reproduce their exact accepted frame | YES | PASS |
| E08-02 | source Profile/Sketch geometry edits do not rotate or flip support | three Rectangle-backed Profiles are resized; every support frame remains bitwise equal to its baseline | YES | PASS |
| E08-03 | provider/evidence traversal order cannot change the frame | evidence is evaluated in multiple orders; the frame map remains identical; current Origin support has no provider-topology input | YES | PASS |
| E08-04 | unsupported future support meaning is not invented | Origin point and X/Y/Z axes return Unsupported / invalid support; no planar-face support representation is introduced | N/A | PASS |

## Accepted Origin-plane frames

The regression proves:

| Support | O | U | V | N |
| --- | --- | --- | --- | --- |
| XY | (0,0,0) | +X | +Y | +Z |
| XZ | (0,0,0) | +X | +Z | -Y |
| YZ | (0,0,0) | +Y | +Z | +X |

Each frame is orthonormal and right-handed.

These values are reconstructed through the existing Part support/SketchPlacement contract. They are not inferred from Viewer orientation, camera state, B-Rep topology enumeration or OCCT face UV parameterization.

## Geometry-edit stability

For each Origin plane the test creates:

```text
PartDocument
→ Part-hosted Sketch on semantic Origin plane
→ authored Rectangle
→ durable Profile
→ neutral Kernel Profile input
```

The Rectangle dimensions are then changed while support remains unchanged.

The Profile remains valid and the neutral Kernel input reports exactly the same O/U/V/N frame as before the edit.

Result: **no 180-degree flip, mirror or rotation** occurs when source geometry changes.

## Traversal-order stability

The three support cases are evaluated in different orders before and after geometry edits.

Because the accepted Origin-plane support frame is obtained from Part semantic support + SketchPlacement, there is no provider-topology enumeration input to reorder. Reordering independent evidence traversal leaves the complete frame map unchanged.

This is the relevant E08-03 proof for the currently accepted support contract; it does not claim planar-face/provider-derived frame semantics that PM-00A explicitly leaves unsupported.

## Cold Save / Close / Reopen

The authored Part is saved, the original `DocumentSession` and all transient neutral Kernel Profile inputs leave scope, and the native Part is reloaded through `PartDocumentStore`.

After reopen:

- each Sketch support is valid;
- persisted SketchPlacement still matches its support;
- every Profile reconstructs the same neutral Kernel frame;
- another traversal order produces the same complete frame map.

No previous-process Viewer, camera, OCCT handle, provider cache or transient Profile input is required.

## Unsupported support meanings

The accepted Part Sketch support contract currently admits only:

- XY plane;
- XZ plane;
- YZ plane.

The regression explicitly verifies that:

- Origin point;
- X axis;
- Y axis;
- Z axis

cannot be converted into valid Sketch support and produce `ReferenceStatus::Unsupported` at the evidence boundary.

Future planar-face support has no accepted durable support representation in PM-00A. E08 therefore does not synthesize one and does not define a face-derived frame algorithm.

## Verification

Windows FULL #1311 checked out exact SHA `51a223dc7f380c8545de2238ef5d3a6ca9e7345d`.

The E08 regression passed in:

```text
core-only                 18/18 PASS
kernel-native aggregate   25/25 PASS
desktop FULL              84/84 PASS
```

The E08 executable is a core-only regression linked against Application, neutral Kernel and Part semantic code. It introduces no Qt or OCCT dependency.

## Architecture implication

E08 strengthens O-04 for the current Origin-plane domain:

1. support-frame orientation is authored/semantic host meaning, not presentation/provider meaning;
2. a complete O/U/V/N frame can be reconstructed deterministically after cold reopen;
3. source Profile geometry does not own or perturb support orientation;
4. provider topology/order is not an authority for current Origin-plane frames;
5. unsupported support kinds fail closed rather than receiving guessed frame semantics.

E08 does **not** freeze the future planar-face frame algorithm or re-support behavior. PM-00B/future bounded contracts must decide those semantics explicitly.

## Scope boundary

E08 introduced no:

- product solid operation;
- Body/Feature persistence;
- planar-face Sketch support;
- Datum/Construction Plane schema;
- re-support UI;
- Qt/Viewer behavior;
- OCCT/provider topology dependency;
- durable provider identity.

**Next:** E09 stale generation/session publication evidence.
