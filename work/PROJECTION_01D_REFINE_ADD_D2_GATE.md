# PG-01D follow-up — OCCT Boolean Add Refine production decision gate

**Status:** D2 PRODUCTION POLICY PROPOSAL, NOT YET ACTIVATED. Owner accepted the **legacy-file compatibility tradeoff** on 2026-10-11; that acceptance is recorded here, not treated as a blanket approval to change unrelated kernel behavior. Work remains on draft PR #309; no PG-01D FINAL PASS, merge approval, PG-01E or PM-06 activation.

**Parent active Work Contract:** `work/PROJECTION_01D_PLANAR_FACE_BOUNDARY_WORK_CONTRACT.md` (program milestone 9B, roadmap v1.30). The active contract §3 currently authorizes a *read-only* Face-boundary kernel query and expressly excludes production B-Rep Boolean/refine policy changes. This proposal **does not override §3**. A separate bounded Owner D2 amendment/contract and modeling-semantics version decision are necessary before enabling the new behavior by default, consistent with ADR-0014 §7.

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

At `d942eeb27137b9b62e946e9f0c8340706cb1178e`, Windows FOCUSED [#2420](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38130667486) **PASS 1/1** for `pm02jr2.add_surface_continuation`. Its experiment checks complete semantic topology, one upper Face in both fixtures, exact zero material difference at every Add stage, existing strict source/support outcomes, Undo/Redo/cold reopen; negative controls preserve radius changes, periodic seam constraints, a planar step and hole wires. Earlier FOCUSED [#2418](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38130655517) passed at an earlier SHA. **These are test-only candidate results, not production FULL evidence.**

A subsequent test-only change further asserts (1) **one upper Face after fresh independent-provider cold Save/Reopen**, rather than trusting aggregate Face/Edge counts; and (2) **zero exact B-Rep material difference at every cold-reopened stage**. This new SHA must obtain its own Windows FOCUSED result before either assertion is marked PASS.

## 3. Proposed narrow D2 production policy (requires explicit separate acceptance)

- **Operation scope:** only post-fuse **Add** for Extrude/Revolve, with history-integrated OCCT simplification. Explicitly keep Cut, edge features and unrelated kernels unchanged. Do not confuse merging genuinely identical underlying faces with suppressing legitimate material boundaries.
- **Version decision:** record an explicit changed *modeling-semantics policy version*, separately from Part document schema v16, EngineeringRevision and DocumentRevision, consistent with ADR-0014. Decide whether new documents pin this policy and how a reader diagnoses older-policy files. **No mass conversion**. Breaking old documents is allowed; silent semantic reinterpretation is not.
- **Identity:** verify per-stage stable semantic lineage after simplification, generated/deleted/modified history, strict material Edge certification, and fresh-generation reconstruction. For one-to-many/zero-matches, issue a typed fail-closed result. Existing PG-01D `AtSingleSemanticPoint` and manual Face Boundary/Planar Face rules remain.
- **Candidate acceptance:** targeted positive **2 -> 1** and **4 -> 1** upper-face outcomes on the originals; negative same-domain/real-boundary controls; B-Rep exact material invariance; new-document projection link authoring and edits; Undo/Redo and cold Save/Reopen using a *normal default* production provider; FOCUSED then relevant FAST and immutable **exact-head Windows FULL**, paired PL/EN + internal documentation and generated Product Browser; Owner manual reproduction and independent PG-01D FINAL PASS with merge permission.
- **Non-goals:** arbitrary OCCT `ShapeUpgrade_UnifySameDomain` pass after history loss, guessing semantic producer, silent data rescue, altering PM-05 Chamfer/Fillet or independent CI issue #302.

## 4. Explicit STOP / next handoff

**Current permission:** bounded experiments, negative/positive kernel tests, evidence and this compatibility decision record **only** under PG-01D. The production default must remain unchanged until the Owner accepts the bounded operation policy **and** the modeling-semantics-version choice. Do not merge PR #309 on experiment-only FOCUSED evidence.

**Next gate:** obtain the new exact-head Windows FOCUSED result for the expanded cold-reopen checks; then complete a separate precise D2 production amendment/version proposal for Owner acceptance before switching default Refine. If a source cannot be uniquely certified under Refine, preserve fail-closed rather than relaxing identity to make the acceptance numbers green.
