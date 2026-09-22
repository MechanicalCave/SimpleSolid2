# SimpleSolid 2.0 — Engineering Constitution

**Status:** FROZEN  
**Version:** 1.0  
**Date:** 2026-09-22

This Constitution defines the small set of engineering rules that apply across SS2.

## 1. Design intent is authoritative

Persist authored semantic intent. Geometry, tessellation, solver output, projected view geometry, caches and viewer objects are derived unless an explicit domain contract states otherwise.

## 2. One concept has one owner

Every durable semantic fact has one authoritative owning subsystem. Other layers consume contracts or derived projections; they do not create a second truth.

## 3. UI is never the model

Persistent mutation follows:

```text
GUI / AI / Script
→ Command
→ Validation
→ Transaction
→ Domain Document
→ Evaluation
```

Widgets, tree rows, screen coordinates and renderer objects never become CAD identity.

## 4. Domain owns meaning; platform owns mechanism

Shared infrastructure may provide reusable mechanics. Part, Assembly and Drawing retain ownership of their semantic meaning.

## 5. Kernel/provider isolation

CAD domains and persistence do not depend on OCCT types, handles, topology ordinals or provider-native identity. OCCT remains behind Kernel API.

## 6. Persistent identity is semantic

Path is location, not identity. Durable IDs represent Project, Document and domain concepts. Runtime topology, viewer tokens and list indices are not durable identity.

## 7. Fail closed

Ambiguous identity, dependency, reference or stale context fails explicitly. Do not silently guess engineering intent.

## 8. Derived state is disposable

A clean rebuild from authored semantic data and legal dependencies is a first-class invariant.

## 9. Commands own persistent mutations

No UI, viewer, importer or AI path may bypass semantic Commands/Transactions for durable model changes.

## 10. Revalidation occurs at execution

Preview, selection and UI enablement are not authority. Commands revalidate current context and staleness before commit.

## 11. Persistence is versioned and migratable

Persistent schemas have explicit versions. A schema change is a contract change. Runtime/provider state must not leak into native formats.

## 12. Units and coordinate meaning are explicit

Physical quantities, coordinate frames and transform directions must be explicit at subsystem boundaries.

## 13. Diagnostics are structured

Failures originate as typed/structured diagnostics in the subsystem that understands them. UI strings do not become model semantics.

## 14. Lifecycle is part of correctness

Persistent authored state must survive Save → Close → Reopen without relying on previous-process caches, provider handles or viewer state.

## 15. Tests prove boundaries

Prefer semantic tests for ownership/identity/lifecycle, provider tests for real geometry and integration tests for persistence/composition.

## 16. No speculative universal frameworks

Create shared abstractions only for intrinsic platform responsibilities or demonstrated multi-domain reuse.

## 17. Owner / Agent authority

Owner decides WHAT and WHY. Agent decides HOW within an approved contract. Agent cannot self-authorize architectural expansion.

## 18. Foundation authority

`governance/FOUNDATION.md` defines product/domain boundaries. Contradicting a CORE Foundation rule requires an explicit Owner-approved Foundation Amendment.
