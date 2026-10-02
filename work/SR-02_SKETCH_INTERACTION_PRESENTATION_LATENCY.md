# SR-02 — Sketch Interaction & Presentation Latency Stabilization

**Status:** ACTIVE  
**Proposed:** 2026-10-02  
**Owner acceptance:** 2026-10-02  
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

The accepted pre-activation audit also confirmed three concrete pieces of repeated work that Phase A must measure explicitly:

- `QtOcctViewerWidget::querySketchPresentations()` accumulates semantic rectangle-query hits in a vector and performs a linear token lookup for every presentation segment. With 5,000 distinct Line tokens this implies `0 + 1 + ... + 4,999 = 12,497,500` token comparisons for one full query pass. This is a static algorithmic count, not a latency measurement.
- Profile hover can call `part::applyProfileAreaEdit()`, which performs its own `sketch::analyzeRegions(model)` even when the controller's outer Profile-analysis cache is fresh. `resolveProfileRegionIntent()` can also fall back to region analysis on invalid boundary reconstruction. The local Profile cache rebuild counter is therefore not a complete measure of region-analysis work.
- common-transform preview currently resolves the same pointer event before preview-stage dispatch and again inside `updateCommonTransformPreview()`. Removing this duplication is allowed only with event-sequence tests because pointer resolution updates OSNAP/OTRACK runtime state.

These findings strengthen the measurement plan; they do not pre-authorize a particular implementation beyond the D1 boundaries below.

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

Measure the complete user-relevant pipeline, not only isolated setter duration.

At minimum measure:

- Line pointer event → semantic resolution/snap/query work → preview geometry → provider scene submission → final redraw completion;
- the same full path for Circle and Arc preview;
- one common-transform/direct-manipulation preview from input event through transform calculation and final redraw;
- accepted Sketch mutation → authoritative authored refresh → final redraw completion;
- `Finish Sketch` request → transient teardown → final non-edit scene → final redraw completion;
- Profile hover with no draft and with an existing draft, including all region-analysis work reached through controller, `applyProfileAreaEdit()` and any fallback resolution paths;
- representative rectangle/nearby queries at increasing scene sizes.

The end of a provider setter is not automatically the end of presentation latency. If rendering is deferred or coalesced, evidence must include the execution of the final redraw and must verify that the intended frame becomes visible. Setter-return timing may be recorded separately but cannot substitute for event-to-visible evidence.

Record for each scenario, where practical:

- semantic entity count;
- derived presentation segment/native-object count;
- total `resolvePointerInput` invocation count per logical event;
- total `analyzeRegions` invocation count across the complete call path;
- Profile hover cache hit/miss count;
- static snap cache hit/miss count;
- nearby/rectangle-query candidate count, presentation segments scanned and semantic-token comparisons where relevant;
- provider scene-set call count;
- `UpdateCurrentViewer` and `Redraw` call counts;
- semantic/controller CPU time, provider/presentation time and complete event-to-visible elapsed time where measurable;
- p50/median, p95 and max over a bounded but meaningful sample set.

Where p95 is reported, sample count must be materially more representative than the historical E2 3–7 sample timing cells; exact sample counts remain D1 but must be recorded with the result and justified if bounded by runner timeout.

Measurements must distinguish semantic/controller work from provider/presentation work enough to prevent optimizing the wrong layer.

## 6. Phase A workloads

Use deterministic fixtures that include, where the scenario is applicable:

- approximately 100 / 1,000 / 5,000 Line entities so current results can be compared with E2 scale evidence;
- a mixed Line/Circle/Arc fixture that exposes presentation-segment/native-object multiplication;
- a dense-intersection fixture that exercises intersection-capable snapping/query behavior;
- a Profile-capable closed-region fixture with no draft;
- a Profile-capable fixture with an existing draft and repeated hover in Add/Subtract modes;
- transform/direct-manipulation preview;
- the Owner-reported workflow: active drawing → Finish Sketch → re-enter Sketch Edit.

Record cold-cache and warm-cache behavior separately where caching applies.

The existing E2 benchmark harness may be reused or extended, but E2's prepared-geometry transform preview is not sufficient by itself because SR-02 requires the current full input → snap/query → transform calculation → presentation path.

Exact fixture sizes and sample counts are D1 if runner constraints require adjustment, but results must retain semantic entity counts, native/derived object counts and enough samples to support any reported percentile honestly.

## 7. Optimization rule

No production optimization is accepted solely from static inspection.

Each changed hot path must have:

- a measured or deterministic call-count baseline;
- a specific hypothesis tied to its measured share of delay or proven repeated work;
- a bounded implementation;
- equivalent semantic/result tests;
- before/after evidence on the same scenario.

Optimization order is driven by measured contribution to user-visible delay. The list below is a risk-minimizing default, not a requirement to optimize a low-impact cache before a dominant redraw or query cost.

An optimization that merely moves cost elsewhere, makes a setter return earlier while the visible frame remains late, or changes interaction semantics is not an SR-02 success.

## 8. Authorized optimization order

After Phase A, prefer the smallest independently measurable changes that remove proven repeated work. Re-measure after each class before escalating.

### 8.1 Remove proven repeated work first

Allowed and specifically targeted:

- resolve one common-transform/direct-manipulation preview event once and pass that resolved semantic result through the remaining transform-preview path; do not silently re-resolve the same event;
- replace the current quadratic semantic-token accumulation in rectangle query with a fast accumulator while preserving stable result order and exact window/crossing semantics;
- cache static snap candidate catalogs while authored state and relevant snap eligibility/settings are unchanged;
- avoid full `SketchModel::state()` copies used only to prove freshness when an authoritative generation/revision key can prove the same condition;
- reuse already-computed Profile analysis and Profile-hover results when the complete semantic cache key is unchanged.

Removing a duplicate resolver requires regression evidence for event sequences, snap capture, OTRACK acquisition/hysteresis and temporary overrides because pointer resolution mutates runtime inference state.

Replacing token accumulation must preserve deterministic token/result ordering. An unordered lookup may be used as an index only if output order remains equivalent to the baseline.

### 8.2 Cache keys and invalidation are semantic preconditions

A cache keyed only by a raw revision number is insufficient.

At minimum semantic/runtime caches must bind the relevant combination of:

- owning Document/session context identity;
- active `SketchId`;
- authoritative Document revision or equivalent Sketch generation;
- relevant OSNAP/OTRACK/Polar/DYN or tool-mode settings;
- any request-local override or eligibility state that changes the computed result.

Profile hover reuse additionally binds:

- current draft `RegionIntent` or absence of draft;
- hovered/target region identity/index as applicable;
- Add/Subtract mode;
- any Profile presentation/runtime option that changes the computed hover result rather than presentation only.

A screen-space projection or spatial index additionally depends on camera/projection state, viewport dimensions and device-pixel-ratio/DPI. A Profile draft or edit-session change has its own invalidation even when authored revision is unchanged.

Caches are runtime-only. Cache contents never become authored identity, persistence or topology authority.

### 8.3 Coalesce private presentation work without hiding latency

SR-02 may introduce one private provider-side render-request/coalescing mechanism if Phase A shows redundant flushes.

Allowed behavior:

- setters synchronously validate and update their runtime scene/object state;
- exact no-op scene replacement may be suppressed;
- multiple render requests created by one logical interaction may be coalesced into the necessary final redraw;
- private object replacement may be batched before that redraw;
- semantic pointer/event processing is **not** dropped or coalesced merely to reduce rendering work.

Deferring rendering is allowed only while E2 provider-failure reporting remains truthful. If a setter can return success before a later redraw that may fail and the existing bool-returning boundary can no longer report that failure coherently, STOP for Owner review. A new public completion callback/batch API or changed `IDocumentViewport` success semantics is D2.

Performance evidence must measure final redraw/visible completion, not only faster setter return.

### 8.4 Conservative spatial acceleration only after re-measurement

Do not introduce a spatial index merely because a full scan exists.

After the repeated-work and render-coalescing changes above are measured, a private runtime spatial prefilter/index may accelerate remaining nearby/rectangle queries if:

- evidence shows query scanning is still material;
- it is conservative and cannot omit an entity that the existing full scan would consider;
- exact semantic snap eligibility/ranking remains the final arbiter;
- no fixed "first N objects" cutoff is used;
- it is rebuilt/invalidated deterministically from current authored/presentation/camera state;
- it is not persisted;
- provider/runtime identity remains non-authoritative.

Tests must cover camera/zoom/DPI changes, window/crossing selection and equal/ranking-sensitive hits where applicable.

A changed snap result, aperture meaning, selection result or exact intersection semantics is outside D1.

### 8.5 Finish Sketch presentation batching

Leaving Sketch edit should not visually dismantle the scene in multiple externally visible stages when one coherent runtime transition can produce the same final state.

SR-02 may:

- prepare transient preview/selection/profile-edit cleanup before requesting the final frame;
- avoid intermediate redraws during teardown;
- perform one final authoritative refresh from the current Document after edit context closes;
- preserve the existing final selection/visibility/camera semantics.

Success requires both no externally visible intermediate teardown frame **and** acceptable measured latency. Replacing several visible stages with one equally long blocking pause is not by itself a performance success.

No authored operation may be deferred, skipped or fused across transaction/history boundaries merely to improve appearance.

### 8.6 Native object multiplication

If remeasurement shows Circle/Arc segmentation/native-object multiplication still dominates after lower-risk work, SR-02 may apply bounded presentation-only optimization inside the existing provider contract.

Prefer reducing native-object count by grouping/aggregating the already-derived segments before changing curve sampling density, because changing segmentation can alter current selection/query approximation and requires a stronger equivalence proof.

If optimization requires a new public curve primitive protocol, new durable sub-element identity or changed semantic picking, STOP for Owner review.

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

- current-stack Windows event-to-visible latency/call-count instrumentation and durable benchmark evidence;
- pointer-resolution hot-path profiling and removal of same-event duplicate resolver work;
- linear-or-better semantic-token accumulation for rectangle query while preserving deterministic result order;
- static snap candidate cache/reuse where evidence supports it;
- removal of unnecessary hot-path full-state copies;
- complete Profile-analysis/hover freshness optimization, including work reached through `applyProfileAreaEdit()`, without topology change;
- conservative private spatial prefilter/index for presentation/entity queries only after remeasurement justifies it;
- transient preview no-op suppression/coalescing;
- private Qt/OCCT redraw/update batching that preserves E2 failure reporting;
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
- one logical common-transform/direct-manipulation preview event invokes semantic pointer resolution once after the accepted optimization;
- the one-resolver path preserves OSNAP/OTRACK/Polar/DYN result, ranking, capture/acquisition/hysteresis and temporary-override behavior for representative event sequences;
- rectangle-query semantic token accumulation is linear-or-better in token lookup work and produces the same ordered token/result set as the baseline for window and crossing rules;
- cached static snap candidates are reused while the complete context/revision/settings key is unchanged and are invalidated on every relevant change;
- cached and uncached snap resolution produce identical semantic result/ranking for representative fixtures;
- total `analyzeRegions` call counts are observed across the entire Profile-hover call path, not only controller cache rebuilds;
- repeated Profile hover with unchanged Document/Sketch revision, draft, target region and Add/Subtract mode performs no new region analysis after the accepted cache/reuse optimization;
- cached and uncached Profile hover produce identical RegionCandidate/Profile diagnostic/composition truth;
- any later spatial prefilter returns a conservative candidate superset/equivalent result versus brute-force semantics across camera/zoom/DPI and window/crossing cases;
- one logical transient presentation batch does not create redundant provider redraw/update work after the accepted optimization;
- exact no-op preview suppression creates no stale visible state;
- accepted authored mutation still refreshes from authoritative Document state;
- Finish Sketch leaves the same final authored/presentation/selection/camera state as baseline and emits no intermediate visible teardown frame after batching;
- provider rejection still leaves authored revision/dirty/history state committed and recoverable exactly as E2 requires;
- deferred/coalesced presentation, if used, cannot lose a provider failure because the setter returned before the failing work;
- no runtime cache/index survives Document/Sketch/context/camera replacement incorrectly;
- full existing desktop regressions remain green.

## 15. Performance evidence and acceptance

SR-02 completion requires a durable before/after result file on the supported Windows runner.

The evidence must show:

- baseline and final exact SHAs;
- machine/build configuration;
- matched before/after scenario/workload sizes, including approximately 100/1,000/5,000 Lines where applicable, mixed curves, dense intersections, Profile-with-draft and drawing → Finish → re-enter;
- cold-cache and warm-cache measurements where caching applies;
- p50/median, p95 and max where timing is meaningful, with sample counts recorded;
- semantic/controller CPU time and provider/presentation time where separable;
- complete event-to-final-redraw/visible elapsed time for interactive scenarios;
- resolver, region-analysis, query, setter, `UpdateCurrentViewer` and `Redraw` call counts relevant to the scenario;
- presentation segments scanned, token-comparison counts and native/derived object counts where material;
- a written explanation of which costs improved and which remain.

No single timing number is a Product guarantee, and historical E2 ~33 ms observations must not be described as an unavoidable OCCT floor unless current evidence actually isolates such a floor.

The package is accepted only if the reported interactive lag and Finish Sketch staging are materially improved in the Owner's manual Windows verification **and** the measurements show that the change reduced the intended runtime work rather than merely making an API return earlier.

A coherent Finish Sketch with no intermediate frames is necessary but not sufficient: one long blocking pause is still a latency problem.

If bounded D1 changes do not materially improve the Owner workflow and the remaining cost requires a public/differential Viewer redesign, stop with evidence rather than introducing unaccepted architecture.

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

- owning Document/session context change;
- authored revision/generation change;
- Sketch context change;
- Profile draft/edit-session or Add/Subtract-mode change where relevant;
- OSNAP/OTRACK/Polar/DYN/tool/request-local settings change where relevant;
- camera/projection, viewport-size or device-pixel-ratio/DPI change for screen-space caches/indexes;
- Document switch/close;
- Viewer/provider reset.

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
- a new public render-completion callback/batch contract or changed setter-success semantics required to make deferred redraw failure reporting truthful;
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

The Owner explicitly accepted SR-02 on 2026-10-02 together with the pre-activation audit amendments recorded in this contract: full event-to-visible measurement, complete Profile-hover region-analysis accounting, one-resolver transform-preview target, deterministic fast token accumulation, complete cache-key invalidation, redraw-completion evidence and evidence-driven optimization ordering.

The synchronized activation candidate must update `work/ACTIVE.yaml` and Roadmap v1.9 current state to SR-02 and pass repository work/governance verification before any performance-changing production implementation.

After that gate passes, Phase A measurement is the first production task. Optimization work must follow the measured contribution to latency and remain inside the D1 boundaries above.

Completion requires:

- durable current-baseline and before/after measurement evidence;
- accepted bounded optimizations with semantic-equivalence regressions;
- required internal documentation;
- exact-head Windows FULL;
- Owner manual Windows PASS;
- work-only CLOSURE closeout.

After SR-02 completion, production scope stops again. SR-03 does not activate automatically and still requires its own accepted Work Contract.
