# E2 — Presentation Scale and Provider Failure

**Status:** ACCEPTED — ACTIVE  
**Proposed:** 2026-09-28  
**Owner acceptance:** 2026-09-28  
**Decision class:** bounded D2 presentation-consistency contract under AUDIT-01; implementation remains D1 while provider-neutral API/ownership is unchanged; any differential-update protocol or `IDocumentViewport` contract expansion requires a separate Owner decision  
**Foundation:** 1.0 (foundation-v1.0)  
**Program authority:** `work/AUDIT-01_ARCHITECTURE_STABILIZATION_PROGRAM.md` v1.0 — Package E2  
**Baseline:** `main` at `164487d553722e1139be4f5ac143c81b133fa4a5` after completed Package E1

## 1. Goal

Measure the current authored-scene refresh and pointer-preview presentation cost on the supported Windows/Qt/OCCT stack, make provider presentation failure explicit and recoverable without rolling back authored CAD state, and decide from evidence whether the current rebuild strategy may be retained.

E2 is a presentation/runtime stabilization package. It does not change Sketch authored meaning, picking grammar, persistence, history, region/profile semantics or solid modeling.

## 2. Confirmed baseline findings

### 2.1 Full authored Sketch presentation is rebuilt

`PartViewportController::refreshPresentation()` rebuilds a complete provider-neutral `SketchScene` from the active authored Sketch.

`buildSketchScene()` clears and recreates the `EntityId ↔ PresentationToken` runtime bindings and allocates new presentation tokens for the scene.

The Qt/OCCT provider `setSketchScene()` clears the current Sketch presentation, creates new AIS objects for every Line and every sampled segment of Circle/Arc presentations, then redraws.

**Finding: CONFIRMED as implementation behavior; scale impact is NOT YET MEASURED.**

### 2.2 Curves are sampled presentation only

Circle/Arc authored geometry is converted to finite presentation segments in the UI/Viewer path. The provider may create multiple `AIS_Line` objects for one semantic curve token.

Those segments are derived renderer data and are already mapped back to one semantic `PresentationToken`.

**Finding: ALREADY FIXED with respect to semantic ownership.** E2 must preserve this boundary. Viewer chords/tessellation never become region/profile geometry.

### 2.3 Pointer preview does not currently rebuild the authored scene

Creation/direct-manipulation pointer movement calls the dedicated preview path (`setSketchPreview*` / `setSketchGeometryPreview`) rather than `refreshPresentation()`.

The Qt/OCCT provider nevertheless clears and recreates the complete transient preview scene for each accepted preview update.

**Finding: authored-scene isolation is ALREADY FIXED; transient preview rebuild cost is NOT YET MEASURED.**

### 2.4 Provider exceptions are contained but presentation failure is not surfaced coherently

The Qt/OCCT boundary uses guarded wrappers that catch provider exceptions, log them and return `false`. Scene replacement code also clears partially created provider objects on exception.

However, `PartViewportController::refreshPresentation()` currently discards the bool results of `setReferenceScene()` and `setSketchScene()`. Several clear/projection paths similarly ignore provider-setter failure.

A semantic command can therefore commit successfully while redraw fails — which correctly leaves the authored model authoritative — but the UI has no explicit presentation-degraded state or user-facing recovery diagnostic.

**Finding: CONFIRMED.**

## 3. Authoritative consistency rule

The following AUDIT-01 rule is binding:

- Command/Validation/Transaction/Domain commit is authoritative.
- Provider presentation happens after or outside the authored mutation.
- A redraw/presentation failure must **not** roll back a successful durable command.
- Document revision, dirty state, Undo/Redo and durable identity remain exactly as committed.
- Presentation failure is runtime-only.
- UI must surface that presentation is stale/unavailable.
- Recovery means rebuilding presentation from the current authored model; it never means reconstructing authored state from provider objects.

## 4. Scope IN

- measure single authored-mutation refresh cost;
- measure pointer-preview update cost;
- record provider-neutral scene size and Qt/OCCT native object/segment implications;
- deterministic provider-call-count tests proving pointer preview does not rebuild unchanged authored scene;
- deterministic failing-viewport tests for authored commit + presentation failure consistency;
- explicit runtime presentation failure reporting in the current UI adapter;
- retry/recovery by a later full refresh from authored state;
- ensure a successful later refresh clears the runtime failure state/diagnostic;
- preserve semantic picking by EntityId/PresentationToken mapping;
- internal as-built docs;
- Product PL/EN failure/recovery documentation because E2 adds a visible failure diagnostic;
- generated Browser;
- normal Windows FULL and native provider/stress regression.

## 5. Scope OUT

- region/profile semantics;
- any use of Viewer tessellation as geometric truth;
- Part/Sketch authored representation changes;
- persistence/schema changes;
- Undo/Redo/history changes;
- stable durable presentation IDs;
- persisting `PresentationToken`;
- OCCT topology identity;
- replacing OCCT;
- redesigning the Viewer architecture;
- changing pointer/selection grammar;
- changing Circle/Arc sampling as geometry semantics;
- speculative GPU renderer work;
- automatic differential updates before measurement;
- a public `IDocumentViewport` differential-update API in the first E2 implementation phase.

## 6. Phase A — measurement and characterization first

After Owner acceptance, Phase A must land before any performance optimization.

Use a dedicated Release measurement target/harness on the supported Windows runner. Reuse the existing Qt/OCCT native setup patterns; do not turn a timing benchmark into a normal pass/fail unit test based on machine-specific latency.

Record at minimum:

- compiler/OS/CPU/logical CPU count/RAM;
- build type;
- warmup/sample counts;
- semantic entity count;
- derived provider segment/object count where observable or deterministically computable;
- median, p95 and max elapsed time;
- operation kind.

Required operation classes:

1. **single authored mutation refresh** — one accepted semantic mutation followed by the same production `refreshPresentation()` path;
2. **native full scene replacement** — provider `setSketchScene()` for equivalent scene sizes;
3. **single-line pointer preview update** — repeated production preview replacement without authored-scene refresh;
4. **transform preview update** — preview geometry scaling with selected semantic entity count.

Required scale matrix must include at least small, medium and materially larger current-Sketch sizes. Exact counts may be adjusted before activation only to avoid runner timeout, but the proposal target is approximately 100 / 1,000 / 5,000 semantic entities, with at least one mixed Line/Circle/Arc workload that exposes curve-segment multiplication.

A bounded cutoff is allowed for a cell that would otherwise exceed the CI timeout. Record the cutoff explicitly rather than hanging the gate.

## 7. Phase A behavioral call-count evidence

Timing alone is insufficient.

A counting/fake `IDocumentViewport` test must prove:

- one ordinary Line/Circle/Arc pointer-move preview update does not call `setSketchScene()`;
- common-transform/COPY/direct-manipulation preview does not call `setSketchScene()`;
- accepted semantic commit causes the expected authored refresh;
- unchanged authored geometry is not rebuilt merely because the pointer moved;
- preview failure does not create authored mutation.

This classifies the AUDIT-01 preview-rebuild concern independently of native timing noise.

## 8. Performance decision gate

E2 does **not** pre-authorize a differential-update architecture.

After Phase A, classify current rebuild scale as one of:

**KEEP CURRENT REBUILD**  
Measured behavior is acceptable for the currently supported Sketch scale and no correctness issue requires differential updates. Record the scale risk and retain the current full-scene replacement design.

**OPTIMIZATION PROPOSAL REQUIRED**  
Measured behavior is materially problematic enough to justify changing update architecture.

If optimization would require any of the following, stop for a second explicit Owner decision before production optimization:

- new public `IDocumentViewport` mutation methods;
- provider-neutral incremental add/update/remove protocol;
- stable runtime token preservation across scene refresh;
- new subsystem ownership;
- provider object caching whose identity participates in semantic behavior.

A small private no-op suppression that merely avoids re-sending an exactly unchanged transient scene may be proposed with before/after evidence, but it must not become semantic identity.

No latency threshold in this contract is a Product guarantee.

## 9. Provider failure behavior

E2 completion must make presentation failure explicit without expanding authored semantics.

Preferred bounded implementation uses the existing bool-returning Viewer boundary and a UI-level runtime failure reporter; it must not require provider exceptions to escape Qt/OCCT.

Required behavior:

1. semantic command commits through the existing transaction/history path;
2. presentation update is attempted;
3. if a provider setter returns false:
   - the authored command remains committed;
   - revision/dirty/Undo state remain unchanged from the successful command;
   - the controller marks/report presentation failure at runtime;
   - UI displays a bounded diagnostic that the model change is committed but presentation update failed;
4. the next normal full presentation refresh is a retry from current authored state;
5. after a complete successful authored/reference presentation refresh, the degraded runtime state clears;
6. repeated failure remains visible but does not create CAD history or dirty-state changes.

Do not silently claim a stale provider scene is synchronized.

## 10. Provider partial-failure containment

Current Qt/OCCT scene replacement clears partial provider objects on exception and guarded wrappers return failure.

E2 must retain fail-closed provider containment.

Tests should use a deterministic fake failing viewport for controller/UI semantics. Do not depend on provoking a real OCCT exception in CI.

Native smoke/stress remains required to prove normal provider operation is unchanged.

## 11. Selection and picking invariants

Any E2 change must preserve:

- authored identity is `SketchId + EntityId`;
- `PresentationToken` is runtime-only and disposable;
- multiple native curve segments map back to one semantic token;
- provider order never chooses semantic primary;
- point/rectangle/grip query behavior remains unchanged;
- selection survives a successful full refresh according to current semantic selection state, not provider object identity.

If an optimization cannot preserve these invariants, stop.

## 12. Tessellation boundary for Package F

E2 must add/retain explicit documentation that:

- Circle/Arc sampled point chains and AIS line segments are presentation data only;
- their segment endpoints do not define closure, intersections, loops, regions or profiles;
- Package F must analyze exact accepted authored/evaluated 2D geometry outside the Viewer.

This is a hard prerequisite for later profile semantics.

## 13. Expected implementation surface after acceptance

Expected bounded files may include:

- `src/ui/part_viewport_controller.hpp/.cpp`;
- minimal `src/ui/cad_workbench.cpp` status adapter wiring;
- existing `src/viewer/include/.../document_viewport.hpp` only if no contract/API shape change is needed; otherwise stop;
- native measurement harness under `benchmarks/` or a dedicated non-default benchmark target;
- tests with counting/failing fake viewports;
- existing native viewer/workbench stress tests where regression coverage is needed;
- `tests/CMakeLists.txt` / benchmark CMake registration;
- temporary/one-shot benchmark CI only if required to collect reproducible Release evidence;
- `docs/internal/CAD_WORKBENCH_VIEWER.md`;
- `docs/internal/BUILD_AND_TEST.md` if benchmark workflow is added;
- paired Product PL/EN documentation for the visible presentation-failure/recovery message;
- generated `docs/browser/index.html`;
- E2 lifecycle records under `work/`.

No Part persistence or Sketch semantic production file is expected.

## 14. Verification

E2 completion requires:

- Phase A Release measurement evidence for authored refresh and pointer preview;
- call-count evidence that pointer preview does not rebuild authored scene;
- current rebuild strategy classified KEEP or escalated through a separately accepted optimization proposal;
- deterministic failing-viewport test proving successful command remains committed when presentation fails;
- revision/dirty/Undo invariants across failure;
- successful later refresh recovery test;
- user-facing diagnostic test;
- existing picking/selection regressions green;
- native Qt/OCCT smoke and WB-01A stress green;
- internal docs and Product PL/EN docs current;
- Browser freshness green;
- exact-head Windows FULL;
- work-only CLOSURE;
- merge to main.

Manual GUI verification is not required if normal successful presentation behavior is unchanged and the failure path is covered by deterministic injected-provider tests. Any normal visible interaction change or new recovery control requires Owner manual verification before closeout.

## Documentation impact

Internal docs: required  
User/Product docs: required  
Reason: E2 records presentation performance/failure ownership internally and introduces an explicit user-visible diagnostic/recovery meaning for the rare case where authored CAD commits successfully but the Viewer cannot update.

## 16. Stop conditions

Stop for Owner review before an unapproved production expansion if E2 would require:

- public/provider-neutral differential update API;
- stable/persistent presentation identity;
- new durable state or schema;
- changing authored command success based on redraw success;
- rolling back a committed command because presentation failed;
- semantic geometry derived from Viewer chords/tessellation;
- changed selection/picking grammar;
- renderer replacement;
- performance limits presented as Product guarantees;
- provider-specific types crossing into Core/Sketch/Part/Application.

## 17. Activation record

**Owner acceptance:** 2026-09-28

E2 is active on `proposal-e2-presentation-scale-provider-failure`.

The Owner additionally directed that verification be consolidated to the necessary minimum. This permits combining multiple required assertions into the smallest practical number of deterministic test executables/targets, but does **not** remove any evidence required by Section 14 and does not permit weakening existing regressions.

Active execution order:

1. Phase A measurement/call-count characterization before optimization;
2. provider-failure consistency/recovery within Sections 3 and 9 using the existing bool-returning provider boundary;
3. classify performance as KEEP CURRENT REBUILD or OPTIMIZATION PROPOSAL REQUIRED;
4. stop for a second explicit Owner decision before any differential-update/public Viewer API expansion.

## 18. Completion boundary

After E2 completion, AUDIT-01 schedules Package F next.

E2 completion does not activate F, profile tooling, region semantics, Extrude or any solid-modeling feature.


## 19. Phase A evidence and performance decision

**Measured SHA:** `401629b8cc4f943e6ad5577918bb1999256fdc37`  
**Windows FULL / Release benchmark:** #727 — PASS  
**Ordinary CTest:** 77/77 PASS  
**Durable results:** `work/E2_PRESENTATION_BENCHMARK_RESULTS.md`

The E2 call-count characterization was consolidated into the existing `sk03a.part_viewport_controller` test; no new CTest executable was added.

Measured native replacement/preview showed an approximately 33 ms floor from one object through roughly 1,000 line objects on the measured runner, rising to roughly 83 ms around 4,000–5,000 native presentation objects. The complete accepted-mutation + authored-refresh production path measured about 100 ms median at 100/1,000 lines and 150 ms at 5,000 lines. Exact values and environment are retained in the durable results file.

**Performance classification: KEEP CURRENT REBUILD.**

The measurements do not justify a differential-update/public Viewer API redesign in E2. Large and highly segmented Sketches remain a documented scale risk, and the measured values are not Product performance guarantees.

Provider-failure consistency/recovery remains the active E2 implementation work. Package F remains inactive.
