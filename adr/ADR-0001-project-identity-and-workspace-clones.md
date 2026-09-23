# ADR-0001 — Tożsamość Projektu a kopie Workspace

**Status:** ACCEPTED  
**Owner acceptance:** 2026-09-23  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Zakres:** Project identity / Project Hub / Recent Projects

## Kontekst

Foundation definiuje `ProjectId` jako trwałą logiczną tożsamość Projektu, a ścieżkę Workspace jako jego aktualną lokalizację.

Zwykłe skopiowanie katalogu Projektu poza SS2 kopiuje również trwałe metadane, więc dwie różne lokalizacje mogą deklarować ten sam `ProjectId`.

Sam fakt istnienia dwóch takich katalogów nie pozwala wiarygodnie stwierdzić, czy druga lokalizacja jest:

- przeniesieniem Projektu,
- kopią bezpieczeństwa,
- kopią roboczą,
- czy próbą utworzenia niezależnego Projektu.

SS2 nie może rozstrzygać tego na podstawie samej ścieżki.

## Decyzja

Obowiązuje inwariant:

```text
jeden ProjectId
→ jedna logiczna tożsamość Projektu
→ maksymalnie jeden aktualnie zarejestrowany/zamontowany Workspace w Hub
```

### Move / Relocate

Jawne przeniesienie lub relokacja Projektu:

```text
Move / Relocate
→ zmiana lokalizacji Workspace
→ ten sam ProjectId
```

Nowa lokalizacja może zastąpić poprzednią tylko po zwalidowaniu zgodności oczekiwanego `ProjectId`.

### Duplicate / Save as New Project

Utworzenie niezależnego Projektu na podstawie istniejącego:

```text
Duplicate / Save as New Project
→ nowa niezależna tożsamość
→ nowy ProjectId
```

Powstanie nowej tożsamości wymaga jawnej operacji użytkownika.

SS2 nie generuje automatycznie nowego `ProjectId` tylko dlatego, że katalog został skopiowany lub znajduje się pod inną ścieżką.

### Zwykła kopia filesystemu

Skopiowanie katalogu poza SS2:

- nie mutuje oryginału,
- nie mutuje kopii,
- nie zmienia `ProjectId`,
- nie tworzy automatycznie nowego logicznego Projektu.

Taka kopia może pełnić rolę backupu bez utraty wierności tożsamości.

### Próba użycia drugiej lokalizacji

Jeżeli Hub zna już Projekt o `ProjectId = A`, a użytkownik przedstawia inną lokalizację również deklarującą `ProjectId = A`, system nie rejestruje jej automatycznie jako drugiego niezależnego Projektu.

Sytuacja jest traktowana jako konflikt wymagający jawnej decyzji użytkownika, na przykład:

- użyj nowej lokalizacji jako relokacji tego samego Projektu,
- albo utwórz z niej niezależny Projekt przez operację, która nada nowy `ProjectId`.

System nie może użyć różnicy ścieżek jako substytutu trwałej tożsamości.

## Zakres wykrywania konfliktu

SS2 nie skanuje całego filesystemu w poszukiwaniu kopii z tym samym `ProjectId`.

Konflikt jest rozpatrywany dopiero wtedy, gdy druga lokalizacja zostaje przedstawiona aplikacji przez operację Open, Relocate, Recent lub inną funkcję rejestrującą/montującą Workspace.

Nieznana aplikacji kopia leżąca na dysku nie jest sama w sobie błędem runtime.

## Recent Projects

Dla jednego `ProjectId` istnieje jeden logiczny wpis Recent.

Relokacja aktualizuje znaną lokalizację istniejącego Projektu zamiast tworzyć drugi logiczny wpis.

Usunięcie wpisu Recent nie zmienia tożsamości ani zawartości Projektu.

## Konsekwencje

Korzyści:

- `ProjectId` pozostaje rzeczywistą trwałą tożsamością,
- przeniesienie Projektu nie tworzy nowego Projektu,
- backup pozostaje wierną kopią,
- system nie tworzy niejawnie nowych tożsamości,
- zmniejszamy ryzyko rozjechania external references i przyszłych relacji między dokumentami.

Koszty:

- użytkownik musi jawnie rozstrzygnąć konflikt dwóch przedstawionych lokalizacji z tym samym ID,
- niezależna kopia robocza wymaga jawnej operacji Duplicate / Save as New Project,
- samo istnienie kopii na dysku nie jest wykrywane globalnie.

## Poza zakresem tej decyzji

ADR nie definiuje:

- synchronizacji dwóch kopii,
- merge Projektów,
- distributed editing,
- cloud locking,
- content hashing całego Workspace,
- globalnego skanowania dysków,
- sposobu implementacji przyszłej operacji Duplicate Project.

Te funkcje wymagają osobnych kontraktów lub ADR-ów, jeżeli staną się potrzebne.
