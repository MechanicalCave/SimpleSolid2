# Dokumenty Part

<!-- doc-id: product.parts -->
<!-- document-kind: product -->

<!-- section-id: product.parts.create -->
## Tworzenie nowego Parta

Po otwarciu Projektu w Workspace użyj `New Part…`.

Podaj ścieżkę względną wewnątrz bieżącego Workspace. Natywny plik Part używa rozszerzenia:

```text
.ss2part
```

Program proponuje kolejne nazwy `Part001.ss2part`, `Part002.ss2part` itd., jeżeli są wolne.

Możesz wskazać istniejący podfolder, np. `Parts/Shaft.ss2part`. Program nie nadpisze po cichu istniejącego pliku.

<!-- section-id: product.parts.identity -->
## DocumentId, nazwa pliku i właściwości

Każdy nowy Part otrzymuje stabilny `DocumentId`.

Nazwa pliku i ścieżka nie są tożsamością Dokumentu. Możesz zmienić nazwę lub przenieść `.ss2part` wewnątrz Workspace; po `Refresh` ten sam DocumentId zostanie odnaleziony pod nową ścieżką.

Pola `Number`, `Title`, `Description` i `Engineering revision` są trwałymi właściwościami Dokumentu i są niezależne od filename.

<!-- section-id: product.parts.edit -->
## Edycja właściwości

Zaznacz poprawny Part na liście i wybierz `Open`.

Panel właściwości pozwala zmienić:

- Number;
- Title;
- Description;
- Engineering revision.

`Apply Properties` wprowadza zmianę do otwartej sesji Dokumentu. Zmiana nie jest jeszcze trwała na dysku, dopóki nie wykonasz `Save`.

`Undo` i `Redo` działają dla zmian właściwości w bieżącej otwartej sesji.

<!-- section-id: product.parts.save-close -->
## Save i zamykanie

`Save` zapisuje bieżący authored state Parta do jego pliku `.ss2part`.

Przy zamykaniu Parta z niezapisanymi zmianami program wymaga decyzji `Save`, `Discard` albo `Cancel`.

Przy zamykaniu całego Projektu lub aplikacji, gdy jakikolwiek Part jest dirty, dostępne są `Save All`, `Discard` i `Cancel`.

Jeżeli zapis się nie powiedzie, Dokument/Projekt pozostaje otwarty.

<!-- section-id: product.parts.restart -->
## Restart i ponowne otwarcie

Po restarcie aplikacji otwórz ten sam Projekt.

SimpleSolid ponownie skanuje Workspace. Zapisany Part zostaje odnaleziony z tym samym DocumentId i zapisanymi właściwościami.

Historia Undo/Redo nie jest zapisywana do pliku i po ponownym otwarciu zaczyna się od nowa.

<!-- section-id: product.parts.conflicts -->
## Konflikt DocumentId i niepoprawne pliki

Jeżeli dwa pliki `.ss2part` w jednym Workspace zawierają ten sam DocumentId, lista pokazuje `Identity conflict` i wszystkie wykryte ścieżki.

Taki wpis nie może zostać otwarty przez DocumentId, dopóki konflikt nie zostanie usunięty. SimpleSolid nie wybiera arbitralnie jednej kopii i nie nadaje automatycznie nowego ID.

Uszkodzony albo nieobsługiwany natywny Part jest pokazywany jako `Invalid Part` zamiast być traktowany jako poprawny Dokument.

<!-- section-id: product.parts.current-limits -->
## Aktualne ograniczenia Parta

Obecny Part przechowuje tożsamość i wspólne właściwości Dokumentu.

Nie zawiera jeszcze Sketch, Bodies, Features, geometrii 3D, Viewer, Material ani narzędzi Assembly/Drawing.
