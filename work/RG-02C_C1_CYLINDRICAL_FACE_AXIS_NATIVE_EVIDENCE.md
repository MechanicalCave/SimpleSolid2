# RG-02C/C1-P0 — native cylindrical Face evidence

**Status:** REAL WINDOWS READY FULL **199/199 PASS** on exact code/test+initial-report SHA `a7242dc19522216f59890edcd94d9365c1ea3fad`; Owner P0 technical acceptance and protected-main merge **NOT YET GRANTED**. Keep independent FOCUSED/FAST as not separately run. Native analytic Face-axis extraction remains GAP (not implemented).

## Scope

Read-only, additive assertions in existing registered native OCCT test `tests/kernel_native/pm04c2_production_revolve_test.cpp`, per Owner-accepted C1-P0 contract and activated `work/ACTIVE.yaml`. Reuse actual resulting single-Body circular Extrude, full and partial Revolve, half-cylinder Boolean Cut and circular-rim Fillet/Chamfer fixtures. Observe actual stage-scoped current Face-to-semantic Surface candidates, `SurfaceKind::cylinder`, other Face kinds, carrier partitions and unsupported/nonunique candidates. Never derive an Axis from operation type or runtime tokens.

**Not proven:** exact cylinder infinite-axis origin/direction/radius, cross-topology source survival, native provider query, persistence, Axis construction, UI, or a positive cylindrical *straight-edge* Fillet if not represented by the reused fixture. Classifying a cylinder does not extract its axis. Circular-edge Fillet may be toroidal; report actual observed type, never infer from operation name.

## Validation — genuine Windows run and scope-specific interpretation

- Exact source/test/initial ledger SHA: `a7242dc19522216f59890edcd94d9365c1ea3fad`.
- Real [Windows Ready FULL #3320](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38876053562) completed **SUCCESS**: Core **25/25**, native Release Kernel/OCCT **57/57**, full Desktop Qt/OCCT **117/117**, total **199/199 PASS**, no skipped FULL count. Actual selected `pm04c2.production_revolve` **Passed (0.10 s)** in the Kernel group, and full graph compiled the changed test source `pm04c2_production_revolve_test.cpp` under MSVC. Official `SS2 documentation verification PASSED` logged.
- The new C1 observer's assertions ran in that real native PM04C2 executable: **at least one current uniquely linked actual cylinder Face and one noncylindrical Face in each** existing full circular Extrude, full Revolve, 90° partial Revolve and half-cylinder Boolean Cut Body fixture. It checked a selected Face has exactly one eligible current-stage semantic Surface carrier, the carrier's native `SurfaceKind::cylinder`, strict Face vs multi-Face partition status and invalid empty-stage semantic reference. Circular-rim Fillet/Chamfer geometries were observed but not required to produce cylinders, since the round-edge Fillet surface can be toroidal.
- **Count caveat:** CTest's default non-verbose successful test output printed `Passed`, not individual `RG02C_C1_P0_FACE_CATALOG` per-Face count lines. Therefore no invented per-fixture surface cardinalities or a claimed positive **straight-edge cylindrical Fillet** witness. A test-level assertion PASS is the proven scope, not proof of every status branch being exercised. Ambiguous/missing/unsupported Face admission and wrong-stage *consumer* rejection are **NOT independently executed negative API fixtures** because C1 has no consumer/provider query yet.
- Real Windows native [FOCUSED #3323](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38878177638) **SUCCESS, 1/1 PASS** at exact SHA `d65decd0a717f02dce8231c102b6d4f83c542d22`: MSVC built the pre-registered Release target `pm04c2_production_revolve_test` and executed exactly `pm04c2.production_revolve` (0.10 s). This is a true independent native focused witness, unlike prior CLOSURE #3322, which is recorded only as docs validation. Draft FAST separate job: **NOT RUN / not counted**. The real Ready FULL includes the actual C1 native test and the full Core/Kernel/Desktop graph but does not retroactively turn skipped FOCUSED/FAST into separate passes.
- This evidence-ledger-only successor HEAD requires its own Windows **CLOSURE**; **PENDING** until completed.
- Owner P0 technical FINAL PASS and protected-main production/test squash: **NOT REQUESTED / NOT AUTHORIZED**.

Preserve first RED and GAP evidence. Only promote any status after checking exact CI logs on the corresponding SHA.

## Documentation impact

Internal docs: not required
User/Product docs: not required
Reason: This P0 test and work-only evidence report do not change shipping product semantics or UI. A later C1 product feature requires new DOC-01 work under an independent Owner contract.
