# PERSIST-01 — Native Document Persistence Architecture

**Status:** ACCEPTED — IMPLEMENTED  
**Owner acceptance:** 2026-09-24  
**Decision class:** D2 Architecture  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Related ADRs:** ADR-0001, ADR-0002, ADR-0003  
**Accepted ADR:** ADR-0004

## 1. Goal

Establish and implement the native Document persistence architecture needed before Sketcher work begins.

PERSIST-01 must separate:

```text
shared persistence mechanism
from
Part-authored semantic schema
```

and prove the architecture by replacing the current experimental Part persistence path without implementing Sketch, Assembly or Drawing semantics.

The result must give the next Sketch contract a stable host persistence boundary: a Part-hosted Sketch will be authored Part state stored inside the Part native Document, not a standalone file owned by Sketch Core.

## 2. Why now

The current `SS2PART` line-based schema v2 stores only Document identity/properties and built-in Origin visibility.

The next major CAD contract will add a substantially richer authored model: embedded 2D Sketch state with stable local entity identities and constraints.

Allowing the Sketch contract to invent file framing, versioning and domain ownership at the same time would combine independent D2 decisions and make persistence architecture accidental.

PERSIST-01 therefore precedes Sketcher.

## 3. Architecture decisions proposed for Owner acceptance

PERSIST-01 proposes:

1. one portable native file per top-level Document;
2. a minimal common native envelope plus domain-owned authored payload;
3. separate container-format and domain-schema versions;
4. DocumentKind + DocumentId in the common envelope;
5. no ProjectId/path/provider/runtime identity in native Document semantics;
6. optional physically isolated derived payloads that are disposable and non-authoritative;
7. whole-file staged + atomic publication as the initial save model;
8. the current experimental Part v1/v2 format is replaced without compatibility/migration support;
9. Part owns all Part semantic serialization, including future hosted Sketches;
10. physical representation is a ZIP-compatible package with mandatory `manifest.json` and `authored/document.json`, plus optional `derived/*`.

These decisions were accepted by Owner on 2026-09-24.

## 4. Decision Gate A — CLOSED

Owner accepted on 2026-09-24:

```text
native SS2 Document
= one ZIP-compatible package
  + manifest.json
  + authored/document.json
  + optional derived/*
```

ZIP is transport/container mechanism, not semantic schema.

Mandatory structured entries use UTF-8 JSON in container v1.

The accepted reasons are:

- natural physical separation of authored and disposable derived entries;
- one portable user-visible file;
- native binary asset support without Base64/hex expansion;
- standard, inspectable container rather than an SS2-specific archive reinvention;
- reusable persistence mechanics across Part/Assembly/Drawing without a universal domain schema;
- good fit for future embedded Sketch data while leaving Sketch semantics to the host domain.

The accepted implementation dependencies are vendored/pinned `miniz` 3.1.2 and `nlohmann/json` 3.12.0. Normal builds must not download them. Their types must not cross CAD-domain public semantic APIs.

Container v1 fixes `authored/document.json`; the manifest therefore contains only `format`, `container_version`, `document_kind`, `document_id` and `domain_schema_version`. `document_id` uses Core's canonical serialization and is parsed by Core; persistence does not define a second UUID contract.


## 5. Scope IN

### Shared persistence

Implement only mechanisms proven common:

- native signature/container recognition;
- bounded envelope parsing;
- container-format version recognition;
- DocumentKind / DocumentId extraction;
- domain-schema version dispatch information;
- safe entry/path rules if entry-based;
- staged complete-file writing;
- atomic publication;
- structured persistence diagnostics.

### Part persistence

Replace PartDocument persistence with the accepted representation.

Preserve current authored semantics:

- DocumentId;
- Number;
- Title;
- Description;
- Engineering Revision;
- built-in Origin visibility.

Part remains owner of Part semantic encoding/decoding.

### Early-development reset

The present private v1/v2 `.ss2part` representation is intentionally unsupported after PERSIST-01.

No legacy reader, migration path or compatibility fixture is required. Existing test Documents may be recreated.

This is a one-time pre-product reset and must not be generalized into a policy of breaking accepted future native schemas.

### Discovery/lifecycle

Current Workspace discovery and canonical DocumentSession behavior must continue to work.

Moving/renaming a file inside the Workspace preserves identity.

Duplicate DocumentId conflicts continue to fail closed.

## 6. Scope OUT

PERSIST-01 does not implement:

```text
Sketch Core
Sketch entities / constraints / solver
Sketch Edit UX
Body / Feature / Extrude
modeled Part B-Rep
persistent topology naming
AssemblyDocument semantic schema
DrawingDocument semantic schema
thumbnail generation
tile/icon browser
preview cache
Material
BOM
Save As / Save Copy As UI
cloud sync / merge
camera or selection persistence
```

Future Assembly/Drawing are architectural consumers of the mechanism only; this contract must not invent their authored semantics.

## 7. Required invariants

### Authored authority

A clean rebuild from authored semantic data must remain possible.

No evaluated B-Rep, tessellation, Viewer object, OCCT handle, runtime topology identity, Undo history or DocumentSession state becomes required native intent.

### Identity

DocumentId remains stable across:

- Save;
- rename;
- move.

Path and filename remain location, not identity.

### Save correctness

Save checkpoint advances only after successful durable publication.

A failed save must not destroy or partially replace the previous valid file.

### Forward failure

Unknown mandatory container/domain schema versions fail explicitly.

No guessing or best-effort semantic interpretation of unknown authored schema.

### Derived isolation

If the selected physical representation supports optional derived entries, they must be ignorable/disposable and must not become a second source of truth.

PERSIST-01 does not need to create any such asset.

## 8. Tests and acceptance

At minimum prove:

1. newly created Part uses the accepted new representation;
2. Save → Close → Reopen preserves all current authored Part state;
3. rename/move preserves DocumentId;
4. duplicate DocumentId conflict still fails closed;
5. unknown container version fails closed;
6. unsupported Part schema fails closed;
7. malformed/truncated mandatory envelope fails closed;
8. malformed/missing authored payload fails closed;
9. invalid DocumentId fails closed;
10. oversized/unsafe content is rejected before unbounded allocation;
11. failed staged write/replacement leaves the old file valid;
12. optional unknown derived content, if supported by the selected representation, does not change authored meaning;
13. no Qt/OCCT/provider identity crosses into Part durable semantics;
14. obsolete v1/v2 fixtures are rejected or removed rather than treated as supported legacy product data;
15. existing Project/Part/Workbench lifecycle tests remain PASS;
16. exact-head Windows docs/verify/build/CTest gate is PASS.

## 9. Delivery slices

```text
Slice A
ACCEPTED: ZIP+JSON physical representation + ADR-0004

Slice B
shared native container/envelope primitives + safety tests

Slice C
Part semantic codec replacement

Slice D
Workspace/discovery/lifecycle integration + persistence safety tests

Slice E
docs + Product Browser + exact-head completion
```

No later slice starts if an earlier persistence invariant is unresolved.

## Documentation impact

Internal docs: required  
User/Product docs: required  
Reason: this work replaces the experimental native Part representation and establishes the durable persistence architecture that future Sketch/Assembly/Drawing will consume.

Internal docs must describe:
- common envelope/container responsibility;
- Part schema ownership;
- version axes;
- versioning and atomic publication;
- authored vs derived boundaries;
- the explicit pre-product compatibility reset.

Product docs PL/EN must describe only user-relevant behavior:
- the accepted native Part format and extension;
- normal Save → Close → Reopen behavior;
- normal rename/move identity behavior remains unchanged.

Because there is no supported pre-PERSIST-01 user data, product documentation does not promise compatibility with the obsolete experimental v1/v2 format.

Generated Product Browser must be regenerated and Git-clean.

## 10. Completion

PERSIST-01 completes only when:

- ADR-0004 is accepted with the physical representation explicitly selected;
- the accepted native architecture is implemented for Part;
- the obsolete experimental v1/v2 path is removed from the supported persistence contract;
- current Part lifecycle remains intact;
- no Sketch/Assembly/Drawing semantic scope leaked into the work;
- documentation is current;
- exact-head Windows gate passes.

Completion does not automatically activate Sketcher. The Owner must accept the subsequent Sketch architecture/implementation contract.


## 11. Completion record

PERSIST-01 implementation is complete and ready for the final exact-head Windows gate.

Implemented state:

- native Part files are ZIP-compatible container v1 packages;
- `manifest.json` is the strict common envelope;
- `authored/document.json` is the Part-owned authored payload;
- `derived/*` is optional, disposable and ignored when safe/unknown;
- miniz 3.1.2 and nlohmann/json 3.12.0 are vendored/pinned with no normal-build network fetch;
- container and domain schema versions are separate;
- Part domain schema is reset to v1 for the new accepted representation;
- the former line-based `SS2PART` bootstrap format is deliberately unsupported;
- current Project/Document discovery, stable DocumentId behavior, atomic save and canonical DocumentSession lifecycle remain intact;
- no Sketch, Body/Feature, Assembly/Drawing semantic schema, thumbnail or modeled B-Rep scope was introduced.

The compiled suite contains 33 tests. The final repository completion condition remains the exact-head Windows docs/verify/build/CTest gate on this completed contract state.
