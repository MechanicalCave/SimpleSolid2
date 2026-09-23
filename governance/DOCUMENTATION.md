# SimpleSolid 2.0 — Documentation Rule

**Status:** ACTIVE  
**Introduced by:** `work/DOC-01_AS_BUILT_DOCUMENTATION_SYSTEM.md`  
**Owner acceptance:** 2026-09-23

This document defines the operational rule for keeping SimpleSolid 2.0 as-built and product documentation current.

It is subordinate to the Engineering Constitution, Foundation, Architecture, accepted ADRs and the active Work Contract. It does not create CAD-domain semantics.

## 1. Documentation layers

SS2 maintains three different information layers.

### Normative governance

Normative product/architecture intent lives in:

- `governance/CONSTITUTION.md`
- `governance/FOUNDATION.md`
- `governance/ARCHITECTURE.md`
- accepted ADRs
- the active Work Contract

As-built documentation must not override these sources.

### Internal as-built documentation

`docs/internal/` describes how the accepted implementation currently works.

It is written for maintainers, reviewers and automation/AI clients. It describes current ownership, lifecycle, persistence, boundaries and operational workflows without becoming a second architecture authority.

### Product/user documentation

`docs/product/pl/` and `docs/product/en/` describe the current accepted product surface for users.

They do not document implementation history, roadmap intent, internal classes or CI evidence.

Polish and English are peer canonical sources. Polish is the default Product Browser language.

## 2. Documentation Impact declaration

Every work contract accepted after DOC-01 must contain:

```text
## Documentation impact

Internal docs: required | not required
User/Product docs: required | not required
Reason: ...
```

The declaration is part of the contract scope.

A work item must not be marked `completed` while documentation declared `required` is stale or missing.

## 3. When documentation is required

Documentation impact is determined by semantic/product effect, not by line count.

Internal documentation normally requires an update when a change affects one or more of:

- subsystem ownership or dependency boundaries;
- durable identity/reference meaning;
- persistence or filesystem meaning;
- lifecycle or runtime coordination;
- public/internal contracts that maintainers must understand;
- project/document structure;
- build, test, launch or repository operating workflow;
- accepted failure/recovery semantics.

User/Product documentation normally requires an update when a change affects one or more of:

- user-visible behavior;
- user workflow;
- available capability;
- command or dialog behavior that changes how a task is performed;
- user-visible failure/recovery behavior;
- file/workspace expectations users must understand;
- supported limits that affect normal use.

Pure implementation refactors with unchanged architecture, contracts and observable behavior may declare both documentation impacts `not required`, with a reason.

## 4. Current-state rule

As-built and product documentation describe **how SS2 works now**.

Implementation history belongs in Git commits, PRs and Work Contracts. Do not turn current-state documentation into a chronological changelog.

When behavior changes, update the existing current-state explanation instead of appending historical layers.

## 5. Canonical source and generated Browser

Markdown is authoritative.

The Product Browser at `docs/browser/index.html` is generated from the canonical Markdown sources and must not be edited as an independent source of truth.

Use:

```powershell
.\ss2.ps1 docs
```

to regenerate the Browser.

`ss2 verify` validates documentation structure and Browser freshness.

## 6. Bilingual product parity

Each product Markdown file must exist in both `pl` and `en`.

Paired files must use:

- the same `doc-id`;
- the same ordered stable `section-id` values.

This validates structural parity without treating automatic translation as authoritative.

## 7. Fail closed

If required documentation is missing, bilingual structure diverges, deterministic local links are broken, the active Work Contract lacks Documentation Impact, or the generated Browser is stale, repository verification fails.
