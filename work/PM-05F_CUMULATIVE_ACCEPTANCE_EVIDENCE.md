# PM-05F — cumulative automated acceptance / Owner evidence

**Status:** automated technical gate PASS on runtime/docs candidate `5996583e99b20bbad16e252abb2f56c6904d69ef`; aggregate Owner acceptance not yet itemized
**Authority:** `work/PM-05_EDGE_FEATURES.md` §27 and `work/PM-05F_OWNER_WINDOWS_ACCEPTANCE.md`
**Date:** 2026-10-08
**PR:** [#298](https://github.com/MechanicalCave/SimpleSolid2/pull/298) — Draft until all final gates

## Exact automated acceptance gate

- [Windows FULL #1902](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37846779905): `workflow_dispatch`, branch `pm-05f-r2-owner-remediation`, exact SHA `5996583e99b20bbad16e252abb2f56c6904d69ef`. `classify`, `windows-msvc-full` and aggregate `windows-msvc` all **success**.
- FULL #1902: Core Release without Qt/OCCT **25/25**; kernel-native Release with OCCT **57/57**; desktop Debug native **113/113**, all PASS, total **195/195**, zero failures. Documentation verification, Product Browser regeneration/freshness, test-tier checks and benchmark gate passed.
- Earlier [FAST #1900](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37844449758) **93/93 PASS** on the same final runtime/docs SHA; [FOCUSED #1898](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37842345000) **1/1 PASS** on the immediately preceding runtime/test candidate `af85a6fafea643739393b4f29673eca56c666519`.
- After #1902, subsequent edits limited to `work/**` are evidence/governance only. They do not mutate compiled runtime or canonical product docs. The repository classifier searches only `event=pull_request` for historical trusted FULL ancestry; manually dispatched FULL #1902 may therefore not be reusable by its automated ancestry calculation. Treat any new automatic CI result on the documentation-sync SHA as a separate gate, not as retroactive evidence on #1902.

## Work Contract §27 — evidence traceability

These are existing executable suite names and accepted earlier package records. **PASS of a suite is not a claim of an individually observed Owner UI step.** Specific fixtures demonstrate supported cases; they do not establish arbitrary-geometry completeness.

| # | Mandatory scenario | Primary executable / package evidence |
| --- | --- | --- |
| 1 | Toolbar Create/Modify grouping | `pm05d2.cad_workbench_edge_features`; PM-05D completion |
| 2 | Fillet selection-first and command-first multi-Edge | `pm05d2.cad_workbench_edge_features`; `pm05d1.edge_feature_draft_preview` |
| 3 | Chamfer selection-first and command-first multi-Edge | Same PM-05D suites |
| 4 | Disconnected Edge set | `pm05a.edge_feature_provider_evidence`; `pm05c2a.production_edge_provider` |
| 5 | Two-Edge connected corner | `pm05a.edge_feature_provider_evidence`; `pm05c2b.part_edge_topology` |
| 6 | Three-Edge trihedral corner | `pm05a.edge_feature_provider_evidence`; `pm05f.fillet_corner_diagnostics`; `pm05d2.cad_workbench_edge_features` |
| 7 | Closed explicit Edge loop | `pm05a.edge_feature_provider_evidence` (accepted PM-05A native matrix) |
| 8 | Mixed connected/disconnected Edge set | `pm05a.edge_feature_provider_evidence` (accepted PM-05A native matrix) |
| 9 | Invalid excessive Radius/Distance, no partial result | `pm05c1.edge_semantic_kernel`; `pm05d1.edge_feature_draft_preview` |
| 10 | Upstream preserved Edge resolves/recomputes | `pm05b2.strict_edge_resolver`; `pm05e4.edge_lifecycle_matrix` |
| 11 | Deleted source Edge becomes Missing/Blocked | Same PM-05B2/PM-05E4 suites |
| 12 | Split singular Edge becomes Ambiguous/Blocked | Same PM-05B2/PM-05E4 suites |
| 13 | No first/nearest/longest semantic fallback | `pm05b2.strict_edge_resolver`; `pm05e4.edge_lifecycle_matrix` |
| 14 | Seam and same-Surface partition not authorable | `pm05b2.strict_edge_resolver`; `pm05c2b.part_edge_topology`; R2 native Revolve evidence |
| 15 | Provider traversal order cannot alter authored input | `pm05a.edge_feature_provider_evidence`; `pm05c2a.production_edge_provider`; `pm05f_r2.native_workbench_edit` |
| 16 | No implicit tangent-chain authored expansion | `pm05a.edge_feature_provider_evidence`; `pm05c2a.production_edge_provider` |
| 17 | Generated ordinary Edge usable downstream | `pm05b3.generated_surface_reference_v14`; `pm05c2b.part_edge_topology` |
| 18 | Fillet → Chamfer | `pm05e3.edge_feature_cold_rebuild`; PM-05E completion |
| 19 | Chamfer → Fillet | `pm05e3.edge_feature_cold_rebuild`; PM-05E completion |
| 20 | Edit preserves FeatureId | `pm05e1.edge_feature_edit`; `pm05e2.cad_workbench_edge_lifecycle` |
| 21 | Cancel/rejected edit is mutation-free | PM-05E1/E2 suites; `pm05f_r2.native_workbench_edit` |
| 22 | Delete/Suppress/Undo/Redo lifecycle | `pm05e4.edge_lifecycle_matrix`; `pm05e2.cad_workbench_edge_lifecycle` |
| 23 | v13 → v14 native persistence migration | `pm05b1.edge_feature_schema_v14` |
| 24 | Save/Close/Reopen | `pm05b1.edge_feature_schema_v14`; `pm05e3.edge_feature_cold_rebuild` |
| 25 | True cold reconstruction | `pm05e3.edge_feature_cold_rebuild`; `pm05f_r2.native_workbench_edit` |
| 26 | Internal + PL/EN Product docs and generated Browser | `docs/internal/{PART_DOCUMENTS,CAD_WORKBENCH_VIEWER}.md`; `docs/product/{pl,en}/PARTS.md`; `docs/browser/index.html`; FULL #1902 documentation verification |
| 27 | Final Owner Windows PASS | **Targeted manual R2 PASS recorded**, but complete 41-step Owner matrix was not separately itemized; aggregate acceptance **PENDING Owner confirmation** |

### Additional R2 / Owner Part008 regression

- `pm05f_r2.native_workbench_edit`: actual Qt/OCCT Edit-stage Edge selection, mixed local signed preview, successful Finish/visible current Body, one-Feature lifecycle, Save/Reopen and staged semantics.
- Owner Part008 three-Edge Chamfer: 6 permutations × distances 1/0.5/0.25 mm = **18** native kernel variants; exact three authored source Edge strips, two distinct source-Vertex corner carriers and verified incidence of both source Edges at their own source Vertex. All **21** resulting Faces have unique semantic ownership; invalid duplicate and unknown runtime-stage Edge tokens fail closed.
- Current production scope remains the Owner-approved bounded D2-B provider-private planar miter fallback. It does not establish blanket validity of arbitrary multi-Edge geometry, general tolerance/healing policy or globally planar two-Edge corners.
- R2-F full structured reason-specific picking diagnostics requires a separate unapproved D2 Viewer query contract. No new public diagnostics metadata is authorized or claimed in this completion evidence.

## Manual Owner evidence and residual gate

- Owner personally exercised earlier R2-A Body visibility and mixed local Fillet Part008 preview/Finish as recorded in the R2 remediation history.
- On the runtime/test candidate `af85a6fafea643739393b4f29673eca56c666519`, Owner reported **"manual test - pass"** after receiving the focused Part008 3-Edge Chamfer / Edit / Undo / Save-Reopen / P1 picking instructions. This is a **scoped manual PASS**; the report did not enumerate 41 outcomes from the full PM-05F Owner Windows workflow.
- The FULL candidate `5996583e99b20bbad16e252abb2f56c6904d69ef` differs from that Owner-tested runtime/test candidate only through evidence/governance files under `work/**`. There are no post-Owner product runtime/test changes. This preserves runtime equivalence, not an invented new manual test.
- **Blocker before claiming PM-05 COMPLETED:** explicitly confirm the aggregate Owner workflow scope, including unsupported/rejected cases and downstream repair. Then reconcile final CI on the evidence-only commit, release status and merge authority. Do not merge #298 or activate PM-06/Projection solely on this matrix.

## Authority boundary

This file describes evidence, not new product semantics or an architecture amendment. ACTIVE continues to point at PM-05 while its closure prerequisites remain open; any future sequencing change for Projection/PM-06 needs its own explicit Owner D2 roadmap decision.
