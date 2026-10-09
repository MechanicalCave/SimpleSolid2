# PM-05F — Edge Features / final acceptance completion

**Status:** COMPLETED — PASS; Owner aggregate PM-05F final PASS 2026-10-09
**Program:** Part Modeling v1 roadmap v1.29, PM-05 (Fillet / Chamfer)
**Work Contract:** `work/PM-05_EDGE_FEATURES.md` §26–§30
**PR:** [#298](https://github.com/MechanicalCave/SimpleSolid2/pull/298)
**Scope:** PM-05A–E previously COMPLETE; PM-05F documentation, cumulative automated evidence, final Owner acceptance

## Owner decision

On 2026-10-09 the Owner explicitly answered **`PM-05F - Pass`** after being asked whether the earlier `manual test - pass` signified full PM-05F product acceptance or only four R2 targeted scenarios. This is explicit **aggregate package acceptance**. A separate 41-step per-row manual outcome transcript was not provided, so this document does not invent such evidence. Previously scoped Owner Part008 Fillet/Chamfer and P1 picking retest results remain valid in their recorded scope.

## Exact technical evidence

| Evidence | Exact candidate | Outcome |
| --- | --- | --- |
| Windows FOCUSED [#1898](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37842345000) | `af85a6fafea643739393b4f29673eca56c666519` | PASS 1/1; R2 mixed Chamfer 18 variants, 21 unique Face owners, source Vertex/Edge incidence, native UI and lifecycle |
| Windows FAST [#1900](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37844449758) | `5996583e99b20bbad16e252abb2f56c6904d69ef` | PASS 93/93 |
| Windows FULL [#1902](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37846779905) | `5996583e99b20bbad16e252abb2f56c6904d69ef` | PASS: Core 25/25, kernel-native 57/57, Desktop 113/113, all 195/195, docs + aggregate `windows-msvc` success |
| Evidence-only FAST [#1903](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37849063919) | `49fb043750c9c6e2ef2af3e585431c1118757436` | PASS 93/93; bootstrap/docs verification + aggregate success |
| Final Owner acceptance | Owner 2026-10-09 | **`PM-05F - Pass`**, complete package |

After the targeted Owner runtime/test candidate, the FULL runtime/docs candidate and the FAST evidence-sync candidate introduced **no additional runtime or test-code changes**. The formal final acceptance documentation is itself `work/**`-only; its final PR-head CI and merge must still be checked separately.

## Contract acceptance coverage

All 27 PM-05 §27 mandatory acceptance scenarios are tied to concrete Core/kernel/native executable evidence in [`work/PM-05F_CUMULATIVE_ACCEPTANCE_EVIDENCE.md`](PM-05F_CUMULATIVE_ACCEPTANCE_EVIDENCE.md), including explicit multi-Edge Fillet/Chamfer, two/three-edge corners, disconnected/loop/mixed sets, strict reference resolution, input authorship without implicit tangent expansion, chained Features, full edit/repair/lifecycle, v13→v14 migration, true cold rebuild and product docs/Browser.

The Owner's 41-step Windows workflow and prior failures/re-tests are preserved in [`work/PM-05F_OWNER_WINDOWS_ACCEPTANCE.md`](PM-05F_OWNER_WINDOWS_ACCEPTANCE.md). Owner acceptance is **aggregate**, not a fabricated independent test record for each of the 41 manual steps.

## Bounded remediation / retained exclusions

- D2-B supports only the accepted OCCT-provider-private planar mixed signed-delta 3-Edge Chamfer fallback with strict provenance and validation. No global two-Edge planar-corner policy, silent fuzzy/healing, alternate durable identity or general "CAD everywhere" result guarantee.
- Only line/circle analytic material Edge semantics already accepted; non-analytic `CurveKind::other` and specific complex unsupported picks still fail closed. Any new Viewer rejection-reason public contract remains separately gated D2.
- Variable-radius Fillet, per-Edge parameters, asymmetric/angle Chamfer, implicit tangent chaining, general topology healing, global surface modeling, multi-body, Projection and Assembly/Drawing remain excluded by the accepted PM-05 scope.
- Nothing in PM-05 acceptance approves PM-06, authorizes Projection implementation or amends the frozen v1.29 sequence. The Owner asked to revisit moving Projection before PM-06; this is a **future explicit program D2 decision**.

## Closure procedure

- Set Work Contract, Owner acceptance, evidence matrix and program activation *status* to PM-05 COMPLETED — PASS, without changing Part-v1 scope/sequence.
- Keep `work/ACTIVE.yaml` pointing to the last accepted PM-05 contract for traceability until a **separately Owner-approved** next Work Contract; no active post-PM-05 product change is authorized.
- Promote PR #298 out of Draft only after this status-only commit, run and verify its **exact-head Windows FULL** from the standard PR gate, then merge using the repository's established squash method if all results PASS and the branch/head remain unchanged.
- Update current run/merge evidence in the PR metadata or subsequent closure history; this historical completion record is not a claim that the merge already occurred.

## Documentation impact

Governance and evidence only; no product runtime, canonical PL/EN Product docs or generated Browser code change in this closure update. Docs/Browser freshness was proven by FULL #1902 and subsequent PR verification remains required.
