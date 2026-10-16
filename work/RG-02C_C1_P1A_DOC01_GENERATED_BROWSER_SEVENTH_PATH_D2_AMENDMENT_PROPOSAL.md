# RG-02C/C1-P1-A — DOC-01 generated Browser seventh-path D2 amendment proposal

**Status: OWNER REVIEW DRAFT ONLY — NOT ACCEPTED, NOT MERGED, NOT ACTIVE.** This is a **strictly one-file addendum proposal** for the accepted and ACTIVE C1-P1-A implementation contract `work/RG-02C_C1_P1A_EXACT_NATIVE_CYLINDRICAL_FACE_AXIS_QUERY_D2_WORK_CONTRACT_PROPOSAL.md`. It requests only the necessary generated `docs/browser/index.html` as a **SEVENTH** allowed file for the existing six-file P1-A branch, without expanding product capabilities or test requirements.

## 1. Genuine first RED, not a native compiler/test result

After Owner's separately authorized #432 squash activated P1-A on protected main at `5215817969d1eb83ab9e17a9324a429c9034d637`, the bounded P1-A source/test/internal-doc/evidence candidate [#433](https://github.com/MechanicalCave/SimpleSolid2/pull/433) at `4253cf218a0602932dc7f787d6fd5484a0186a50` triggered actual [Windows DRAFT FAST #3335](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38893894889). **RED** at pre-build official DOC-01:

> `[BROWSER] docs/browser/index.html is stale. Run '.\\ss2.ps1 docs' and commit the generated file.`

No C++ compile, no P1-A native geometry or CTest PASS happened in that run. The P1-A Work Contract requires `docs/internal/PART_EDGE_PROJECTION_KERNEL.md` be updated (`Internal docs: required`), but its exact six-path allowlist explicitly **forbids any additional `docs/**` file** without new Owner D2 approval. This is an authority contradiction, not a reason to weaken DOC-01, remove the internal doc, or silently edit Browser.

Confirmed existing deterministic build pipeline: `scripts/docs/ss2-docs-core.ps1::Get-CanonicalDocPaths` enumerates `docs/internal/*.md`, `New-BrowserHtml` embeds their canonical markdown, and `scripts/docs/ss2-docs-validation.ps1::Test-BrowserFreshness` requires `docs/browser/index.html` equal the generated result. Thus a changed internal canonical source necessarily changes the generated Browser artifact, even though there is **no change to user-visible CAD behavior**.

## 2. Proposed exact Owner D2 change — ONE generated additional path ONLY

Extend the existing activated P1-A six-path allowlist by exactly:

7. **`docs/browser/index.html` — GENERATED ONLY** from the current canonical documentation by the existing deterministic repository `ss2.ps1 docs` / `scripts/ss2-docs.ps1` workflow, followed by official `ss2.ps1 verify` or equivalent DOC-01 check. This is a synchronized artifact of the already approved internal as-built `docs/internal/PART_EDGE_PROJECTION_KERNEL.md`; do not hand-edit Browser source, template, scripts, package, navigation logic or other internal/PL/EN docs. The generator may include the unchanged canonical PL/EN docs but no PL/EN source edit is authorized.

**No other mutation authority added.** Original six paths remain unchanged and binding: (1) new `src/kernel/include/simplesolid2/kernel/exact_cylindrical_face_axis.hpp`, (2) existing `src/kernel_occt/include/simplesolid2/kernel_occt/solid_modeling_kernel.hpp`, (3) existing `src/kernel_occt/solid_modeling_kernel.cpp`, (4) existing registered native `tests/kernel_native/pm04c2_production_revolve_test.cpp`, (5) required internal as-built `docs/internal/PART_EDGE_PROJECTION_KERNEL.md`, (6) factual `work/RG-02C_C1_P1A_EXACT_CYLINDRICAL_FACE_AXIS_QUERY_NATIVE_EVIDENCE.md`.

**Reason and semantic impact:** P1-A remains strictly a read-only provider-neutral exact analytic cylinder Face-axis query with typed Body-generation-scoped failure; no new Part-owned Axis reference, v1 schema, viewer, GUI/UX, command line, Revolve consumer, Face/Surface persisted identity P1-B, CMake/CI workflow, test target, kernel semantics or Foundation/roadmap change. `Internal docs: required` remains binding. `User/Product docs: not required` continues to mean no edits of PL/EN canonical Product sources; regenerating their offline Browser container as a mandatory consequence of internal as-built updates is not an authorization for new product capabilities.

## 3. Separate gated sequencing — STOP before the seventh-file mutation

1. Owner must **explicitly accept this exact seventh-file D2 addition**; project priority or earlier P1-A acceptance does not imply it.
2. Record acceptance in this proposal and pass exact-head Windows governance CLOSURE; obtain **separate guarded squash merge authorization** into protected `main`.
3. Prepare a **separate ACTIVE-only PR** that retains `active_work` pointing at the original C1-P1-A contract but prepends one factual `next:` history addendum to `work/ACTIVE.yaml`, binding the accepted seventh-file exception to P1-A. Require its own exact-head Windows CLOSURE and independent guarded squash Owner authorization. Only **after this ACTIVE-only amendment merges** can #433 update/generated `docs/browser/index.html` as its seventh file. The original six remain frozen.
4. Re-run genuine P1-A FOCUSED `pm04c2.production_revolve`, separate FAST and Ready FULL Core+Kernel+Qt/OCCT, mandatory DOC-01, final exact-head CLOSURE after factual ledger. The first RED #3335 must remain recorded as RED; passing docs later does not convert an unexecuted native test into PASS. Independent Owner P1-A technical FINAL PASS and subsequent safeguarded production squash remain distinct gates.

**Stop without exception:** if DOC-01 still requires another path or any provider/test defect needs an eighth file, return for another exact Owner decision. No silent expansion, bypass or self-authorization.

## Documentation impact

Internal docs: not required
User/Product docs: not required
Reason: This work-only addendum proposes bounded permission to regenerate a Browser artifact after a previously Owner-required internal documentation change; it does not itself modify production code, canonical documentation or Browser. If approved and activated, as-built internal and generated Browser parity are mandatory in the implementation PR under the same original P1-A semantics.
