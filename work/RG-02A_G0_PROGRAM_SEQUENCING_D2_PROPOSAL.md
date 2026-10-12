# RG-02A/G0 — Part-v1 sequencing amendment v1.32 (D2 PROPOSAL)

**Status:** PROPOSED / NOT YET OWNER-ACCEPTED AS ROADMAP AMENDMENT  
**Date:** 2026-10-12  
**Target roadmap:** `work/PART_MODELING_V1_ROADMAP.md` v1.31 on `main` `fa0f8074dc7bb0eb5e2c45ab0fa313644febf55c`  
**Bounded Work Contract:** `work/RG-02A_G0_VERTEX_CHARACTERIZATION_WORK_CONTRACT_DRAFT.md` in Draft [PR #328](https://github.com/MechanicalCave/SimpleSolid2/pull/328) — its **bounded evidence-only scope** was explicitly Owner-accepted on 2026-10-12 (`akcepyuje - kontynuuj`), **not yet merged or activated**.  
**Decision class:** D2 Owner program sequencing; not a production implementation contract or merge approval.

## 1. Specific Owner decision proposed

Amend only the **next Part-v1 program gate order** from accepted roadmap v1.31 to proposed **v1.32**:

```text
UI-01                           COMPLETED — Owner FINAL PASS, merged PR #324
    |
RG-02A/G0                       NEW, EVIDENCE-ONLY NATIVE VERTEX CHARACTERIZATION
    |
PG-01E                          UNACCEPTED; separate missing-capability inventory
    |
PM-06                           BLOCKED; separately gated Part-v1 completion
```

The current v1.31 order is `UI-01 -> PG-01E -> PM-06`. Insert **exactly one new evidence-only milestone** `RG-02A/G0` between completed UI-01 and the still separately gated PG-01E.

**Do not activate RG-02A implementation, RG-02B/C/D, Mirror, Pattern or additional Part-v1 product features** by this roadmap-only change.

## 2. Why the insertion is a bounded and useful exception

Owner and assistant have accepted a future direction for improved Datum Plane/Axis tools before designing solid Mirror, but no such feature belongs to the current frozen Part-v1 implementation scope. The limited RG-02A/G0 evidence gate can establish whether real Body Vertices already possess durable semantic `FeaturePointAddress` identities or require a later Owner D2 design decision.

This gate modifies **existing native test source and evidence documentation only after its own formal activation**; it never changes production data, UI behavior, Part persistence, B-Rep algorithms, Kernel/Part APIs or Part-v1 accepted product commitments.

Observations that need production code/schemas cannot be "fixed" under RG-02A/G0. The evidence report is the deliverable; further reference-geometry implementation requires a future separately accepted roadmap/product/scope decision.

## 3. Frozen invariants preserved

- Preserve the single-Body Part definition; keep UI-01 accepted runtime unaffected.
- PG-01E is **still required**, unaccepted and not marked PASS. PM-06 stays blocked until its own entry gate.
- Keep current Project Geometry, Feature topology, persisted Part schema v16 and modeling semantics v2 unchanged.
- Do not insert Mirror or Pattern into Part Modeling v1: they remain excluded absent later, explicit program/product amendment.
- Preserve historical completed PM-02/03/04/05/Projection/R1/UI-01 evidence.
- No changes to `work/ACTIVE.yaml` or its `program_roadmap_version` before a separately approved activation change after a genuinely accepted roadmap v1.32 lands on main.
- The owner-mandated **separate RG-02 UI/UX design consultation and explicit approval BEFORE any UI implementation** remains a hard gate, including MID/CENTER markers, vertex hover/picking changes, Tree/Operations and Command Line UX. G0 includes **no UI work**.

## 4. Required governance steps after Owner accepts sequencing

1. Owner **separately accepts the exact v1.31 → v1.32 sequencing proposal** (this document). Approval of the limited RG-02A/G0 Work Contract text alone did not amend v1.31.
2. Prepare a **roadmap-only implementation PR** changing `work/PART_MODELING_V1_ROADMAP.md` to v1.32. In Section 6 insert `RG-02A/G0` immediately after UI-01; reconcile section 20 amendment authority, dates/status and add an explicit v1.32 section, while leaving historic v1.31 material legible. Avoid unrelated cleanup of historical snapshots.
3. Separately obtain Owner explicit merge permission for that roadmap-only PR after required exact-head governance checks. No auto-merge.
4. Independently obtain Owner explicit merge approval for the already Owner-accepted bounded G0 contract in Draft PR #328, after its exact-head governance review.
5. **Only then**, prepare a **dedicated activation PR** to update `work/ACTIVE.yaml -> active_work` to the accepted RG-02A/G0 Work Contract and synchronize the accepted roadmap version to `1.32`. Require exact-head validation and another distinct Owner merge approval.
6. Only after that activation has merged on main may a **fresh production/evidence branch** begin native tests in the three contract-allowed files. Results are `UNRUN` until real Windows CI executes them. No unapproved algorithm/code mutations.
7. G0 closure must receive Owner review of the evidence and a separate decision on any future semantic VERTEX identity extension. This roadmap does not silently schedule RG-02B/C/D.

If a gate, PR or merge cannot be completed, leave the current accepted Part-v1 program frozen and RG-02A/G0 unactivated.

## 5. Exact proposed new milestone row (for subsequent accepted roadmap patch)

| Order | Gate / package | Required outcome | Activation |
| --- | --- | --- | --- |
| 9D (new, before PG-01E) | **RG-02A/G0 — Body VERTEX semantic-reference evidence gate** | OCCT native Vertex classification across Extrude/Cut/Revolve/Fillet/Chamfer; stage-scoped resolved/unsupported/ambiguous/missing referenceability, pickability distinction, upstream-change and lifecycle characterization; **evidence report only, no CAD/UX implementation** | **G0 bounded Work Contract Owner-accepted 2026-10-12; NOT merged / NOT ACTIVATED; pending roadmap D2/merge/ACTIVE gates** |

The existing PG-01E should follow this new row without dropping its current acceptance obligations; PM-06 remains after PG-01E. The exact numbering scheme can be made internally consistent in the roadmap-only PR. This is a sequencing proposal, not a frozen version-number change yet.

## Documentation impact

Internal docs: not required.  
User/Product docs: not required.  
Generated Browser: not required.  
Reason: proposed program sequencing and existing-feature evidence only; no change to current application behavior. The later RG-02 implementation stages must each declare their own documentation impact.
