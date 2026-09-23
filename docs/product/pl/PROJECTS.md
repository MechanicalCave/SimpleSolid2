# Projekty

<!-- doc-id: product.projects -->
<!-- document-kind: product -->

<!-- section-id: product.projects.concept -->
## Projekt i Workspace

Projekt ma trwałą logiczną tożsamość `ProjectId`.

Workspace jest folderem na dysku, w którym Projekt aktualnie się znajduje. Ścieżka może się zmienić bez zmiany tożsamości Projektu.

Prywatne metadane SimpleSolid znajdują się wewnątrz Workspace w:

```text
.simplesolid/project.json
```

<!-- section-id: product.projects.create -->
## Tworzenie Projektu

Wybierz `Create Project…`.

Dialog zawiera:

- `Project name` — nazwę wyświetlaną Projektu;
- `Location` — istniejący folder nadrzędny, np. `D:\Projekty`;
- `Project folder` — nazwę nowego folderu Workspace.

Program proponuje `Project folder` na podstawie nazwy Projektu, ale możesz go zmienić niezależnie.

Przykład:

```text
Project name:   Prasa hydrauliczna
Location:       D:\Projekty
Project folder: PrasaHydrauliczna

Final path:
D:\Projekty\PrasaHydrauliczna
```

SimpleSolid tworzy nowy folder Workspace. Docelowy folder nie może już istnieć. Program nie adoptuje, nie inicjalizuje i nie nadpisuje istniejącego folderu podczas Create.

Po poprawnym utworzeniu Projekt zostaje od razu otwarty i trafia do Recent Projects.

<!-- section-id: product.projects.open -->
## Otwieranie istniejącego Projektu

`Open Project…` służy do wskazania istniejącego, wcześniej zainicjalizowanego Workspace SimpleSolid.

Zwykły folder bez poprawnych metadanych `.simplesolid/project.json` nie zostanie po cichu zamieniony w Projekt.

<!-- section-id: product.projects.recent -->
## Recent Projects

Recent Projects pokazuje wcześniej otwierane Projekty.

Po zaznaczeniu poprawnego wpisu dostępne są:

- `Open`;
- `Locate…`;
- `Remove from Recent`.

Bez zaznaczenia te akcje są nieaktywne.

Usunięcie wpisu z Recent Projects nie usuwa ani nie zmienia plików Projektu.

<!-- section-id: product.projects.availability -->
## Gdy Workspace nie jest dostępny

Hub sprawdza zapamiętaną lokalizację Projektu i może oznaczyć wpis jako:

- `Workspace not found` — zapamiętany folder nie istnieje;
- `Invalid Project` — folder istnieje, ale nie zawiera poprawnego Projektu SimpleSolid;
- `Project mismatch` — pod zapamiętaną ścieżką znajduje się inny ProjectId.

Dla problematycznego wpisu `Open` pozostaje nieaktywne, ale `Locate…` i `Remove from Recent` są dostępne po zaznaczeniu.

Wpis nie jest automatycznie usuwany tylko dlatego, że Workspace został przeniesiony.

<!-- section-id: product.projects.move-relocate -->
## Przenoszenie Projektu i Locate

Możesz przenieść lub zmienić nazwę całego folderu Workspace poza SimpleSolid.

To nie zmienia `ProjectId`.

Po przeniesieniu stary wpis Recent zostanie oznaczony jako brakujący. Użyj `Locate…` i wskaż nową lokalizację.

SimpleSolid sprawdzi tożsamość Projektu przed aktualizacją zapamiętanej ścieżki. Folder zawierający inny ProjectId zostanie odrzucony.

<!-- section-id: product.projects.copy -->
## Kopiowanie folderu Projektu

Zwykłe kopiowanie folderu w systemie plików kopiuje również ProjectId.

Taka kopia nie staje się automatycznie nowym, niezależnym Projektem.

Jeżeli SimpleSolid zna już jeden Workspace dla danego ProjectId i przedstawisz inną lokalizację z tym samym ProjectId, program traktuje to jako konflikt tożsamości zamiast rejestrować drugi niezależny Projekt.

Funkcja `Duplicate / Save as New Project`, która utworzy nowy ProjectId, nie jest jeszcze dostępna.

<!-- section-id: product.projects.remove -->
## Remove from Recent

`Remove from Recent` usuwa wyłącznie zapamiętany wpis aplikacji.

Nie usuwa Workspace, `.simplesolid/project.json` ani innych plików Projektu.

Projekt można później ponownie otworzyć przez `Open Project…`.

<!-- section-id: product.projects.current-limits -->
## Aktualne ograniczenia

Obecny Workspace Shell nie zawiera jeszcze dokumentów CAD ani narzędzi modelowania.

Project Hub i opisane tutaj operacje Projektu są aktualnie zaimplementowaną powierzchnią produktu.
