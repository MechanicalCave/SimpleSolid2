# PERSIST-01 — Native Document Persistence Architecture & Part Migration

**Status:** DRAFT — OWNER REVIEW REQUIRED  
**Decision class:** D2 Architecture  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Related ADRs:** ADR-0001, ADR-0002, ADR-0003  
**Proposed ADR:** ADR-0004

## 1. Goal

Establish and implement the native Document persistence architecture needed before Sketcher work begins.

PERSIST-01 must separate:

```text
shared persistence mechanism
from
Part-authored semantic schema
```

and prove the architecture by migrating the existing Part persistence path without implementing Sketch, Assembly or Drawing semantics.

The result must give the next Sketch contract a stable host persistence boundary: a Part-hosted Sketch will be authored Part state stored inside the Part native Document, not a standalone file owned by Sketch Core.

## 2. Why now

The current `SS2PART` line-based schema v2 stores only Document identity/properties and built-in Origin visibility.

The next major CAD contract will add a substantially richer authored model: embedded 2D Sketch state with stable local entity identities and constraints.

Allowing the Sketch contract to invent file framing, migration and domain ownership at the same time would combine two independent D2 decisions and make persistence architecture accidental.

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
8. legacy Part v1/v2 read compatibility and migration-on-successful-Save;
9. Part owns all Part semantic serialization, including future hosted Sketches;
10. physical container representation remains a D2 Owner decision after the evidence slice below.

These decisions become active only after Owner accepts ADR-0004 / this contract.

## 4. Decision Gate A — physical representation

Before production persistence implementation, compare at minimum:

### Candidate A — standard archive/container + structured manifest

Examples: ZIP-compatible archive with a small manifest and separate authored/derived entries.

Strengths:
- natural physical separation of entries;
- standard tooling and binary payload support;
- good portability.

Risks:
- no supported archive dependency is currently present in SS2;
- Qt private ZIP APIs are not acceptable as an architecture dependency;
- adding a library/toolchain dependency is itself part of the D2 decision.

### Candidate B — minimal SS2 framed container

A small SS2-owned binary framing format with magic/version and length-bounded named entries.

Strengths:
- no third-party dependency;
- exact control over validation and deterministic semantics;
- straightforward whole-file atomic write.

Risks:
- SS2 owns container parsing forever;
- easy to reinvent archive concerns badly;
- compression/checksum/evolution must be deliberately bounded.

### Candidate C — single structured document representation

A single structured textual/binary object containing envelope + authored state and optional encoded derived payloads.

Strengths:
- simplest implementation/dependency story.

Risks:
- weak physical isolation of derived assets;
- binary payload encoding overhead;
- malformed optional payload may be harder to isolate from authored semantics;
- can become unwieldy as domains grow.

### Gate criteria

The selected representation must be justified against:

- safety under malformed/untrusted files;
- single-file portability;
- independent authored/derived integrity;
- domain/provider dependency boundaries;
- future Sketch nested data;
- future Assembly/Drawing reuse;
- migration complexity;
- dependency/toolchain burden;
- testability and deterministic failure modes.

**No production format implementation begins until Owner explicitly accepts the selected representation.**

## 5. Scope IN after Gate A acceptance

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

Migrate PartDocument persistence to the accepted representation.

Preserve current authored semantics:

- DocumentId;
- Number;
- Title;
- Description;
- Engineering Revision;
- built-in Origin visibility.

Part remains owner of Part semantic encoding/decoding.

### Legacy migration

- load current legacy v1/v2 `.ss2part`;
- do not rewrite on load;
- successful Save publishes the new accepted representation;
- preserve DocumentId and all authored source semantics;
- failure leaves previous file authoritative.

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

- legacy migration;
- Save;
- rename;
- move.

Path and filename remain location, not identity.

### Save correctness

Save checkpoint advances only after successful durable publication.

A failed migration/save must not destroy or partially replace the previous valid file.

### Forward failure

Unknown mandatory container/domain schema versions fail explicitly.

No guessing or best-effort semantic interpretation of unknown authored schema.

### Derived isolation

If the selected physical representation supports optional derived entries, they must be ignorable/disposable and must not become a second source of truth.

PERSIST-01 does not need to create any such asset.

## 8. Tests and acceptance

At minimum prove:

1. current legacy Part v1 loads;
2. current legacy Part v2 loads;
3. legacy v1/v2 load does not rewrite source;
4. Save of a loaded legacy Part writes the accepted new representation;
5. migrated file reopens with the same DocumentId;
6. migrated properties and Origin visibility are identical;
7. newly created Part uses the new representation;
8. Save → Close → Reopen preserves all current authored Part state;
9. rename/move preserves DocumentId;
10. duplicate DocumentId conflict still fails closed;
11. unknown container version fails closed;
12. unsupported Part schema fails closed;
13. malformed/truncated mandatory envelope fails closed;
14. malformed/missing authored payload fails closed;
15. invalid DocumentId fails closed;
16. oversized/unsafe content is rejected before unbounded allocation;
17. failed staged write/replacement leaves the old file valid;
18. optional unknown derived content, if supported by the selected representation, does not change authored meaning;
19. no Qt/OCCT/provider identity crosses into Part durable semantics;
20. existing Project/Part/Workbench lifecycle tests remain PASS;
21. exact-head Windows docs/verify/build/CTest gate is PASS.

## 9. Delivery slices

```text
Slice A
D2 evidence + select physical representation + accept ADR-0004

Slice B
shared native container/envelope primitives + safety tests

Slice C
Part semantic codec migration + legacy v1/v2 compatibility

Slice D
Workspace/discovery/lifecycle integration + migration tests

Slice E
docs + Product Browser + exact-head completion
```

No later slice starts if an earlier persistence invariant is unresolved.

## Documentation impact

Internal docs: required  
User/Product docs: required  
Reason: this work changes native Part file representation, migration behavior and the durable persistence architecture that future Sketch/Assembly/Drawing will consume.

Internal docs must describe:
- common envelope/container responsibility;
- Part schema ownership;
- version axes;
- migration and atomic publication;
- authored vs derived boundaries;
- supported legacy compatibility.

Product docs PL/EN must describe only user-relevant behavior:
- existing Part files remain readable;
- opening does not silently rewrite them;
- successful later Save may migrate them;
- normal rename/move identity behavior remains unchanged.

Generated Product Browser must be regenerated and Git-clean.

## 10. Completion

PERSIST-01 completes only when:

- ADR-0004 is accepted with the physical representation explicitly selected;
- the accepted native architecture is implemented for Part;
- legacy v1/v2 compatibility and migration are proven;
- current Part lifecycle remains intact;
- no Sketch/Assembly/Drawing semantic scope leaked into the work;
- documentation is current;
- exact-head Windows gate passes.

Completion does not automatically activate Sketcher. The Owner must accept the subsequent Sketch architecture/implementation contract.
