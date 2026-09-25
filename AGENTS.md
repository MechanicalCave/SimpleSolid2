# AGENTS.md — SimpleSolid 2.0

## Authority

Read in this order before changing production code:

1. `governance/CONSTITUTION.md`
2. `governance/FOUNDATION.md`
3. `governance/ARCHITECTURE.md`
4. relevant accepted ADRs
5. `work/ACTIVE.yaml`
6. any accepted program roadmap referenced by `work/ACTIVE.yaml`
7. active work contract
8. `governance/DOCUMENTATION.md` for documentation completion rules

If `work/ACTIVE.yaml` references a program roadmap, reading that roadmap is mandatory context reconstruction before implementation. The roadmap governs staged program direction but does not override Foundation or accepted ADRs.

The repository is authoritative for implementation state. Foundation is authoritative for product/domain boundaries.

## Decision classes

- **D0 Implementation** — agent may decide inside the active contract.
- **D1 Local design** — agent may decide if no public contract, ownership, dependency, identity or persistence rule changes.
- **D2 Architecture** — agent may propose; Owner approval required.
- **D3 Product / irreversible architecture** — Owner decides.

## Before every persistent code mutation

Confirm:

- the active work item is known;
- any referenced program roadmap has been read;
- the requested change is in scope;
- the target files are permitted by the active contract;
- the active contract identifies the applicable roadmap milestone when a roadmap governs the work;
- no CORE Foundation rule is contradicted;
- no D2/D3 decision is being made implicitly;
- current authoritative state has been read;
- validation/test strategy matches the risk.

Ambiguity about authority or scope fails closed.

## Engineering rules

- Do not bypass semantic Commands/Transactions for durable model mutations.
- Do not persist UI, viewer, OCCT or transient topology identity as CAD intent.
- Do not silently guess missing/ambiguous references.
- Do not weaken/delete tests merely to obtain a pass.
- Do not import SS1 architecture by copying a subsystem wholesale.
- SS1 may be used as a donor of proven concepts, tests and selected code only after checking compatibility with SS2 Foundation.
- Keep changes bounded to the active work contract.
- Do not silently change a frozen roadmap/ADR decision inside a local implementation contract.
- Update durable architecture documentation only when the underlying accepted architecture changes.
- Apply `governance/DOCUMENTATION.md`: every active Work Contract accepted after DOC-01 declares Documentation Impact, and required as-built/product documentation is updated before completion.
- Treat `docs/internal/` and `docs/product/` as current-state documentation, not implementation history.
- Treat `docs/browser/index.html` as generated; canonical documentation lives in Markdown.

## Handoff

A work item is complete only when:
- acceptance conditions are met;
- relevant tests pass;
- lifecycle/persistence behavior is covered where applicable;
- no known architecture contradiction remains;
- active work state points to the next concrete action;
- any applicable program roadmap state/impact is current;
- Documentation Impact declared by the active contract is satisfied;
- documentation validation passes and the generated Product Browser is current.
