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

Na pasku narzędzi bezpośrednio nad viewportem 3D użyj `Sketch`, a następnie wskaż `XY Plane`, `XZ Plane` albo `YZ Plane` z Origin. Płaszczyznę możesz wybrać w Document Tree lub — gdy jest widoczna — w viewporcie 3D.

Po poprawnym wyborze Part tworzy trwały Sketch w tym samym Workbench i viewporcie. Kamera początkowo ustawia się prostopadle do płaszczyzny Sketchu, a siatka przechodzi do jego lokalnego układu, ale Pan, Zoom, Orbit i ViewCube pozostają dostępne i nie dirty'ują Dokumentu.

Podczas Sketch Edit pasek narzędzi udostępnia `Select`, `Line`, `Circle` i `Arc`. Operations pokazuje etap bieżącego narzędzia oraz akcje kontekstowe, a kompaktowy Command Line obsługuje `SELECT`, `LINE`, `CIRCLE` i `ARC`.

Zapisane geometrie Line, Circle i Arc są prezentowane razem ze znacznikiem lokalnego Origin `(0,0)` Sketchu. `Finish Sketch` kończy edycję, ale authored Sketch pozostaje w Parcie i po Save/Close/Reopen zachowuje SketchId, support, placement oraz authored entities. Istniejący Sketch można ponownie otworzyć dwuklikiem w Document Tree albo przez `Edit Sketch` bez zmiany authored state.

`Select` działa addytywnie dla wszystkich trzech typów prymitywów. Zwykły lewy klik dodaje niezaznaczoną entity i czyni ją primary; klik już zaznaczonej zachowuje członkostwo i ustawia primary. Ctrl+lewy klik przełącza członkostwo. Drag z lewej do prawej to Window, z prawej do lewej to Crossing; zwykły prostokąt dodaje trafienia, Ctrl+prostokąt je przełącza. Klik w puste tło lub Esc w Select czyści selection. `Delete Selection` albo Delete usuwa mieszany zestaw Line/Circle/Arc atomowo jako jedną operację Undo.

Każda zaznaczona edytowalna entity pokazuje kwadratowe runtime grips. Idle to mały pusty kwadrat, hover ma pusty kwadrat z cyjanowym podkreśleniem, a grip aktywny/captured jest wypełniony na żółto i może być nieco większy. Rozmiar pozostaje screen-space/DPI-aware, a tolerancja trafienia jest niezależna od widocznych pikseli. Grip ma priorytet trafienia nad geometrią pod nim.

Semantyka gripów:

- Line: Start/End zmienia tylko tę Line; Center przesuwa cały zamrożony selection.
- Circle: Center przesuwa cały zamrożony selection; cztery gripy quadrant zmieniają tylko promień właściciela przy stałym center.
- Arc: Center przesuwa cały zamrożony selection; Start/End zmieniają tylko ten Arc z zachowaniem canonical center/radius i reguł gałęzi; Arc/Mid zmienia wyłącznie radius przy zachowaniu center oraz kierunków Start/End.

Podgląd direct manipulation jest wyłącznie runtime: nie dirty'uje Dokumentu, nie zmienia revision, nie przydziela EntityId i nie tworzy historii Undo. Po aktywacji gripa możesz puścić przycisk myszy, swobodnie poruszać wskaźnikiem, a następny LMB lub Enter zatwierdza bieżący poprawny preview. Esc anuluje tylko preview i zachowuje selection. Zatwierdzony mieszany Move jest jedną atomową operacją i jednym krokiem Undo; edytowane entities zachowują EntityId. Undo/Redo najpierw anuluje transient interaction, potem wykonuje zwykłą historię Dokumentu.

Narzędzia tworzenia zachowują wcześniejsze selection, ale ukrywają/dezaktywują grips podczas działania; nowo utworzona geometria nie jest automatycznie zaznaczana.

- `Line`: kliknij pierwszy punkt, potem kolejne punkty ciągłych segmentów; każdy zatwierdzony segment to jeden krok Undo.
- `Circle`: kliknij Center, potem punkt Radius. Zerowy promień jest ignorowany. Po poprawnym commit Circle pozostaje aktywny dla następnego okręgu.
- `Arc`: kliknij Start, Through, potem End. Te trzy punkty wyznaczają skierowaną ścieżkę kołową, w tym CW/CCW i wariant short/long. Duplikaty/kollinearność/niepoprawna geometria nie commitują. Po sukcesie Arc pozostaje aktywny dla następnego łuku.

Operations udostępnia Finish/Cancel dla aktywnego narzędzia tworzenia. Esc najpierw anuluje bieżący niekompletny etap punktowy, a dopiero potem wraca narzędziem do Select; nie kończy całego Sketchu.

Support Sketchu jest obecnie ograniczony do trzech płaszczyzn Origin. Snapping/inference, coordinate/Dynamic Input, constraints/solver, authored dimensions, wspólne transformacje/Copy z R7, płaszczyzny Construction/Datum i płaskie ściany modelu pozostają późniejszymi etapami.

<!-- section-id: product.parts.navigation -->
## Nawigacja 3D i Navigation Cube

Workbench Parta udostępnia Pan środkowym przyciskiem myszy, Orbit przez Shift + środkowy przycisk myszy, Zoom kółkiem, Fit oraz projekcję Orthographic/Perspective.

W prawym górnym rogu viewportu 3D znajduje się przestrzenny Navigation Cube, który podąża za aktualną orientacją kamery. Kliknij opisaną ścianę, aby przejść do Front, Back, Left, Right, Top albo Bottom; krawędź wybiera odpowiadający widok dwuosiowy, a narożnik — odpowiadający widok izometryczny. Przejścia orientacji są animowane.

Gdy widok jest wyrównany do ściany, kontrolki wokół Cube umożliwiają dokładne przejścia o 90° do sąsiednich widoków oraz roll o dokładnie 90° zgodnie lub przeciwnie do ruchu wskazówek zegara. Home ustawia Top-Front-Right ISO i wykonuje Fit All.

Projekcja jest niezależna od orientacji Cube. Kontrolka `ORTHO/PERSP` obok Cube przełącza wyłącznie Orthographic/Perspective. Jeśli model jest w Perspective, kliknięcie ściany, krawędzi, narożnika albo Home zachowuje Perspective; analogicznie akcje te zachowują Orthographic, gdy jest ono aktywne.

Zmiany nawigacji i kamery są tymczasowym stanem widoku. Nie dirty'ują Parta, nie wymagają Save i nie tworzą wpisów CAD Undo.

<!-- section-id: product.parts.save-close -->
## Save i zamykanie

`Save` zapisuje bieżący authored state Parta, w tym widoczność Origin oraz utworzone Sketches z ich trwałą tożsamością/supportem/placementem, geometrią Line/Circle/Arc i stabilnymi EntityId, do pliku `.ss2part`.

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

Obecny Part zapewnia tożsamość/właściwości Dokumentu, wbudowany Origin, trwałą widoczność referencji, ustabilizowany fundament Workbench/Viewer 3D oraz trwałe Sketches na płaszczyznach Origin z authored geometrią Line/Circle/Arc.

Bieżący Sketch UI zapewnia Select/Line/Circle/Arc, addytywne point/Window/Crossing selection, semantyczne primary i hover, kwadratowe grips kodujące stan, Move mieszanego selection, ograniczony owner-only reshape, atomowy mieszany Delete, Undo/Redo, Operations oraz kompaktowy Command Line.

Bardzo wczesne testowe pliki `.ss2part` sprzed obecnego natywnego formatu nie są obsługiwanym formatem danych i nie są automatycznie migrowane.

Nadal brakuje snapping/inference, coordinate/Dynamic Input, constraintów/solvera, authored dimensions, wspólnych transformacji/Copy z R7, supportu Sketchu na Datum/płaskiej ścianie modelu, Bodies, Features, modelowanej geometrii bryłowej, Material oraz narzędzi Assembly/Drawing.
