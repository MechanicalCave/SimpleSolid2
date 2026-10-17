# Sketcher S2 B03a/B04 — cumulative acceptance evidence

**Status:** IN PROGRESS — NOT TECHNICAL FINAL PASS; NOT OWNER MANUAL PASS; NO PRODUCT MERGE AUTHORITY
**Active contract:** `work/SKETCHER_REPAIR_S2_POINT_REQUEST_OSNAP_GRIP_D2_WORK_CONTRACT_PROPOSAL.md`
**Program:** Sketcher Roadmap v1.10; Part Modeling v1.34 priority companion
**Authority:** Owner-accepted S2 §§0–8/16 paths, guarded governance PR #479 and ACTIVE-only PR #480 on protected main `cdc6f4b49569d10b6eb2115af27d800fb33b443e`. Owner explicitly accepted the exact §9 17th existing R10 test path and separately authorized guarded squash PR #486; merged protected main `8c4c296916fa1796697c7ec503d4a2b60d06cbd1` after Ready Windows CLOSURE #3591 success at PR HEAD `2230d407125ab75824d0823088bc7bceb9dab314`.
**Working product PR:** #483, Draft, no implementation squash authorization; original test-first RED PR #482 remains Draft/unmerged.

## Windows evidence — never transfer results to another SHA

| Evidence | Exact SHA | Result and scope |
| --- | --- | --- |
| Windows FAST #3583 | original S2 test-only PR #482 | Genuine 92/93 RED: Circle size-stage request unavailable |
| Windows Ready FULL #3584 | original S2 test-only PR #482 | Core 25/25 and native Kernel 57/57 PASS; Desktop 115/117 RED for independent B03a/B04 fixtures |
| Windows FAST #3586 | `3743eec8788012f02a6515e3e74523ea4c80e823` | PASS 93/93; does not cover Core R10 conflicting assertion |
| Windows Ready FULL #3587 | `3743eec8788012f02a6515e3e74523ea4c80e823` | FAIL at existing `sk07f.precision_input_state` line 286, obsolete no-PointRequest expectation; no PASS claim |
| Ready Windows CLOSURE #3591 | `2230d407125ab75824d0823088bc7bceb9dab314` | PASS; D2 §9 governance only |
| Windows FAST #3593 | `0e52847df9682bec88468b7c2b6e74b802d93e32` | CANCELLED/superseded after a further test-only commit; not PASS |
| Windows FAST #3594 | `ace9544dcf710fbbb566de35e14b5e3727bf4505` | PASS 93/93; adds real Qt-controller B04 TAN/PER moved-end geometry scenarios |
| Windows Ready FULL #3595 | `ace9544dcf710fbbb566de35e14b5e3727bf4505` | FAIL; Core and kernel-native stages PASS, Desktop 115/117. B04 controller fixture used stale runtime source token after presentation refresh; distinct pre-existing `pm05f_r2.native_workbench_edit` failed at native S3A MID-hover re-entry. Both remain factual RED at this SHA. |
| Windows FOCUSED #3596 | `0e897881ac0e66408d1912ea91678a822904bc65` | FAIL 0/1 after first stale-token fixture repair; S2 fixture changed transient selection, breaking a later unrelated R10 assertion. No test removed. |
| Windows FOCUSED #3597 | `392e531655ac29f80aabb11688a99b65f41bc907` | PASS 1/1 `sk07f.precision_input_controller`; test reacquires current source tokens and restores the prior selection state, preserving all downstream R10 controls. |

## Current implementation findings

- B03a: Circle center acceptance exposes one Center-based request for the size-stage; Qt controller uses the shared point resolver rather than raw pointer position, while explicit typed Diameter/Radius retains its own length grammar and numeric authority.
- B04: Line Start/End Reshape keeps its original grip pivot in `PointRequest.base` while a transient `snap_reference_base` uses the stationary opposite endpoint for request-relative TAN/PER only.
- Registered Core tests cover centered request availability and base semantics. Registered Qt tests cover actual off-cursor Circle size OSNAP, actual TAN/PER snap source, preview/committed line endpoint geometry and original opposite endpoint unchanged. Existing tests also cover numeric/Grip Copy/Rotate negative compatibility; further exact-HEAD FULL and Owner manual checks remain mandatory.
- The #3595 native MID-hover failure is **not resolved by #3597**, which executes only the focused controller test. It remains a regression/failure investigation item; no edit to nonallowlisted native test file or global UI semantics is authorized.
- No new public interface, native schema, authored snap/constraint, Part/OCCT source, global snap tolerance, S3 behavior or generalized rebind has been introduced.

## Remaining gates

- DOC-01 current-state internal and PL/EN product docs plus generated Browser freshness and exact-head verification;
- independent Windows FOCUSED where appropriate, FAST and Ready FULL with all Core/native Kernel/Desktop PASS and no hidden historical failures;
- repeat/diagnose native MID-hover case without weakening or modifying its existing out-of-scope test;
- exact-head Windows CLOSURE after factual evidence synchronization;
- Owner manual Windows B03a/B04 acceptance, independent technical FINAL PASS, and separate guarded product merge authorization.

Nothing in this ledger asserts any unperformed 41-step manual transcript, Owner manual S2 PASS, technical FINAL PASS, or permission to implement S3/PM-06.
