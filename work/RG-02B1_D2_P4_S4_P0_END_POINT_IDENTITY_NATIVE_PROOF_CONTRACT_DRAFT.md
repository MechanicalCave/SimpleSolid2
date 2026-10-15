# RG-02B1 / P4-S4 P0 — Native END-to-Point identity proof (proposed executable Work Contract)

**Status: PROPOSED / NOT OWNER-ACCEPTED AS EXECUTABLE / NOT ACTIVE / NO SOURCE-CODE AUTHORITY.**
**Date:** 2026-10-15.
**Governance baseline:** protected main 615c34e701a9736af695c2190d9b14d189fc2517, after Owner-manual-PASS S3A #386. Parent Owner-accepted D2 design direction is work/RG-02B1_D2_P4_S4_THREE_END_POINTS_DESIGN_PROPOSAL.md (PR #389, still separately gated for protected-main squash). work/ACTIVE.yaml still targets S3A, not P0. The existing Part Modeling v1.33, Foundation 1.0, Constitution, Architecture, ADR-0012/0013/0014/0016/0017, DOC-01 and native Q1/Q2 evidence remain authoritative.

## 0. Purpose and precise non-goals

The Owner approved **Datum Plane → 3 Points (END-only)** as the third constructor of the existing tool beside Offset and Edge + Point; no MID inside 3 Points, no separate Edge operand, no new tool, no global hover redesign, and no future independent off-infinite-line Point+Edge feature. This P0 has **no product/UI/schema implementation**.

Native Q1 already establishes 12 bounded material Edges, 24 distinct certified **edge-local END** references and 8 trihedral corners on a canonical OCCT box (verifyQ1CanonicalSelectedEnds in the existing pm05c2b native test). This does NOT yet establish that all 24 edge-relative clicks can be represented as 8 **unique durable semantic Points**, including independent cold replay. P0 must prove or reject that specific prerequisite before any versioned three-Point Datum model/schema is attempted.

## 1. Source-free native evidence work

Inside the existing registered real Windows/OCCT Part native test, add one named S4-P0 witness using **current public interfaces only**:

1. **P0-01 canonical 24→8:** author all 24 certified SelectedMaterialEdgeEnd tuples from 12 material Edges × 2 terminals. Check each terminal's certified FeaturePointAddress and current BodyStageRef; cross-check the catalog's FeaturePointResolution as uniquely Resolved with exactly one current native vertex. Canonical (BodyStageRef, FeaturePointAddress) dedup must yield **8 unique semantic Point keys**. Each trihedral corner is three distinct Edge+END values but exactly one key. No XYZ/token equivalence.
2. **P0-02 actual three-Point input:** choose three different semantic END keys from potentially different material Edges at the SAME declared Body stage, with real native 3D positions. Check distinct, noncollinear and collinear/duplicate examples as geometric input facts only; **do not implement/author a new Datum frame** or change kernel tolerances in P0.
3. **P0-03 identity replay:** cold rebuild on an independently instantiated OCCT provider, prove the same stage-qualified semantic Point keys still resolve despite runtime token changes; modify a feature dimension while retaining producer FeatureId and verify same keys, potentially moved positions; a cloned/decoy FeatureId cannot silently certify old sources.
4. **P0-04 genuine negative families:** inspect existing real partial Revolve / half-cylinder Cut / Fillet-Chamfer generated cases with Unsupported, Missing or Ambiguous global Points. Keep these statuses fail-closed even when an edge-local Q1 END itself can be resolved. Include foreign document, foreign Body, stale stage/revision and repeated same physical corner via different incident Edges. No promotion by screen coordinates/provider order.
5. **P0-05 bounded supported-coverage report:** name exactly which current visible END families have unique durable Point identity. If a commonplace END cannot become a uniquely certified source, report **P0 FAIL / OWNER D2 STOP** and an evidence-led provenance alternative. Do not silently store three SelectedMaterialEdgeEnd values as edge-dependent point meaning or manufacture XYZ-based identities.
6. **P0-06 zero product mutation:** all work is an additive registered native test and a written evidence log. Preserve every Q1/Q2 invariant/test. No production Part/UI/Viewer/API, new CMake, schema v18, generated docs, save fixture or modeled Datum change.

## 2. Exact executable file allowlist (pending Owner approval and activation)

- tests/kernel_native/pm05c2b_part_edge_topology_test.cpp — additive S4-P0 assertions to the existing registered native test.
- work/RG-02B1_D2_P4_S4_P0_END_POINT_IDENTITY_NATIVE_PROOF_EVIDENCE.md — real fixture/status/identity/CI evidence, including failing classes.

Everything else is OUT in the implementation PR: src/part, src/application, src/kernel, src/kernel_occt, src/ui, src/viewer, src/viewer_qt_occt, native Part schema v18, CMake/CI, unrelated tests, ADR, roadmap, application shell, product/internal docs and work/ACTIVE.yaml. Any new production interface/test location must STOP for a specific Owner D2 allowlist amendment.

## 3. Native acceptance matrix (not yet run)

- **P0-F01:** current native OCCT box proves 12 Edges / 24 distinct END / **8** stage-qualified unique semantic Points, with three incident END sources per corner and no runtime token identity.
- **P0-F02:** real independent provider, altered-dimension same FeatureId and changed FeatureId decoy behave correctly. Partial Revolve, half-cylinder, Fillet/Chamfer negative statuses stay typed and fail-closed.
- **P0-F03:** distinct/noncollinear versus duplicate/collinear triples classified without geometry heuristics; selected source is semantic Point, never the incidental source Edge.
- **P0-F04:** registered Windows native FOCUSED, FAST and Ready FULL actual CTest PASS on exact tested SHA; no removal/weakening of existing Q1/Q2/Datum suites; closure governance and DOC-01 applicable checks PASS.
- **P0-F05:** exact-HEAD evidence log records counts and fixtures, successes and all historical RED failures, positive proof or explicit STOP; Owner grants separate P0 technical PASS/FAIL and a separate merge authorization for exact production PR HEAD.
- **P0-F06:** no owner manual GUI PASS is claimed here: P0 is headless evidence, not shipped 3 Points interaction.

## 4. Strict workflow / dependencies

1. Parent accepted-design PR #389 must pass exact-head governance and receive **its own separate explicit Owner authorization** before protected-main merge.
2. Owner separately reviews and accepts this **exact 2-path native P0 executable contract**. Its governance PR needs verified Windows closure and independently authorized protected-main squash.
3. Separate **work/ACTIVE.yaml-only activation PR**, explicitly approved and merged, points to the accepted P0 contract. **Before activation, do not edit either allowlisted test or evidence file as P0 production work.**
4. New proof branch on activated protected main; native FOCUSED → FAST → Ready FULL, exact-HEAD evidence → Owner technical P0 verdict → independent production merge authority.
5. Only after positive P0, draft new detailed and separately accepted/activated **S4A durable triple-Point/v18 schema** contract, then S4B GUI constructor under another bounded contract. S4A may not simply assume all clickables have a globally valid FeaturePointAddress.
6. S3A remains closed on main. No automatic work/ACTIVE, broad roadmap, version number or app UI mutation follows from D2 direction approval or this proposed contract.

## Documentation impact

Internal docs: not required
User/Product docs: not required
Reason: a headless native proof test and work evidence log do not change shipped behavior, ownership, native file format or UI. Subsequent S4A/S4B execution contracts must explicitly update as-built Internal and canonical PL/EN Product Markdown and regenerate Browser via the official generator when those capabilities ship.
