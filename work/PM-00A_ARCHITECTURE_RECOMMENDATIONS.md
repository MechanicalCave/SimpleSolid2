# PM-00A — Part Modeling Architecture Recommendations

**Status:** OWNER REVIEW COMPLETED — ACCEPTED VIA ADR-0014 ON 2026-10-03  
**Decision class:** D2 evidence synthesis; accepted normative architecture is ADR-0014  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Program authority:** `work/PART_MODELING_V1_ROADMAP.md` v1.3  
**Evidence contract:** `work/PM-00A_PART_MODELING_ARCHITECTURE_EVIDENCE_GATE.md`  
**Final source candidate:** `ac34a713c6fee4a53d513bfcce2a2044ce1a0ffb`  
**Windows FULL:** #1320 — PASS

## 1. Conclusion

PM-00A has sufficient evidence for the Owner to make the PM-00B architecture decision.

The frozen E01-E10 matrix is complete. Stable semantic references survive their declared edits/cold rebuilds; split/merge/deletion/similarity cases fail closed; provider seams and geometry similarity never become identity; stale revision/session/request results do not publish; support frames are deterministic for the currently accepted Origin-plane supports; and the numerical probe separates modeling policy from display/picking state.

No PM-00A STOP condition was triggered.

This document proposes resolutions. It does not accept them.

## 2. Evidence ledger

| Evidence | Primary conclusion |
| --- | --- |
| A0 | semantic/core, kernel-native Release and desktop verification are separate and exact-head testable |
| E01 | Profile boundary-use provenance can define Extrude caps/sides while provider lineage stays transient |
| E02 | producer/consumed stage is semantic context; reference validity is separate from downstream geometric feasibility |
| E03/E04 | singular split/merge cardinality fails closed: 0 Missing, 1 Resolved, >1 Ambiguous unless independent semantic meaning narrows it |
| E05 | geometry similarity is diagnostics/corroboration only and never automatic identity |
| E06 | full-Revolve periodic seam is provider topology, not semantic authored identity |
| E07 | accepted outcomes reconstruct after runtime/provider teardown and cold reopen |
| E08 | current XY/XZ/YZ O/U/V/N support frames are authored/semantic and do not depend on topology/camera |
| E09 | publication authority requires DocumentId + current DocumentRevision + runtime session/request generation |
| E10 | display/pick settings do not enter modeling; invalid gaps are not silently healed; refine/fuzzy policy must be explicit/versioned |

Final combined verification on `ac34a713c6fee4a53d513bfcce2a2044ce1a0ffb`: core-only 19/19 PASS, kernel-native 27/27 PASS, desktop 84/84 PASS.

**Cumulative false-Resolved count: 0.**

## 3. Proposed O-01 — single-Body result semantics

Propose for PM-00B:

- Part v1 has exactly one durable Body; `BodyId` is distinct from `DocumentId`.
- An Empty Body is a valid authored state.
- A successful additive feature must produce exactly one valid solid as the current Body result.
- The first Add may create that one solid from Empty.
- A later Add that creates a detached second solid is invalid for v1; it must not silently become multi-body.
- A no-effect result and a multi-solid result are explicit evaluated failure classes, not successful features and not automatic geometry repair.
- Creation Finish requires a currently valid result; rejected Finish performs no authored mutation.
- If an already-authored feature later becomes no-effect/multi-solid/geometrically invalid because an upstream edit changed, preserve its authored intent/ID and mark evaluation failed; do not publish a stale last-good solid as current truth.
- Downstream evaluation then follows O-09 Blocked/failure semantics.

This is a product/architecture proposal constrained by the accepted single-Body roadmap and one-solid evidence. PM-00A did not introduce a Body schema.

## 4. Proposed O-04 — support-frame and re-support semantics

Propose:

- every Sketch support resolves to a complete right-handed metric `O/U/V/N` frame with `N = U × V`;
- Origin-plane frames remain exactly the E08 mappings: XY U=X,V=Y,N=+Z; XZ U=X,V=Z,N=-Y; YZ U=Y,V=Z,N=+X;
- support identity and explicit placement remain authored Part semantics; camera, Viewer tokens, provider UV parameterization and topology order are never frame authority;
- re-support preserves existing authored Sketch U/V coordinates by default and changes only the host mapping;
- missing/ambiguous support fails closed; no fallback to another plane/face;
- future Datum frames derive from their accepted semantic constructors/parent frame;
- future planar-face support requires a separately specified deterministic semantic frame algorithm tied to the accepted face reference. Until that algorithm is accepted, planar-face support remains Unsupported rather than using raw OCCT face UV or first-edge orientation.

E08 proves the Origin-plane rule; it deliberately does not prove the future planar-face algorithm.

## 5. Proposed O-05 — topology-reference selector and lineage

Propose a durable selector meaning composed from semantic data such as:

`producer FeatureId + consumed/producer stage + semantic role/source provenance + role-local key`.

Exact serialization/type layout remains a PM-00B decision.

Required behavior:

- resolve inside the declared producer/stage/semantic context, not by global final-Body search;
- singular reference cardinality: 0 → Missing, 1 → Resolved, >1 → Ambiguous;
- a split does not select the first fragment;
- deletion does not retarget to a similar neighbor;
- merge collapse does not preserve two distinct meanings merely because provider history says Modified;
- independent producer/role/provenance meaning may legitimately narrow technical candidates to one;
- provider Generated/Modified/Deleted/unchanged data is transient reconstruction evidence only;
- area/centroid/normal/length/proximity are diagnostics only and cannot independently produce Resolved;
- periodic/provider seams without authored semantic source remain Unsupported;
- cold rebuild must reconstruct the same semantic outcome without previous provider objects;
- explicit user repair, when later implemented, updates authored semantic reference intent through one normal Command/Transaction; there is no silent auto-rebind.

## 6. Proposed O-09 — failure, retry, Delete and Suppress

Propose separate states for authored intent, reference resolution and operation execution.

A feature may remain authored while evaluation is:

- UpToDate;
- Failed with structured geometric/reference diagnostic;
- Blocked because an upstream required result is unavailable;
- Suppressed by explicit authored command.

Rules:

- a geometric failure does not rewrite a still-valid input reference as Missing/Ambiguous;
- an existing failed feature keeps its durable FeatureId and editable inputs so repair is possible;
- downstream features must not evaluate against a stale last-good shape as if it were current; they become Blocked where required input is unavailable;
- reevaluation/retry occurs from current authored state and a fresh request generation after relevant changes or explicit recompute;
- cancelled/failed/stale requests permanently lose publication authority;
- Delete removes the target authored feature transactionally; downstream authored objects are preserved unless an explicit later command says otherwise, and unresolved dependencies become Missing/Blocked;
- Suppress is distinct from visibility and Delete, preserves identity/inputs, is Undoable, and removes the feature's contribution for evaluation; downstream semantics are recomputed from the preceding available stage;
- no retry, suppress or failure transition creates hidden Undo history outside semantic Commands.

Exact UI wording and whether a last-good shape may be shown only as non-authoritative diagnostic presentation remain later UI decisions.

## 7. Proposed O-11 — numerical / refine / healing policy

Propose a distinct durable `modelingSemanticsVersion` axis separate from container version, Part schema version, EngineeringRevision and runtime DocumentRevision.

For initial Part-v1 semantics:

- camera, zoom, projection, screen pick aperture, OSNAP display settings and tessellation never enter modeling decisions;
- Profile → Kernel input uses exact evaluated semantic Line/Circle/Arc boundary data, not tessellation;
- no iterative fuzzy escalation and no silent gap healing;
- ordinary provider Boolean fuzzy value is explicit and starts at 0 unless an owning operation contract provides measured evidence for a different fixed value;
- OCCT `Precision::Confusion()` and similar provider constants are implementation diagnostics, not persisted/user modeling tolerance;
- refine/unify is an explicit operation policy, not a hidden provider default;
- the PM-00B candidate should prefer explicit same-domain edge/face unification for Boolean result cleanup where the operation's reference regression proves semantic lineage remains explainable; operations not covered by that evidence must not inherit the policy by accident;
- any change to fuzzy/healing/refine policy that can change geometry/topology/reference outcomes requires a modeling-semantics version change or an explicit compatibility/migration decision;
- unsupported/invalid geometry fails closed instead of increasing tolerance until success.

PM-00A recommends **not** freezing a single user-visible linear “modeling tolerance” from the E10 provider threshold. The provider threshold is too implementation-specific to become authored engineering intent.

## 8. Proposed O-12 — identity, snapshot and read-boundary foundations

Propose:

- `DocumentId` identifies the durable Part definition;
- `BodyId`, `FeatureId`, future `DatumId` and `PublishedReferenceId` are typed Part-local durable identities and are addressed externally together with DocumentId;
- Sketch `EntityId` remains Sketch-local and requires SketchId outside the Sketch;
- local durable ID allocation follows existing non-aliasing/high-water principles so abandoned identities are not silently reused after Undo branching;
- `DocumentRevision` is runtime authored-state freshness only, not a durable engineering/model version;
- E09 session generation/lease and request/evaluation generation are runtime-only and never serialized;
- native file checkpoint remains runtime persistence-concurrency state, not CAD identity;
- future read snapshots are immutable/provider-neutral and carry at least DocumentId, the source DocumentRevision and typed semantic result/reference data needed by the caller;
- future Assembly reads only an accepted Part read/published-reference contract; it does not depend on internal feature objects, Viewer state or OCCT topology;
- transform APIs name direction explicitly using a `parent_from_local` convention: `P_parent = T_parent_from_local(P_local)`; ambiguous unnamed transform direction is not accepted at a domain boundary;
- container/schema/modeling-semantics/EngineeringRevision/DocumentRevision remain separate axes.

The exact snapshot C++ types and final Published Reference surface remain PM-00B/PM-06 decisions.

## 9. O-03 / O-06 proposed Part-v1 product-scope matrix

| Package | Proposed v1 scope | Explicit later/excluded variants |
| --- | --- | --- |
| PM-01 Extrude Add | one valid existing Profile; one-sided linear distance normal to support frame; explicit reverse/direction semantics; one resulting attached solid | symmetric/two-sided, draft/taper, thin-wall, multi-profile/multi-body variants |
| PM-02 Extrude Cut | same bounded one-sided distance grammar against the current Body; Origin-based offset datum support required by the slice | through-all/up-to-face/up-to-next unless separately accepted; arbitrary multi-body target sets |
| PM-03 Projection | exact materialized snapshot curves for Line/Circle/Arc; atomic selected planar-face boundary capture including holes; UnsupportedCurve for unrepresentable exact curves | spline/ellipse/general curve approximation, tessellated/polyline authoritative projection, live cross-feature projected geometry dependency |
| PM-03 Face support | planar-face support only after O-05 reference + deterministic O-04 face-frame algorithm; explicit repair on Missing/Ambiguous | non-planar support; provider-UV/ordinal frame fallback |
| PM-04 Revolve Add/Cut | one semantic axis from accepted straight Sketch line/datum; one direction; bounded angle up to and including full 360°; full-turn seam remains non-semantic | symmetric/two-direction revolve, thin revolve, provider seam references |
| PM-05 Fillet | constant-radius edge set with semantic edge references at the correct stage | variable radius, setback/advanced blend modes |
| PM-05 Chamfer | constant-distance edge set with semantic edge references at the correct stage | advanced asymmetric/multi-mode chamfer unless separately accepted |
| v1 common | exact lifecycle: preview/Finish/Edit/Cancel/Undo/Redo/Delete/visibility/Save/Reopen/failure repair for every activated variant | Loft, Sweep, Shell, Draft, Pattern, solid Mirror, general surfaces/direct face editing, multi-body |

Owning package contracts may narrow this matrix. Expanding it requires explicit scope acceptance; no package may silently broaden variants because OCCT exposes them.

## 10. Reference survival/failure summary

| Condition | Proposed status |
| --- | --- |
| unique semantic descendant at declared stage/role | Resolved |
| semantic source removed, even with identical replacement geometry | Missing |
| singular source splits into multiple valid descendants | Ambiguous |
| distinct meanings merge with no independent winner | Ambiguous |
| requested aggregate/set meaning not declared | Unsupported |
| provider seam with no authored source | Unsupported |
| valid reference but downstream Kernel operation fails | reference remains Resolved; operation Failed |
| stale revision/session/request completion | rejected publication; never current |
| geometry similarity only | cannot produce Resolved |

## 11. Risks / counterarguments

- Explicit refine/unify can reduce technical topology but can also collapse distinctions; O-05 must remain authoritative and regressions must accompany every operation using the policy.
- Rejecting no-effect features is simpler and cleaner but may feel stricter than some CAD products; accepting them would require a clear semantic reason rather than provider convenience.
- Preserving authored failed/downstream objects improves repairability but makes evaluator/status UI more complex than deleting invalid dependents.
- Snapshot-only projection avoids hidden dependencies but requires explicit Refresh/Reproject UX when users want updated geometry.
- Deferring the exact planar-face frame algorithm avoids premature provider coupling but means PM-03 must include a real architecture sub-gate before face-supported Sketch product work.
- A separate modeling-semantics version adds persistence/replay complexity, but omitting it risks silent result changes when refine/fuzzy/provider policy changes.

## 12. Completion boundary

The evidence is sufficient to enter PM-00B Owner architecture review.

No recommendation above is accepted merely because PM-00A is complete. The proposed ADR in `work/PM-00A_PART_FEATURE_ARCHITECTURE_ADR_CANDIDATE.md` and proposed PM-00B Work Contract in `work/PM-00B_PART_FEATURE_ARCHITECTURE_FREEZE.md` require explicit Owner acceptance before they can become architecture/implementation authority.
