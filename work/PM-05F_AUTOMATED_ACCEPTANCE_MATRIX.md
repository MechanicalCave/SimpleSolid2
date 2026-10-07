# PM-05F — Automated Acceptance Matrix

**Status:** DOCS PASS / OWNER FAIL — Fillet bounded remediation active; fresh exact-head FULL and repeat Owner workflow required
**Parent Work Contract:** work/PM-05_EDGE_FEATURES.md
**Program authority:** work/PART_MODELING_V1_ROADMAP.md v1.29
**Checkpoint:** PM-05F — documentation / cumulative automated evidence / Owner Windows acceptance
**Date:** 2026-10-07

## Purpose

This matrix closes the automated-evidence side of PM-05 without redefining accepted Fillet/Chamfer semantics. PM-05A through PM-05E are already completed and merged. PM-05F makes current documentation authoritative, regenerates the Product Browser, runs one final exact-head Windows FULL and prepares the supported-Windows Owner workflow.

Owner manual acceptance reported FAIL on 2026-10-07 before final FULL dispatch due to unstable multi-Edge Fillet behavior. Existing DOCS PASS remains valid as documentation evidence, but final automated/package acceptance is blocked until bounded remediation receives fresh exact-head evidence.

## Evidence authority

| Checkpoint | Exact candidate | Gate | Merged main | Primary evidence |
| --- | --- | --- | --- | --- |
| PM-05A | c70980d63d836ea54d1f88be29082023e44b1c38 | Windows FULL #1605 PASS; report-sync CLOSURE #1606 PASS | aa9e86f6b88ebf429390f7a4764a534b400e757b | provider/topology evidence; Owner-accepted E1/M1/T1/U1/C1/P1-P3/G1-G2/R1/K1 |
| PM-05B1 | abb758a17e52849db4345e03dc84915543718ad4 | Windows FULL #1613 PASS | bfe197b78ff1bc81a8e2e4bd26f2f3e204b429ab | MaterialEdgeReference / Feature model / schema v14 |
| PM-05B2 | 9b6c6318599ec85300c7b3ebb0ebc24720d5c287 | Windows FULL #1617 PASS | 13e2c14505be6d25b733b95c71ca891d16a7e4b4 | strict resolver / application draft foundation |
| PM-05B3 | 384835c58c5823e5f5ac20c619eb960f3a5acb7a | Windows FULL #1624 PASS | cab3cea663a07102e1749d08dc027a85764db790 | generated-Surface durable provenance |
| PM-05C1 | 2f55b8382b6fe8022ad26843a7f0eb4dcb15bd1c | Windows FULL #1632 PASS | 9547b986ccbd4dc7533c446c797eb5c4886ad0a6 | provider-neutral Edge-feature operation/evaluator |
| PM-05C2 | e5826858a1ef2580a3c00913f94791653696dc50 | Windows FULL #1660 PASS | 85c7df1395c59a35b1a80fdf446cdb9c2f77775c | production OCCT Fillet/Chamfer + topology lineage |
| PM-05D | 7f57726b368c8a5011f8007369f3de945ae0d833 | Windows FULL #1697 PASS | 83ece0b03889e11f8dc7769498d949caee3c0be1 | Workbench / Viewer / exact preview / Command Line |
| PM-05E | 88e07b1a2dbdb6ded73d0c2f90dd0a1443242cbe | Windows FULL #1721 PASS — core 25/25, kernel-native 54/54, desktop 112/112 | 1a1491f7687f32089c1424025123329c74d7b6ca | Edit / repair / lifecycle / persistence / reverse-chain canonical-frame remediation |
| PM-05F docs | 19700794577a516281fcc3e512c898cc1ececfec | Windows DOCS #1725 PASS; synchronized matrix head d08e97c19e2717dbb9e4bc81adf9d11ac1925f84 passed DOCS #1726 | 685bf3f797aa022582952906e8f24cf6d668f2bb | internal + paired PL/EN Product + generated Browser |
| PM-05F final automated | pending exact candidate | Windows FULL pending | pending | cumulative final runtime/docs gate |
| PM-05F Owner Windows attempt 1 | 7721863e22269474f8f4cca6315eafed86396eff | OWNER FAIL — multi-Edge Fillet blocker | not closable | work/PM-05F_OWNER_WINDOWS_ACCEPTANCE.md |

## Contract matrix

| PM-05 acceptance row | Automated authority | State |
| --- | --- | --- |
| toolbar Create/Modify grouping | PM-05D / pm05d2.cad_workbench_edge_features | PASS |
| selection-first and command-first multi-Edge Fillet | PM-05D | PASS |
| selection-first and command-first multi-Edge Chamfer | PM-05D | PASS |
| disconnected Edge set | PM-05A provider matrix / PM-05C production provider | PASS |
| two-edge connected corner | PM-05A / PM-05C | PASS |
| three-edge trihedral corner | PM-05A / PM-05C | PASS |
| closed loop | PM-05A / PM-05C | PASS |
| mixed connected/disconnected set | PM-05A / PM-05C | PASS |
| invalid excessive Radius/Distance -> Failed, no partial result | PM-05A/C | PASS |
| upstream preserved Edge -> Resolved/recompute | PM-05A + pm05e4.edge_lifecycle_matrix | PASS |
| upstream deleted Edge -> Missing/Blocked | PM-05A/E + pm05e4.edge_lifecycle_matrix | PASS |
| upstream split singular Edge -> Ambiguous/Blocked | PM-05A/B/E | PASS |
| merge without unique strict winner -> Ambiguous/Blocked | pm05e4.edge_lifecycle_matrix | PASS |
| no first/nearest/longest fallback | PM-05A/B/E | PASS |
| seam and same-Surface partition not authorable | PM-05B/C/D | PASS |
| explicit input set unaffected by provider traversal order | PM-05A M1 / C | PASS |
| no silent tangent-chain authored expansion | PM-05A T1 + production contour membership enforcement | PASS |
| generated ordinary engineering Edge usable by later accepted edge Feature | PM-05C2b/E3 | PASS |
| Fillet -> Chamfer | PM-05A/C/E3 | PASS |
| Chamfer -> Fillet | PM-05A/E3; canonical-frame lineage remediation in PM-05E | PASS |
| exact whole-candidate preview / stale preview rejection | pm05d1.edge_feature_draft_preview / PM-05D2 | PASS |
| Edit preserves FeatureId | pm05e1.edge_feature_edit / PM-05E2 | PASS |
| Cancel/rejected edit zero mutation | PM-05E1/E2 | PASS |
| explicit Missing repair; no automatic rebind | pm05e2.cad_workbench_edge_lifecycle | PASS |
| Missing/Ambiguous/Unsupported structured distinction + failing input | PM-05E2/E4 | PASS |
| Delete/Suppress/Undo/Redo | PM-05E2 | PASS |
| v13 -> v14 migration / no synthetic edge Features | PM-05B1 | PASS |
| generated Surface provenance persistence | PM-05B3 | PASS |
| Save/Close/Reopen | PM-05E3 | PASS |
| true cold reconstruction without provider/runtime identity persistence | pm05e3.edge_feature_cold_rebuild | PASS |
| current internal as-built docs | 19700794577a516281fcc3e512c898cc1ececfec / Windows DOCS #1725 | PASS |
| paired PL/EN Product docs | 19700794577a516281fcc3e512c898cc1ececfec / Windows DOCS #1725 | PASS |
| generated Product Browser freshness | 19700794577a516281fcc3e512c898cc1ececfec / Windows DOCS #1725 | PASS |
| final exact-head automated package gate | post-remediation PM-05F replacement candidate | PENDING FULL |
| Owner supported-Windows workflow | post-remediation PM-05F replacement candidate | FAIL — REPEAT REQUIRED |

## Documentation closure requirements

Current-state docs must describe strict stage-scoped MaterialEdgeReference and branch discrimination; canonical explicit multi-Edge sets; no click-order authority; provider contour-membership enforcement; no silent tangent expansion; no generic refine/unify/healing/fuzzy escalation; generated topology provenance; Create/Modify organization; selection-first/command-first interaction; exact preview; FeatureId-preserving Edit from the exact predecessor stage; explicit Missing/Ambiguous/Unsupported repair; Failed versus Blocked; Suppress/Delete/Undo/Redo; schema-v14 Save/Reopen and true cold reconstruction; both chain directions; and the accepted bounded variants/exclusions.

The Product Browser is generated from canonical Markdown and must not be edited as an independent source.

## Completion rule

Automated PM-05F is complete only when the exact final runtime/docs candidate passes the Windows FULL tier including canonical documentation and Browser freshness checks.

That automated PASS still does not close PM-05. Only the Owner's explicit supported-Windows PASS recorded in work/PM-05F_OWNER_WINDOWS_ACCEPTANCE.md authorizes final package closure.

## Documentation impact

Internal docs: required — PM-05F synchronizes as-built architecture/lifecycle/persistence documentation.
User/Product docs: required — Fillet/Chamfer are current user-visible Part features.
Product Browser: required — generated from the updated canonical Markdown.
