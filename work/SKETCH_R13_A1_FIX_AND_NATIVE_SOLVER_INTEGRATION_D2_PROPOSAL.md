# R13-A1 — Fix(Line) contract alignment and proprietary-offline native solver — D2 decision proposal

**Status:** **PROPOSED / NOT ACCEPTED / NOT ACTIVE / NO PRODUCT OR DEPENDENCY MUTATION AUTHORITY** (2026-10-20). Independent governance-only Draft PR. **Two separately decidable D2 items (A and B)**; approving either does NOT imply approving the other. This file does not amend `work/ACTIVE.yaml`, the accepted A1 Work Contract, ADRs, roadmaps, `main` product files or PR #506. A later *separate* exact-text Owner-approved contract amendment / guarded governance merge is required to make a decision operative.

**Current reference:** `MechanicalCave/SimpleSolid2` protected `main@9ea2a449ed3e2ed0de53f5f9f3d28e8c6fd8354e`; active pointer `work/SKETCH_R13_A1_LOCAL_RELATIONS_D2_WORK_CONTRACT_PROPOSAL.md`, Sketch Roadmap 1.11, Foundation 1.0; product Draft [PR #506](https://github.com/MechanicalCave/SimpleSolid2/pull/506), exact runtime HEAD `d7e5491723e983b2b1505214a15fdde6e80abed2`. Exact-head native [FULL #3869](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/39370971346): Core 26/26, Kernel 59/59, Desktop 118/118 PASS, aggregate PASS. Those tests use a scripted numeric backend; **no native PlaneGCS solver, vNext persistence or user UI is accepted**. PR #506 remains Draft / unmerged.

## 0. Authority and disposition

Constitution → Foundation/Architecture → accepted ADR → Sketch Roadmap 1.11 / companion Part v1.35 → active A1 Work Contract → code/tests; DOC-01 applies. `Fix(Line)` was explicitly approved by Owner as a sixth local relation in the PR #506 conversation; that is a **product scope decision**, not a retroactive change to the frozen original five-relation contract text. A1's accepted §3 finite file allowlist does not include a vendor tree, native DLL ABI, LGPL notices, installer or arbitrary documentation. The separate Owner confirmation that **proprietary/offline SS2 may distribute LGPL library-only sources** does not approve a particular linkage mechanism or source delivery implementation.

Two independently reviewable dispositions:

- **D2-A:** align the A1 contract with the expressly Owner-selected `Fix(Line)` semantics already staged in Draft PR #506. Still requires formal exact amendment accepted and integrated before package closure; this proposal is NOT that acceptance.
- **D2-B:** select an engineering direction for a **genuinely user-replaceable native PlaneGCS DLL** coupled only through an SS2-owned solver-neutral interface. This is a **design selection proposal only**. **NO source import, vendor CMake, adapter implementation, installer, or release** may start without the mandatory artifact-byte/license proof and separately Owner-accepted **exact per-file D2 extension**.

PM-06, RG-02, A2/A3 dimensions, Qt relation controls and product merge are out of scope.

## 1. D2-A — exact Fix(Line) formalization proposal

**User intent:** manual `Fix(Line)` (Grounded Line) fixes both Line endpoints at their authored Sketch-local coordinates. No Auto-Constraint, no new Point entity, no Body source/projection invention.

1. Append `ConstraintKind::fix` without renumbering the original five enum values, with the existing monotonic Sketch-local `ConstraintId` and semantic full-Line `EntityId` reference. Duplicate/invalid/missing references and contradictory fixed pairs reject atomically, consuming no ID or Undo entry.
2. This is the **sixth authored relation, but NOT a sixth numeric equation**. It contributes exact fixed parameter ownership. `LocalSolveProblem.equations` and numerical residual list contain only Coincident, Horizontal, Vertical, Parallel, Perpendicular; fixed lines never move in accepted evaluated candidates. User Fix and PG-01 fixed projected Lines share fixed-input semantics without turning PG-01 into an authored local relation.
3. Direct Update/Trim/Extend/ExtendBoth attempts to modify a fixed Line fail closed; using a fixed Line as a non-mutating geometric reference remains valid. Explicit Unfix removes only the relation. Sketch geometry Delete cascades its owned relations; retained Part records become repair-required rather than silently deleted.
4. Create/Erase, Undo/Redo, live revision/dirty/equality semantics and source-fixed numerical conflicts remain atomic under DocumentSession. No persistence schema is authorized by this item: v20 still fails closed when given unpersistable constraints; a later exact successor schema gate remains mandatory.
5. Preserve existing D2-A implementation/test path scope **only** as already listed in the A1 contract. `Fix` tests and code already present in Draft PR #506 need independent final technical review; this proposal does not declare FINAL PASS or waive numeric solver qualification.

**Owner acceptance needed:** accept exact D2-A semantics and authorize a *separate governance-only contract amendment* naming Fix as sixth manual relation, with DOC-01 and Windows exact-head CLOSURE. Do not merge product #506 solely because D2-A is accepted.

## 2. D2-B — native solver deployment architecture for Owner choice

**Product constraints previously stated by Owner:** SS2 executable and SS2-owned sources remain proprietary; installation, first launch, activation, Sketch solve, Part/OCCT, native Save/Reopen and uninstall work with the network physically disconnected; LGPL PlaneGCS **library sources and required notices** may be distributed separately and locally to users.

**Recommended conditional selection:** an independently **replaceable** local `ss2_planegcs.dll` using a versioned plain-C ABI behind `sketch::ILocalSketchSolver`. It is not enough to compile existing upstream `_planegcs` (a Python extension) or rename a DLL: native MSVC x64 compatibility, actual replacement of a binary by an interface-compatible user build, redistribution notices/source and the installed consumer must be demonstrated.

A proposed technical ABI for later **separately accepted** D2 file-scope review:

- Explicit `uint32_t abi_version/struct_size` and `extern "C"` exported entrypoints; no STL, C++ exceptions, RTTI, Eigen/Boost types, Python runtime, Qt, OCCT objects or cross-CRT allocations in exported signatures.
- Caller-owned bounded input/output arrays, checked lengths/capacities, finite millimeter doubles, unambiguous endpoint roles, semantic `EntityId`/`ConstraintId`, fixed flags, and typed capacity/ABI/malformed-input errors. No callbacks into the SS2 application or online loading.
- Solve-local correspondence from durable SS2 IDs to ephemeral PlaneGCS tags; returned results mapped by semantic identity, not upstream insertion order or pointer/token. Fixed PG-01 and Fix parameters remain exactly unchanged.
- SS2 retains revision/stage checks, ownership, acceptance residuals, explicit Coincident-only derived endpoint closure, conflict/redundancy/DoF validation, Profile/Feature evaluation and stale-publication rejection **outside** the DLL. An incompatible or missing DLL fails closed with a visible diagnostic; never fallback to stale authored seeds or last-good BRep as current.
- Offline delivery must include the full LGPL-2.1 license, relevant upstream copyrights, corresponding source including SS2 modifications to the LGPL library, dependency notices, reproducible DLL build/replacement instructions and a genuinely replaceable DLL despite code-signing/install layout. Owner/legal independent compliance review is mandatory before distribution.

**Alternatives requiring an explicit different decision:** static linkage with actual relinkable application objects and legally compliant distribution, or a commercially licensed/other acceptable solver. Proprietary SS2 source must **not** be disclosed merely as an assumption of this proposal. If library-source disclosure becomes forbidden, GPL/LGPL-derived PlaneGCS import is a STOP until another lawful license/solver is chosen.

## 3. Evidence and hard blockers — no false PASS

**Source pin, factual:** PyPI `planegcs==0.8.0` publishes original `planegcs-0.8.0.tar.gz` (2026-06-22), SHA-256:

`09e77e6e3a27fa77d3b5a69065ea4c82ae6ed35753246a2e518b1a6005fea162`

with publisher provenance tied to upstream `spookylukey/planegcs@5a60d5328bedd85f1e4aef81b9b57f5c4410ae52`. Upstream frozen Git tree/CMake inspected independently: five C++ TUs `Constraints.cpp`, `GCS.cpp`, `Geo.cpp`, `SubSystem.cpp`, `qp_eq.cpp`, corresponding headers, `Util.h`, plus FreeCAD shim headers. Upstream `CMakeLists.txt` requires Eigen3/Boost, builds Python `_planegcs` via pybind11, **does NOT** deliver a stable replaceable SS2 DLL. `LICENSE` is a short LGPL notice/link (627 bytes), not the full LGPL-2.1 terms; some wrapper/shim source files lack per-file SPDX. This is **not** a blanket assertion that those files are unlawfully licensed; independent provenance and packaging review must classify them.

**Original archive proof still BLOCKED:** real HTTPS download of the exact PyPI `files.pythonhosted.org` artifact failed with DNS/name-resolution in the inspection environment on 2026-10-20. **No file bytes were obtained and NO independent archive SHA or extracted-file audit occurred.** PyPI signed metadata and a GitHub tree/archive are not equivalent to hashing that exact PyPI tarball. Newer upstream versions are NOT silently substituted for 0.8.0.

Mandatory staged gates:

| Gate | Required proof | Until then |
| --- | --- | --- |
| **B0 — before vendor/import** | Independently acquire ORIGINAL sdist bytes, SHA-256 match, inspect actual archive member inventory, bundled copyright/SPDX/full LGPL and shim/wrapper provenance; legal review of chosen DLL obligations; Owner-approved exact vendor/build/notice file allowlist | **STOP vendor and DLL build in SS2** |
| **B1 — before native backend acceptance** | Actual MSVC VS2022 x64 Release exported ABI, clean SS2 native caller, missing/incompatible/replacement DLL tests; five geometric equations + Fix grounding, fixed PG-01, conflict/redundant/DoF and finite residuals, successful real Profile→OCCT Feature, revision/stage; registered RED→GREEN / FULL | **NOT A1.2 PASS** |
| **B2 — before product FINAL / release** | Accepted new native persistent schema with v20 migration and corrupted-record rejection, cold constrained Save/Reopen, real 3D UI Owner walkthrough, offline installer/VM, LGPL corresponding source/notices/replacement verified, DOC-01 PL/EN/internal and regenerated Browser, independent technical FINAL | **NOT READY / DO NOT MERGE** |

### B0 operator runbook — original sdist byte verification (proposal, unexecuted)

**Environment:** a network-permitted, controlled Windows workstation or CI audit host, *outside* SS2 source/build/vendor trees. This recipe obtains **only** the original publisher URL, verifies the exact published SHA and records the archive member list without extracting or executing the package. Neither a successful HTTPS request nor a matching checksum alone equals legal/compliance acceptance. It is not an SS2 application dependency or build step.

Official release metadata: [PyPI planegcs 0.8.0](https://pypi.org/project/planegcs/0.8.0/), uploaded 2026-06-22; original sdist URL was read from that release's explicit download link on 2026-10-20. Verify the source URL against current PyPI metadata before executing:

```powershell
$ErrorActionPreference = 'Stop'
$expected = '09e77e6e3a27fa77d3b5a69065ea4c82ae6ed35753246a2e518b1a6005fea162'
$url = 'https://files.pythonhosted.org/packages/6c/6d/2ae123c5913387f52ab5b969f25bbc513d41f04546ecc5662ab86464a495/planegcs-0.8.0.tar.gz'
$root = Join-Path $env:TEMP ('ss2-planegcs-b0-' + [Guid]::NewGuid().ToString('N'))
New-Item -Path $root -ItemType Directory | Out-Null
$archive = Join-Path $root 'planegcs-0.8.0.tar.gz'
Invoke-WebRequest -Uri $url -OutFile $archive
$size = (Get-Item -LiteralPath $archive).Length
if ($size -le 0 -or $size -gt 10000000) { throw "Unexpected archive length: $size" }
$actual = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
if ($actual -cne $expected) { throw "B0 FAIL: SHA mismatch, expected $expected, actual $actual" }
$members = @(& tar -tzf $archive)
if ($LASTEXITCODE -ne 0 -or $members.Count -eq 0) { throw 'B0 FAIL: cannot inventory tar' }
foreach ($member in $members) {
    $normalized = $member.Replace('\', '/')
    if ($normalized -match '(^/|^[A-Za-z]:/|(^|/)\.\.(/|$))') {
        throw "B0 FAIL: suspicious tar member path: $member"
    }
}
$members | Set-Content -LiteralPath (Join-Path $root 'archive-members.txt') -Encoding UTF8
[ordered]@{
    source_url = $url
    sha256 = $actual
    bytes = $size
    archive_members = $members.Count
    observed_utc = [DateTime]::UtcNow.ToString('o')
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $root 'archive-evidence.json') -Encoding UTF8
Write-Output "B0 BYTE-HASH VERIFIED; inspect archive-evidence.json and archive-members.txt in $root"
```

**Evidence handling:** retain `archive-evidence.json`, complete `archive-members.txt`, original sdist bytes in an isolated quarantine, and exact host/tool provenance. Compare the actual member inventory against required files, detect links/duplicate member names and suspicious size/metadata before any extraction; a safe extraction requires a separate isolation decision. Independently review the actual extracted `LICENSE`, full LGPL-2.1 text/COPYING.LIB availability, attribution/modified-file notices, any Python wrapper/shim provenance and transitive Eigen/Boost license materials. A hash-matching archive with incomplete license materials is **B0 legal STOP**, not automatic permission to vendor. Do not commit downloaded bytes, local paths, evidence secrets or extracted source to PR #507 or SS2 product branch.

**Observed 2026-10-20 from this assistant environment:** fetching `https://pypi.org/pypi/planegcs/0.8.0/json` via Python HTTPS failed with `URLError [Errno -3] Temporary failure in name resolution`; a separate direct artifact download tool failed as well. The exact PyPI webpage, file URL, SHA and attestation were nevertheless independently read as **publisher metadata**. Thus the PowerShell procedure above is **UNEXECUTED**, original TAR bytes **NOT POSSESSED**, sha256 **NOT independently computed**, member list **NOT audited**. GitHub source-tree listing is never a substitute for this proof.

### Frozen upstream native include dependency audit (2026-10-20, source-only)

A read of the pinned upstream `spookylukey/planegcs@5a60d5328bedd85f1e4aef81b9b57f5c4410ae52` (NOT the original TAR) confirms:

- Five C++ translation units and five same-stem public-internal headers contain per-file `SPDX-License-Identifier` identifiers; `Util.h` also contains an SPDX identifier.
- `Constraints.cpp` uses Boost Graph; `GCS.cpp` uses `<Eigen/OrderingMethods>`, `<Eigen/Sparse>`, Boost Graph and FreeCAD `<Base/Console.h>` / `<FCConfig.h>` shims. `qp_eq.cpp` uses `<Eigen/QR>`; the internal `Geo.h` pulls `<boost/math/constants/constants.hpp>`. **Both Eigen and Boost are real native compile-time dependencies, not Python-only packaging details.**
- The exact `SketcherGlobal.h`, `shims/FCConfig.h`, `shims/FCGlobal.h`, `shims/Base/Console.h` and `shims/boost_graph_adjacency_list.hpp` have **no individual SPDX line in their checked source content**; this is a provenance question to resolve, **not a conclusion that their use is forbidden**.
- The upstream `src/wrapper.h` facade also lacks an individual SPDX header and exposes a large C++ API. The proposed narrow SS2-side C ABI is new design work requiring its own license allocation/technical review, not a preexisting exported upstream contract.
- Compilation/ABI portability, completeness of archive source inventory, static/dynamic linker outcome and Windows dependency redistribution remain **UNTESTED**. No direct MSVC DLL build has been performed.

### Candidate *future* source paths, NOT an approved mutation allowlist

Only to support B0 review of the pinned Git tree; actual original sdist inventory can differ:

```text
third_party/planegcs-0.8.0/CMakeLists.txt
third_party/planegcs-0.8.0/src/planegcs/Constraints.cpp
third_party/planegcs-0.8.0/src/planegcs/GCS.cpp
third_party/planegcs-0.8.0/src/planegcs/Geo.cpp
third_party/planegcs-0.8.0/src/planegcs/SubSystem.cpp
third_party/planegcs-0.8.0/src/planegcs/qp_eq.cpp
third_party/planegcs-0.8.0/src/planegcs/Constraints.h
third_party/planegcs-0.8.0/src/planegcs/GCS.h
third_party/planegcs-0.8.0/src/planegcs/Geo.h
third_party/planegcs-0.8.0/src/planegcs/SubSystem.h
third_party/planegcs-0.8.0/src/planegcs/qp_eq.h
third_party/planegcs-0.8.0/src/planegcs/Util.h
third_party/planegcs-0.8.0/src/planegcs/SketcherGlobal.h
third_party/planegcs-0.8.0/src/planegcs/shims/FCConfig.h
third_party/planegcs-0.8.0/src/planegcs/shims/FCGlobal.h
third_party/planegcs-0.8.0/src/planegcs/shims/Base/Console.h
third_party/planegcs-0.8.0/src/planegcs/shims/boost_graph_adjacency_list.hpp
```

Future LGPL text/notice/provenance files, narrow ABI caller/library files, tests, CMake, dependency licenses and installer package paths MUST be audited for exact necessity, named in a finite **Owner-approved D2 addendum**, and separately tested. No `third_party/**` wildcard, no download-at-build-time, no unreviewed code copy, no release of Python extension as substitute. Source inventory above is NOT legal sign-off.

## 4. Owner gate and completion policy

- **D2-A (Fix):** accept / amend / reject exact sixth manual relation semantics above.
- **D2-B (DLL):** choose replaceable LGPL DLL / static-relink / alternative solver, with named compliance review owner. This is a **design preference** until B0 and an exact separate D2 file/path addendum are accepted.
- **B0 archive:** supply or authorize retrieval of the *original* `planegcs-0.8.0.tar.gz` on a network-permitted host; preserve the downloaded bytes, SHA-256, member manifest, license/provenance report. Do not infer that prior failure grants a waiver.
- **B2 persistence & UI:** explicit native vNext schema and 3D Sketch relation-controls Owner signoff in separate scoped decisions. Do not bundle them into the DLL authorization.

This one-file Draft PR is a **decision proposal**, not a Work Contract activation. It requires technical review, exact Owner approval, its own Windows governance CLOSURE and separate guarded-squash authorization for any later proposed normative contract amendment. The existing Draft product PR #506 cannot be merged from any decision here. Do not change protected main without those gates.

## Documentation impact

Internal docs: not required
User/Product docs: not required
Reason: this isolated, unaccepted governance **proposal** does not change any current product behavior, owner implementation boundary or public contract. Once an A1 D2 amendment is accepted and product behavior is delivered, A1's existing **required** internal + bilingual Product Markdown and generated Browser duties remain fully binding.
