# PM-02 — Final Acceptance Matrix

**Status:** POST-REMEDIATION AUTOMATED + DOCS PASS; OWNER WINDOWS RE-TEST PENDING  
**Work Contract:** `work/PM-02_BODY_SEMANTIC_TOPOLOGY_FACE_SUPPORTED_SKETCH.md`  
**Final post-remediation runtime candidate:** `b9e70c684c2d054da21662ef3a8ae56ac59f60c0`  
**Final post-remediation runtime gate:** Windows FULL #1473 — PASS  
**Merged post-remediation runtime main:** `7baf13b255f7e5a587a8caeffde19d0ab69434d0`  
**PM-02J activation main:** `e28c756318e59cb68a28c375ddb96fc19d7ea67f`  
**Documentation candidate:** `65919f28f8c8dade4ba2904ec699957f9077d1f1`  
**Documentation gate:** Windows DOCS #1454 — PASS (`.\\ss2.ps1 docs` produced zero Browser diff)  
**Merged documentation main:** `300459629658432bb94f938e51cb1df4a17b353e`  
**Post-remediation documentation candidate:** `1dbaef95b2ea48f8e529425a154b795984d6551f`  
**Post-remediation documentation gate:** Windows DOCS #1474 — PASS (`.\\ss2.ps1 docs` zero diff)  
**Merged post-remediation documentation main:** `bdb718f292f1432e1156219edfa664b1a8aab92c`  
**Owner Windows manual acceptance:** initial FAIL on 2026-10-05; automated remediation PASS; re-test PENDING — see `work/PM-02J_MANUAL_ACCEPTANCE_REMEDIATION.md`

## 1. Gate interpretation

PM-02J does not add new modeling semantics. It closes documentation and final acceptance for the already-implemented PM-02A..PM-02I production slice.

The original pre-manual runtime gate was FULL #1451. Owner manual acceptance exposed bounded defects, now remediated through J-R2..J-R5. The current post-remediation runtime authority is exact candidate `b9e70c684c2d054da21662ef3a8ae56ac59f60c0`, Windows FULL #1473 PASS. R2/R3/R4 exact evidence is #1470/#1471/#1472 respectively.

PM-02 completion still requires:

1. canonical internal documentation current;
2. PL/EN product documentation current and structurally paired;
3. Product Browser regenerated from canonical Markdown;
4. final documentation/closure verification;
5. Owner Windows manual workflow PASS.

## 2. Automated evidence summary

| Area | Requirement closed | Final evidence |
| --- | --- | --- |
| PM-02A | complete evaluated Body-stage topology catalog; stage freshness; rectangular Add accounting | candidate `58368a6ada5f6c30e03cb845682dbf5408ecb763`, FULL #1397 PASS |
| PM-02B | Surface/Face semantics; planar carrier frames; split/delete/similarity guardrails | candidate `05434516fea0892aac2babae09da772043987f29`, FULL #1400 PASS |
| PM-02C | Edge/Curve + Vertex/Point semantics; representation-artifact accounting; branch ambiguity | candidate `9135baf1d749f01eeb5dcae4f6ea7cffc856d8a0`, FULL #1402 PASS |
| PM-02D | atomic topology-aware Body presentation; picking; View Styles; Properties; Feature Contribution | candidate `c325fcf681afff44ca61abb02fd239495db5d67f`, FULL #1421 PASS |
| PM-02E | schema v9 support persistence; v8→v9 migration; deterministic support frames | candidate `8b5e2c6844c93648e1884464d138d9a98e2a7a9a`, FULL #1431 PASS |
| PM-02F | exact-stage Sketch/Profile evaluation; Missing/Ambiguous/Unsupported; no stale frame consumption | candidate `87d4043f9114952df8228681e4cf16937c6f664b`, FULL #1434 PASS |
| PM-02G | planar Face Sketch create/re-support; one Finish transaction; cycle/stale rejection; GUI/CLI parity | candidate `f45ed8ee09aabe098ab88bc7e47a0c614841fb40`, FULL #1443 PASS |
| PM-02H | existing Extrude Add/Cut from face-backed Profile; stage-aware preview; upstream recompute/failure | candidate `0540a762be42b3f4329e7032545e652b21332486`, FULL #1446 PASS |
| PM-02I | Undo/Redo, Delete interactions, persistence/cold rebuild, repair, survival matrix, zero false Resolved | candidate `b642a5af625a48d668a21efeaf3d78bfb9bf7c79`, FULL #1451 PASS |

## 3. Contract acceptance requirement mapping

### Persistence and identity

- v8→v9 migration preserves DocumentId/BodyId/FeatureId/SketchId/EntityId/ProfileId — **PASS**, PM-02E / #1431.
- semantic Sketch support persists while absolute derived world placement does not — **PASS**, PM-02E / #1431.
- Save/Close/Reopen and true cold rebuild reconstruct support and Add/Cut chain from fresh runtime/provider tokens — **PASS**, PM-02I / #1451.
- provider identity, topology catalog, evaluated frames and Viewer tokens are not serialized — **PASS**, PM-02E + PM-02I.
- runtime DocumentRevision is not treated as persisted CAD identity — **PASS**, PM-02I / #1451.

### Topology accounting and semantic references

- canonical rectangular Add stage accounts 6 Faces / 12 Edges / 8 Vertices — **PASS**, PM-02A.
- chained Add/Cut stages maintain complete current topology accounting — **PASS**, PM-02A..C.
- Face/Surface, Edge/Curve and Vertex/Point remain distinct semantic levels — **PASS**, PM-02B/C.
- material Faces/Edges/Vertices are present and inspectable; representation artifacts remain accounted but are not ordinary material-pick authority — **PASS**, PM-02C/D.
- split semantic Surface may remain Resolved while strict bounded Face becomes Ambiguous — **PASS**, PM-02B/I.
- deleted support becomes Missing; alias/collapse ambiguity remains Ambiguous — **PASS**, PM-02B/I.
- identical or near-identical replacement geometry does not steal identity — **PASS**, PM-02B/C/I.
- zero false Resolved across accepted survival matrix — **PASS**, PM-02B/C/I.

### Viewer, selection and Properties

- committed Body shading and Face/Edge/Vertex presentation come from one current RuntimeSolid/evaluation generation — **PASS**, PM-02D.
- Body scene installs atomically with runtime-only generation; stale generation/token reuse cannot commit — **PASS**, PM-02D.
- Viewer never reconstructs CAD identity from triangle adjacency — **PASS**, PM-02D.
- one current Edge presentation backs edge rendering and picking — **PASS**, PM-02D.
- View Style supports Shaded / Shaded + Edges / Shaded + Hidden Edges and is non-authored — **PASS**, PM-02D.
- hidden-edge display does not enable occluded select-through — **PASS**, PM-02D.
- ordinary all-kind acquisition priority is visible Vertex → Edge → Face with bounded DPI-aware apertures — **PASS**, PM-02D.
- active-tool kind filters remain acquisition filters; semantic admissibility remains Part-owned — **PASS**, PM-02D/G.
- multiple visible candidates cycle by Tab/Shift+Tab without provider-order identity authority — **PASS**, PM-02D.
- stale preselection/candidate stack cannot commit across scene/evaluation replacement — **PASS**, PM-02D/G.
- topology Properties report carrier, producer/stage/provenance, referenceability and diagnostics without exposing provider identity — **PASS**, PM-02D.
- Body accounting and Feature Contribution Properties match current semantic queries — **PASS**, PM-02D.
- Sketch Properties expose authored support intent + current Resolved/Missing/Ambiguous/Unsupported state — **PASS**, PM-02D/F.
- hover/candidate cycling does not become Properties authority until committed selection — **PASS**, PM-02D.
- deleted direct topology selection clears rather than geometry-rebinding — **PASS**, PM-02D.
- diagnostic resolved-prefix presentation is visibly non-authoritative and cannot feed successful command/reference truth — **PASS**, PM-02D/F/H.

### Feature Contribution and overlays

- tree hover shows temporary Current Feature Contribution — **PASS**, PM-02D.
- tree selection shows persistent Current Feature Contribution without historical-stage substitution — **PASS**, PM-02D.
- split Surface contribution highlights all current fragments as a set-valued display rather than singular ambiguity — **PASS**, PM-02D.
- Cut contribution highlights direct Cut-created topology without stealing upstream Surface ownership — **PASS**, PM-02D.
- Feature Contribution and support/carrier overlays reuse current topology presentation and do not create duplicate selectable geometry — **PASS**, PM-02D.
- command-owned solid preview remains separate from committed Body reference identity — **PASS**, PM-01/PM-02D/H.

### Face-supported Sketch lifecycle

- planar cap support — **PASS**, PM-02G/H.
- planar lateral support — **PASS**, PM-02G.
- Cut-exposed planar support — **PASS**, PM-02G.
- non-planar Face remains acquireable but standard Sketch support returns Unsupported — **PASS**, PM-02G.
- strict Face split with surviving Surface keeps Surface-backed Sketch valid — **PASS**, PM-02B/I.
- upstream dimensional edit moves derived world frame while local U/V is unchanged — **PASS**, PM-02F/H.
- no stale last-good frame after support loss/ambiguity — **PASS**, PM-02F/H/I.
- re-support preserves SketchId/EntityIds/local U/V — **PASS**, PM-02G/I.
- self/downstream support cycle is rejected before mutation — **PASS**, PM-02G.
- stale revision/session/evaluation/provider acquisition cannot commit — **PASS**, PM-02G/I.
- one Finish = one Create/Re-support transaction; Cancel/rejected target = zero mutation — **PASS**, PM-02G.
- GUI / Command Line / semantic command parity — **PASS**, PM-02G.

### Face-backed downstream modeling

- existing Extrude Add consumes face-backed Profile — **PASS**, PM-02H.
- existing Extrude Cut consumes face-backed Profile — **PASS**, PM-02H.
- candidate evaluation and delta preview resolve the same exact support stage — **PASS**, PM-02H.
- upstream edits recompute from current support or fail structurally — **PASS**, PM-02H.
- downstream Features cannot consume stale support truth — **PASS**, PM-02F/H/I.

### History, Delete and repair

- support-producing Feature Delete that would invalidate authored support structure rejects atomically — **PASS**, PM-02I.
- deleting a consumed Profile preserves authored downstream Feature intent and yields MissingProfile/UpstreamUnavailable until repaired/undone — **PASS**, PM-02I.
- Undo/Redo restores semantic support intent and stable IDs — **PASS**, PM-02G/I.
- explicit re-support is the repair path; no geometry-similarity automatic repair exists — **PASS**, PM-02G/I.
- repair, failure and cold reopen do not corrupt durable identity/provenance — **PASS**, PM-02I.

## 4. Final verification status

- original semantic/core verification — **PASS** in FULL #1451.
- post-remediation complete Windows FULL — **PASS** in #1473 on exact `b9e70c684c2d054da21662ef3a8ae56ac59f60c0`; this includes core-only, kernel-native, complete desktop CTest, FAST/SUBSYSTEM selectors, SR-02 and CI-04 parity evidence.
- J-R2 Add Surface continuation / representation-partition Edge — **PASS**, FULL #1470.
- J-R3 exact support-stage face-Sketch Edit presentation — **PASS**, FULL #1471.
- J-R4 topology hover/preselection/candidate cycling/pick stability — **PASS**, FULL #1472.
- J-R5 Create Sketch / Apply Support labeling and Profile window-size containment — **PASS**, FULL #1473.
- canonical internal + PL/EN remediation documentation — **PASS**, exact `1dbaef95b2ea48f8e529425a154b795984d6551f`.
- Product Browser regeneration via `.\ss2.ps1 docs` — **PASS**, Windows DOCS #1474 with zero generated diff.
- J-R6 automated/docs readiness bookkeeping — **PASS**, merged through #231.
- Owner Windows re-test — **PENDING**; this is the sole remaining PM-02 completion gate.

## 5. Owner Windows manual workflow

**2026-10-05 initial result:** FAIL.  
**J-R2..J-R5 automated remediation:** PASS through Windows FULL #1473.  
**Owner Windows re-test:** PENDING.

Blocking/manual findings include unstable or absent topology preselection/candidate cycling, unstable ordinary pick in one box orientation, stage-incorrect face-supported Sketch Edit presentation after downstream Extrude, and coplanar Add Surface/partition behavior that can block Sketch support. User-facing Create Sketch labeling and a reproducible top-level window resize also require remediation before the next Owner PASS.

Detailed authority: `work/PM-02J_MANUAL_ACCEPTANCE_REMEDIATION.md`.



Run on the final accepted Windows build. Record PASS/FAIL for each step.

1. Create an Origin Sketch, closed Profile and Extrude Add Body.
2. Pick Body topology and verify ordinary near-corner → Vertex, near-boundary → Edge, Face-interior → Face acquisition.
3. Inspect Face/Edge/Vertex Properties: semantic carrier + producer/stage/provenance/referenceability are visible; geometry values remain diagnostics.
4. Create an overlapping/multiple-hit view; use Tab and Shift+Tab to cycle visible candidates. Verify no authored-state/Undo change and no Properties churn until click.
5. Cycle Shaded / Shaded + Edges / Shaded + Hidden Edges. Verify no dirty-state or Undo change.
6. Verify dashed hidden Edges are not selectable through opaque material.
7. Hover and select multiple Extrude Features; verify Current Feature Contribution overlay without replacing current Body with a historical stage.
8. Exercise a later trim/split scenario; verify all surviving current contribution fragments are shown and deleted outputs are not ghosted.
9. Create a Sketch on a planar cap and Finish.
10. Create a Sketch on a planar lateral Face and Finish.
11. Create a Sketch on a Cut-exposed planar Face when available in the scenario.
12. Select a cylindrical/non-planar Face for Sketch; verify structured Unsupported and no fallback to adjacent Edge/Face.
13. From a face-supported Sketch, author Profile and existing Extrude Add/Cut.
14. Edit an upstream dimension so the semantic support moves but survives; verify local Sketch geometry is unchanged and downstream result recomputes.
15. Exercise support loss/ambiguity; verify Missing/Ambiguous state and no stale modeled geometry.
16. Repair with Change Sketch Support / `RESUPPORT`; verify Sketch identity/local geometry remain intact.
17. Undo and Redo the support lifecycle.
18. Save, close, reopen; verify the semantic support, stable authored IDs and Add/Cut chain reconstruct correctly.

**Owner result:** PENDING  
**Owner date:** PENDING  
**Owner notes:** PENDING

## 6. Completion rule

Do not mark PM-02 completed and do not activate PM-03 until:

- canonical docs and generated Browser are verified;
- final docs/closure gate passes;
- the Owner records manual Windows **PASS** for the workflow above.

PM-03 Datum Reference Geometry and Projection remain inactive until that closure.
