# RG-02C/B1 — native reversed B-Rep Edge orientation: bounded D2 decision proposal

**Status:** D2 PROPOSAL FOR OWNER REVIEW — NOT ACCEPTED; no added executable allowlist, no product/source mutation authority, no merge authority.
**Date:** 2026-10-15.
**Governing active scope:** `work/ACTIVE.yaml` → `work/RG-02C_B1_SINGLE_STRAIGHT_EDGE_AXIS_LIFECYCLE_D2_WORK_CONTRACT_PROPOSAL.md` §0A / F03 / F13; Part Modeling v1.33; frozen Foundation/Architecture; Q0/Q2/A0 native evidence. Branch from unchanged protected `main` `336809a2a605e239d495d03146ce97dda1d93334`. Governance-only independent review; do not activate an unrelated work package.

## Owner decision requested — ONE missing product acceptance proof

Select **A (recommended)** to authorize the smallest actual native-provider test fixture, OR **B** to explicitly defer the fixture as a named product risk. Neither choice is implicit in the Owner's current manual `test - pass - kontynuuj` statement. That statement is evidence of the requested manual Axis lifecycle test, **not** technical FINAL PASS or permission to weaken native test gates.

### What is proven versus unproven

1. Actual B1 product semantics and native v19 Save/Reopen, strict stage-qualified material Edge reference, GUI, Axis/Create/Edit/Delete/Undo/Redo, and toolbar regression passed genuine Windows Ready FULL [#3301](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38806561061): Core 25 + native Kernel 57 + Desktop Qt/OCCT 117 = **199/199 PASS**. DOC-01 PL/EN/internal/browser passed [#3302](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38807293433) and [#3303](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38807529149). Owner reported manual full-scope PASS after the earlier separate toolbar-only PASS. These witness actual source/test SHAs in [B1 candidate evidence](https://github.com/MechanicalCave/SimpleSolid2/blob/531a542a1cd1123e36055d9dcea7e7adad263af2/work/RG-02C_B1_EDGE_AXIS_ACCEPTANCE_EVIDENCE.md); nothing is retroactively inferred about untested cases.
2. A0/B1 compares an unoriented geometric line with `t` and `-t` and checks native straight-Edge endpoint samples on generated Bodies. **No native test has actually reversed a `TopoDS_Edge` topological `Orientation()` and passed that reversed real native Edge through the production exact-Edge query and B1 Axis resolver.** This is a precisely named GAP, not a product regression or waiver.
3. Existing `src/kernel_occt/solid_modeling_kernel.cpp` holds `OcctRuntimeSolid::inventory_edges` (a private `std::map<uint64_t, TopoDS_Edge>`) entirely inside its implementation. Public `OcctSolidModelingKernel::bindExactEdgeToBody(RuntimeSolidHandle, RuntimeEdgeToken)` and `queryExactBoundedEdge3d(RuntimeSolidHandle, ScopedExactEdge)` require the identical real runtime-owned inventory; the Part test only has opaque runtime `shared_ptr<const RuntimeSolid>` and token. `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp` cannot force `TopoDS_Edge::Reversed()` into that inventory through any existing public seam. Creating an unrelated OCCT Edge in the test or negating the returned tangent proves neither production ownership nor B1 behavior under a real reversed topological Edge.

## Option A — recommended: real native-provider fixture with minimal D2 scope

Owner would separately authorize the following *exact additions* to the current B1 §0A allowlist, **solely if needed by an independently reviewed test seam**:

- `src/kernel_occt/include/simplesolid2/kernel_occt/solid_modeling_kernel.hpp` — at most one narrowly scoped OCCT-native **test fixture seam**, with no `TopoDS_*` type crossing the neutral `src/kernel/**` API, no persisted/reflected Part topology tokens, no new user-visible CAD behavior and no coupling of Part implementation to provider internals.
- `src/kernel_occt/solid_modeling_kernel.cpp` — implement a test-only path that makes an independent, ephemeral, provider-owned native runtime witness with the **same exact Edge geometry and real reversed topological orientation**; preserve runtime ownership, inventory token scope and Face/Vertex integrity. Do not mutate an existing production Body/Edge in place or silently alter normal production code paths. Define exact API only after the minimal seam is proved feasible; fail closed if not.
- Existing already-allowlisted `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp` — add positive and negative cases to the registered test target, without modifying `tests/kernel_native/CMakeLists.txt`, workflow, `src/kernel/**`, Part schema or existing assertions.

**Mandatory concrete test witness:** verify the before/after `TopoDS_Edge::Orientation()` is genuinely opposite on native B-Rep, while same underlying bounded analytic curve and physical terminal set remain consistent; bind the reversed Edge to the correct fresh provider runtime, invoke the **real existing production Q0 exact-edge query** and **the unchanged Part-owned B1 straight Edge Axis semantic resolver**, compare the same unoriented infinite line with strictly stage-qualified meaning, and verify reversed terminal observations cannot author Revolve's signed direction. Negative checks must reject foreign runtime/Edge token, ambiguity/stale source and wrong stage. Tests must not merely compare `+t/-t` nor coerce a mocked provider response. Report whether the fixture is technically possible before implementing a broader production API.

**Acceptance:** explicit Owner D2 text approval and separate governance-only guarded merge of this allowlist amendment; exact native FOCUSED → Draft FAST → Ready FULL native+Qt/OCCT, DOC-01 if as-built contracts change, factual evidence, new Owner technical FINAL PASS and distinct production merge permission. Do not enable this path from this unmerged proposal.

**STOP:** If the test needs new `src/kernel/**`, viewer, CI/CMake or persistent model APIs, produce an amended technical D2 proposal; do not widen by agent judgement. A provider diagnostic/test hook accidentally available as a production mutation command is unacceptable.

## Option B — explicit Owner risk deferral instead of native proof

Owner may explicitly accept that B1 cannot currently prove invariance under a truly reversed native B-Rep Edge via the production exact-Edge pipeline. Record this as **open product risk**, not `PASS` or `skipped passed`: provider-generated topology orientation differences could invalidate assumed terminal mapping/derived Axis behavior after regeneration even when semantic source identity is unchanged. Keep A0 `+t/-t`, true cold-reopen, real OCCT tests and existing gates unchanged. Owner must state the intended scope and risk acceptance; **no implicit deferral** from manual GUI PASS, FULL 199/199 or a generic `kontynuuj`.

## Documentation impact

Internal docs: not required (proposal only)
User/Product docs: not required (proposal only)
Reason: This D2 review does not change as-built Part behavior. Any implemented fixture/public provider testing API requires separate impact determination before technical completion.
