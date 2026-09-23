# Dokumenty Part

<!-- doc-id: product.parts -->
<!-- document-kind: product -->

<!-- section-id: product.parts.create -->
## Tworzenie nowego Parta

Po otwarciu Projektu użyj `New Part…` w Workbench.

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

Pola `Number`, `Title`, `Description` i `Engineering revision` są trwałymi właściwościami Dokumentu i są niezależne od nazwy pliku.

<!-- section-id: product.parts.workbench -->
## Otwieranie Partów i Workbench

Użyj `Open Part…`, aby wybrać poprawny natywny Part.

Kilka Partów może pozostawać otwartych jednocześnie. Dolne Document Tabs przełączają aktywny Part. Ten sam Workbench zmienia Tree, Properties i kontekst viewportu 3D zgodnie z aktywną zakładką.

Po lewej znajduje się Document Tree, pośrodku viewport 3D, a po prawej Properties i Operations. Status/diagnostyka znajduje się pod zakładkami.

<!-- section-id: product.parts.edit -->
## Edycja właściwości Dokumentu

Gdy żaden obiekt/referencja nie jest primary selection, Properties pokazuje pola Dokumentu:

- Number;
- Title;
- Description;
- Engineering revision.

`Apply Properties` wprowadza zmianę do otwartej sesji Dokumentu. Zmiana nie jest trwała na dysku, dopóki nie wykonasz `Save`.

`Undo` i `Redo` działają dla authored changes bieżącej otwartej sesji.

<!-- section-id: product.parts.origin -->
## Origin, zaznaczenie i widoczność

Każdy Part ma w Document Tree grupę `Origin` zawierającą:

- Origin Point;
- X Axis, Y Axis, Z Axis;
- XY Plane, XZ Plane, YZ Plane.

Te referencje zawsze istnieją. Ukrycie referencji jej nie usuwa.

Możesz zaznaczyć jedną albo kilka referencji Origin w Tree. W menu kontekstowym użyj `Show` lub `Hide`, aby ustawić widoczność całego obsługiwanego zaznaczenia.

Grupowa zmiana widoczności jest jedną operacją Undo/Redo.

Tree i viewport 3D używają wspólnego zaznaczenia. Primary selected Origin reference jest pokazywana w Properties wraz z rolą, typem, semantyczną tożsamością i bieżącą widocznością.

Po `Save` widoczność Origin przeżywa zamknięcie i restart aplikacji.

<!-- section-id: product.parts.navigation -->
## Nawigacja 3D i ViewCube

Workbench Parta udostępnia:

- Pan środkowym przyciskiem myszy;
- Orbit przez Shift + środkowy przycisk myszy;
- Zoom kółkiem myszy;
- Fit;
- widoki Front, Back, Left, Right, Top i Bottom;
- Isometric oraz orientacje narożne;
- przełączanie Orthographic / Perspective.

ViewCube w prawym górnym rogu viewportu 3D służy do standardowych orientacji, Fit i przełączania projekcji.

Zmiany nawigacji i kamery są tymczasowym stanem widoku. Nie dirty'ują Parta, nie wymagają Save i nie tworzą wpisów CAD Undo.

<!-- section-id: product.parts.save-close -->
## Save i zamykanie

`Save` zapisuje bieżący authored state Parta, w tym widoczność Origin, do pliku `.ss2part`.

Przy zamykaniu Parta z niezapisanymi zmianami program wymaga decyzji `Save`, `Discard` albo `Cancel`.

Przy zamykaniu całego Projektu lub aplikacji, gdy jakikolwiek Part jest dirty, dostępne są `Save All`, `Discard` i `Cancel`.

Jeżeli zapis się nie powiedzie, Dokument/Projekt pozostaje otwarty.

<!-- section-id: product.parts.restart -->
## Restart i ponowne otwarcie

Po restarcie aplikacji otwórz ten sam Projekt.

SimpleSolid ponownie skanuje Workspace. Zapisany Part zostaje odnaleziony z tym samym DocumentId, zapisanymi właściwościami i zapisaną widocznością Origin.

Historia Undo/Redo, aktywne zaznaczenie i stan kamery nie są zapisywane do pliku i po ponownym otwarciu zaczynają się od nowa.

<!-- section-id: product.parts.conflicts -->
## Konflikt DocumentId i niepoprawne pliki

Jeżeli dwa pliki `.ss2part` w jednym Workspace zawierają ten sam DocumentId, `Open Part…` pokazuje konflikt tożsamości i wszystkie wykryte ścieżki.

Taki wpis nie może zostać otwarty przez DocumentId, dopóki konflikt nie zostanie usunięty. SimpleSolid nie wybiera arbitralnie jednej kopii i nie nadaje automatycznie nowego ID.

Uszkodzony albo nieobsługiwany natywny Part jest pokazywany jako `Invalid Part` zamiast być traktowany jako poprawny Dokument.

<!-- section-id: product.parts.current-limits -->
## Aktualne ograniczenia Parta

Obecny Part zapewnia tożsamość/właściwości Dokumentu, wbudowany Origin, trwałą widoczność referencji oraz wspólny fundament Workbench/Viewer 3D.

Nie zawiera jeszcze Sketch, Bodies, Features, modelowanej geometrii bryłowej, Material ani narzędzi Assembly/Drawing.
