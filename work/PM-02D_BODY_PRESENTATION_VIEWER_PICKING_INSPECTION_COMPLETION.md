# PM-02D — Topology-Aware Body Presentation / Viewer Picking / Inspection Completion

**Status:** COMPLETED — PASS  
**Parent Work Contract:** `work/PM-02_BODY_SEMANTIC_TOPOLOGY_FACE_SUPPORTED_SKETCH.md`  
**Final runtime candidate:** `c325fcf681afff44ca61abb02fd239495db5d67f`  
**Final Windows gate:** FULL #1421 — PASS on the exact final candidate  
**Merged final runtime main:** `9dd893ffa0e9cd1e5287b9590c02b317d543cbd3`  
**Scope boundary:** PM-02D only; no schema v9, authored face-supported Sketch mutation, Datum or Projection

## 1. Closure statement

PM-02D is COMPLETED — PASS.

The production application now has one topology-aware committed Body presentation authority, direct current Body topology picking, accepted View Styles, Document Tree Current Feature Contribution overlays and semantic topology Properties/inspection on top of the PM-02A/B/C semantic catalog.

This closes the presentation/selection/inspection checkpoint only. The next active checkpoint is PM-02E — Sketch support schema v9 + deterministic frame resolver.

## 2. Slice evidence

PM-02D was delivered in five bounded runtime slices:

| Slice | Runtime candidate | Windows FULL | Merged main | Result |
| --- | --- | --- | --- | --- |
| D1 — atomic topology-aware BodyScene | `cea8d47d8d09bcf977c1a30fdac144753f70f2d9` | #1405 PASS | `7cffa778eba803f8fb6daeb8e9697905df2af2a9` | PASS |
| D2 — direct Body picking + ViewStyle runtime | `c76d64bbc8ccfc72c3f8eeb5059ec1c5fda62892` | #1406 PASS | `35f40afe39db95232e17022d9d34e1d8afe99c1c` | PASS |
| D3 — hover/cycling + View Style HUD | `5887b43df5970d8baf3d161c477adfe0bb263a8a` | #1416 PASS | `d5e4e008771f260e9eb9681f05e8318c8bc5bb22` | PASS |
| D4 — Current Feature Contribution tree overlay | `549c97bd5979f856d3f6494f8ce7dd4b137a79ce` | #1417 PASS | `09a6ff00cb79deebe65fbd4a7d748c144a800393` | PASS |
| D5 — semantic topology Properties/inspection | `c325fcf681afff44ca61abb02fd239495db5d67f` | #1421 PASS | `9dd893ffa0e9cd1e5287b9590c02b317d543cbd3` | PASS |

The failed intermediate D5 candidates were not accepted as evidence. Final D5 evidence is the exact candidate above after compile-only fixes.

## 3. Delivered production boundary

### 3.1 One committed Body presentation authority

PM-02D provides an atomic topology-aware committed Body scene for one current evaluation/provider generation.

The same current presentation authority supplies:

- shaded Body triangles;
- Face presentation records;
- Edge presentation records;
- Vertex presentation records;
- direct topology picking;
- visible/hidden Edge display;
- Feature Contribution overlays.

No production Face/Edge identity is reconstructed from triangle adjacency.

### 3.2 Generation-scoped selection

Body PresentationTokens are runtime-only and generation-scoped.

Scene/evaluation/provider replacement invalidates stale hover/candidate/selection authority. Numeric token reuse in another generation does not preserve CAD meaning.

Direct picking resolves through the current Part topology catalog rather than persisting Viewer/provider identity.

### 3.3 Direct topology acquisition

Ordinary Body selection implements the accepted interaction boundary:

- visible Vertex before visible Edge before front-visible Face;
- active Face/Edge/Vertex filters;
- bounded screen-space acquisition policy;
- candidate cycling without provider-order semantic authority;
- stale candidate rejection;
- hidden-edge rendering does not enable select-through;
- representation artifacts remain accounted but do not pollute ordinary material picking.

### 3.4 View Styles

The viewport presentation HUD provides:

- Shaded;
- Shaded + Edges;
- Shaded + Hidden Edges.

View Style remains presentation state only: it does not author the Part, dirty the document or create CAD Undo/Redo history.

Visible and hidden Edge presentation reuse the same current topology presentation records.

### 3.5 Current Feature Contribution

Document Tree Feature hover/selection visualizes Current Feature Contribution from the Part semantic query.

The overlay is set-valued and uses the current Body topology tokens. It does not create a second selectable topology authority and does not replace the current Body with a historical stage.

### 3.6 Semantic topology Properties

The existing Properties panel now supports adaptive current topology inspection.

It distinguishes:

```text
current topology presence
!= durable semantic referenceability
```

Inspection covers:

- Face / Surface meaning and Sketch-support capability;
- Edge / Curve meaning, representation-artifact state and adjacency;
- Vertex / Point meaning with XYZ as diagnostics only;
- Body topology/accounting summary;
- Feature Current Feature Contribution summary.

Hover/candidate cycling does not churn the Properties subject. Direct topology selection is cleared when its Body-scene generation/provider authority becomes stale.

No RuntimeFace/Edge/VertexToken, PresentationToken, provider traversal ordinal or TopoDS identity is exposed as product identity.

## 4. Gate result

The final D5 exact candidate passed Windows FULL #1421, including:

- complete desktop build/test graph;
- core-only build/test;
- kernel-native Release build/test;
- FAST/SUBSYSTEM selector verification;
- complete test execution;
- SR-02 latency evidence;
- CI-04 warm FULL parity and comparative timing evidence.

Combined with D1–D4 exact-head PASS evidence, this closes every PM-02D delivery/gate obligation currently owned by the active Work Contract.

## 5. Explicit non-claims

PM-02D does not claim:

- persistent Body topology identity;
- schema v9 completion;
- authored Body-Surface Sketch support;
- stage-aware face-supported Profile materialization;
- repair/re-support lifecycle;
- Datum;
- Projection;
- Fillet/Chamfer.

Those remain owned by later PM-02 checkpoints or later packages.

## 6. Next checkpoint

PM-02E is next:

**Sketch support schema v9 + deterministic frame resolver**

It must introduce the new semantic SketchSupport persistence boundary, migrate valid v8 files without losing IDs/geometry, remove redundant authored absolute world-placement authority, preserve Origin support parity and reconstruct the same support meaning after Save/Reopen.

## Documentation impact

Internal/Product documentation: final as-built and PL/EN product documentation remain explicitly owned by PM-02J under the accepted Work Contract. This checkpoint closure changes only governance/evidence records and does not claim PM-02 package completion.

Product Browser: no canonical product documentation changes in this closure PR; regeneration is deferred to the owning PM-02J documentation checkpoint.
