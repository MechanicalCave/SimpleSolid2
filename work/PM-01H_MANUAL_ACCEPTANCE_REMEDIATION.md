# PM-01H — Manual Acceptance Remediation

**Status:** ACTIVE REMEDIATION CHECKPOINT  
**Parent Work Contract:** `work/PM-01_FIRST_SOLID_VERTICAL_SLICE.md`  
**Authority:** ADR-0014 + ADR-0015  
**Baseline:** `ede4e6050d09bb492101eb41cd35464565a2c841`

## Trigger

Owner manual Windows acceptance on 2026-10-03 found reproducible defects before PM-01 completion.

Observed supported-profile matrix:

- Line-only rectangle: first Extrude Add works;
- whole Circle: first Extrude Add works;
- Circle with hole: first Extrude Add works;
- mixed Line + Arc profiles: first Extrude Add can be rejected or geometrically misinterpreted;
- Add preview can overlap committed geometry and z-fight;
- Cut preview does not communicate the subtractive result correctly and can tint the whole Body;
- cylindrical presentation is visibly faceted/flat-shaded;
- invalid/failed model state is not sufficiently visible in Tree.

PM-01 remains ACTIVE. The previous runtime/docs gates are evidence for the old candidate, not acceptance of these newly discovered defects.

## Progress

- **H1 COMPLETED — PASS:** PR #155, exact-head Windows FULL #1341 PASS on `b74dae97e3740c1a583ef2a56c1505ef271003e4`; merged to main as `b8b89c204afeadf42ff25bd99cf4a9279c242f64`.
- **H2 COMPLETED — PASS:** PR #156, exact-head Windows FULL #1342 PASS on `3dbe6b0de2ae7dd7805f132eea71fe51d184b0d0`; merged to main as `f80f5426dfb7c896f52a580505f328eea927da38`.
- **H3 COMPLETED — PASS:** PR #157, exact-head Windows FULL #1343 PASS on `931fa47e93333832778b56349bd8b6ee6ddfd64c`; merged to main as `78558b825f1ec2b0ad622166774774ac26e5f417`.
- **Owner re-test after H1/H2/H3:** PARTIAL PASS; remaining H4 findings are recorded below.
- **H4a COMPLETED — PASS:** PR #160, exact-head Windows FULL #1346 PASS on `818ed69a37ddee3a54be7b65c819cd67ab67dcc1`; merged to main as `af1800cbcde8f7b45b5c3cb2e10795f0b76e2a66`.
- **H4b COMPLETED — PASS:** PR #161, exact-head Windows FULL #1348 PASS on `86b5a4238635953f013ea10be35c8d21bbb07b03`; merged to main as `53bde160abeabd854d99fb96535c528f8402858f`.
- **Documentation sync COMPLETED — PASS:** PR #163, Windows DOCS #1350 PASS; latest main `f19b52f1cdc77ef1405aeb95a1a2ed042d9c8b02`.
- **Owner re-test after H4:** PARTIAL PASS; remaining H5 findings are exact operation-delta preview/Profile z-fighting and a Cut face-contact case incorrectly accepted as effect.
- **H5 ACTIVE:** exact Add/Cut delta preview + transient source-Profile hide + volumetric no-effect Cut.

## H1 — Profile → Kernel fidelity blocker

Goal: one canonical resolved Profile boundary meaning; the Kernel adapter must realize it without re-interpreting traversal semantics.

Scope:

1. add production regressions for first Add from mixed Line+Arc closed profiles;
2. cover Arc traversal in both source-forward and source-reverse boundary uses;
3. cover the same boundary through subsequent Add/Cut;
4. fix only the adapter/provider translation necessary to preserve resolved boundary orientation;
5. add a bounded Profile→Kernel face fidelity assertion where useful: loop cardinality/provenance and geometric executability;
6. no provider handle persistence, no fuzzy escalation, no alternate Profile semantics.

Acceptance:

- valid mixed Line+Arc Profile reaches a valid one-solid first Add when geometrically admissible;
- reverse boundary traversal produces the same intended curve segment with reversed orientation, not a different arc;
- existing rectangle/Circle/hole behavior remains green;
- zero false-Resolved lineage regression remains intact.

## H2 — Extrude preview and solid presentation blocker

Goal: preview must communicate the candidate operation without corrupting or duplicating committed Body presentation.

Scope:

- eliminate committed-body/preview z-fighting for Add;
- Cut preview must show a comprehensible candidate/result instead of only whole-Body alarm tint;
- verify preview clear/replace on parameter change, Finish, Cancel and stale context;
- improve curved-surface presentation so cylindrical surfaces do not appear as visibly flat polygon panels at normal working zoom;
- presentation policy remains non-semantic and cannot alter modeling results.

Acceptance:

- Add preview has no overlapping coplanar duplicate triangles;
- Cut preview communicates removed/result geometry consistently;
- committed Body presentation returns exactly after Cancel/Finish;
- cylinder rendering is visually smooth enough for normal interaction while remaining bounded.

## H3 — bounded PM-01 UX remediation

After H1/H2 PASS:

- move Body below Sketches in Part Tree;
- show clear visual Tree state for Invalid Profile and Failed/Blocked/Suppressed Feature;
- new Extrude draft defaults to a positive unit-aware distance equivalent to 10 mm in canonical authored units;
- support command-first Extrude: activating Extrude without a selected Profile enters an explicit "select one valid Profile" state; selection-first remains supported.

These are presentation/interaction changes only. They must not change durable Feature schema or modeling meaning.

## Deferred decision — Feature highlight

Selecting a Feature in Tree should improve identification in the viewport, but implementation is deferred until the visual meaning is fixed explicitly:

- source Profile highlight;
- Body state after that Feature;
- or geometric delta contributed/removed by that Feature.

Do not add provider topology picking or a speculative per-feature persistent geometry model merely to implement highlight.

## H4 — post-H1/H2/H3 refinement from Owner re-test

Owner re-test on the merged H1/H2/H3 candidate found four remaining presentation/interaction defects:

- typed Extrude Distance blocks on synchronous full candidate evaluation plus tessellation;
- full-body translucent preview is semantically too broad: committed Body should stay in its normal opaque presentation while only the current Extrude tool volume is preview-colored/translucent;
- denser tessellation does not solve curved shading because the current Viewer reconstructs every neutral triangle as an independent planar BRep face; cylindrical continuity therefore remains flat-shaded and the denser mesh only adds cost;
- Tree warning state is derived correctly but its evaluation snapshot is stale until a later full refresh such as Save.

### H4a — responsiveness + live diagnostics

- debounce only the expensive preview evaluation/tessellation triggered by QLineEdit typing;
- mutate the runtime Extrude draft immediately and invalidate stale Finish evidence immediately;
- Enter/Finish must flush the current debounced preview before commit;
- recompute Tree evaluation when authored DocumentRevision changes through Sketch interaction, not on selection-only runtime changes;
- add native warning icons for Invalid Profile, Failed/Blocked Feature and unavailable Body;
- no authored semantics or persistence changes.

### H4b — operation preview + smooth shading

- keep committed Body visible with normal opaque/default presentation during preview;
- preview only the current Extrude tool volume, colored by Add/Cut;
- final candidate Body evaluation still remains authoritative for Finish validity;
- restore the pre-H2 display tessellation density after smooth shading is implemented;
- render neutral solid triangles with smoothing continuity rather than rebuilding every triangle as an independent planar BRep face;
- preserve sharp edges by smoothing only within compatible local normal groups; do not alter modeled geometry, tolerances or topology identity.

## H5 — exact operation delta preview + volumetric no-effect

Owner re-test on latest main after H4 found two remaining PM-01 acceptance defects.

### H5a — exact operation delta preview

Accepted presentation semantics:

- committed Body remains in its normal opaque/default presentation;
- Add preview shows only material that would actually be added: `tool - upstream Body`;
- Cut preview shows only material that would actually be removed: `tool ∩ upstream Body`;
- preview tone is additive blue for Add and subtractive orange for Cut;
- when a valid solid preview is ready, the source Profile is hidden transiently to avoid coplanar z-fighting against a Body face; authored Profile visibility policy is unchanged and normal visibility returns on invalid preview, Cancel, Finish or context exit;
- full candidate Body evaluation remains authoritative for whether Finish is legal;
- delta preview is runtime-only derived geometry and must not enter persistence, semantic identity or topology picking.

### H5b — volumetric no-effect

PM-01 already requires explicit Failed for no-effect Add/Cut. H5b strengthens provider evaluation so geometric contact without removed/added volume cannot pass as a modeling effect.

For Cut:

- compute/check the volumetric common between upstream Body and Extrude tool;
- face/edge/point-only contact counts as zero volumetric effect and returns `no_effect`;
- only positive-volume intersection may continue to the Boolean Cut;
- Finish is disabled for `no_effect` and no Feature/Undo entry may be authored.

Existing post-Boolean unchanged-result checks remain a secondary invariant, not the sole no-effect detector.

Acceptance regressions:

- Add delta excludes the portion of tool already inside upstream Body;
- Cut delta equals only the upstream/tool volumetric overlap;
- Cut tool touching only a Body face is `no_effect`;
- Cut tool touching only an edge/point is `no_effect`;
- ordinary volumetric Cut remains accepted;
- source Profile does not z-fight while a valid preview is active and its authored visibility state is not mutated.

## Verification sequence

1. H1 focused kernel/core regressions.
2. H1 exact-head FULL before merge.
3. Owner re-test of the five representative Profiles.
4. H2 presentation/desktop regressions + exact-head FULL.
5. Owner preview/cylinder re-test.
6. H3 desktop interaction regressions + exact-head FULL.
7. repeat the final PM-01 manual acceptance matrix.
8. only then perform governance completion.

PM-02 and later packages remain inactive.
