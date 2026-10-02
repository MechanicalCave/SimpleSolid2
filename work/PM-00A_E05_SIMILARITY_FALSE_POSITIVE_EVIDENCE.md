# PM-00A — E05 Similarity False-Positive Evidence

**Status:** COMPLETED — PASS  
**Date:** 2026-10-02  
**Exact source candidate:** `7e0d6109cf927a2e8d85d2794b5718dcc2feab22`  
**Windows FULL:** #1252 — PASS  
**Regression:** `pm00a.e05_similarity_guardrails`  
**Matrix:** `work/PM-00A_E01_E10_EVIDENCE_MATRIX.md`

## Result

E05-01 through E05-04 PASS.

False-Resolved count: **0**.

The evidence demonstrates that surface type, area, centroid, canonical surface axis and geometric proximity may be useful diagnostics, but they do not create SS2 reference identity. `Resolved` requires semantic lineage/provenance.

## Evidence summary

| Row | Expected | Actual | Cold | Result |
| --- | --- | --- | --- | --- |
| E05-01 | geometrically similar faces remain distinct semantic targets | two equal coplanar Extrude side faces share the same diagnostic surface class/area/axis, but each resolves uniquely through its own Profile boundary-use provenance | YES | PASS |
| E05-02 | removed semantic target with a near/identical replacement remains Missing | source Line EntityId is erased and replaced by a new EntityId with the same geometry; old provenance resolves Missing while the replacement exists as a separate decoy | YES | PASS |
| E05-03 | surviving semantic target does not switch to a geometrically closer decoy | upstream edit moves the semantic top side far enough that a different side is closer to the old centroid; exact provenance still resolves the intended top side | YES | PASS |
| E05-04 | similarity-only evidence is never sufficient for Resolved | two matching diagnostic classes return Ambiguous; a unique geometry-only diagnostic class returns Unsupported; neither path can return Resolved | N/A | PASS |

## Diagnostic surface

The neutral evidence layer exposes optional transient face diagnostics for the bounded PM-00A probe:

- surface kind;
- area;
- centroid;
- canonical surface axis.

These fields exist to prove that geometrically similar candidates can be deliberately constructed and observed. They are not selectors and are not persisted authored identity.

The E01 regression was tightened accordingly: changes in Extrude distance are allowed to change geometry diagnostics while semantic side role, provenance, reference status and candidate cardinality remain stable.

## False-positive guardrail

The accepted evidence rule after E05 is:

```text
semantic provenance / producer role / lineage
        ↓
candidate interpretation
        ↓
Resolved / Missing / Ambiguous / Unsupported

geometry similarity
        ↓
diagnostic evidence only
        ↓
never creates Resolved
```

A candidate that is nearest, equal-area, same-normal, same-surface-class or even geometrically identical is not allowed to inherit a removed semantic target.

## Cold-rebuild result

E05-01, E05-02 and E05-03 are replayed after Save/Close/Reopen from durable Part/Sketch/Profile state.

The cold replay does not depend on:

- previous-process OCCT handles;
- provider caches;
- Viewer tokens;
- topology ordinals;
- previous transient face objects.

The removed-target scenario remains Missing after cold rebuild.

## Verification history

The final accepted source candidate is `7e0d6109cf927a2e8d85d2794b5718dcc2feab22` with Windows FULL #1252 PASS.

Earlier runs were retained diagnostically:

- #1245 proved `pm00a.e05_similarity_guardrails` itself PASS, but exposed an E01 regression caused by comparing newly added geometry diagnostics as if they were semantic identity;
- E01 was corrected to compare semantic role/provenance/status/cardinality rather than mutable diagnostics;
- #1248 exposed a focused-kernel dispatcher defect before any E05 test executed: top-level `ss2.ps1` did not forward `-NoConfigure`;
- PR #120 fixed that bounded verification-infrastructure defect and Windows FULL #1249 passed;
- #1252 is the final combined exact-head FULL after synchronizing E05 with the corrected `main`.

## Architecture implication

E05 supports the O-05 direction that persistent topology references must be interpreted through semantic producer context and lineage, with geometry characteristics used only as diagnostics or corroborating evidence.

This result does **not** freeze a serialized selector schema. PM-00B owns that architecture decision.

## Scope boundary

E05 introduced no:

- durable BodyId / FeatureId persistence;
- product Part Feature Tree;
- product Extrude/Cut/Fillet command;
- persisted B-Rep;
- persisted OCCT handles;
- similarity-based automatic topology identity;
- user-visible reference repair UI.

The next evidence package is **E03 + E04 — split/merge cardinality semantics**, followed by E02 multi-stage lineage.
