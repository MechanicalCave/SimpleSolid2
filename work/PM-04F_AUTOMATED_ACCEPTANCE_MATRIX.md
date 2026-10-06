# PM-04F — Automated Acceptance Matrix

**Status:** COMPLETED — PASS  
**Parent Work Contract:** `work/PM-04_AXIS_REVOLVE.md`  
**Program authority:** `work/PART_MODELING_V1_ROADMAP.md` v1.25  
**Date:** 2026-10-06

## Purpose

This matrix closes the automated-evidence side of the accepted PM-04 Axis / Revolve package without redefining product or architecture semantics.

PM-04A through PM-04E are already merged and exact-head gated. Owner pre-close review accepted a bounded Axis-designation UX amendment on 2026-10-06. That amendment is now implemented and exact-head gated: final runtime/docs candidate `8d6f082137a573aab08a4eee3b383a9923d98a49` passed Windows FULL #1578 attempt 2. The Owner subsequently executed the supported Windows workflow on the exact runtime candidate and reported PASS with no errors. PM-04F product acceptance is complete.

## Superseding PM-04F Owner amendment

Normative amendment: `work/PM-04F_AXIS_DESIGNATION_UX_AMENDMENT.md` — ACCEPTED 2026-10-06.

Closure evidence now proves contextual Line Axis designation, atomic Line+Axis authoring/history, new-authoring source uniqueness with legacy duplicate-source compatibility, removal of the GUI Axis toolbar action, retained AXIS Command Line parity, refreshed documentation/Browser and exact-head Windows FULL. Revised Owner Windows PASS remains mandatory.

## Evidence authority

| Checkpoint | Exact candidate | Gate | Merged main | Primary evidence |
| --- | --- | --- | --- | --- |
| PM-04A | `9a01ec707c12ac699dcd1d5b9e1a4e7d8bc2578a` | Windows FULL #1531 PASS | `4c6ed90e59e365fe6d53a88c68184b6332743749` | Axis semantic model / schema v12 |
| PM-04B1 | `37bdbd55cbb8ee2fc2120c88d0de80358c47b6aa` | Windows FULL #1533 PASS | `3d6d36c7e2253c0f071090d96d836abfe6c44d57` | Axis lifecycle / shared draft |
| PM-04B2 | `da5e64dcaca0238391e4710f2edaacdfd198413b` | Windows FULL #1544 PASS | `e6660287c19ab245d622fad470e6d58323aeca24` | Axis Tree / Properties / Viewer / Workbench |
| PM-04C1 | `57cb332f2c6e8e3032a3647df3c8457d4d1583a3` | Windows FULL #1547 PASS | `704879e90aa56f8338164d1ccdb918d122d0e03b` | provider-neutral Revolve semantics / evaluator |
| PM-04C2 | `6e627749b850524c0dc3730ed19c7f9506b37536` | Windows FULL #1549 PASS | `70d1b5986fa87d736626afbe858a6c096eabd36d` | OCCT Revolve / topology catalog |
| PM-04D1 | `24975b9bdd8f2e97cc11f2c7fef77bdbc9c48de3` | Windows FULL #1551 PASS | `84bddafb57eab4f7776b78621b81369c2917899b` | shared Revolve draft / commands / exact preview |
| PM-04D2 | `c275045387b7fa928dcc955ba3837565796b1328` | Windows FULL #1555 PASS | `92348a8af84679ee2ce77b099d9c337dfec2dc84` | Operations / CAD Input / Viewer / Tree |
| PM-04E1 | `dad92ce54fb0cf69ae96b7210d5f49bfce3f9402` | Windows FULL #1560 PASS | `a559f7b2d6a667e4d4b34cbfd7af6f2d93cd65e1` | schema v13 / Revolve persistence |
| PM-04E2 | `bdf87a29b5cdc27c608023af863e32c44dc55ef1` | Windows FULL #1562 PASS | `e9a1056da4ec3639458aa1cdf4faacfc1621fcc9` | lifecycle / repair / Save-Reopen / cold rebuild |
| PM-04E3 | `b7dcd55ab9c04fdf208a534f6e09de7ffac0e00f` | Windows FULL #1563 PASS | `1a7e1228a91b23f89bf13f6bb5c07c8abe9afae3` | Profile-source and Sketch-support edits |
| PM-04F docs | `5dfd00100f16f0e5c195cdb53c12fb2ba65bf894` | Windows DOCS #1565 PASS | `72c40b8cfb39ea63859d5485f80a8adfbba35094` | internal + PL/EN Product + generated Browser |
| PM-04F final automated | `8d6f082137a573aab08a4eee3b383a9923d98a49` | Windows FULL #1578 attempt 2 PASS — core 25/25, kernel-native 47/47, desktop 104/104 | pending merge | final Axis-designation runtime/docs candidate |
| PM-04F Axis-designation amendment | `8d6f082137a573aab08a4eee3b383a9923d98a49` | implementation + exact-head FULL #1578 attempt 2 PASS | pending merge | `work/PM-04F_AXIS_DESIGNATION_UX_AMENDMENT.md` |
| PM-04F Owner Windows | `8d6f082137a573aab08a4eee3b383a9923d98a49` | Owner manual Windows PASS — 2026-10-06 | PASS | `work/PM-04F_OWNER_WINDOWS_ACCEPTANCE.md` |

## Contract matrix

| PM-04 acceptance row | Automated authority | State |
| --- | --- | --- |
| Axis from Regular Line | PM-04B1 | PASS |
| Axis from Construction Line without role conversion | PM-04B1 | PASS |
| AxisId non-aliasing / high-water | PM-04A/B1 | PASS |
| Edit Axis re-source preserves AxisId and visibility | PM-04B1/E2 | PASS |
| source Line edit recomputes current Axis | PM-04B1/E2 | PASS |
| source Sketch support edit recomputes Axis | PM-04E3 | PASS |
| source Line Delete -> Axis unavailable; no stale line | PM-04E2 | PASS |
| explicit Axis repair | PM-04B1/E2 | PASS |
| Axis Show/Hide independent from Sketch/modeling | PM-04B2/D2 | PASS |
| Delete referenced Axis retains Revolve MissingAxis intent | PM-04E2 | PASS |
| Undo restores exact AxisId and repairs consumer | PM-04E2 | PASS |
| GUI Axis creation is contextual Line designation; no standalone Axis toolbar button | PM-04F / FULL #1578 | PASS |
| Regular/Construction geometry role is orthogonal to Axis designation | PM-04F / FULL #1578 | PASS |
| new Line + Axis commits atomically in one transaction / one Undo step | PM-04F / FULL #1578 | PASS |
| one-shot Axis designation resets only after successful Line+Axis commit | PM-04F / FULL #1578 | PASS |
| existing selected Line OFF->ON / ON->OFF preserves Line identity and exact delete/Undo semantics | PM-04F / FULL #1578 | PASS |
| Create/Re-source rejects duplicate exact source Line with zero partial mutation | PM-04F / FULL #1578 | PASS |
| pre-amendment duplicate-source Axis state loads without identity rewrite and UI fails closed/indeterminate | PM-04F / FULL #1578 | PASS |
| `AXIS` Command Line remains available and shares semantic AxisDraft path | PM-04F / FULL #1578 | PASS |
| Origin X/Y/Z direct AxisReference; no synthetic AxisId | PM-04A/C/D | PASS |
| hidden source Axis may be runtime-emphasized without authored visibility mutation | PM-04D2 | PASS |
| Revolve Add first Body Feature | PM-04C/D/E | PASS |
| later Add / Cut | PM-04C/D | PASS |
| OneSide partial/full + Reverse | PM-04C/D | PASS |
| Midplane partial/full total-angle semantics | PM-04C/D | PASS |
| default new Revolve 360 degrees | PM-04D | PASS |
| Origin and authored AxisReference | PM-04C/D | PASS |
| Profile/Axis coplanarity and one-half-plane admission | PM-04C | PASS |
| touch / boundary-on-axis accepted; interior crossing rejected | PM-04C | PASS |
| non-coplanar/skew Axis rejected | PM-04C | PASS |
| detached/no-effect/remove-all/multi-solid rejected | PM-04C | PASS |
| Edit Revolve preserves FeatureId | PM-04D/E | PASS |
| change Profile / AxisReference | PM-04D/E | PASS |
| Axis/Profile/support source edits recompute downstream | PM-04E2/E3 | PASS |
| MissingProfile / MissingAxis / AxisUnavailable structured failure | PM-04E2 | PASS |
| bounded stage-cycle rejection | PM-04C + accepted PM-03E support-chain evidence | PASS |
| Suppress/Delete/Undo/Redo | PM-04E2 | PASS |
| Save/Close/Reopen | PM-04E1/E2/E3 | PASS |
| true cold rebuild with fresh provider/runtime generation | PM-04E1/E2/E3 | PASS |
| GUI / Operations / Command Line same semantic draft | PM-04B/D | PASS |
| stale draft/revision/session cannot commit | PM-04B/D | PASS |
| complete Revolve Face/Edge/Vertex accounting | PM-04C2 | PASS |
| deterministic partial-angle start/end roles | PM-04C | PASS |
| 360-degree seam remains representation artifact where non-material | PM-04C2 | PASS |
| no provider identity or geometry-similarity rebinding | PM-04A/C/E | PASS |
| exact preview delta and committed Body presentation isolation | PM-04D | PASS |
| schema v13 reconstructs semantic intent without provider continuity | PM-04E1/E2/E3 | PASS |
| internal as-built documentation | exact candidate `8d6f082137a573aab08a4eee3b383a9923d98a49` / Windows FULL #1578 docs verification | PASS |
| PL/EN Product documentation with paired section structure | exact candidate `8d6f082137a573aab08a4eee3b383a9923d98a49` / Windows FULL #1578 docs verification | PASS |
| generated Product Browser freshness | exact candidate `8d6f082137a573aab08a4eee3b383a9923d98a49`; FULL #1578 ran canonical docs generator + diff check | PASS |
| final exact-head automated package gate | `8d6f082137a573aab08a4eee3b383a9923d98a49` / Windows FULL #1578 attempt 2 — core 25/25, kernel-native 47/47, desktop 104/104 | PASS |
| Owner Windows workflow | `8d6f082137a573aab08a4eee3b383a9923d98a49` | **PASS — 2026-10-06** |

## Documentation closure requirements

The PM-04F docs candidate must describe current state, not implementation history:

- Origin X/Y/Z as direct built-in AxisReference values with no synthetic AxisId;
- authored Sketch-Line Axis identity, source, visibility, repair and Delete semantics;
- Regular and Construction source Lines without role conversion;
- Revolve Add/Cut, OneSide/Midplane, default 360 degrees and One-Side Reverse;
- explicit Profile + Axis acquisition and coplanarity/crossing restrictions;
- exact transient preview and runtime-only Axis emphasis;
- structured Missing/Blocked repair behavior;
- schema v13 Save/Reopen/cold rebuild;
- full-360 periodic seam representation-artifact rule;
- explicit exclusions: Datum Axis, Body-Edge/Curve Axis constructors, Projection prerequisite, >360-degree/multi-turn Revolve, multi-Body.

The Product Browser is generated from canonical Markdown and must not be edited as an independent source.

## PM-04F completion result

PM-04F is **COMPLETED — PASS**.

Accepted final evidence:

- runtime/docs authority: `8d6f082137a573aab08a4eee3b383a9923d98a49`;
- Windows FULL #1578 attempt 2: core 25/25, kernel-native 47/47, desktop FULL 104/104;
- canonical internal + PL/EN Product documentation and generated Product Browser verification: PASS;
- Owner supported-Windows acceptance: **PASS — 2026-10-06**, no errors reported.

The first #1578 attempt was infrastructure-only and failed before checkout/build/test while downloading a GitHub action; the unchanged exact SHA passed attempt 2. Earlier #1577 correctly caught a test-only private-API use and the regression was repaired to exercise the public Tree -> Edit Sketch workflow before the accepted FULL.

Work-only closure run #1579 was cancelled twice while the self-hosted runner could not resolve/download `actions/checkout@v4` from `launch.actions.githubusercontent.com:443`. Those cancellations executed no governance/documentation invariant and have no product-evidence meaning. A successful final work-only closure gate remains required for the PR merge; it does not replace or invalidate the accepted runtime authority above.

PM-05 and PM-06 remain inactive pending their own accepted authority.

## Documentation impact

Internal docs: required — this matrix is PM-04F acceptance evidence and accompanies the as-built documentation update.  
User/Product docs: required — PM-04 changes current user-visible Axis/Revolve workflows and recovery semantics.  
Product Browser: required — generated from the updated canonical Markdown.
