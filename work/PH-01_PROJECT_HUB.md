# PH-01 — Testowalny Hub Projektów

**Status:** ACCEPTED  
**Owner acceptance:** 2026-09-23  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Powiązany ADR:** `ADR-0001-project-identity-and-workspace-clones.md`

## 1. Cel

Zaimplementować i przetestować minimalny, rzeczywisty cykl życia Projektu:

```text
Start aplikacji
→ Project Hub
→ Create Project / Open Project
→ walidacja trwałej tożsamości Projektu
→ ProjectSession
→ pusty Workspace Shell
→ Close Project
→ restart aplikacji
→ Recent Projects
→ Reopen
→ ten sam ProjectId
```

PH-01 ma udowodnić działanie trwałej tożsamości Projektu, podstawowej persystencji metadanych, rozdzielenia Projektu od jego lokalizacji, runtime lifecycle przez `ProjectSession`, historii `Recent Projects`, obsługi błędów zgodnej z fail-closed oraz podstawowego przepływu UI bez implementacji domen CAD.

PH-01 nie implementuje modelowania CAD.

## 2. Zakres IN

PH-01 obejmuje:

- start aplikacji do `Project Hub`,
- utworzenie nowego Projektu we wskazanej lokalizacji,
- otwarcie istniejącego, poprawnie zainicjalizowanego Projektu SS2,
- trwały `ProjectId` niezależny od ścieżki,
- minimalne metadane: `ProjectId`, `DisplayName`, `SchemaVersion`, `CreatedAt`,
- prywatną infrastrukturę Projektu pod `.simplesolid/`,
- runtime `ProjectSession`,
- przejście `Hub → Workspace Shell`,
- zamknięcie Projektu i powrót do Hub,
- `Recent Projects` jako stan aplikacji/użytkownika,
- restart aplikacji i ponowne otwarcie tego samego `ProjectId`,
- relokację przez jawną weryfikację oczekiwanego `ProjectId`,
- jedno logiczne wystąpienie danego `ProjectId` w Hub/Recent zgodnie z ADR-0001,
- strukturalną diagnostykę błędów,
- automatyczne testy lifecycle i persistence.

## 3. Project Workspace

`Project Workspace` oznacza aktualny root filesystemu Projektu.

Workspace jest lokalizacją, nie tożsamością.

Zmiana ścieżki, nazwy katalogu lub przeniesienie katalogu nie zmienia `ProjectId`, jeżeli zwalidowane metadane nadal deklarują ten sam Projekt.

```text
path      → gdzie znajduje się Projekt
ProjectId → który to Projekt
```

## 4. Trwała tożsamość Projektu

`ProjectId`:

- jest generowany podczas tworzenia Projektu,
- jest trwały,
- nie jest wyprowadzany ze ścieżki ani nazwy katalogu,
- nie jest identyfikatorem Qt, Viewer ani OCCT,
- nie jest runtime handle,
- nie zmienia się podczas restartu aplikacji,
- nie zmienia się po prawidłowym przeniesieniu Workspace.

## 5. Minimalne metadane

PH-01 utrwala co najmniej:

```text
ProjectId
DisplayName
SchemaVersion
CreatedAt
```

Prywatna infrastruktura Projektu znajduje się pod:

```text
.simplesolid/
```

Dokładny prywatny podział implementacyjny i nazwy plików są decyzją D0/D1, o ile nie zmieniają semantyki Foundation ani publicznego kontraktu Projektu.

Metadane muszą być zwalidowane przed utworzeniem autorytatywnego `ProjectSession`.

## 6. ProjectSession

Otwarcie Projektu tworzy runtime object `ProjectSession`.

`ProjectSession`:

- istnieje tylko podczas otwartego Projektu,
- reprezentuje runtime context Projektu,
- zna aktualną lokalizację Workspace,
- udostępnia zwalidowaną tożsamość Projektu,
- nie jest trwałą tożsamością Projektu,
- nie jest zapisywany jako część modelu Projektu,
- po zamknięciu Projektu przestaje istnieć,
- po ponownym otwarciu może być nową instancją reprezentującą ten sam `ProjectId`.

## 7. Workspace Shell

Po prawidłowym otwarciu Projektu aplikacja przechodzi z Hub do minimalnego `Workspace Shell`.

W PH-01 Workspace Shell może być pusty. Jego celem jest udowodnienie przejścia:

```text
Hub
→ zwalidowany ProjectSession
→ Workspace Shell
```

Nie należy dodawać funkcji CAD tylko po to, aby Workspace wyglądał bardziej kompletnie.

## 8. Close Project

Zamknięcie Projektu:

- kończy bieżący `ProjectSession`,
- zwalnia runtime state Projektu,
- wraca do `Project Hub`,
- nie usuwa danych Projektu,
- nie zmienia `ProjectId`,
- nie wykonuje niejawnej mutacji trwałego modelu Projektu.

## 9. Recent Projects

`Recent Projects` jest stanem aplikacji/użytkownika, nie częścią trwałego modelu Projektu.

Dla jednego `ProjectId` istnieje jeden logiczny wpis Recent. Zmiana lokalizacji aktualizuje znaną lokalizację tego Projektu zamiast tworzyć drugi logiczny Projekt.

Historia może przechowywać między innymi:

```text
ProjectId
ostatnia znana lokalizacja Workspace
DisplayName lub dane pomocnicze UI
```

Usunięcie wpisu z Recent nie może usuwać ani modyfikować Projektu.

Recent Projects nie jest źródłem prawdy o tożsamości Projektu. Źródłem prawdy pozostają zwalidowane dane samego Projektu.

## 10. Restart aplikacji

Po zamknięciu i ponownym uruchomieniu aplikacji:

1. `Recent Projects` jest ponownie dostępne.
2. Poprzedni runtime `ProjectSession` nie istnieje.
3. Otwarcie wpisu Recent ponownie waliduje Projekt.
4. Powstaje nowy `ProjectSession`.
5. Zwalidowany `ProjectId` musi być zgodny z oczekiwaną tożsamością.

## 11. Relokacja Projektu

Jeżeli zapisana lokalizacja Workspace nie istnieje, użytkownik może wskazać nową lokalizację.

Nowa lokalizacja zostaje zaakceptowana wyłącznie wtedy, gdy zwalidowany `ProjectId` jest równy oczekiwanemu `ProjectId`.

Jeżeli ID jest inne, operacja kończy się kontrolowanym błędem.

System nie może uznać innego Projektu za oczekiwany tylko dlatego, że użytkownik wskazał jego katalog.

## 12. Kopie Workspace i jedno ProjectId

Semantyka jest określona przez ADR-0001.

W skrócie:

```text
Move / Relocate
→ zachowuje ProjectId

Duplicate / Save as New Project
→ nowa niezależna tożsamość
→ nowy ProjectId

dwa Workspace z tym samym ProjectId
→ nie mogą być jednocześnie zamontowane/zarejestrowane
  jako dwa niezależne Projekty
```

Samo skopiowanie katalogu poza SS2 nie mutuje żadnej kopii i nie generuje automatycznie nowego `ProjectId`.

PH-01 nie musi implementować pełnej operacji `Duplicate Project`. Musi natomiast egzekwować brak automatycznego rozszczepiania tożsamości i brak jednoczesnego montażu dwóch lokalizacji jako dwóch niezależnych Projektów o tym samym ID.

## 13. Fail Closed

Otwarcie Projektu musi zakończyć się kontrolowanym błędem, jeżeli między innymi:

- wskazany katalog nie jest zainicjalizowanym Projektem SS2,
- brakuje wymaganych metadanych,
- metadane są uszkodzone,
- wymagane pole jest nieobecne,
- `ProjectId` jest niepoprawny,
- `SchemaVersion` nie jest obsługiwany,
- filesystem uniemożliwia wiarygodną walidację,
- relokowany Projekt ma inny `ProjectId`,
- operacja próbowałaby zarejestrować drugi niezależny Workspace z już znanym `ProjectId`.

System nie może naprawiać takich przypadków przez zgadywanie.

## 14. Tworzenie Projektu

`Create Project`:

- tworzy nowy Projekt we wskazanej lokalizacji,
- generuje nowy stabilny `ProjectId`,
- tworzy minimalne metadane,
- tworzy wymaganą prywatną infrastrukturę `.simplesolid/`,
- nie może bez jawnej decyzji nadpisywać istniejącego poprawnego Projektu SS2.

PH-01 nie obejmuje automatycznego adoptowania dowolnego niezainicjalizowanego katalogu jako Projektu.

## 15. Błędy i diagnostyka

Warstwy niższe niż UI zwracają strukturalną informację o błędzie.

UI może przetłumaczyć ją na komunikat użytkownika.

Niedozwolony kierunek:

```text
Persistence
→ QMessageBox
```

Preferowany kierunek:

```text
Persistence / Application
→ Result / Error / Diagnostic
→ UI
→ komunikat użytkownika
```

## 16. Poza zakresem PH-01

PH-01 jawnie nie obejmuje:

- `Part`,
- `Assembly`,
- `Drawing`,
- `DocumentSession`,
- feature tree,
- modelowania geometrycznego,
- Commands CAD,
- Transactions CAD,
- OCCT,
- Kernel,
- Viewer 3D,
- selekcji topologii,
- trwałych topology IDs,
- importu/eksportu CAD,
- szkicownika,
- constraints,
- browsera Part/Assembly,
- pluginów,
- AI Commands,
- automatyzacji CAD,
- generalized object framework,
- rozbudowanego template systemu,
- rekursywnego Project Browser,
- migracji architektury SS1.

## 17. Dozwolona implementacja

Po aktywacji kontraktu można tworzyć minimalne komponenty potrzebne dla PH-01, przykładowo w:

```text
src/core/
src/application/
src/persistence/
src/ui/
tests/
```

Dokładne prywatne nazwy katalogów, klas i plików są decyzją D0/D1, jeżeli nie zmieniają ownership, dependency direction, public API, durable identity, persistence semantics, Foundation ani Architecture.

Nie tworzymy abstrakcji wyłącznie dlatego, że mogą kiedyś być potrzebne.

## 18. SS1 — donor audit

SS1 pozostaje wyłącznie donor/reference.

### Port / reuse po oczyszczeniu

Najbardziej wartościowy donor dla PH-01:

```text
ProjectWorkspaceMetadata
```

oraz odpowiadające mu testy.

Z SS1 zachowujemy inwarianty:

- generowanie stabilnego ID,
- odczyt tego samego ID po ponownym otwarciu,
- minimalne metadane,
- prywatna infrastruktura `.simplesolid`,
- walidacja schema,
- odrzucenie niezainicjalizowanego katalogu,
- zachowanie niezależnych plików użytkownika.

Nie kopiujemy automatycznie struktury klas SS1.

### Concept only — RecentProjectStore

Zachowujemy koncepcje:

- globalnej historii użytkownika,
- bezpiecznego usuwania wpisów Recent,
- relokacji przez walidację `ProjectId`.

Nie kopiujemy polityki location-as-entry-identity. W SS2:

```text
ProjectId = identity
path = location
```

### Concept only — historyczny ApplicationWorkspace

Testy SS1 mogą być donorami scenariuszy lifecycle.

Nie kopiujemy automatycznie architektury `ApplicationWorkspace`, Document management ani domen CAD.

## 19. Testy akceptacyjne

PH-01 jest ukończony dopiero wtedy, gdy automatyczne testy udowadniają co najmniej:

1. Utworzenie Projektu generuje dokładnie jeden poprawny `ProjectId`.
2. Ponowne otwarcie Projektu zwraca ten sam `ProjectId`.
3. `ProjectId` nie zależy od ścieżki katalogu.
4. Przeniesienie Workspace nie zmienia `ProjectId`.
5. Brak wymaganych metadanych powoduje kontrolowany błąd.
6. Uszkodzone metadane powodują kontrolowany błąd.
7. Nieobsługiwany `SchemaVersion` powoduje kontrolowany błąd.
8. Zamknięcie Projektu kończy `ProjectSession`.
9. Ponowne otwarcie tworzy nowy runtime session dla tego samego Projektu.
10. Recent Projects przeżywa restart aplikacji.
11. Otwarcie przez Recent prowadzi do tego samego `ProjectId`.
12. Usunięcie wpisu Recent nie modyfikuje ani nie usuwa Projektu.
13. Relokacja akceptuje lokalizację z oczekiwanym `ProjectId`.
14. Relokacja odrzuca lokalizację z innym `ProjectId`.
15. Zwykły katalog nie jest cicho inicjalizowany podczas operacji Open.
16. Druga lokalizacja z już znanym `ProjectId` nie jest rejestrowana jako drugi niezależny Projekt.
17. Zwykła kopia filesystemu nie dostaje automatycznie nowego `ProjectId`.
18. Testy PH-01 nie wymagają `Part`, `Assembly`, `Drawing`, Viewer ani OCCT.
19. Cały build i wszystkie testy repozytorium przechodzą bez osłabiania testów.

## 20. Kryterium ukończenia

PH-01 jest ukończony, gdy:

```text
Start
→ Hub
→ Create/Open
→ ProjectSession
→ Workspace Shell
→ Close
→ Restart
→ Recent
→ Reopen
→ ten sam ProjectId
```

działa w rzeczywistej aplikacji i jest zabezpieczone automatycznymi testami.

Po ukończeniu należy zapisać zakończenie work itemu, zaktualizować `ACTIVE.yaml` i wskazać kolejny jawny kontrakt. Nie rozpoczynamy automatycznie domen CAD bez następnego zaakceptowanego zakresu.

## Zasada przewodnia

PH-01 nie ma udowodnić, że SS2 potrafi narysować bryłę.

Ma udowodnić, że SS2 wie czym jest Projekt, gdzie aktualnie się znajduje, kiedy jest otwarty, jak bezpiecznie go zamknąć i jak odnaleźć dokładnie ten sam Projekt po restarcie.
