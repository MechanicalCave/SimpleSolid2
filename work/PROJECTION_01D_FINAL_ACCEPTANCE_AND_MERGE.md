# PG-01D — final acceptance and merged closure evidence

**Status:** COMPLETED — Owner practical FINAL PASS and separately authorized squash MERGE, 2026-10-11.  
**Active Work Contract:** `work/PROJECTION_01D_PLANAR_FACE_BOUNDARY_WORK_CONTRACT.md`, including accepted D2-F1–F6, D2-V16, manual multi-Face Face Boundary extension, and D2-R1–R4 Refine Add/strict-version decision.  
**Accepted PR:** [#309](https://github.com/MechanicalCave/SimpleSolid2/pull/309), final source HEAD `45e097af752b748888050dc51f866d3053431fd9`.  
**Merged `main` squash commit:** `5adc94f1463ad1c5b0785b3027dc5d93f830a112` (GitHub reports PR merged and the `main` branch points to that commit).  
**Owner manual acceptance:** direct confirmation `potwierdzam pass - kontynuujmy`, recorded in the [PR conversation](https://github.com/MechanicalCave/SimpleSolid2/pull/309#issuecomment-6108923757). A separate subsequent Owner reply `akceptuje - kotynuuj` was explicitly given in response to the request to **merge PR #309 to main**. This is aggregate practical acceptance; no unprovided separate per-test manual transcript is implied.

## Verification of the exact accepted candidate

- [Windows FULL #2440](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38136149906) **PASS** at **exact** source HEAD `45e097af752b748888050dc51f866d3053431fd9`: Core **25/25**, native Kernel **57/57**, Desktop Qt/OCCT **117/117**; total **199/199**; documentation dispatcher, aggregate `windows-msvc` and FULL jobs all **success**.
- Independent [Kernel FOCUSED #2434](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38135489072) **PASS 1/1** proves ordinary production Refine and cold source lineage; [Workbench FOCUSED #2436](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38135815695) **PASS 1/1**; [PartStore and native Face Boundary FOCUSED #2438](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38136036415) **PASS 4/4**. Those are iteration gates, not substitutes for exact-head FULL.
- Earlier RED runs were diagnosed and corrected without disabling tests. #2426 incorrectly mixed Kernel target with Desktop FOCUSED mode; #2427–#2433 exposed legacy-vs-v2 test-provider mismatch and unsupported-semantics error-classification evaluation-order defect; #2435/#2437 exposed test-only unavailable JSON header dependencies.

## Accepted product and semantic scope

1. One Project Geometry tool with **Edges**, **Planar Face** and **Face Boundary** modes. Planar Face acquires one exact current bounded planar Face. Face Boundary accepts manually selected multiple exact bounded planar/nonplanar Faces from the same legal earlier stage, reads each outer and inner native wire, and deduplicates **only exact matching material Edge identities**, not geometry or logical perimeter. No automatic adjacent-Face traversal or region healing.
2. Strict stage- and revision-bound source identity, including the approved `AtSingleSemanticPoint` material Edge reference for projection-only use in native Part **schema v16**. Independently geometrically Unsupported/degenerate projection members may be skipped in Face Boundary with exact typed location; semantic ambiguity or bad provider state is **fail-closed**. Only supported individual Sketch Entities/Edge links persist; Face picks and overlays remain transient.
3. One atomic Finish/Undo, safe partial projection, command/Operations/Viewport consistency, stale-provider refusal, provenance-aware linked Sketch/Profiles and dependent edits, explicit Save, Undo/Redo and fresh-provider cold reopen.
4. Production-default OCCT `Extrude Add` and `Revolve Add` use nondestructive, history-aware zero-fuzzy Fuse `SimplifyResult(true,true)`, preserving exact occupied material, genuine steps, holes, cylinders and necessary seams. The Owner's two native examples demonstrate unwanted top plane partitions **2→1** and **4→1** after temporary **test-only** v2 authored reconstructions. Historical unrefined topology stays test-only; Cut, Fillet and Chamfer policies are unchanged.
5. **ModelingSemanticsVersion 2** is independent of native Part container **schema v16**. Existing modeling-v1 Part files, including the two untouched original Owner v16 files, fail with typed `unsupported_modeling_semantics`. **No implicit migration, data backfill or in-product converter was authorized or delivered**. Temporary test-only v2 copies do not constitute a shipping migration feature.
6. PL/EN product documentation, internal as-built documentation and deterministic offline Product Browser were updated and verified in #2440.

## Status and next gate

- The **implementation** PR #309 is **merged to `main`**; it is not an open or Draft candidate and needs no additional production change under PG-01D.
- This file and its two related `work/**` status updates are **historical closure only** on a **separate proposed governance PR**, not a new product feature, schema change, recovery policy or program activation.
- `work/ACTIVE.yaml` intentionally continues to reference the **completed** PG-01D contract for traceability. This does **not** authorize new mutations under that contract.
- **PG-01E program closeout** is a possible next *separately gated* scope; **PM-06** Part-v1 completion comes afterward only if a separately accepted program/Work Contract authorizes it. No automatic transition, public API decision, Assembly work or CI issue #302 fix follows from the merged PG-01D.
- Future work must separately define and obtain Owner approval for its bounded Work Contract, required documentation impact and governance activation **before** production implementation.

## Documentation impact

Internal as-built documentation: **not required** — this is an acceptance/status history update; current semantics were already implemented and documented in merged PR #309.  
User/Product docs PL/EN: **not required** — no new product behavior or user workflow is introduced by these closure records.  
Generated Browser: **not required** — canonical Product/as-built Markdown remains unchanged.  
Verification: `work/**`-only closure gate on this proposed PR; do not present its own gate as new runtime proof.
