# Sketcher S2 — SK07F Circle Size PointRequest Legacy-State Test — D2 Addendum PROPOSAL

**Status:** **D2 ADDENDUM PROPOSED FOR SEPARATE EXACT OWNER APPROVAL**; not integrated, not activated; no authority for out-of-allowlist test edits.
**Governing active work:** `work/SKETCHER_REPAIR_S2_POINT_REQUEST_OSNAP_GRIP_D2_WORK_CONTRACT_PROPOSAL.md` (Owner accepted exact §§0–8 and 16 conditional implementation paths; governance #479, active-only #480 fully integrated to protected main `cdc6f4b49569d10b6eb2115af27d800fb33b443e`).
**Decision class:** narrow D2 **test-path extension only**, not product input-grammar change or new snap policy.
**Evidence:** genuinely current-main Windows test-only RED #3583 FAST 92/93 B03a missing active PointRequest; #3584 FULL B03a/B04 2 Qt test RED; first scoped implementation candidate #483 Windows FAST #3586 **93/93 PASS** but Ready FULL [#3587](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/39055941394) **FAIL** at `tests/sk07f_precision_input_state_test.cpp:286`, legacy `CHECK(!circle.activePointRequest().has_value())`. The accepted S2 B03a contract explicitly requires an active centered PointRequest during Circle `await_radius`. This is an obsolete legacy assertion, not permission to avoid the new behavior. The FULL run stopped after Core 24/25; native/Qt final acceptance cannot be claimed for that SHA.

## 0. Decision boundary

The Owner's prior statement `masz moje wszystkie autoryzacje i akceptacje do rozpoczęcia pracy - kontynuuj` enabled the separately gated #481 correction/#480 activation and work within the **then-exact 16-path S2 contract**. It did not name or review this newly discovered 17th test path. Before changing this file, require **separate Owner acceptance of this exact addendum**, current-head governance CLOSURE, independently authorized guarded squash to protected main and a distinct single-file ACTIVE-only addendum-entry transition with its own CLOSURE and Owner guarded merge authority. Do not borrow general permission to start S2 to silently widen the allowlist.

## 1. Minimal additional allowlisted path

Add **exactly ONE existing registered Core test file**, conditionally:

- `tests/sk07f_precision_input_state_test.cpp` — ONLY obsolete Circle `await_radius` PointRequest expectation and directly adjacent S2 B03a positive/negative precision-input assertions. Replace the historical `!circle.activePointRequest()` statement with stronger explicit checks: active request exists; valid; base equals accepted Circle Center; supported relative pointer/numeric fields remain compatible; committed typed Radius/Diameter size remains numerically correct; invalid/zero radius, lock/cancel/next-tool and document-mutation semantics remain covered. Preserve and strengthen other existing test assertions, never delete or mark XFAIL.

Original **16 paths remain byte-for-byte permitted with their original semantic conditional roles**. This addendum would make 17 conditional implementation paths total, not authorize other tests such as unrelated `tests/sk07a*`, not introduce a new test/CMake target, and not relax coverage or R10/R11 snap order. Runtime source allowance, R11 shared-input owner, one request per interaction and B04 stationary opposite-grip snap semantics remain unchanged.

## 2. Red-to-green acceptance

1. Preserve the exact original FULL RED #3587 (registered `sk07f.precision_input_state` at line 286) with old assertion and SHA `3743eec8788012f02a6515e3e74523ea4c80e823`; do not reclassify it as source defect once B03a correctly changes the state.
2. Only AFTER new scope is activated, edit solely the allowed existing test file on a controlled new implementation revision and require the same test to assert positive B03a behavior while retaining legacy R/D numeric and cancellation controls.
3. Genuine FOCUSED registered `sk06a.circle_arc_interaction_state`, `sk07e.space_cycle_edit_mode`, `sk07f.precision_input_state`, `sk07f.precision_input_controller`, negative R11, full Windows FAST/Ready FULL Core/native OCCT/Qt, canonical PL/EN docs with generated Browser and DOC-01, Owner manual B03a/B04, independent technical FINAL PASS and separate guarded feature merge are required under the parent S2 contract.
4. Do not present test-expectation alignment as proof of actual end-to-end Circle OSNAP or Line Grip geometry; positive Qt/controller fixtures are independently required. No source/schema/public API/R10/R11/D2-A changes are licensed by this supplemental test path.

## 3. Explicit STOP and handoffs

- Any other out-of-allowlist file, broader numeric grammar, persistent PointRequest, non-original Grip numeric pivot, world tolerance rewrite or S3 feature requires independent Owner D2/D3 approval.
- This proposal changes one NEW `work/` governance file only. NO S2 product code/test merge, no direct protected main push, no activation, no altered accepted ADR/Roadmap.
- Owner exact addendum acceptance → exact-head Windows CLOSURE → separate guarded-squash authorization/merge → separate one-file `work/ACTIVE.yaml` **addendum history** entry (keep `active_work` pointing at accepted parent S2 contract) → separate Ready CLOSURE, Owner guarded merge → permitted test edit → real Windows evidence.

## Documentation impact

Internal docs: not required
User/Product docs: not required
Reason: This governance addendum only aligns one existing registered precision-input test with previously accepted S2 B03a runtime Circle Size PointRequest semantics. Canonical Shared 2D, OSNAP, paired PL/EN and generated Browser remain required by the existing S2 parent contract and implementation companion; this test allowlist correction itself changes no accepted runtime behavior and needs no independent product documentation.

## Decision log

2026-10-17 — After #479 exact Owner-approved contract and separately #480 active main integration, genuine Windows #3587 exposed one obsolete Circle size null-PointRequest assertion outside the exact 16-path conditional allowlist. **STOP on test edit**; propose bounded one-file Core registered-test path addendum for Owner review. Previous Owner blanket authorizations to begin work are not assumed to include this unforeseen path or waive explicit gates.
