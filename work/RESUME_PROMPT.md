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
5. zaakceptowane ADR-y istotne dla bieżącego/ostatniego work itemu
6. `work/ACTIVE.yaml`
7. aktywny work contract, a jeśli `ACTIVE.yaml` ma status `completed` — ostatni zakończony work contract
8. `governance/DOCUMENTATION.md`
9. aktualny branch / HEAD / otwarte PR-y / ostatnie relewantne commity / status relewantnego CI

Nie zgaduj, jeśli któregoś elementu brakuje. Repo i governance mają pierwszeństwo przed tym promptem.

Po rekonstrukcji raportuj krótko:

1. repo / branch / HEAD,
2. status work itemu i kontrakt,
3. ostatni zakończony krok i następny dozwolony krok,
4. blokery,
5. co jest D0/D1, a co wymaga D2/D3,
6. jeśli nie ma aktywnego zaakceptowanego kontraktu — **nie rozpoczynaj nowej implementacji produktu**; wskaż potrzebę decyzji Ownera.

## Zasady robocze SS2

- Authority: Constitution → Foundation/Architecture → ADR → Work Contract → Code + Tests.
- Foundation v1.0 jest frozen; tag bazowy: `foundation-v1.0`.
- Zmiana CORE wymaga jawnego Foundation Amendment zaakceptowanego przez Ownera.
- D0/D1 możesz wykonywać samodzielnie tylko wewnątrz aktywnego zaakceptowanego kontraktu.
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

Stan po zakończeniu DOC-01:

- PH-01 / MAIN v0.1 oraz PH-02A / PH-02B / PH-02C są zakończone
- `work/DOC-01_AS_BUILT_DOCUMENTATION_SYSTEM.md` jest zaakceptowany i zakończony
- `work/ACTIVE.yaml` powinien mieć status `completed` i wskazywać DOC-01 jako ostatni work item
- zaakceptowany ADR: `adr/ADR-0001-project-identity-and-workspace-clones.md`
- operacyjna reguła dokumentacji: `governance/DOCUMENTATION.md`
- canonical internal as-built docs: `docs/internal/` (English)
- canonical user/product docs: `docs/product/pl/` i `docs/product/en/`
- polska i angielska dokumentacja produktu są równorzędnymi źródłami; PL jest domyślnym językiem Browsera
- generated Product Browser: `docs/browser/index.html`
- Markdown jest source of truth; Browser jest deterministycznie generowany i freshness-checked
- `ss2-docs` / `.\ss2.ps1 docs` regeneruje Browser
- `ss2 verify` wykonuje dokumentacyjny validator + self-testy fail-closed
- każdy przyszły zaakceptowany work contract musi zawierać sekcję `## Documentation impact` z Internal docs / User/Product docs / Reason
- work item nie może zostać completed, jeśli zadeklarowana wymagana dokumentacja jest nieaktualna
- bieżący Project Hub: Create przez Project name + parent Location + Project folder; ProjectId pozostaje trwałą tożsamością; Recent/Locate/availability działają jak opisano w canonical docs
- po DOC-01 nie ma aktywnego kolejnego kontraktu implementacyjnego, dopóki Owner jawnie go nie zaakceptuje
- nie traktuj żadnego SHA z tego pliku jako bieżącego `main` HEAD

Windows CI:

- self-hosted runner: `SS2-Windows`
- label: `simplesolid2-native`
- workflow: `.github/workflows/windows-pr-gate.yml`
- gate sprawdza exact SHA → setup → verify (w tym docs validation) → build → test
- compiled CTest suite pozostaje na 10 testach; DOC-01 dodaje repo/documentation validation w `ss2 verify`
- validator self-tests obejmują brak pary PL/EN, mismatch section-id, broken local link, brak Documentation Impact, stale Browser oraz external Browser dependency
- tryb uruchomienia runnera (interaktywny vs Windows Service) jest stanem lokalnym maszyny; nie zakładaj go bez sprawdzenia

Lokalne hinty maszyny Ownera:

- workspace: `D:\SimpleSolid2`
- Qt root: `D:\Qt`
- OCCT env: `D:\SimpleSolid2\.simplesolid-env`
- self-hosted runner: `D:\runner-ss2`
- `START_SS2.cmd` uruchamia runner, kopiuje ten prompt do schowka i otwiera SS2 PowerShell
- komendy sesji: `ss2-status`, `ss2-run`, `ss2-docs`, `ss2-resume`

## Punkt kontroli architektonicznej

Obecna implementacja metadanych generuje i waliduje `ProjectId` jako UUIDv4. Semantyka ProjectId jest zaakceptowana przez Foundation/ADR-0001, ale nie traktuj automatycznie tekstowego formatu UUIDv4 jako zamrożonego publicznego kontraktu. Jeżeli kolejny work item miałby rozszerzyć publiczne zależności od tego formatu albo uczynić go częścią trwałego API/reference contract, zaklasyfikuj to konserwatywnie i w razie D2 zatrzymaj się po decyzję Ownera.

## Zakończony milestone MAIN v0.1

Zrealizowany przepływ:

`App start → Hub → Create/Open Project → stable ProjectId + metadata → ProjectSession → empty Workspace Shell → Close → restart → Recent Projects → reopen same ProjectId`

PH-01 nie autoryzuje Part / Assembly / Drawing / DocumentSession / geometrii / OCCT / Viewera ani innych domen CAD. Dalsza implementacja wymaga nowego jawnego work contract zaakceptowanego przez Ownera.
