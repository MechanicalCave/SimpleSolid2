# PG-01D follow-up — OCCT Boolean Add Refine production decision gate

**Status:** Owner D2-R1–R4 ACCEPTED 2026-10-11; production implementation authorized, acceptance/merge pending. Owner accepted the **legacy-file compatibility tradeoff** on 2026-10-11; that acceptance is recorded here, not treated as a blanket approval to change unrelated kernel behavior. Work remains on draft PR #309; no PG-01D FINAL PASS, merge approval, PG-01E or PM-06 activation.

**Parent active Work Contract:** `work/PROJECTION_01D_PLANAR_FACE_BOUNDARY_WORK_CONTRACT.md` (program milestone 9B, roadmap v1.30). The active contract §3 currently authorizes a *read-only* Face-boundary kernel query and expressly excludes production B-Rep Boolean/refine policy changes. This proposal **does not override §3**. A separate bounded Owner D2 amendment/contract and modeling-semantics version decision are necessary before enabling the new behavior by default, consistent with ADR-0014 §7.

## Owner D2-R1–R4 — ACCEPTED 2026-10-11 (supersedes proposal-only STOP)

**Owner decision:** `zatwierdzam - kontynuuj` after reviewing the explicit D2-R1 through D2-R4 gate, including **strict modeling-semantics v2 rejecting existing v1 Part files**. This authorizes the following **bounded production implementation on active PR #309**, without authorizing final PG-01D PASS, default-branch merge, global healing, file migration or any later package.

- **R1:** the default production `OcctSolidModelingKernel` must apply OCCT history-integrated, nondestructive, zero-fuzzy `SimplifyResult(true,true)` **only after successful Extrude Add / Revolve Add Fuse**, retaining `finishBoolean` material and semantic validation. Cut and edge features retain their current policy.
- **R2:** `part::current_modeling_semantics_version` advances **1 → 2**, independently of native Part schema **v16**. Reader must **reject** modeled v1 files with a distinct typed unsupported-semantics error (not reinterpret v1 as v2). No automatic migration or silent re-save. Existing old files need not open. A Part created under v2 writes and cold-opens v2.
- **R3:** keep both original Owner v1 `.ss2part` fixtures byte-for-byte unchanged. In **tests only**, create separate explicitly marked v2 semantic reconstructions by repackaging their authored document JSON with `modeling_semantics_version: 2`; never call that production migration or claim that original v1 opens. Preserve source IDs/history and verify strict v1 rejection, positive 2→1 and 4→1, zero occupied-material delta per Feature stage, semantic references, Undo/Redo/Save/Reopen.
- **R4:** allow the narrowly required edits to provider Fuse, Part semantics-version declaration, Part authored-state validation/reader diagnostics and suitable existing tests; current docs PL/EN/internal and generated Browser must reflect **actual** implementation. No global OCCT change, Cut/Fillet/Chamfer switch, project schema bump, arbitrary identity rebind, or CI issue #302 edits.

**Regression obligations:** no test may be disabled to obtain green; historical unrefined control tests may request an explicit test-only legacy provider. Prove that ordinary default production evaluation is refined; negative cases preserve real geometry. Native FOCUSED and a **new exact-head Windows FULL** must pass before Owner manual retest. Owner PG-01D FINAL PASS and merge permission remain distinct.

**Pre-implementation immutable experimental gate:** Windows FULL [#2424](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38132815185) on `961567092da82661e9e0a761b352e96c30acb175` passed **199/199** (Core 25, native Kernel 57, Desktop 117), including native Refine cold-reopen geometry and UI/docs bootstrap. **This is not yet production-default v2 evidence**.

## 1. Owner decision already accepted — compatibility is not a veto

**2026-10-11 Owner approval in conversation:** an eventual correctly specified production Refine may regress compatibility of **preexisting native Part files and their old reference resolutions**. Historical Face/Edge counts, previously stored links resolving exactly as before, or identical evaluation of already saved documents are **not mandatory release acceptance criteria**.

**Still mandatory, without exception:**

1. Correct occupied material: a valid single solid and **zero exact positive-volume added/removed B-Rep difference at each evaluated Feature stage** for the reproducer, including cold reopen and accepted edits. No presentation-mesh or face-count-only substitute.
2. Real geometry/topology: remove unnecessary same-domain representation splits on the two Owner native v16 Add reproductions, **without** removing actual steps, holes, distinct-cylinder boundaries or required periodic seams.
3. New/current documents: complete, deterministic semantic Surface/Curve/Point provenance, uniquely resolved strict material Edge identity and current Sketch supports where applicable, safe Project Geometry authoring, exact source-stage resolution, and Edit/Undo/Redo/explicit Save/cold Reopen.
4. Fail closed: no geometric proximity rebinding, arbitrary predecessor/producer choice, phantom resolved link, stale last-good fallback, silent different-Edge attachment or partial state mutation. A legacy file that cannot be resolved safely may become **typed Broken/invalid**. The permission to break compatibility is **not** permission to fabricate correctness.
5. No automatic migration/bulk rewrite of old files; the accepted PG-01D v16 user-initiated Save rule stays separate. No unsupported claim that file/schema v16 alone expresses a new **modeling-semantics version**.

## 2. Observed and experimental implementation (not release approval)

Owner provided the two **unchanged-byte-content** test inputs documented in `tests/fixtures/PG01D_SAME_DOMAIN_FIXTURES.md`:

- `pg01d_same_domain_extrude_add_v16.ss2part`: Extrude Add + Extrude Add; unwanted upper plane **2 Faces** in the ordinary provider.
- `pg01d_same_domain_extrude_revolve_add_v16.ss2part`: the same Adds + Revolve Add; unwanted upper plane **4 Faces** in the ordinary provider.

Experimental opt-in: `OcctSolidModelingKernel::AddRefinementExperiment::simplify_same_domain`. On each Extrude **Add** and Revolve **Add**, after a successful zero-fuzzy `BRepAlgoAPI_Fuse::Build()`, use `SetNonDestructive(true)` and history-aware `SimplifyResult(true, true)`. Final topology/provenance still runs through the ordinary provider's `finishBoolean`. **Default provider remains disabled**. No Cut, Fillet, Chamfer, global repair or tolerance escalation is being silently changed.

At `d942eeb27137b9b62e946e9f0c8340706cb1178e`, Windows FOCUSED [#2420](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38130667486) **PASS 1/1** for `pm02jr2.add_surface_continuation`. Its experiment checks complete semantic topology, one upper Face in both fixtures, exact zero material difference at every Add stage, existing strict source/support outcomes, Undo/Redo/cold reopen; negative controls preserve radius changes, periodic seam constraints, a planar step and hole wires. Earlier FOCUSED [#2418](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38130441821) passed at an earlier SHA. **These are test-only candidate results, not production FULL evidence.**

A subsequent test-only change further asserts (1) **one upper Face after fresh independent-provider cold Save/Reopen**, rather than trusting aggregate Face/Edge counts; and (2) **zero exact B-Rep material difference at every cold-reopened stage**. This new SHA must obtain its own Windows FOCUSED result before either assertion is marked PASS.

## 3. Proposed narrow D2 production policy (requires explicit separate acceptance)

- **Operation scope:** only post-fuse **Add** for Extrude/Revolve, with history-integrated OCCT simplification. Explicitly keep Cut, edge features and unrelated kernels unchanged. Do not confuse merging genuinely identical underlying faces with suppressing legitimate material boundaries.
- **Version decision:** record an explicit changed *modeling-semantics policy version*, separately from Part document schema v16, EngineeringRevision and DocumentRevision, consistent with ADR-0014. Decide whether new documents pin this policy and how a reader diagnoses older-policy files. **No mass conversion**. Breaking old documents is allowed; silent semantic reinterpretation is not.
- **Identity:** verify per-stage stable semantic lineage after simplification, generated/deleted/modified history, strict material Edge certification, and fresh-generation reconstruction. For one-to-many/zero-matches, issue a typed fail-closed result. Existing PG-01D `AtSingleSemanticPoint` and manual Face Boundary/Planar Face rules remain.
- **Candidate acceptance:** targeted positive **2 -> 1** and **4 -> 1** upper-face outcomes on the originals; negative same-domain/real-boundary controls; B-Rep exact material invariance; new-document projection link authoring and edits; Undo/Redo and cold Save/Reopen using a *normal default* production provider; FOCUSED then relevant FAST and immutable **exact-head Windows FULL**, paired PL/EN + internal documentation and generated Product Browser; Owner manual reproduction and independent PG-01D FINAL PASS with merge permission.
- **Non-goals:** arbitrary OCCT `ShapeUpgrade_UnifySameDomain` pass after history loss, guessing semantic producer, silent data rescue, altering PM-05 Chamfer/Fillet or independent CI issue #302.

## 4. Explicit STOP / next handoff

**Current permission:** Owner has expressly approved the narrow R1–R4 production implementation and strict v2; production code changes inside the accepted bounded exception may proceed on PR #309. **Do not merge** without new exact-head full evidence and separate Owner FINAL PASS.

**Next gate:** implement the accepted default + strict v2, test-only source fixture reconstructions and typed rejection; obtain rebuilt FOCUSED, FAST as needed, exact-head Windows FULL, Owner practical tests and final PASS.  If a source cannot be uniquely certified under Refine, preserve fail-closed rather than relaxing identity to make the acceptance numbers green.

## Implementation checkpoints (2026-10-11)

- `5b60f596cc29f29c450c5fd9aea7f72e0955ed4b`: R1 default Add Refine in ordinary OCCT provider; R2 persistent semantics v2 and typed reject of existing v1; R3 explicit temporary test-only v2 repackages of the two original v1 Owner fixtures; no file migration, no Cut/Fillet/Chamfer change. **Not yet GREEN.**
- Windows [#2426](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38134403973) **RED before any test**: mixed Desktop FOCUSED targets included `pm02jr2_add_surface_continuation_test` (native kernel-only target absent from Desktop CMake build tree), `MSB1009`; neither semantic code nor native test executed. Repeat with `SS2-Focus-Mode: kernel`, kernel-native target/test only, then run FULL for PartStore/Desktop. Do not label #2426 a behavioral failure or modify independent CI configuration.

## Windows exact-SHA production-v2 acceptance evidence (2026-10-11)

- **Native Kernel / production Refine:** Windows [FOCUSED #2434](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38135489072) PASS 1/1 at `d0ff070e0bf2d8a5f27825c72425238da5cf0693`, `pm02jr2.add_surface_continuation`. Both original Owner native v16/v1 documents are *correctly rejected* by strict v2. Temporary test-only v2 copies retain their original feature/sketch history; normal default provider reduces 2→1 and 4→1 upper planar Faces. All source-stage material B-Rep differences are zero, linked Sketch source/support cold reopen and Edit/Undo/Redo checks pass, and genuine step/hole, cylindrical radius and seam negatives remain unchanged. Historical unrefined topology E0 studies use explicit test-only disabled mode rather than relying on the new default.
- **Native Desktop / PM-05F and PG-01D:** Windows [FOCUSED #2436](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38135815695) PASS 1/1 at `86548e4da6406a472949846a92d38dcada76bed8`, `pm05f_r2.native_workbench_edit`, 12.54 sec, plus docs verification PASS. Original sanitized v1 fixtures are verified incompatible and copied only to temporary test-only v2 payloads; no originals are modified or persisted migration performed. PL/EN, internal as-built and generated Browser synchronized at that SHA.
- **Earlier candidate errors are not ignored or reported green:** #2426 mixed desktop/kernel target CI selection RED before build; #2427–#2433 progressively exposed and corrected old-vs-new OCCT test provider policy assumptions and an actual `std::move(parse_error)` argument-evaluation diagnostic bug; #2435 unavailable JSON header in Desktop test target RED. No tests were removed or disabled to force a green result.
- **OPEN:** exact-head focused PartStore + isolated Face Boundary UI/negative tests, then the immutable Windows FULL (Core + native Kernel + Desktop + docs), Owner's practical Windows check and separate explicit PG-01D FINAL PASS/merge approval. Old experimental [FULL #2424](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38132815185) 199/199 is NOT production-v2 acceptance evidence.
