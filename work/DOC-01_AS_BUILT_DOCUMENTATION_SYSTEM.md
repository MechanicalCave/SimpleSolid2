# DOC-01 — As-built Documentation System

**Status:** ACCEPTED  
**Owner acceptance:** 2026-09-23  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Related baseline:** `work/PH-01_PROJECT_HUB.md`, `work/PH-02A_PROJECT_HUB_SELECTION_UX.md`, `work/PH-02B_RECENT_PROJECT_AVAILABILITY_UX.md`, `work/PH-02C_PROJECT_CREATION_UX.md`

## Goal

Establish a durable as-built documentation system for SimpleSolid 2.0 before CAD-domain implementation begins, and backfill the current Project-platform state.

The documentation must answer two different questions:

- internal/as-built: how SS2 is currently built and behaves,
- product/user: what the current accepted product does and how a user operates it.

Documentation describes the current accepted state, not implementation history or roadmap.

## Scope IN

### Internal as-built documentation

Create canonical English Markdown under `docs/internal/` covering:

- overview and authority boundaries,
- Project platform,
- application lifecycle,
- persistence,
- Project Hub UI behavior,
- build/test/run workflow.

Internal documentation is explanatory/as-built. Normative authority remains Constitution → Foundation/Architecture → accepted ADRs → active Work Contract.

### Product/user documentation

Create canonical bilingual Markdown under:

- `docs/product/pl/`
- `docs/product/en/`

Initial product documentation covers:

- product overview,
- Projects: Create/Open, Workspace, Recent Projects, relocation, availability states, move/copy behavior and Remove from Recent.

Polish is the default Product Browser language. PL and EN are peer, reviewable sources; neither language is generated from the other.

### Product Browser

Create a generated self-contained browser at `docs/browser/index.html` with:

- Markdown as source of truth,
- PL default,
- PL / EN switch for product documentation,
- navigation,
- search,
- clear separation of Product and Internal/as-built documentation,
- no runtime network dependency.

### Documentation Impact Rule

Add an operational documentation rule requiring every future active work contract to contain:

```text
## Documentation impact

Internal docs: required | not required
User/Product docs: required | not required
Reason: ...
```

A work item may not be marked completed while documentation declared required by its contract is stale or missing.

Documentation impact is based on semantic/product effect, not line count. Relevant triggers include user-visible workflow, persistence, identity/reference meaning, subsystem ownership/lifecycle, public contracts, project/document structure, build/test/run behavior and accepted capabilities.

### Automated validation

Add deterministic repository tooling that verifies:

- required documentation structure,
- matching PL/EN product file pairs,
- matching stable product section identifiers between PL and EN,
- local Markdown link targets where deterministically checkable,
- Documentation Impact section on the current active work contract,
- generated Product Browser freshness.

Integrate the check with canonical `ss2 verify` and the Windows PR gate.

### Backfill

Document the accepted current SS2 state after PH-01 / PH-02A / PH-02B / PH-02C, including:

- Project / ProjectId,
- Workspace,
- project metadata,
- ProjectSession,
- Create/Open Project,
- Recent Projects,
- relocation,
- availability states,
- Project Hub,
- build / test / run.

## Scope OUT

- no CAD-domain implementation,
- no Part / Assembly / Drawing / DocumentSession implementation,
- no changes to ProjectId, persistence semantics or Foundation architecture,
- no historical changelog documentation,
- no automatic machine translation as a canonical source,
- no external documentation hosting/service dependency.

## Acceptance

DOC-01 is complete when:

1. canonical internal Markdown exists and accurately describes current as-built SS2 Project platform;
2. canonical user/product Markdown exists in complete PL/EN pairs;
3. PL/EN product documents use matching stable document/section identifiers;
4. generated Product Browser starts in Polish, switches PL/EN, supports navigation/search and includes Internal documentation separately;
5. Product Browser is generated from canonical Markdown and contains no runtime network dependency;
6. documentation validation fails on missing PL/EN pair, section mismatch, broken deterministic local link, missing Documentation Impact section, or stale generated Browser;
7. `ss2 verify` executes documentation validation;
8. Windows PR gate runs for documentation-system/source changes and passes;
9. future work contracts are operationally required to declare Documentation Impact;
10. no existing PH-01 / PH-02A / PH-02B / PH-02C behavior or tests are weakened.

## Documentation impact

Internal docs: required  
User/Product docs: required  
Reason: DOC-01 creates the documentation system itself and backfills the complete currently accepted Project-platform behavior.

## Decision level

D0/D1 repository process, documentation and tooling. No Foundation, durable identity, persistence meaning, CAD ownership or public CAD-domain contract changes.
