# RG-02C/C1-P1-B1 H1 — native v20 schema header: exact-path D2 addendum proposal

**Status: OWNER ACCEPTED EXACT ONE-PATH H1 D2 AMENDMENT (2026-10-16); THIS GOVERNANCE PR IS STILL OPEN/UNMERGED, AND THE ACTIVE H1 WORK CONTRACT STILL NEEDS ITS OWN ONE-PATH AMENDMENT AND PROTECTED-MAIN INTEGRATION BEFORE EDITING THE NEWLY APPROVED HEADER.**
**Date:** 2026-10-16.
**Authority:** Constitution/Foundation/Architecture, accepted ADRs, Part Modeling v1.33, Owner-accepted and merged [P1-B1/H1 headless Work Contract](RG-02C_C1_P1B1_H1_STRICT_CYLINDRICAL_FACE_AXIS_HEADLESS_D2_WORK_CONTRACT_PROPOSAL.md) [#440](https://github.com/MechanicalCave/SimpleSolid2/pull/440), correctly activated through [#443](https://github.com/MechanicalCave/SimpleSolid2/pull/443) on main `249d677cdb98213838643a02ca9d045f78e6d6a8`, after prior DOC-01 activation RED #3365 and independently integrated one-file DOC-01 remediation #442. Owner explicitly authorized headless H1 implementation and activation, but **H1 §2 itself explicitly forbids mutating paths outside its 18-file allowlist without Owner-accepted exact D2 addendum**.

## Recorded Owner exact-path D2 acceptance — 2026-10-16

After the Agent explicitly asked: **“czy akceptujesz dodanie `src/part/include/simplesolid2/part/part_document_store.hpp` jako 19. dozwolonej ścieżki H1, wyłącznie w celu aktualizacji wersji schematu z 19 do 20?”**, the Owner answered **`akceptuje - kontynuuj`**. This is an **explicit D2 acceptance of precisely the single additional header path and narrowly defined v19→v20 schema constant edit** below. All other H1 contract and architecture/UX decisions remain unchanged, including legacy reader support, strict singleton Face option A, no B/C fallback, and deferred B0 native GAP.

This acceptance **does not** authorize guarded squash merging PR #445 merely by approving its text; the corrected OWNER-ACCEPTED PR HEAD requires a fresh exact-head Windows governance CLOSURE and **separate protected-main merge authority**. Nor does acceptance by itself mutate the already active H1 contract §2, which must independently be amended and integrated to make the 19th executable path available to implementation PR #444. Until these completed gates, the header remains **NOT MUTABLE**, and writer v20 remains **NOT YET IMPLEMENTED / NOT TESTED**.

## 1. Actual source discovery: existing v19 constant is outside H1 §2

The current native Part schema writer uses **`PartDocumentStore::current_schema_version`**, defined in **`src/part/include/simplesolid2/part/part_document_store.hpp`**, line **89**, as:

```cpp
static constexpr int current_schema_version = 19;
```

The existing writer in `src/part/part_document_store.cpp` references that symbol while setting the native descriptor; the native reader refuses descriptors newer than that same constant. Yet the accepted H1 §1 and §2 requires an **explicit new v20 native writer** for the third `StrictCylindricalFaceAxisSource{SurfaceReference}`, with existing v19/earlier `sketch_line` and `straight_body_edge` layouts/readers preserved. The approved 18 paths include `src/part/part_document_store.cpp`, but **omit its header**. Thus the v20 promise cannot be implemented truthfully within the exact current allowlist; leaving the constant 19 would yield an invalid schema claim, and changing the header without amendment would breach AGENTS.md fail-closed scope control.

This is an actual source-path contradiction detected **before** any v20 writer change; it is not an executed test failure, not a native GAP being promoted to PASS, and not a reason to weaken a test or shadow a version constant.

## 2. Requested narrow Owner D2 amendment

**OWNER ACCEPTED: add precisely ONE executable file to H1 §2** (19 rather than 18), with exact purpose:

```
src/part/include/simplesolid2/part/part_document_store.hpp
```

**Allow exactly:** change the existing `PartDocumentStore::current_schema_version` declaration **from `19` to `20`**, with immediately relevant comments only; maintain the class API, writer/reader responsibility, and all unrelated declaration/layout unchanged.

**Already authorized, unchanged:** existing `src/part/part_document_store.cpp` implements explicit v20 `cylindrical_body_face_strict` source encoding, source `SurfaceReference` stage+address, and strict native validation; existing allowed headless Axis and native tests implement the required compatibility. No new `src/ui/**`, CMake/CI, generated Browser hand edits, `tests/CMakeLists.txt`, product UX or feature semantics. **Do not add other file paths under this proposal.** Additional actual real Windows RED assertions requiring other files must be separately reported and separately amended with their own exact names and proof, not guessed into this gate.

**Not changed by this addendum:** option A singleton strict bounded Face, no B/C same-Surface partition inference, P1-A analytic cylinder provider, normal unoriented Axis direction, existing `AxisId`, Undo/Redo, Revolve Reverse, and explicitly deferred native Face partition/orientation/degenerate GAPs. Preserve old file reader behavior; no silent migration into Body Edge or Face Axis kinds. UI H2 still separately scoped and not active.

## 3. Gate order and decision requested

1. **CLOSED — Owner explicitly accepted this exact one-path D2 amendment on 2026-10-16** independently of prior broad implementation intent. The header still remains **STOP / NOT AUTHORIZED FOR CODE MUTATION** until the separately governed contract amendment is on protected main.
2. **Owner acceptance RECORDED in this governance-only addendum**. **PENDING** fresh exact-head Windows governance CLOSURE on the acceptance-record HEAD, and **separate Owner protected-main guarded squash authority**.
3. Update the **already active H1 Work Contract** by a separate bounded governance-only exact-path amendment declaring a 19th path; pass exact-head Windows CLOSURE and guarded squash **before editing the newly allowed header**. Do not modify `work/ACTIVE.yaml` since it already references the H1 contract.
4. Only then change the version in header and actual writer/reader in the activated production H1 implementation PR, with real Windows native FOCUSED / independent FAST / Ready FULL Core+native Kernel+Qt/OCCT Desktop, DOC-01 and final exact-head CLOSURE; any RED requiring more paths → new explicit STOP.

**Current implementation:** [headless H1 WIP #444](https://github.com/MechanicalCave/SimpleSolid2/pull/444) is already confined to its current 18 authorized paths (Axis source, strict native query/authoring, existing native test). It **does not** modify the omitted header or claim native v20 writer/reader PASS. Genuine first new native Windows FOCUSED #3370 is queued/being checked; its result must not be assumed. Future full H1 code merge still needs separate Owner technical FINAL PASS and protected-main guarded squash.

## Documentation impact

Internal docs: not required
User/Product docs: not required
Reason: This is a one-file governance-only exact allowed-path amendment proposal; it changes no shipped identity, schema or user behavior. The activated H1 contract already declares `Internal docs: required` for the future actual headless v20 implementation, which remains obligatory.
