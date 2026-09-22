# SimpleSolid 2 — canonical continuation prompt

Kontynuuj pracę nad **SimpleSolid 2.0 (SS2)**. Nie polegaj na pamięci poprzedniej rozmowy ani na domysłach. **Repozytorium jest źródłem prawdy o stanie implementacji i governance.**

## Repozytoria

- SS2 (autorytatywne): `MechanicalCave/SimpleSolid2`
- SS1 donor/reference: `MechanicalCave/SimpleSolid`
- lokalny workspace właściciela: `D:\SimpleSolid2`
- Qt root: `D:\Qt`
- lokalne środowisko OCCT: `D:\SimpleSolid2\.simplesolid-env`

SS1 traktuj jako **read-only donor/reference**, chyba że Owner wyraźnie autoryzuje jego modyfikację. Nie kopiuj architektury SS1 do SS2 bez sprawdzenia zgodności z Foundation SS2.

## Obowiązkowy start po utracie kontekstu

Użyj podłączonego GitHub connectora i najpierw odtwórz bieżący stan. Przeczytaj w tej kolejności:

1. `AGENTS.md`
2. `governance/CONSTITUTION.md`
3. `governance/FOUNDATION.md`
4. `governance/ARCHITECTURE.md`
5. zaakceptowane ADR-y istotne dla aktywnej pracy
6. `work/ACTIVE.yaml`
7. aktywny work contract wskazany przez `work/ACTIVE.yaml`
8. bieżący branch/HEAD, otwarte PR-y i ostatnie commity związane z aktywną pracą

Jeżeli aktywny contract lub wskazany plik nie istnieje, **nie zgaduj jego treści**. Zgłoś brak jako blocker albo przygotuj jego propozycję zgodnie z governance.

## Niezmienne zasady pracy

- `governance/FOUNDATION.md` v1.0 jest FROZEN; tag bazowy: `foundation-v1.0`.
- Zmiana CORE wymaga jawnego Foundation Amendment zaakceptowanego przez Ownera.
- Hierarchia autorytetu: Constitution → Foundation/Architecture → ADR → Work Contract → Code + Tests.
- D0/D1 możesz realizować autonomicznie wyłącznie wewnątrz zaakceptowanego contractu.
- D2/D3 wymagają decyzji Ownera; nie podejmuj ich implicite.
- Persistent CAD mutation ma iść przez semantyczny Command → Validation → Transaction → owning Domain Document → Evaluation.
- UI/Viewer/Qt/OCCT/topology IDs nie są trwałą tożsamością CAD.
- Fail closed; nie zgaduj design intent ani brakujących referencji.
- SS1 jest donorem. Preferuj: SS2 contract → invariant/test → donor audit → reuse/port/concept-only → verification.
- Nie rozszerzaj scope'u po cichu. Gdy contract jest błędny, zatrzymaj implementację i zaproponuj amendment.
- Nie osłabiaj testów tylko po to, żeby uzyskać PASS.

## Aktualny produktowy kierunek bazowy

Pierwszy milestone Foundation to `MAIN v0.1 — Testable Project Hub`: start aplikacji → Hub → create/open Project → stabilny `ProjectId` i minimalne metadata → `ProjectSession` / pusty workspace shell → close/restart → Recent Projects → reopen tego samego `ProjectId`.

To jest kierunek bazowy, ale **bieżący `work/ACTIVE.yaml` i zaakceptowany work contract rozstrzygają, co dokładnie robimy teraz**. Nie przeskakuj do Part/Assembly/Drawing/OCCT tylko dlatego, że są opisane w Foundation.

## Jak wznowić pracę

Po odczytaniu źródeł:

1. podaj krótko: repo / branch / HEAD;
2. wskaż aktywny work item i jego contract;
3. wskaż ostatni zakończony krok oraz następny konkretny krok;
4. zgłoś niespójności/blockery;
5. określ, czy następny krok jest D0/D1 czy wymaga D2/D3;
6. jeśli nie ma blockera ani decyzji D2/D3, **kontynuuj pracę od razu**, bez proszenia Ownera o powtarzanie historii zapisanej już w repo.

Przed każdą trwałą modyfikacją ponownie sprawdź scope i autorytet. Nie modyfikuj `main` bezpośrednio, jeśli repo wymaga pracy przez branch/PR.

Jeśli do tego promptu dołączony jest blok `LOCAL STATE APPENDED BY ss2-resume`, traktuj go jako pomoc w handoffie, ale zweryfikuj stan z repo przed mutacją.
