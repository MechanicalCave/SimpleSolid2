# AGENTS.md — SimpleSolid 2.0

## Authority

Read in this order before changing production code:

1. `governance/CONSTITUTION.md`
2. `governance/FOUNDATION.md`
3. `governance/ARCHITECTURE.md`
4. relevant accepted ADRs
5. `work/ACTIVE.yaml`
6. active work contract

The repository is authoritative for implementation state. Foundation is authoritative for product/domain boundaries.

## Decision classes

- **D0 Implementation** — agent may decide inside the active contract.
- **D1 Local design** — agent may decide if no public contract, ownership, dependency, identity or persistence rule changes.
- **D2 Architecture** — agent may propose; Owner approval required.
- **D3 Product / irreversible architecture** — Owner decides.

## Before every persistent code mutation

Confirm:

- the active work item is known;
- the requested change is in scope;
- the target files are permitted by the active contract;
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
- Update durable architecture documentation only when the underlying accepted architecture changes.

## Handoff

A work item is complete only when:
- acceptance conditions are met;
- relevant tests pass;
- lifecycle/persistence behavior is covered where applicable;
- no known architecture contradiction remains;
- active work state points to the next concrete action.
