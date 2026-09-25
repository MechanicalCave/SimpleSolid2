# Dokumenty Part

<!-- doc-id: product.parts -->
<!-- document-kind: product -->

<!-- section-id: product.parts.create -->
## Tworzenie nowego Parta

Po otwarciu Projektu widzisz Project Workspace Dashboard bez aktywnego edytora Part. Na stałym pasku Project Workspace użyj `New Part…`, aby utworzyć Dokument i otworzyć jego Part Workbench. Ten pasek pozostaje widoczny także podczas edycji Parta.

Dialog pokazuje strukturę katalogów bieżącego Workspace. Wybierz folder docelowy, wpisz nazwę pliku Parta i potwierdź utworzenie.

Natywny Part jest jednym przenośnym plikiem i używa rozszerzenia:

```text
.ss2part
```

Cała trwała zawartość Dokumentu należy do tego jednego pliku. Przeniesienie lub zmiana nazwy pliku zmienia lokalizację, nie DocumentId.

Program proponuje kolejne nazwy `Part001.ss2part`, `Part002.ss2part` itd., jeżeli są wolne.

Użyj `Create Folder`, jeżeli chcesz utworzyć nowy katalog wewnątrz Workspace przed utworzeniem Parta.

Picker lokalizacji jest ograniczony do bieżącego Workspace. Nie pokazuje prywatnego `.simplesolid` jako normalnego miejsca zapisu i nie pozwala utworzyć Dokumentu poza Workspace.

Istniejący plik docelowy nigdy nie jest po cichu nadpisywany.

<!-- section-id: product.parts.identity -->
## DocumentId, nazwa pliku i właściwości

Każdy nowy Part otrzymuje stabilny `DocumentId`.

Nazwa pliku i ścieżka nie są tożsamością Dokumentu. Możesz zmienić nazwę lub przenieść `.ss2part` wewnątrz Workspace; po `Refresh` ten sam DocumentId zostanie odnaleziony pod nową ścieżką.

Pola `Number`, `Title`, `Description` i `Engineering revision` są trwałymi właściwościami Dokumentu i są niezależne od nazwy pliku.

<!-- section-id: product.parts.workbench -->
## Otwieranie Partów i Workbench

Użyj `Open…` na stałym pasku Project Workspace, aby wybrać wykryty Dokument. `Open…`, `New Part…` i `Refresh` pozostają dostępne również wtedy, gdy edytujesz już inny Part. Samo otwarcie Projektu nie uruchamia Part Workbench; edytor pojawia się dopiero po utworzeniu lub otwarciu konkretnego Dokumentu.

Dialog pokazuje typ Dokumentu, nazwę, lokalizację i status. Szerokości kolumn możesz zmieniać ręcznie przeciągając separatory nagłówka; domyślnie więcej miejsca otrzymuje lokalizacja niż nazwa. Obecny produkt dostarcza tylko Dokumenty Part, ale sam interfejs Open nie jest Part-specific.

Niepoprawne pliki natywne i konflikty DocumentId pozostają widoczne, ale nie można ich otworzyć jako resolved Documents.

Kilka Partów może pozostawać otwartych jednocześnie. Dolne Document Tabs należą do Project Workspace i przełączają aktywny Dokument. Ponowne otwarcie Parta, który jest już otwarty, aktywuje istniejącą zakładkę/sesję zamiast tworzyć drugą mutowalną sesję.

Przycisk `Workspace` na górnym pasku wraca do Project Workspace Dashboard bez zamykania otwartych Dokumentów. Ich taby pozostają dostępne i możesz wrócić do dowolnego Parta klikając jego zakładkę. Sam powrót do Workspace nie wykonuje Save i nie zmienia authored state. Zamknięcie ostatniego Dokumentu również wraca do Workspace bez zamykania Projektu.

W aktywnym Part Workbench po lewej znajduje się Document Tree, pośrodku viewport 3D z paskiem narzędzi nad nim, a po prawej Properties i kontekstowy panel Operations. Status/diagnostyka należy do aktywnego Workbench.

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

Zachowanie zaznaczenia w Viewporcie:

- lewy klik — zastępuje zaznaczenie;
- Ctrl + lewy klik — dodaje/usuwa element z zaznaczenia;
- lewy klik w puste tło — czyści zaznaczenie;
- prawy klik — jest zarezerwowany dla menu kontekstowego; tam, gdzie menu nie istnieje, obecnie nic nie robi.

Po `Save` widoczność Origin przeżywa zamknięcie i restart aplikacji.

<!-- section-id: product.parts.sketch-host -->
## Tworzenie i wyświetlanie Sketchu

Na pasku narzędzi bezpośrednio nad viewportem 3D użyj `Sketch`, a następnie wskaż jedną z płaszczyzn `XY Plane`, `XZ Plane` albo `YZ Plane` w Origin. Płaszczyznę możesz wskazać w Document Tree albo — gdy jest widoczna — w viewporcie 3D.

Operations nie jest stałą listą narzędzi. Pokazuje sterowanie bieżącą operacją: podczas wyboru płaszczyzny Sketchu zawiera kontekst wyboru/Cancel, a podczas edycji — kontekstowe akcje Sketchera. Po wyjściu ze Sketch Edit pasek narzędzi i Operations wracają do kontekstu Part modeling.

Po poprawnym wyborze Part tworzy trwały Sketch i pozostaje w tym samym Workbench oraz tym samym viewporcie 3D. Kamera automatycznie ustawia się prostopadle do płaszczyzny Sketchu, a siatka przechodzi do jego lokalnej płaszczyzny. Podczas Sketch Edit pasek narzędzi przełącza się na `Select` i `Line`; Operations pokazuje stan i akcje bieżącego narzędzia Sketchera; a pod viewportem pojawia się kompaktowy Command Line. Na tym etapie Command Line obsługuje `SELECT` i `LINE`.

Każda authored geometria Line już zapisana w tym Sketchu jest prezentowana w viewporcie razem ze znacznikiem Origin lokalnego `(0,0)` Sketchu.

Automatyczne ustawienie widoku nie blokuje kamery. Podczas aktywnego Sketchu nadal możesz używać Pan, Zoom, Orbit i ViewCube. Nawigacja nie zmienia położenia Sketchu i sama nie dirty'uje Dokumentu.

Użyj `Finish Sketch`, aby zakończyć bieżący tryb edycji. Sketch pozostaje authored obiektem Parta, jest widoczny w Document Tree i po `Save` przeżywa Close/Reopen z tym samym SketchId, supportem i placementem.

Aby ponownie edytować istniejący Sketch, kliknij go dwukrotnie w Document Tree albo użyj jego menu kontekstowego `Edit Sketch`. Samo wejście do edycji nie zmienia authored state i nie wymaga Save.

W Sketch Edit domyślnym narzędziem jest `Select`. Lewy klik na Line zastępuje semantyczne zaznaczenie, Ctrl+lewy klik przełącza wskazaną Line, a przeciągnięcie prostokąta wykonuje Window selection z lewej do prawej albo Crossing selection z prawej do lewej. Rectangle selection zastępuje bieżące zaznaczenie. `Delete Selection` w Operations lub klawisz Delete, gdy viewport ma focus w Sketch Select, usuwa zaznaczone Lines jako jedną operację Undo.

Po aktywacji `Line` kliknij pierwszy punkt, a potem kolejne punkty, aby tworzyć ciągłe segmenty. Rubber-band preview jest tylko stanem runtime; każdy zatwierdzony segment jest osobnym krokiem Undo. `Finish Line` i `Cancel Line` wracają do Select bez wycofywania już zatwierdzonych segmentów. Esc najpierw anuluje bieżący etap Line, a następnie wraca z Line do Select; nie kończy całego Sketchu. Undo/Redo najpierw porzuca oczekujący etap Line, a potem wykonuje zwykłą historię Dokumentu.

Support może być obecnie tylko jedną z trzech płaszczyzn Origin. Arc/Circle, grips/direct manipulation, snapping/inference, wprowadzanie współrzędnych, constraints, wymiary, solver, płaszczyzny Construction/Datum oraz płaskie ściany modelu pozostają późniejszymi etapami.

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

ViewCube jest overlayem w prawym górnym rogu viewportu 3D. Gdy Editor Surface staje się wąski, ViewCube przechodzi w układ kompaktowy zamiast nachodzić na Properties albo wymuszać szeroki Viewport.

Do standardowych orientacji, Fit i przełączania projekcji możesz używać normalnego albo kompaktowego wariantu ViewCube.

Zmiany nawigacji i kamery są tymczasowym stanem widoku. Nie dirty'ują Parta, nie wymagają Save i nie tworzą wpisów CAD Undo.

<!-- section-id: product.parts.save-close -->
## Save i zamykanie

`Save` zapisuje bieżący authored state Parta, w tym widoczność Origin oraz utworzone Sketches z ich trwałą tożsamością/supportem/placementem i osadzoną authored geometrią Line, do pliku `.ss2part`.

Przy zamykaniu Parta z niezapisanymi zmianami program wymaga decyzji `Save`, `Discard` albo `Cancel`.

Przy zamykaniu całego Projektu lub aplikacji, gdy jakikolwiek Part jest dirty, dostępne są `Save All`, `Discard` i `Cancel`.

Jeżeli zapis się nie powiedzie, Dokument/Projekt pozostaje otwarty. Zamknięcie ostatniego otwartego Parta pozostawia Projekt otwarty i pokazuje Project Workspace Dashboard. Samo użycie `Workspace` nigdy nie zamyka Parta.

<!-- section-id: product.parts.restart -->
## Restart i ponowne otwarcie

Po restarcie aplikacji otwórz ten sam Projekt.

SimpleSolid ponownie skanuje Workspace. Zapisany Part zostaje odnaleziony z tym samym DocumentId, zapisanymi właściwościami, zapisaną widocznością Origin oraz zapisanymi Sketches.

Historia Undo/Redo, aktywne zaznaczenie, aktywny tryb edycji Sketchu i stan kamery nie są zapisywane do pliku i po ponownym otwarciu zaczynają się od nowa.

<!-- section-id: product.parts.conflicts -->
## Konflikt DocumentId i niepoprawne pliki

Jeżeli dwa pliki `.ss2part` w jednym Workspace zawierają ten sam DocumentId, `Open…` pokazuje wpis konfliktu tożsamości oraz informacje o wykrytych lokalizacjach.

Takiego wpisu nie można otworzyć przez DocumentId, dopóki konflikt nie zostanie usunięty. SimpleSolid nie wybiera arbitralnie jednej kopii i nie nadaje automatycznie nowego ID.

Uszkodzony albo nieobsługiwany natywny Part jest pokazywany jako niepoprawny wpis zamiast być traktowany jako poprawny Dokument.

<!-- section-id: product.parts.current-limits -->
## Aktualne ograniczenia Parta

Obecny Part zapewnia tożsamość/właściwości Dokumentu, wbudowany Origin, trwałą widoczność referencji, wspólny ustabilizowany fundament Workbench/Viewer 3D, trwałe Sketches z osadzonymi authored danymi Line oraz prezentację aktywnego Sketchu (Lines + Origin) na płaszczyznach Origin.

Bardzo wczesne testowe pliki `.ss2part` utworzone przed wprowadzeniem obecnego natywnego formatu nie są obsługiwanym formatem danych i nie są automatycznie migrowane.

Bieżący UI zapewnia pierwszy kompletny workflow authored Sketch Line: kontekstowe narzędzia Select/Line, ciągłe tworzenie Line z preview, point selection i Window/Crossing rectangle selection, atomowy Delete, integrację Undo/Redo, Operations oraz kompaktowy Command Line dla SELECT/LINE. Nadal brakuje grips/direct manipulation, snapping/inference, wprowadzania współrzędnych, constraintów/solvera, supportu Sketchu na Datum/płaskiej ścianie modelu, Bodies, Features, modelowanej geometrii bryłowej, Material oraz narzędzi Assembly/Drawing.
