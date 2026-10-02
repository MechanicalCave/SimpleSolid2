# SR-02 — Sketch Interaction & Presentation Latency Stabilization

**Status:** PROPOSED / INACTIVE  
**Proposed:** 2026-10-02  
**Owner acceptance:** pending  
**Decision class:** bounded D1 measurement/runtime optimization under existing ownership; STOP for D2 if a public Viewer mutation protocol, differential authored-scene architecture, new subsystem ownership or CAD semantic change is required  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Architecture:** ADR-0003, ADR-0006, ADR-0008, ADR-0009, ADR-0010, ADR-0011, ADR-0012  
**Program roadmap:** `work/SKETCH_ROADMAP.md` v1.9  
**Baseline:** `main` at `61961192390401dc7ecbb26634f5bd15b56d461e` after completed SR-01  
**Milestone:** pre-Part stabilization — SR-02 of SR-01 → SR-02 → SR-03 → readiness re-test

## 1. Goal

SR-02 addresses the concrete post-R12/SR-01 report that Sketcher interaction can feel delayed while drawing and that finishing a Sketch can be visibly slow or staged.

The package is deliberately measurement-first:

1. reproduce and quantify current event-to-presentation latency on the supported Windows/Qt/OCCT stack;
2. identify which existing runtime stages dominate the reported lag;
3. apply only bounded optimizations that preserve all current authored semantics, command/history rules, selection grammar and Viewer ownership;
4. record before/after evidence on the same runner and representative workloads;
5. stop for Owner review if satisfactory improvement requires a new public differential/incremental Viewer architecture.

SR-02 does not promise a Product latency SLA and does not authorize broad renderer redesign by default.

## 2. Authority and preserved architecture

The Engineering Constitution, Foundation, accepted ADRs and completed E2 presentation contract remain authoritative.

In particular:

- authored CAD state remains authoritative over presentation;
- Shared 2D owns Sketch geometry/edit semantics;
- Part owns hosted Sketch/Profile meaning;
- pointer, snap, hover, preview, selection, caches, spatial acceleration and Viewer objects are runtime-only;
- mutation remains Command → Validation → Transaction → owning Document → Evaluation;
- provider failure never rolls back an accepted authored mutation;
- Viewer/provider tokens remain ephemeral presentation identity only;
- exact snapping, region/profile and topology meaning may not be approximated for speed;
- no cache or acceleration structure may become persistence or semantic authority.

E2's KEEP CURRENT REBUILD decision remains the historical baseline, not a prohibition on later evidence-driven private optimization. SR-02 may optimize private implementation after current-stack measurement, but it may not introduce a public differential-update protocol without a new Owner decision.

## 3. Confirmed baseline risks to measure

The current codebase contains several plausible hot paths that must be measured rather than assumed to be the sole cause:

- `SketchModel::state()` materializes complete Line/Circle/Arc state vectors by value;
- pointer snap resolution rebuilds static semantic candidates from the active Sketch and may request nearby entities on ordinary pointer movement;
- the Qt/OCCT nearby-presentation query scans current Sketch presentation objects and intersection-capable paths may amplify candidate work;
- Circle/Arc presentation and preview are segmented into many transient/native line objects;
- preview scene replacement clears/recreates provider objects;
- provider scene setters may trigger viewer update/redraw work per logical runtime update;
- Profile analysis currently has runtime cache logic whose validation may still copy/compare full Sketch state;
- accepted Sketch mutations still perform a full authored `refreshPresentation()`;
- leaving Sketch edit clears several transient presentation surfaces and then performs the final non-edit refresh, which may explain staged visual teardown.

These are investigation hypotheses, not pre-approved root-cause conclusions.

## 4. Historical E2 evidence

E2 measured the earlier presentation stack on the supported Windows runner and classified the then-current design KEEP CURRENT REBUILD.

Durable E2 results include approximately:

- ~33 ms native replacement/preview floor from one object through about 1,000 Line objects;
- ~83 ms native replacement/preview around 4,000–5,000 native presentation objects;
- ~100 ms median accepted mutation + authored refresh at 100/1,000 Lines;
- ~150 ms median accepted mutation + authored refresh at 5,000 Lines;
- mixed Line/Circle/Arc presentation visibly multiplying native objects.

Those measurements predate the later R10–R12 interaction stack and SR-01. SR-02 must therefore remeasure the current pipeline before choosing optimization work.

## 5. Phase A — current-stack measurement

Before performance-changing production code, create durable measurement evidence for the current SR-01-complete baseline.

Measure at minimum:

- pointer move → resolved semantic point/candidate → preview submission → provider presentation completion for Line;
- the same for Circle and Arc preview;
- a representative selection/direct-manipulation or common-transform preview;
- accepted Sketch mutation → authored presentation refresh;
- `Finish Sketch` request → transient teardown → stable non-edit presentation;
- Profile hover/analysis interaction where Profile tooling is active.

Record for each scenario, where practical:

- semantic entity count;
- derived presentation segment/object count;
- relevant cache rebuild/hit counts;
- nearby-query candidate count;
- provider scene-set call count;
- provider redraw/update count;
- median, p95 and max elapsed time over a bounded sample set.

Measurements must distinguish semantic/controller work from provider/presentation work enough to prevent optimizing the wrong layer.

## 6. Phase A workloads

Use deterministic fixtures that include:

- small ordinary engineering Sketch;
- medium Line-heavy Sketch;
- medium mixed Line/Circle/Arc Sketch;
- a larger bounded fixture sufficient to expose scaling without exceeding normal CI timeout;
- at least one Profile-capable closed-region fixture;
- the Owner-reported manual workflow: active drawing followed by Finish Sketch.

The existing E2 benchmark harness may be reused or extended.

Exact fixture sizes are D1, but results must retain semantic entity counts and derived/native object counts so old E2 evidence and current evidence can be compared honestly.

## 7. Optimization rule

No production optimization is accepted solely from static inspection.

Each changed hot path must have:

- a measured or deterministic call-count baseline;
- a specific hypothesis;
- a bounded implementation;
- equivalent semantic/result tests;
- before/after evidence.

An optimization that merely moves cost elsewhere or changes interaction semantics is not an SR-02 success.

## 8. Authorized optimization order

After Phase A evidence, optimize in this order unless measurements clearly disprove an earlier hypothesis.

### 8.1 Revision-keyed semantic/runtime caches

Allowed:

- cache static snap candidate catalogs by active `SketchId`, current Document/Sketch revision-equivalent generation and relevant snap-mode set;
- reuse cached read-only semantic data across pointer samples while authored state and eligibility inputs are unchanged;
- invalidate deterministically on authored mutation, Sketch/context change or settings change;
- key Profile analysis cache by authoritative revision/generation data rather than copying/comparing complete Sketch state when equivalent freshness can be proven.

Caches are runtime-only. Cache contents never become authored identity, persistence or topology authority.

### 8.2 Avoid unnecessary full-state copies

Where a hot path currently calls `SketchModel::state()` only to inspect unchanged authored geometry, SR-02 may replace that use with:

- existing direct semantic lookup;
- bounded const traversal/view helpers exposing the already-authored Line/Circle/Arc entities;
- revision-keyed cached derived data.

Any new read-only Shared-2D accessor must expose existing semantic entities only. It must not create a second model, alternate identity or mutable bypass around commands/transactions.

### 8.3 Conservative spatial acceleration

A private runtime spatial prefilter/index may accelerate nearby Sketch presentation/entity queries if:

- it is conservative and cannot omit an entity that the existing full scan would consider;
- exact semantic snap eligibility/ranking remains unchanged after candidate reduction;
- it is rebuilt/invalidated from current authored/presentation state deterministically;
- it is not persisted;
- provider/runtime identity remains non-authoritative.

A changed snap result, changed aperture meaning or changed exact intersection semantics is outside D1.

### 8.4 Coalesced transient presentation

SR-02 may reduce redundant clear/set/redraw work so one logical pointer sample causes only the necessary final transient presentation update.

Allowed private changes include:

- suppressing exact no-op transient scene replacement;
- batching internal provider object replacement before one redraw/update;
- avoiding multiple viewer flushes generated by one logical preview/snap/inference state update;
- reusing private runtime provider objects where their identity has no semantic meaning.

This may not change public `IDocumentViewport` ownership or make provider objects stable CAD identity.

### 8.5 Finish Sketch presentation batching

Leaving Sketch edit should not visually dismantle the scene in multiple externally visible stages when one coherent runtime transition can produce the same final state.

SR-02 may:

- clear transient preview/selection/profile-edit overlays as one bounded runtime transition;
- avoid redundant intermediate redraws;
- perform one final authoritative refresh from the current Document after edit context closes;
- preserve the existing final selection/visibility/camera semantics.

No authored operation may be deferred, skipped or fused across transaction/history boundaries merely to improve appearance.

### 8.6 Native object multiplication

If Phase A confirms Circle/Arc segmentation/native-object multiplication materially contributes to latency, SR-02 may apply bounded presentation-only optimization inside the existing provider contract.

Allowed examples include private batching/aggregation or adaptive derived presentation detail that preserves the accepted visual/selection semantics.

If this requires a new public curve primitive protocol, new durable sub-element identity or changed semantic picking, STOP for Owner review.

## 9. Public Viewer architecture boundary

SR-02 does **not** authorize by default:

- new public incremental add/update/remove methods on `IDocumentViewport`;
- a provider-neutral differential authored-scene protocol;
- stable runtime token preservation as a semantic requirement;
- persistent provider-object caches;
- a second authored scene/model;
- new ownership shared between UI and Viewer.

If Phase A/B evidence shows these are necessary for acceptable interaction, record the evidence and stop. A separate D2 proposal must freeze that architecture before implementation.

## 10. Interaction semantics that must not change

Performance work must preserve:

- current Select/LMB/Ctrl/window/crossing grammar;
- current OSNAP/OTRACK/Polar/Dynamic Input precedence and exact semantic ranking;
- current command-first and selection-first workflows;
- current Line/Circle/Arc/Rectangle/Move/Copy/Rotate/Scale/Mirror/Trim/Extend/Extend Both semantics;
- SR-01 Delete and explicit Delete Profile arbitration;
- current Profile/RegionIntent meaning and fail-closed topology behavior;
- Undo/Redo granularity and revision-bound transactions;
- text-editor/global CAD input ownership;
- current provider-failure reporting/recovery contract.

Faster but semantically different is a failure.

## 11. Scope IN

- current-stack Windows latency/call-count instrumentation and durable benchmark evidence;
- pointer-resolution hot-path profiling;
- static snap candidate cache/reuse where evidence supports it;
- removal of unnecessary hot-path full-state copies;
- Profile-analysis freshness optimization without topology change;
- conservative private spatial prefilter/index for presentation/entity queries;
- transient preview no-op suppression/coalescing;
- private Qt/OCCT redraw/update batching;
- Finish Sketch transient teardown/final-refresh batching;
- bounded presentation-only Circle/Arc native-object optimization if measured necessary;
- automated semantic-equivalence and call-count regressions;
- Windows before/after benchmark;
- Owner manual responsiveness verification;
- internal documentation and work evidence.

## 12. Scope OUT

- SR-03 responsive Workbench shell/layout;
- public differential/incremental Viewer API without a new D2 decision;
- persistent caches or presentation identity;
- authored geometry/schema changes;
- new Sketch entities or curve kinds;
- new snap modes or changed snap ranking/meaning;
- new topology/healing/tolerance semantics;
- Profile RegionIntent changes;
- constraints/solver/dimensions;
- projected/reference geometry;
- ordinary Select RMB context redesign;
- Axis/Centerline semantics;
- Split/Join;
- Part Feature Tree;
- solid modeling / Extrude;
- asynchronous/background authored mutation;
- Product latency guarantees.

## 13. Expected implementation surface

Expected bounded surfaces include:

- `src/ui/part_sketch_interaction_controller.*`;
- `src/ui/part_viewport_controller.*`;
- `src/viewer_qt_occt/qt_occt_viewer_widget.cpp`;
- existing Viewer/provider-private helpers;
- `src/sketch/**` only for bounded read-only traversal/cache-support helpers that expose existing semantics without new ownership;
- affected performance/regression tests;
- existing or extended benchmark tooling;
- `work/**` durable measurement/results;
- affected `docs/internal/**`.

Broad `cad_workbench.cpp` UI composition/layout changes belong to SR-03 and are not expected here.

## 14. Required automated evidence

At minimum, prove:

- passive pointer movement still does not rebuild unchanged authored Sketch scene;
- cached static snap candidates are reused while revision/context/modes are unchanged and are invalidated on every relevant change;
- cached and uncached snap resolution produce identical semantic result/ranking for representative fixtures;
- any spatial prefilter returns a conservative candidate superset/equivalent result versus the existing brute-force semantics;
- Profile analysis cache freshness produces the same RegionCandidate/Profile diagnostic truth as the baseline implementation;
- one logical transient preview sample does not create redundant provider redraw/update work after the accepted optimization;
- exact no-op preview suppression creates no stale visible state;
- accepted authored mutation still refreshes from authoritative Document state;
- Finish Sketch leaves the same final authored/presentation/selection/camera state as baseline while reducing redundant intermediate presentation work where measured;
- provider rejection still leaves authored revision/dirty/history state committed and recoverable exactly as E2 requires;
- no runtime cache/index survives Document/Sketch context replacement incorrectly;
- full existing desktop regressions remain green.

## 15. Performance evidence and acceptance

SR-02 completion requires a durable before/after result file on the supported Windows runner.

The evidence must show:

- baseline and final exact SHAs;
- machine/build configuration;
- scenario/workload sizes;
- median/p95/max where timing is meaningful;
- relevant call counts;
- native/derived object counts where material;
- a written explanation of which costs improved and which remain.

No single timing number is a Product guarantee.

The package is accepted only if the reported interactive lag and Finish Sketch staging are materially improved in the Owner's manual Windows verification **and** the measurements show that the change reduced the intended runtime work rather than hiding it.

If measured work is already dominated by an unavoidable provider frame floor and bounded D1 changes do not materially improve the Owner workflow, stop with evidence rather than introducing unaccepted architecture.

## 16. Manual Windows verification

Owner manual verification must include:

1. the same representative Sketch workflow in which drawing previously felt delayed;
2. repeated Line/Circle/Arc pointer preview with OSNAP/OTRACK behavior unchanged;
3. selection and at least one Move/Copy/direct-manipulation preview;
4. Profile hover/edit smoke on a closed-region Sketch;
5. repeated Finish Sketch / re-enter Sketch Edit cycles, checking that disappearance/refresh is no longer visibly staged beyond what the final presentation requires;
6. generic Delete, explicit Delete Profile, Ctrl+Z/Ctrl+Y and text-editor keyboard ownership smoke from SR-01;
7. Save → Close → Reopen regression.

The Owner reports PASS/observations against the final exact runtime candidate.

## 17. Persistence and lifecycle

SR-02 introduces no authored schema change.

All new caches, indexes, counters, timing instrumentation and provider batching state are runtime-only.

They must be cleared or invalidated on the appropriate:

- authored revision/generation change;
- Sketch context change;
- Document switch/close;
- Viewer/provider reset;
- relevant runtime settings change.

No performance structure is serialized into CAD documents.

## Documentation impact

Internal docs: required  
User/Product docs: not required unless implementation changes a documented user-visible behavior beyond reduced latency/smoother presentation  
Reason: SR-02 is primarily an internal runtime/performance stabilization package with frozen user semantics.

Internal documentation must describe:

- measured hot paths and retained ownership boundaries;
- cache/index invalidation rules;
- any provider redraw batching/no-op suppression;
- Finish Sketch presentation sequencing after optimization;
- durable before/after evidence and remaining scale risks.

If any Product documentation wording changes, PL/EN parity and regenerated deterministic Product Browser become required.

## 19. Delegated D1 tuning

The Owner may delegate after activation:

- exact benchmark fixture sizes/sample counts;
- exact private cache data structures;
- exact conservative spatial index implementation;
- exact internal instrumentation counters/timers;
- exact Qt/OCCT private batching mechanism;
- exact derived Circle/Arc presentation detail within existing semantic/picking contract;
- exact local helper extraction;
- exact regression/benchmark file placement.

D1 may not alter CAD semantics, snap ranking/meaning, persistence, public Viewer ownership or transaction/history boundaries.

## 20. Stop conditions

Stop for Owner review if implementation requires or attempts:

- a new public `IDocumentViewport` mutation API;
- provider-neutral differential authored-scene protocol;
- stable provider/runtime token identity as semantic state;
- persistence of caches/indexes/presentation state;
- changed authored Sketch/Profile semantics;
- changed OSNAP/OTRACK/Polar/DYN eligibility or ranking;
- new topology/tolerance/healing behavior;
- changed RegionIntent/profile truth;
- asynchronous authored mutation or new concurrency ownership;
- SR-03 layout work;
- Part Feature Tree or solid modeling.

## 21. Activation and completion boundary

This file is proposal-only and inactive.

No SR-02 production implementation is authorized until the Owner explicitly accepts this exact Work Contract and the synchronized activation candidate updates `work/ACTIVE.yaml` to SR-02 and passes repository work/governance verification.

After activation, Phase A measurement is the first production task. Phase B optimizations must follow the evidence and remain inside the D1 boundaries above.

Completion requires:

- durable current-baseline and before/after measurement evidence;
- accepted bounded optimizations with semantic-equivalence regressions;
- required internal documentation;
- exact-head Windows FULL;
- Owner manual Windows PASS;
- work-only CLOSURE closeout.

After SR-02 completion, production scope stops again. SR-03 does not activate automatically and still requires its own accepted Work Contract.
