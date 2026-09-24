# ADR-0004 — Native Document Container and Persistence Architecture

**Status:** ACCEPTED  
**Date:** 2026-09-24  
**Owner acceptance:** 2026-09-24  
**Decision class:** D2 Architecture  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Related:** ADR-0001, ADR-0002, ADR-0003

## Context

SS2 currently persists Part Documents in a small private text schema (`SS2PART`, schema v2). That format is sufficient for DocumentId, common properties and built-in Origin visibility, but the next major CAD phase will introduce authored 2D Sketch models embedded in Part.

Sketch is not a top-level Document. The Part host owns Sketch placement, references, lifecycle and persistence. Therefore the persistence architecture must be explicit before Sketch persistence is designed.

Foundation requires:

- authored semantic intent is authoritative;
- each CAD domain owns its durable semantic schema;
- shared persistence owns format/version recognition, atomic file mechanics, common helpers and migration dispatch;
- provider/runtime state does not leak into native files;
- schemas are explicitly versioned; future accepted formats must define their compatibility/migration policy deliberately;
- one Document remains independently persistent with stable DocumentId independent of path.

This ADR defines the native Document persistence boundary for Part now and for future Assembly/Drawing without prematurely defining those domains' semantic schemas.

## Proposed decision

### 1. One portable native file per top-level Document

A native Part remains one physical user-visible file.

The same rule applies architecturally to future AssemblyDocument and DrawingDocument.

Moving/copying the file moves/copies the complete persistent unit. Path remains location, not identity.

### 2. Native file has a common container/envelope and a domain-owned authored payload

Conceptually:

```text
Native Document file
├── common envelope
│   ├── container format version
│   ├── DocumentKind
│   ├── DocumentId
│   └── domain schema version
│
├── authored/
│   └── domain-owned semantic payload
│
└── derived/               optional
    └── disposable assets
```

The common envelope exists for safe recognition, routing, identity and migration dispatch.

It does not own Part/Assembly/Drawing engineering meaning.

### 3. Two version axes

The native format distinguishes:

- **container format version** — shared persistence framing/representation;
- **domain schema version** — semantic schema owned by Part, Assembly or Drawing.

Changing either is a persistence contract change.

This prevents a change in archive/framing mechanics from being confused with a change in Part authored semantics.

### 4. Minimal common envelope

The common envelope contains only information intrinsic to native Document recognition:

- format signature / recognizable magic;
- container format version;
- DocumentKind;
- DocumentId;
- domain schema version.

ProjectId is not embedded in a Document.

Filename/path, UI state, viewer state and provider state are not embedded as identity.

Common Document Properties are not duplicated in the envelope merely to accelerate browsing; authoritative semantic values remain in the owning domain schema unless a later accepted architecture explicitly introduces a shared semantic sub-schema.

### 5. Domain payload ownership

Part owns the complete durable semantic payload of PartDocument.

Future Assembly owns AssemblyDocument payload.

Future Drawing owns DrawingDocument payload.

Shared persistence may transport bytes/structured entries but must not interpret domain meaning beyond the minimal envelope required for dispatch.

### 6. Sketch persistence ownership

Future Sketch Core does not own a standalone native file format.

A Part-hosted Sketch is authored semantic state embedded in the Part-owned payload. The next Sketch contract may define the Sketch model/schema, but it must serialize through the Part host boundary established here.

The same reusable Sketch Core may later be hosted by Assembly or Drawing under those domains' persistence ownership.

### 7. Derived assets are optional and disposable

The container may carry optional derived assets in a physically isolated area.

Rules:

- derived assets are never authoritative design intent;
- absence of derived assets does not invalidate the Document;
- a damaged/unsupported derived asset must be ignorable when the container and authored payload remain valid;
- deleting all derived assets must leave a reconstructible valid Document;
- derived assets do not change DocumentId or authored dirty/save semantics.

This ADR does **not** require thumbnail generation, icon/tile browser UX or any specific derived asset.

### 8. Whole-file atomic publication

Initial SS2 persistence writes a complete replacement file to a staged sibling and atomically publishes/replaces the target only after the new complete native Document is valid enough to publish.

No in-place mutation of the authoritative native file is required by this ADR.

DocumentSession advances its save checkpoint only after successful durable publication.

### 9. Early-development compatibility reset

Owner explicitly decided on 2026-09-24 that the current private `SS2PART` v1/v2 files do not require compatibility or migration.

SS2 is still before meaningful user/project data exists. PERSIST-01 may therefore replace the current experimental format with the accepted native format as a clean break.

After the new format is accepted:

- newly created Parts use only the new representation;
- current private v1/v2 test files are not part of the supported product contract;
- no production migration reader is required;
- tests/fixtures are recreated for the new format rather than preserving obsolete compatibility.

This reset applies only to the present pre-product format. Future accepted native schemas remain versioned, and compatibility/migration policy for later schema changes must be decided explicitly rather than assumed.

### 10. Fail-closed recognition and bounded parsing

Persistence must reject or explicitly diagnose:

- unknown mandatory container version;
- unsupported domain schema;
- invalid DocumentId;
- declared kind/extension mismatch where the current domain contract requires it;
- malformed mandatory envelope;
- missing or malformed authored payload;
- duplicate mandatory entries;
- truncated/oversized content;
- path traversal or unsafe entry names if the physical representation is entry-based.

Unknown optional derived entries may be ignored.

### 11. Accepted physical representation — ZIP-compatible package

Owner accepted the physical representation on 2026-09-24.

A native SS2 Document is one user-visible file whose physical representation is a ZIP-compatible package.

Initial native extensions are domain-specific:

```text
Part      → .ss2part
Assembly  → .ss2asm     (reserved; not implemented by PERSIST-01)
Drawing   → .ss2draw    (reserved; not implemented by PERSIST-01)
```

The package structure is:

```text
<document file>
├── manifest.json             mandatory common envelope
├── authored/
│   └── document.json         mandatory domain-owned authored payload
└── derived/
    └── ...                   optional disposable assets
```

ZIP is container mechanism only. It does not define CAD semantics.

JSON is the initial structured representation for the common manifest and domain-authored payload. A later domain schema may deliberately introduce additional/binary authored entries through a new accepted schema contract without changing the rule that authored meaning belongs to the domain.

### 12. Container v1 structural rules

The initial container contract requires:

- exactly one root `manifest.json`;
- exactly one `authored/document.json`;
- zero or more entries below `derived/`;
- no mandatory semantic entry outside the common manifest and domain-owned authored namespace unless a later accepted schema adds it;
- UTF-8 JSON for mandatory JSON entries;
- canonical forward-slash entry names;
- no absolute paths, `..`, backslash traversal aliases or unsafe entry names;
- no duplicate entry names;
- no encrypted entries;
- bounded compressed/uncompressed sizes and entry count before allocation/extraction;
- unknown mandatory structure fails closed;
- unknown optional `derived/*` entries may be ignored.

The common manifest v1 carries:

```text
format
container_version
document_kind
document_id
domain_schema_version
authored_entry
```

with `authored_entry` equal to `authored/document.json` in container v1.

The exact lexical serialization contract of `DocumentId` is not expanded by this ADR beyond the accepted Core identity contract. PERSIST-01 must not create a second identity format in persistence.

### 13. Extension/kind coherence

For native files recognized by extension, the extension and `document_kind` must agree.

A `.ss2part` declaring another DocumentKind fails closed. The same rule applies to future implemented Assembly/Drawing native extensions.

### 14. Dependency boundary

ZIP and JSON libraries, if used, are persistence-format implementation dependencies only.

They must not become CAD-domain semantic APIs, leak into Part/Assembly/Drawing public model contracts, or introduce Qt/OCCT/provider types into durable semantics.

The concrete maintained ZIP/JSON implementation dependency is selected within PERSIST-01 before production container code is committed and documented in the build/toolchain.

## Consequences

The next Sketch contract can focus on authored 2D semantics rather than inventing a file framing architecture.

Part remains the persistence owner for Part-hosted Sketches.

Future Assembly/Drawing can reuse persistence mechanics without sharing one universal semantic schema.

The current `SS2PART` text representation is an experimental bootstrap format and is replaced rather than supported as a legacy product format.

## Explicit non-goals

This ADR does not define:

- Sketch entities/constraints/solver schema;
- Body/Feature/Extrude semantics;
- modeled B-Rep persistence;
- topology naming;
- Assembly occurrence schema;
- Drawing sheet/view/annotation schema;
- thumbnail generation or browser tiles;
- Save As / Save Copy As UI;
- cloud synchronization or semantic merge;
- final extensions for future Assembly/Drawing files.

