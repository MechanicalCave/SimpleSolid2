# SimpleSolid 2.0 — prompt wznowienia nowego kontekstu

Kontynuujesz projekt **SimpleSolid 2.0 (SS2)**. Pracuj po polsku, inżyniersko i krytycznie.

## Źródło prawdy

Repo: `MechanicalCave/SimpleSolid2`. Ten prompt jest jedynie handoff hintem; **repozytorium i governance są autorytatywne**. Nie traktuj branch, SHA, numerów runów ani statusów opisanych poniżej jako aktualnych bez ponownego sprawdzenia GitHub.

## Obowiązkowa rekonstrukcja — w kolejności

1. `AGENTS.md`.
2. `governance/CONSTITUTION.md`.
3. `governance/FOUNDATION.md` (v1.0 FROZEN).
4. `governance/ARCHITECTURE.md`.
5. Relevant accepted ADRs (w obecnej fazie szczególnie ADR-0014, ADR-0016, ADR-0017 oraz ADR-0012/0013, gdy dotyczą aktualnej pracy).
6. `work/ACTIVE.yaml`.
7. Dokładny `program_roadmap` wskazany przez ACTIVE. Historycznie: `work/PART_MODELING_V1_ROADMAP.md` v1.29.
8. `work/SKETCH_ROADMAP.md` v1.9 jako subordinate roadmap, gdy potrzebny jest kontekst Sketch/Shared 2D.
9. Aktywny Work Contract wskazany przez ACTIVE oraz jego zaakceptowane amendments.
10. `governance/DOCUMENTATION.md`.
11. Aktualne branch/HEAD/PR/CI i wymagane bramki Ownera.

## Zamknięty AUDIT-01 — obowiązuje jego historia

`work/AUDIT-01_ARCHITECTURE_STABILIZATION_PROGRAM.md` v1.0 to **COMPLETED / CLOSED**. Wszystkie pakiety A, B2, B1, C1, C2, D, E1, E2 i F zostały rozliczone w repo. Nie reaktywuj ich na podstawie starych promptów. Zamrożone reguły Foundation, wcześniejsze zaakceptowane ADR i ograniczenia programu nadal obowiązują.

Part Modeling v1 jest odrębnym programem. Nie implementuj funkcji spoza aktywnego kontraktu. PM-06 nie jest automatycznie autoryzowane przez wykonanie PM-05.

## Expected checkpoint w chwili aktualizacji 2026-10-08 — ZWERYFIKUJ

- Aktywny program: **Part Modeling v1.29**; PM-01 do PM-04 ukończone; PM-05A–E ukończone; **PM-05F** nadal ACTIVE.
- Aktywny contract: `work/PM-05_EDGE_FEATURES.md` z przyjętą remediation `work/PM-05F_R2_OWNER_ACCEPTANCE_REMEDIATION.md`.
- PR **#298**, branch `pm-05f-r2-owner-remediation`, Draft; nie merguj do main bez wszystkich bramek.
- Owner zaakceptował bounded **D2-B** provider-private fallback Chamfer (dokładny Part008 mixed 3-Edge), ale **nie** D3 global planar-only corner policy. Dokładne ograniczenia: `work/PM-05F_R2_CHAMFER_MIXED_CORNER_D2_PROPOSAL.md`.
- Ostatni kodowo/testowy kandydat ręcznie sprawdzony przez Ownera: `af85a6fafea643739393b4f29673eca56c666519`; Owner zgłosił **"manual test - pass"** dla skierowanego re-testu Part008 Chamfer / Edit / Save-Reopen / P1 selection. To scoped Owner PASS bez odrębnych raportów z każdego kroku 41-punktowej pełnej macierzy.
- Windows FOCUSED **#1898 PASS 1/1** na `af85a6f`: 18 wariantów (6 kolejności × 3 odległości), kompletność 21 Face, identyfikacja źródłowych 3 Edge strips + 2 source Vertex corners, w tym weryfikacja fizycznej incydencji źródłowych Edge z Vertex; testy lifecycle.
- Windows FAST **#1893 PASS 93/93** dotyczy wcześniejszego commitu; nie jest exact-head FAST.
- Po code/test candidate wprowadzono evidence/governance docs; **brak nowego exact-head Windows FULL** dla finalnego SHA. Odczytaj najnowszy status CI zamiast zakładać PASS.
- Następny gate: doprowadzić końcowego kandydata do exact-head Windows FULL; przejrzeć wymagane pełne Owner acceptance, dokumentację i PR. Do tego momentu PR pozostaje Draft i PM-06 nieaktywne.
- Open D2 STOP: ewentualna rozszerzona diagnostyka Viewer rejected-pick (`work/PM-05F_R2_PICK_REJECTION_DIAGNOSTICS_D2_PROPOSAL.md`); nie poszerzać API bez zgody Ownera.

## Reguły bez wyjątków

- Authority: Constitution → Foundation/Architecture → accepted ADR → Program Roadmap → Work Contract → Code/Tests.
- D0/D1 tylko w aktywnym kontrakcie; D2/D3 wymagają Ownera.
- Nie zmieniaj bezpośrednio `main`; nie usuwaj ani nie osłabiaj testów.
- Authored mutation: Command → Validation → Transaction → owning Domain Document → Evaluation.
- Model nie zależy od UI/Viewer/Qt/OCCT tokenów, provider topology ordinals ani geometrii podobnej jako identity.
- Fail closed: bez nearest/rebind, nieautoryzowanej tolerancji, cichego tangent-chain expansion lub partial multi-Edge success.
- `docs/browser/index.html` jest generowany. Regeneruj `./ss2.ps1 docs` ze źródłowego Markdown; nie edytuj HTML ręcznie.
- Brak zaakceptowanego aktywnego Work Contract oznacza brak nowej implementacji produktu.

## Raport po wznowieniu

Podaj krótko: (1) repo/branch/HEAD/PR, (2) ACTIVE/roadmap/contract, (3) obecny PM-05F R2 i findings, (4) ostatni dokładny CI tier i Owner manual evidence, (5) następny dozwolony gate, (6) D2/D3/STOP conditions. Repo ma pierwszeństwo przed tym hintem.
