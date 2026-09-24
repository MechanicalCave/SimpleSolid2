# PERSIST-01 — Native Document Persistence Architecture

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
D2 evidence + select physical representation + accept ADR-0004

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
