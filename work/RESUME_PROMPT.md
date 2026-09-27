# SimpleSolid 2.0 — prompt wznowienia nowego kontekstu

Kontynuujesz projekt **SimpleSolid 2.0 (SS2)**. Rozmawiaj ze mną po polsku. Pracuj technicznie, precyzyjnie i krytycznie.

## Źródło prawdy

Repozytorium: `MechanicalCave/SimpleSolid2`.

Ten plik jest wyłącznie handoff hintem. Repozytorium i governance są autorytatywne. Nie traktuj zapisanych tutaj SHA jako aktualnych bez sprawdzenia GitHub.

## Obowiązkowa rekonstrukcja

Czytaj w kolejności:

1. `AGENTS.md`
2. `governance/CONSTITUTION.md`
3. `governance/FOUNDATION.md`
4. `governance/ARCHITECTURE.md`
5. relevant accepted ADRs
6. `work/ACTIVE.yaml`
7. program roadmap wskazany przez ACTIVE — obecnie powinien to być `work/AUDIT-01_ARCHITECTURE_STABILIZATION_PROGRAM.md`
8. `work/SKETCH_ROADMAP.md` v1.4 jako subordinate feature roadmap, gdy praca dotyczy Shared 2D / Sketch
9. aktywny Work Contract
10. `governance/DOCUMENTATION.md`
11. aktualny branch/HEAD/PR/CI

Jeżeli stan repo różni się od tego hintu, repo ma pierwszeństwo.

## Trwały program po audycie 2026-09-27

Owner zaakceptował **AUDIT-01 Architecture Stabilization and Audit Implementation Program v1.0**.

Nie wolno zgubić ani pominąć jego pakietów przy zmianie kontekstu.

Domyślna kolejność programu:

`A (WB-02 closeout) -> B2 transaction freshness -> B1 save conflict -> C1 history copy -> C2 measured history design -> D semantic input/core build -> E1 numerics -> E2 presentation scale/failure -> F region/profile semantics`.

Każdy pakiet wymaga własnego bounded Work Contract przed implementacją, poza A1-A4, które są zaakceptowanym amendmentem aktywnego WB-02.

Otwarte decyzje wskazane przez AUDIT-01 (np. dokładna polityka file concurrency, głębsza reprezentacja historii, tolerancje, snapshot-vs-reference profilu) nie są automatycznie rozstrzygnięte. Muszą zostać jawnie zamknięte w odpowiednim przyszłym kontrakcie.

Nie implementuj prowizorycznego Extrude przed ukończeniem zaakceptowanej bramki region/profile F i osobnym kontraktem Part solid operation.

## Aktualny expected checkpoint — zweryfikuj

Oczekiwany stan w chwili aktualizacji tego promptu:

- PR #78: WB-02 workspace-global CAD Input;
- branch: `proposal-wb02-global-cad-input`;
- zweryfikowany kandydat przed Package-A hardening: `519f9c54e26ec85be0d216585b59e8a7f158d2fa`;
- Windows FULL #618: PASS, 73/73;
- Owner manualnie potwierdził działanie keyboard-first Command Line i refinements reject/stałej geometrii;
- niezależny audyt architektury został przejrzany;
- A1-A3 audytu są statycznie potwierdzone, A4 wymaga integracyjnego dowodu;
- WB-02 pozostaje ACTIVE i nie może być zamknięty/mergowany przed Package A + nowym exact-head FULL + Owner manual PASS;
- po merge WB-02 następnym domyślnym kontraktem programu jest B2 stale transaction protection.

## Nienaruszalne reguły

- Authority: Constitution -> Foundation/Architecture -> ADR -> Program Roadmap -> Work Contract -> Code/Tests.
- Foundation v1.0 frozen.
- D0/D1 tylko wewnątrz aktywnego kontraktu; D2/D3 wymagają Ownera.
- Nie zmieniaj bezpośrednio main.
- Nie osłabiaj testów.
- Durable mutation: Command -> Validation -> Transaction -> owning Domain Document -> Evaluation.
- UI/Viewer/Qt/OCCT/presentation token nie są trwałą CAD identity.
- Fail closed.
- Nie rozszerzaj scope po cichu.
- `docs/browser/index.html` jest generowany; używaj obowiązującego generatora, nie edytuj go ręcznie.
- Brak aktywnego zaakceptowanego kontraktu oznacza brak implementacji produktu.

## Raport po rekonstrukcji

Podaj krótko:

1. repo/branch/HEAD/PR;
2. ACTIVE + program roadmap + aktywny contract;
3. bieżący pakiet AUDIT-01 i status jego findings;
4. ostatni exact-head gate/manual evidence;
5. następny dozwolony krok;
6. ewentualne D2/D3/stop conditions.
