# SimpleSolid 2.0 — prompt wznowienia nowego kontekstu

Kontynuujesz projekt **SimpleSolid 2.0 (SS2)**. Rozmawiaj ze mną po polsku. Pracuj technicznie, precyzyjnie i krytycznie; nie potwierdzaj automatycznie moich pomysłów, jeśli widzisz ryzyko architektoniczne.

## Źródło prawdy

Repozytorium autorytatywne:

`MechanicalCave/SimpleSolid2`

SS1 jest wyłącznie donor/reference:

`MechanicalCave/SimpleSolid`

Ten plik jest tylko **handoff hintem**. Nie jest źródłem prawdy o aktualnym stanie. Po utracie kontekstu najpierw odtwórz stan przez podłączony GitHub.

## Obowiązkowa rekonstrukcja kontekstu

Przeczytaj w tej kolejności:

1. `AGENTS.md`
2. `governance/CONSTITUTION.md`
3. `governance/FOUNDATION.md`
4. `governance/ARCHITECTURE.md`
5. zaakceptowane ADR-y istotne dla aktywnej pracy
6. `work/ACTIVE.yaml`
7. aktywny work contract wskazany przez `ACTIVE.yaml`
8. aktualny branch / HEAD / otwarte PR-y / ostatnie relewantne commity / status relewantnego CI

Nie zgaduj, jeśli któregoś elementu brakuje. Repo i governance mają pierwszeństwo przed tym promptem.

Po rekonstrukcji raportuj krótko:

1. repo / branch / HEAD,
2. aktywny work item i kontrakt,
3. ostatni zakończony krok i następny krok,
4. blokery,
5. co jest D0/D1, a co wymaga D2/D3,
6. jeśli nie ma blokera ani D2/D3 — kontynuuj pracę od razu.

## Zasady robocze SS2

- Authority: Constitution → Foundation/Architecture → ADR → Work Contract → Code + Tests.
- Foundation v1.0 jest frozen; tag bazowy: `foundation-v1.0`.
- Zmiana CORE wymaga jawnego Foundation Amendment zaakceptowanego przez Ownera.
- D0/D1 możesz wykonywać samodzielnie tylko wewnątrz zaakceptowanego kontraktu.
- D2/D3 wymaga decyzji Ownera.
- Przed każdą trwałą zmianą ponownie sprawdź authority, scope i bieżący stan repo.
- Nie rób bezpośrednich zmian na `main`; używaj branch + PR.
- Nie osłabiaj testów, żeby uzyskać PASS.
- SS1 audytuj jako donor inwariantów/scenariuszy, nie kopiuj jego architektury hurtowo.
- Trwała mutacja CAD w przyszłości: semantic Command → Validation → Transaction → owning Domain Document → Evaluation.
- UI / Viewer / Qt / OCCT / topology IDs nie są trwałą tożsamością CAD.
- Fail closed.
- Nie rozszerzaj scope po cichu.

## Aktualny handoff hint — MUSISZ ZWERYFIKOWAĆ

Stan na 2026-09-23 po ostatnim zakończonym kroku:

- ostatni zakończony produktowy slice hint: `5f568689e4fd42422c876c7b9efbf69fe133a3a1` (`ProjectSession`); nie traktuj żadnego SHA z tego pliku jako bieżącego `main` HEAD
- aktywne: `work/PH-01_PROJECT_HUB.md`
- zaakceptowany ADR: `adr/ADR-0001-project-identity-and-workspace-clones.md`
- Project metadata persistence jest na `main`
- runtime-only `ProjectSession` jest na `main`
- następny kontraktowy krok: **Recent Projects persistence z logiczną tożsamością po ProjectId**
- po tym: minimalny Project Hub / Workspace Shell i pełny lifecycle create/open/close/restart/reopen
- na końcu poprzedniego kontekstu nie było otwartych PR-ów

Windows CI:

- self-hosted runner: `SS2-Windows`
- label: `simplesolid2-native`
- workflow: `.github/workflows/windows-pr-gate.yml`
- gate został zweryfikowany end-to-end: exact SHA → setup → verify → build → test = PASS
- tryb uruchomienia runnera (interaktywny vs Windows Service) jest stanem lokalnym maszyny; nie zakładaj go bez sprawdzenia

Lokalne hinty maszyny Ownera:

- workspace: `D:\SimpleSolid2`
- Qt root: `D:\Qt`
- OCCT env: `D:\SimpleSolid2\.simplesolid-env`
- self-hosted runner: `D:\runner-ss2`

## Punkt kontroli architektonicznej

Obecna implementacja metadanych generuje i waliduje `ProjectId` jako UUIDv4. Semantyka ProjectId jest zaakceptowana przez Foundation/ADR-0001, ale nie traktuj automatycznie tekstowego formatu UUIDv4 jako zamrożonego publicznego kontraktu. Jeżeli kolejny krok miałby rozszerzyć publiczne zależności od tego formatu albo uczynić go częścią trwałego API/reference contract, zaklasyfikuj to konserwatywnie i w razie D2 zatrzymaj się po decyzję Ownera.

## Kierunek produktu PH-01

Minimalny cel pozostaje:

`App start → Hub → Create/Open Project → stable ProjectId + metadata → ProjectSession → empty Workspace Shell → Close → restart → Recent Projects → reopen same ProjectId`

PH-01 nie autoryzuje implementacji Part / Assembly / Drawing / DocumentSession / geometrii / OCCT / Viewera ani innych domen CAD.
