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

Na pasku nad viewportem 3D użyj `Sketch`, a następnie wskaż `XY Plane`, `XZ Plane` albo `YZ Plane` z Origin. Płaszczyznę możesz wybrać w Document Tree lub — gdy jest widoczna — w viewporcie 3D.

Podczas Sketch Edit narzędzia są uporządkowane w grupy:

```text
Select

Create
  Line
  Circle
  Arc

Modify
  Move
  Rotate
  Scale
  Mirror
```

Kompaktowy Command Line obsługuje słowa `SELECT`, `LINE`, `CIRCLE`, `ARC`, `MOVE`, `ROTATE`, `SCALE` i `MIRROR`.

Tworzenie Line/Circle/Arc i zwykłe selection zachowują dotychczasową gramatykę. Zaznaczone edytowalne entities pokazują grips kodujące stan: pusty kwadrat w idle, pusty cyan na hover i pełny żółty dla aktywnego/captured gripa. Center grip przesuwa cały zamrożony mieszany selection; owner-only reshape zachowuje dotychczasową semantykę prymitywu.

### Wybór obiektów dla Modify

Move, Rotate, Scale i Mirror działają na dowolnym mieszanym selection Line/Circle/Arc.

Jeżeli obiekty są już zaznaczone, uruchom wybrane narzędzie Modify; bieżący selection zostaje od razu zamrożony i komenda przechodzi do pierwszego punktu odniesienia.

Jeżeli nic nie jest zaznaczone, uruchom narzędzie najpierw. Komenda przechodzi do **Select objects**:

- klik dodaje obiekt;
- Ctrl+klik przełącza członkostwo;
- drag z lewej do prawej używa Window;
- drag z prawej do lewej używa Crossing;
- Ctrl+prostokąt przełącza trafienia;
- blank LMB niczego nie czyści;
- Enter, Space albo RMB kończy wybór, gdy zaznaczono co najmniej jeden obiekt.

### Move

Wskaż **Base Point**, a potem destination. Cały zamrożony selection pokazuje preview jednego sztywnego przesunięcia `destination - base`.

LMB albo Enter zatwierdza poprawny destination. Przesunięcie o zero jest czystym no-op: nie zmienia authored state i nie tworzy kroku Undo.

### Rotate

Wskaż **Base Point**, potem **Reference Point**, a następnie destination definiujący kierunek końcowy.

Reference Point musi różnić się od Base Point. Obrót jest liczony jako podpisany kąt od wektora Base→Reference do wektora Base→destination w płaszczyźnie Sketchu; dodatni kierunek jest przeciwny do ruchu wskazówek zegara.

Obrót o zero jest poprawnym no-op i nie tworzy zmiany ani kroku Undo.

### Scale

Wskaż **Base Point**, potem **Reference Point**, a następnie destination.

Skala jest liczona z odległości od Base Point:

```text
factor = |destination - base| / |reference - base|
```

Factor musi być skończony i większy od zera. Wartości pomiędzy `0` i `1` pomniejszają geometrię, `1` jest no-op, a wartości większe od `1` powiększają. Zero i wartości ujemne nie są prawidłowym Scale.

### Mirror

Wskaż pierwszy, a następnie drugi punkt osi. Dwa różne punkty definiują nieskończoną linię odbicia.

Mirror odbija cały zamrożony selection. Line/Circle/Arc zachowują swoje EntityId, a kierunek Arc po odbiciu pozostaje geometrycznie zgodny z odbitym łukiem. Jeżeli geometria po odbiciu jest dokładnie taka sama, operacja kończy się jako no-op.

### Zatwierdzanie i wpisywanie wartości

Preview Move/Rotate/Scale/Mirror jest tylko runtime. Jeden rzeczywisty commit jest jedną atomową operacją i jednym krokiem Undo; zachowuje EntityId i współpracuje ze zwykłym Undo/Redo. Save/Close/Reopen zachowuje przekształconą geometrię w istniejącym formacie Part.

Esc anuluje bieżącą transformację i zachowuje objęty nią selection. LMB na końcowym etapie albo Enter zatwierdza bieżący poprawny preview.

Obecna wersja **nie obsługuje wpisywania numerycznych kątów, odległości ani współczynników skali**. Wpisanie np. `0.5`, `2` albo `90` przy focusie viewportu nie zastępuje wartości wynikającej z położenia kursora. Enter zatwierdza wtedy bieżący preview.

Space wpisany przy focusie pola tekstowego pozostaje znakiem tekstowym i nie uruchamia akcji CAD.

Narzędzia tworzenia zachowują wcześniejsze selection, ale ukrywają/dezaktywują grips podczas działania; nowa geometria nie jest automatycznie zaznaczana. `Finish Sketch` kończy edycję. Support Sketchu jest obecnie ograniczony do trzech płaszczyzn Origin.

Copy/repeated Copy, Repeat Last Command, snapping/inference, coordinate/Dynamic Input, constraints/solver, authored dimensions, płaszczyzny Construction/Datum i płaskie ściany modelu pozostają późniejszymi etapami.

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

Bieżący Sketch UI zapewnia Select oraz grupy Create i Modify z Line/Circle/Arc oraz Move/Rotate/Scale/Mirror, addytywne point/Window/Crossing selection, semantyczne primary i hover, kwadratowe grips kodujące stan, selection-first i command-first common transforms, Center-grip Move, ograniczony owner-only reshape, atomowy mieszany Delete, Undo/Redo, Operations oraz kompaktowy Command Line.

Bardzo wczesne testowe pliki `.ss2part` sprzed obecnego natywnego formatu nie są obsługiwanym formatem danych i nie są automatycznie migrowane.

Nadal brakuje Copy/repeated Copy, Repeat Last Command, numerycznego wpisywania kątów/odległości/skali i Dynamic Input, snapping/inference, constraintów/solvera, authored dimensions, supportu Sketchu na Datum/płaskiej ścianie modelu, Bodies, Features, modelowanej geometrii bryłowej, Material oraz narzędzi Assembly/Drawing.

