# RG-02B1 / P4-S4A — native Qt/OCCT FULL concurrency isolation: test-registration D2 amendment

**Status: OWNER D2 ACCEPTED 2026-10-15 — governance PR #398 UNMERGED, separate ACTIVE-only extension PENDING; not executable until both guarded merges.**
**Proposed:** 2026-10-15. This file is **governance-only** and is not a change to registered test scheduling.
**Base active:** `work/RG-02B1_D2_P4_S4A_THREE_END_POINTS_V18_HEADLESS_WORK_CONTRACT_DRAFT.md` (Owner accepted; work #393, ACTIVE #394), plus exact five-file legacy fixture addendum (work #396, ACTIVE #397). Protected `main` prior to this proposal: `6121bcdf527c1384fab2e89d7d231b4f41e57ba7`.
**Production:** [#395](https://github.com/MechanicalCave/SimpleSolid2/pull/395), Draft/unmerged and **NO Owner technical FINAL PASS**.

## 1. Reproduced blocker, not a waiver

1. Windows Ready FULL [#3155](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38714198601), exact `a6402068cc5b1f3e7b1150766e31c38def8f547b`: **RED 198/199**, Core 25/25 PASS, kernel-native 57/57 PASS, Desktop 116/117, DOC-01 PASS. Sole failed historical unmodified `pm05f_r2.native_workbench_edit`: `tests/pm05f_r2_native_workbench_edit_test.cpp:552`, hidden active Operations widget in live Feature Edit.
2. Windows Ready FULL [#3156](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38715570901), exact `6c3d007ff5031eb2db56660e0ac55687e90a5f93`, differs only in S4A **evidence ledger**: again **RED 198/199** with all Core/kernel/native/schema/other Desktop tests green, DOC-01 PASS. Same native GUI executable failed a **different** existing assertion at `pm05f_r2_native_workbench_edit_test.cpp:2312`, a MID hover/click expected `Resolved MID` but remained `Click the highlighted MID Point to continue.`
3. The FULL CTest run uses `SS2_TEST_PARALLELISM=4`, and GitHub output shows `pm05f_r2.native_workbench_edit` starts **concurrently** with `wb01a.workbench_native_stress`, `pg01c.native_project_edge_command` and `pg01d.native_face_ui`; these use real native Windows Qt/OCCT and some reuse the same test executable in separate processes.
4. **Independent native FOCUSED #3158 PASS 1/1** on exact `6300d4c086ba1fbcdc2fb6ebb652d111cdc51fcd`: the **unchanged** registered `pm05f_r2.native_workbench_edit` executes the full real Qt/OCCT integration test in 40.49 s when compiled and run alone. All other product/test source code is unchanged from the two Ready FULL candidates; only accepted S4A evidence ledger synchronized.
5. These observations **support but do not prove** scheduling interference in concurrent Windows native GUI tests; there remains a possible nondeterministic UI/product defect. Therefore **no test is disabled, asserted PASS under FULL, or accepted as a known harmless flake**. Two failures require a real scheduling isolation experiment to distinguish cause.

## 2. Exact D2 test-registry-only change proposed

Add **one** additional source path to current S4A acceptance allowlist:

`tests/CMakeLists.txt`

Allow exactly this one behavior change in that file, with *no* other edits: set a shared **CTest `RESOURCE_LOCK ss2_native_qt_occt_workbench`** on these already-registered existing test names:

- `wb01a.workbench_native_stress`
- `pm05f_r2.native_workbench_edit`
- `pg01c.native_project_edge_command`
- `pg01d.native_face_boundary_d0`
- `pg01d.native_face_ui`
- `pg01d.native_face_fail_closed`

Using the native CTest `RESOURCE_LOCK` property lets the full Windows CTest tier retain `--parallel 4` for independent work while preventing the above real Qt/OCCT Workbench/Viewer tests from executing simultaneously. Preserve **all** original `add_test` registrations, native test binaries, `LABELS`, timeouts, arguments, counts and source assertions; **no** `RUN_SERIAL` blanket for unrelated targets and no skip/exclude/quarantine/retry/allow-failure. This is a scheduling-isolation **hypothesis to verify**, not a preapproved workaround for red tests. Do not assert it fixes the issue until observed under true independent Ready FULL.

No modifications to `.github/workflows/**`, `scripts/**`, other `tests/**`, `src/**`, native Part format v18, modeling geometry/tolerance, Qt UI/Viewer, kernel provider, DOC-01, Roadmap/ADR, S4B, or new test targets. In particular do not alter `tests/pm05f_r2_native_workbench_edit_test.cpp` or remove/relax the Operations or MID END hover checks. Existing separate CI-04 FAST build-vs-label membership issue is documented but **not** repaired under this narrow authority.

## Owner decision (2026-10-15)

In direct answer to the requested **exact one-file, six-existing-test `RESOURCE_LOCK` amendment #398** and conditional authorization for its guarded squash plus separate ACTIVE-only activation, Owner stated **„akceptuję - kontynuuj”**. This accepts the bounded D2 test-scheduling experiment (not a result or exception to FULL), and conditionally authorizes the two governance merges **only when** their exact-head Windows CI and protected-main preflight checks pass. Does not authorize modifying the Qt/OCCT test body, UI/Viewer, the kernel, the FAST build-target mismatch, or merging S4A product #395 without its own technical FINAL PASS.

## 3. Finite acceptance — red stays red until proven otherwise

- Before modification: both Ready FULL #3155/#3156 RED exact failure positions preserved; native isolated FOCUSED #3158 PASS. Provenance of source/test/docs SHAs retained.
- Owner accepts exactly this D2 one-file, six-test `RESOURCE_LOCK` proposal; governance-only PR passes exact-head Windows closure and separately Owner-authorized protected-main squash.
- Separate `work/ACTIVE.yaml`-only history amendment receives independently authorized governance closure and protected-main squash, retaining existing S4A `active_work`.
- Only then production PR #395 may add the bounded registration property in `tests/CMakeLists.txt`. Keep test counts and assertions unchanged.
- Run real Windows kernel-native FOCUSED (existing three-point headless), native GUI FOCUSED (unmodified PM05F R2), FAST, **Ready FULL Core+Kernel+Desktop 199/199 PASS**, official DOC-01 and exact-head ledger. FULL logs must verify the affected native GUI cases actually run and are serialized; no pass from skipped tests. If FULL still RED (or any other new native failure), treat as real blocker, **STOP for a new concrete Owner D2 decision**, never weaken GUI tests or classify failure as flake.
- Owner S4A **technical FINAL PASS/FAIL** and separately authorized exact-head squash of product PR #395 remain mandatory. S4B existing `Datum Plane` UI/Command Line third constructor remains a wholly different contract.

## Documentation impact

Internal docs: none for pure test scheduling; product PL/EN docs: none. Existing S4A v18 DOC-01/Browser output must remain exactly reproducible after the test-only experiment.
