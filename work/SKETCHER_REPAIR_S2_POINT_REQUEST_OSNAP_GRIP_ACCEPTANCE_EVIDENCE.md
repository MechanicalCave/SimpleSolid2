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
| Windows FOCUSED #3598 | `5073307444785965860a5a29f5f09f5ff76b7c67` | PASS 2/2: `sk07f.precision_input_controller` and unchanged `pm05f_r2.native_workbench_edit` native GUI. Original #3595 RED remains historical, not reclassified. |
| Windows Ready FULL #3599 | `5073307444785965860a5a29f5f09f5ff76b7c67` | **PASS 199/199**: Core 25/25, native Kernel 57/57, Qt/Desktop 117/117; aggregate `windows-msvc` SUCCESS. Official `.\\ss2.ps1 docs`, generated Browser no-diff and DOC-01 verification PASS. |

## Current implementation findings

- B03a: Circle center acceptance exposes one Center-based request for the size-stage; Qt controller uses the shared point resolver rather than raw pointer position, while explicit typed Diameter/Radius retains its own length grammar and numeric authority.
- B04: Line Start/End Reshape keeps its original grip pivot in `PointRequest.base` while a transient `snap_reference_base` uses the stationary opposite endpoint for request-relative TAN/PER only.
- Registered Core tests cover centered request availability and base semantics. Registered Qt tests cover actual off-cursor Circle size OSNAP, actual TAN/PER snap source, preview/committed line endpoint geometry and original opposite endpoint unchanged. Existing tests also cover numeric/Grip Copy/Rotate negative compatibility; further exact-HEAD FULL and Owner manual checks remain mandatory.
- The #3595 native MID-hover failure **remains a historical genuine FAIL**, not a retroactive PASS. It did not recur in focused #3598 or in the exact same code/docs FULL #3599, with **no changes** to the nonallowlisted native test or global UI semantics. No general claim that future intermittent failure is impossible.
- No new public interface, native schema, authored snap/constraint, Part/OCCT source, global snap tolerance, S3 behavior or generalized rebind has been introduced.

## Remaining gates

- DOC-01 internal/PL+EN docs, generated Browser and no-diff verification were PASS on exact code/docs HEAD `5073307444785965860a5a29f5f09f5ff76b7c67`; a later evidence-only commit must preserve those blobs;
- FOCUSED #3598 and Ready FULL #3599 are PASS on exact code/docs SHA `5073307444785965860a5a29f5f09f5ff76b7c67`. FAST #3594 passed on prior source/test SHA; it cannot be called exact-head FAST for #3599;
- preserve the #3595 native MID-hover FAIL and the subsequent #3598/#3599 PASS evidence without removing, weakening or modifying the out-of-scope test;
- exact-head Windows CLOSURE after factual evidence synchronization;
- Owner manual Windows B03a/B04 acceptance, independent technical FINAL PASS, and separate guarded product merge authorization.

Nothing in this ledger asserts any unperformed 41-step manual transcript, Owner manual S2 PASS, technical FINAL PASS, or permission to implement S3/PM-06.

## 2026-10-17 — Owner manual Windows FAIL and bounded diagnostic follow-up

Owner reported **MANUAL FAIL** on the S2 candidate, with two GUI screenshots in the live handoff conversation: Circle Center→Size versus radius/diameter grip edit gave different apparent snap behavior (END/CEN/MID visible but TAN-to-Line/Circle not as expected); Line Grip Reshape displayed TAN but the cyan guide appeared non-tangent to Circle, while a hover near an EXT marker did not expose the expected PER. The two pictures are actual Owner observations, **not** proof of post-click authored endpoint coordinates. Owner did not certify Undo/Redo or Save/Reopen; those remain **UNTESTED**. Formal issue record: [PR #483 Owner manual FAIL comment](https://github.com/MechanicalCave/SimpleSolid2/pull/483#issuecomment-6239750120).

Scope clarification (no new product behavior silently inferred): R11 point-relative TAN creates contact points on Circle/Arc for a segment from a base, not on straight Line, and using a Circle's own Center as base does not itself give a Circle–Circle tangency constraint. Circle Center→Size **PER** against a finite Line constructs a radius perpendicular to that source and thus a geometrically tangent Circle. PER and TAN are disabled by default unless explicitly enabled/overridden. The B04 active moving-grip `PointRequest.base` is intentionally still the numeric/dynamic-input pivot, while exact TAN/PER uses the stationary opposite endpoint; the screenshot's cyan cursor guide is not confirmed to be an authored final line.

### New measured regression evidence; no false closure

- `da892c8ab0f03cea80fcf859a64d50eb4ca5be59` — [Windows FOCUSED #3601](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/39063660823): **PASS** existing registered `sk07f.precision_input_controller`. Added Circle creation size-stage one-shot TAN-to-Circle test and verified both native controller preview endpoint coordinates (moving Start TAN and moving End PER), not only final source geometry.
- `9b70fabbc3a83030772bf673474588d81bca61a8` — [Windows FOCUSED #3602](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/39063818001): **PASS** same registered test. Added negative TAN-to-finite-Line (no candidate/no commit) and valid PER-to-finite-Line Circle Center→Size (radius 50, original source unmodified). These are R11 behavior/grammar clarifications, **not** a verified runtime repair of the Owner's GUI failure.
- Product runtime source `src/sketch/**` and `src/ui/**` has **not** changed during this diagnostic follow-up; an in-scope code defect has **not yet** been reproduced. Existing #3599 FULL PASS belongs to prior code/test/docs SHA only, not to these test commits.
- Outstanding: repeat a real Windows GUI test with explicit TAN/PER enabled and known finite-foot/tangent contact, capture clicked grip role (Start or End), source and resulting endpoints/coordinates, preview vs accepted line, and exact build SHA. If the final accepted segment is non-tangent/non-perpendicular even under a valid request, fix a demonstrated in-scope controller defect test-first; otherwise avoid rewriting global snap priorities or inventing new Circle tangency constructors.
- Owner manual **FAIL stays open**, technical FINAL PASS/Owner GUI PASS/implementation merge authority all absent. PR remains DRAFT; protected main unmodified.

## 2026-10-17 — bounded post-Owner-FAIL CI checkpoint, NOT GUI approval

- Candidate code/test/docs SHA: `29b5dde23a2ed6661c18f29956b7bf73b9acfd45`; no runtime source modifications since code/docs `5073307444785965860a5a29f5f09f5ff76b7c67`. Only registered `tests/sk07f_precision_input_controller_test.cpp`, PL/EN/internal current-state docs, generated Browser and factual work ledger changed in this remediation iteration.
- [Windows FOCUSED #3601](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/39063660823) on `da892c8ab0f03cea80fcf859a64d50eb4ca5be59` PASS; [Windows FOCUSED #3602](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/39063818001) on `9b70fabbc3a83030772bf673474588d81bca61a8` PASS. These cover preview endpoints, Circle TAN-from-point against Circle, rejection of non-unique TAN-to-Line and valid Circle Center→Size PER-to-finite-Line.
- [Windows Draft FAST #3603](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/39064119728) **PASS** on `29b5dde23a2ed6661c18f29956b7bf73b9acfd45`.
- [Windows Ready FULL #3604](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/39064220736) **PASS 199/199** on the exact same SHA: Core 25/25, Kernel-native 57/57, Qt/Desktop 117/117 and aggregate `windows-msvc` SUCCESS. Official `.\\ss2.ps1 docs` regenerated the Browser with **no diff**, and DOC-01 verification PASS.
- Despite all automated checks passing, **Owner manual B03a/B04 FAIL remains unresolved** until fresh real Windows interaction demonstrates actual committed endpoints/radius and request-stage-specific OSNAP settings. Do NOT relabel this as Owner PASS or technical FINAL PASS. The screenshots do not establish a post-commit geometry mismatch, and the exact reported runtime sequence still needs differentiation from one-shot mode consumption, out-of-aperture finite foot, and numeric-pivot guide versus stationary-base final segment.
- Evidence-only current next HEAD should receive its own independent Windows CLOSURE; no product or documentation changes derive from this ledger edit. PR #483 remains DRAFT/UNMERGED; protected main `8c4c296916fa1796697c7ec503d4a2b60d06cbd1` stays unchanged; separate Owner manual, technical FINAL PASS and guarded merge authority are still required.

## 2026-10-17 — Owner aggregate Windows manual retest: PASS (supersedes previous FAIL for retested S2)

**Owner's actual report, verbatim:** `manual test  - pass - kontynuuu`. Received after the prior Owner manual FAIL/screenshots and after delivery of the bounded S2 retest instructions for current PR #483. Treat as **aggregate Owner manual PASS of the S2 B03a/B04 retest**, not as fabricated per-step observations. It supersedes the earlier *current blocker* for those rechecked features without erasing the historically real FAIL, screenshots, or unresolved interpretations recorded above.

**Candidate identification:** At the time of Owner report PR #483 was DRAFT, SHA `c30d150d8ff7a709bf652f3d017d4ea3f5480c97`. This was an evidence-only revision of source/test/docs candidate `29b5dde23a2ed6661c18f29956b7bf73b9acfd45` (equal runtime/test/docs blobs); no intervening changes or protected-main drift observed before recording. Owner did not paste `git rev-parse HEAD` with the report; SHA is the exact then-current candidate from the immediately preceding test instruction, **not** independently verified from Owner's machine.

**Manual coverage caveat:** The short Owner PASS does not contain itemized confirmations of specific TAN/PER branch choices, numerical pivot, negative cases, Undo/Redo, cold Save/Close/Reopen or S1/R11 controls. No 41-step transcript or independent on-device logs were supplied. Do not invent one. The Owner manual B03a/B04 retest status is PASS *as an aggregate declaration*; supporting automatic regression coverage is separately tied to GitHub runs below. Whether this aggregate attests the entire broader §6 manual checklist must be established explicitly before claiming a fully exhaustive independent technical FINAL PASS.

**Automation already complete before Owner retest:** FOCUSED #3601/#3602 PASS, FAST #3603 PASS, Ready FULL #3604 **199/199 PASS** (Core 25/25, Kernel 57/57, Desktop 117/117, DOC-01 and generated Browser no-diff) at source/test/docs `29b5dde23a2ed6661c18f29956b7bf73b9acfd45`. Evidence-only CLOSURE #3605 SUCCESS at `c30d150d8ff7a709bf652f3d017d4ea3f5480c97`. This new **Owner-manual-report ledger edit** requires its own exact-head CLOSURE before a final technical decision.

**Governance:** No merge authorization is implied by `kontynuuu`. Keep PR #483 DRAFT/UNMERGED and protected main untouched until independent technical FINAL PASS, exact-head lease/scope recheck and separate explicit Owner guarded-squash authority. S3/PM-06 remain independently gated.

## 2026-10-17 — Owner explicit full §6 manual-scope confirmation: PASS

**Owner clarification, verbatim:** `tak potwierdzam - pass`, responding specifically to: `czy Twój manual test – PASS obejmuje również Undo/Redo, Save/Close/Reopen i kontrolę wcześniejszych funkcji R11/S1, czy tylko ostatnie testy Circle OSNAP i Line Grip TAN/PER?` This explicitly confirms that **the previously declared Owner manual PASS also covers Undo/Redo, Save/Close/Reopen and prior R11/S1 regression checks**. Thus the previous aggregate manual-coverage ambiguity in this ledger is now resolved; this latest section supersedes that earlier *pending clarification*, not the historical Owner FAIL and its supporting evidence.

**Manual assessment:** Owner aggregate **B03a Circle Center→Size OSNAP PASS; B04 Line Grip TAN/PER PASS; Undo/Redo PASS; Save/Close/Reopen PASS; existing R11/S1 controls PASS**. These are scope categories expressly confirmed by Owner, **not** an assertion that detailed on-device coordinates, each negative case, all Arc orientations, or an invented 41-step log were supplied. The exact Git SHA was not separately copied from Owner's Windows machine; the then-current offered source/test/docs candidate remains `29b5dde23a2ed6661c18f29956b7bf73b9acfd45`, with only cumulative factual work-ledger changes since.

**Independent technical evidence already checked:** focused #3601/#3602 PASS, FAST #3603 PASS, Ready FULL #3604 **199/199 PASS** including Core 25, native Kernel 57, Qt/Desktop 117, generated Browser no diff and DOC-01 PASS. Closure #3605 was PASS on the former evidence SHA `c30d150d8ff7a709bf652f3d017d4ea3f5480c97` and #3606 was PASS on the immediately preceding Owner-aggregate evidence SHA `0b12e333fd5717cade46dd56b27f6b262ca5e8e0`. Owner-accepted S2 17-path allowlist remains observed; protected main `8c4c296916fa1796697c7ec503d4a2b60d06cbd1` was unchanged at this checkpoint.

**FINAL technical gate disposition:** S2 B03a/B04 meets functional/manual, regression, scope, numeric/identity, documentation and clean full-Windows CI requirements in the available independent evidence. The *new evidence-only HEAD* from recording this confirmation must receive independent exact-HEAD Windows CLOSURE SUCCESS before declaring unconditional **technical FINAL PASS**. No code/test/doc modifications are introduced by this evidence edit. Do not infer permission to squash product PR #483 from the Owner's `pass`; **separate guarded merge authorization and immediate premerge protected-main/HEAD/scope verification are mandatory**.

**Next program gate:** only after separate guarded product merge: protected-main confirmation and future S3 contract/Owner approval. This manual S2 acceptance does not activate S3, PM-06 or broaden global OSNAP.
