# RG-02B1/P4-S3A — add native Workbench gesture integration test path

**Status: PROPOSED OWNER D2 TEST-ONLY ADDENDUM — NOT ACCEPTED OR ACTIVE.**
**Date:** 2026-10-14.
**Parent executable Work Contract:** `work/RG-02B1_D2_P4_S3A_DATUM_END_MID_GUI_WORK_CONTRACT_DRAFT.md`, explicitly Owner-accepted in #382 with strict UI consistency, identical END/MID hover and existing Command Line, activated separately by #385. Protected-main baseline `311de7859f0e1cbf5ee52e4e6ccf8a364b30c445`.
**Implementation candidate:** PR #386, isolated clean post-activation S3A implementation. Windows Ready FULL #3007 and #3008 each **199/199 PASS** on distinct exact source SHAs; #3008 at `7a1f8f78702cdfde2e5cc50b94f2313eab9cf923` confirms actual Qt/OCCT MID-over-Edge hit priority and both pointer event orders. This DOES NOT prove end-to-end Part semantics or Owner manual GUI acceptance.

## Testability gap, not additional product scope

The existing accepted `tests/sk04b_viewer_native_selection_query_test.cpp` already runs real Qt/OCCT Viewer and tests direct MID and END/Edge pointer routing, but has no Part Workbench or certified native Part source. The accepted `tests/pm03c2_cad_workbench_datum_plane_test.cpp` exercises Operations and Command Line/invalid-kernel fail-closed paths, but its kernel is deliberately an EmptyKernel, not a real provider. The accepted `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp` proves actual OCCT S2 semantic geometry/persistence but is not a Qt Workbench target. A complete positive S3A Workbench + native provider + mouse-to-transaction test would otherwise require changing prohibited build/CI targets.

Already registered `tests/pm05f_r2_native_workbench_edit_test.cpp` uniquely offers all the prerequisites **without changing CMake/CI**: a real `QtOcctViewerWidget`, `ui::CadWorkbench`, `kernel_occt::OcctSolidModelingKernel`, Part/Application and `Qt6::Test`, linked in the existing `pm05f_r2.native_workbench_edit` test target (full-only). It already contains real desktop pointer gestures and independent cold persistence witnesses. It is **NOT** currently in the exact accepted S3A §3 allowlist.

## Exactly ONE additive path and finite accepted edits (if Owner approves)

Add to S3A §3 allowlist **only**:
- `tests/pm05f_r2_native_workbench_edit_test.cpp`.

Allow adding an **isolated** S3A block to this existing registered native integration test, preserving all prior PM-05F/PG-01C and other feature test assertions and mode-routing. New tests must exercise actual native current Body scene, viewport hover and clicks in both `Edge→END/MID` and `END/MID→Edge` orders including shared-corner chosen-Edge END; same MID marker hover/click priority; Operations and global Command Line `DATUMPLANE`, `REVERSE`, `FINISH`/owning-context empty Enter, `CANCEL`/Esc and focus; current-stage certified provider-derived Frame3; one authored DatumId and Undo; real Edit semantic source replacement preserving ID/dependents; Save/Close/Reopen on an independent native provider; stale/ambiguous/off-line unsupported failures change zero Part state. All persistence must use already accepted S2 native v17 and same transaction semantics, never screen XYZ/token.

The test may reuse existing native helpers and real Workbench session; it must not import fake geometry, relax any existing negative, swallow unsupported input, or mark a mock-only GUI result as real positive evidence. If the existing test does not make this possible without modifying `CMakeLists.txt`, any other `src/**`, UI shell or shared public APIs, **STOP** for a new explicit D2 file-scope decision. No new test executable, CMake/CI changes, new public APIs, persistent Part schema, viewer point semantics or extra command grammar are authorized.

## Owner gate, execution, acceptance

Owner expressly accepts this **single pre-existing test file** addendum → one-document work-only governance PR exact-head Windows closure and guarded protected-main squash → separate `work/ACTIVE.yaml`-only activation entry retaining the already-active S3A contract pointer, with its own Windows governance → only then add positive native Workbench regression to PR #386. Follow real FOCUSED/FAST/Ready FULL including this actual full-only native test, required internal + bilingual product docs with official Windows `.\\ss2.ps1 docs` Browser regeneration, and Owner manual click-path FINAL PASS. This test scope alone is not S3A product acceptance or authorization to merge #386.

## Documentation impact

Internal docs: not required
User/Product docs: not required
Reason: this proposal is a bounded additional test-only path for already accepted S3A behavior; the parent S3A implementation still separately requires internal and PL/EN product documentation.
