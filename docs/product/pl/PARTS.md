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
  Copy
  Rotate
  Scale
  Mirror

Inspect
  Measure
```

Command Line jest wspólną powierzchnią Workspace i nie wymaga kliknięcia przed normalnym wprowadzaniem CAD. Gdy focus ma viewport lub inna zwykła powierzchnia CAD, możesz od razu pisać — znaki pojawią się w Command Line, ale focus pozostanie w viewport. Enter wysyła wpisany token do aktywnego kontekstu. Kliknięcie Command Line nadal działa i edytuje dokładnie ten sam bufor.

W aktywnym Sketchu obsługiwane są słowa `SELECT`, `LINE`, `CIRCLE`, `ARC`, `MEASURE`, `MOVE`, `COPY`, `ROTATE`, `SCALE` i `MIRROR`. Command Line jest kontekstowy: jeżeli aktywny etap oczekuje punktu, bieżący semantic PointRequest ma pierwszeństwo przed uruchomieniem nowej komendy. W etapach obsługujących Direct Distance samodzielna wartość liczbowa jest interpretowana jako odległość. Podczas aktywnej manipulacji gripem `C` + Enter jest lokalnym słowem kluczowym włączającym Grip Copy; poza aktywnym gripem `C` nie staje się globalnym aliasem komendy. Enter konsumuje wpisany token także wtedy, gdy jest błędny: model oraz aktywne narzędzie/etap pozostają bez zmian, edytowane pole jest czyszczone, a diagnostic informuje o błędzie bez zmuszania użytkownika do ręcznego kasowania odrzuconego tekstu.

Prawdziwe pola tekstowe — np. edycja właściwości — zachowują własny focus i wpisywanie do nich nie zasila Command Line. Skróty aplikacji z Ctrl/Alt/Meta również nie są zamieniane na tekst CAD. Menu, popup, okno modalne, inne okno aplikacji oraz drugi widoczny Workspace SS2 zachowują własność klawiatury i nie zasilają Command Line działającego w tle.

Częściowo wpisany token CAD należy do semantycznego requestu, w którym rozpoczęto wpisywanie. Zmiana requestu/narzędzia/Sketcha/Dokumentu czyści taki token, zanim mógłby wykonać się w nowym kontekście. Zwykły ruch kursora zmieniający kierunek w ramach tego samego PointRequest nie czyści wpisu. Zmiana Dokumentu lub przejście do Workspace również usuwa częściowy input, więc nie może on trafić do ukrytego Dokumentu.

Przy focusie viewportu i niepustym buforze Command Line klawisz Delete należy do aktywnego inputu i nie usuwa zaznaczonej geometrii. Ostatni znak usuwa Backspace. Gdy bufor jest pusty, zwykłe Delete Selection działa jak wcześniej. Jeżeli focus ma sam Command Line albo inne prawdziwe pole tekstowe, Delete pozostaje normalną edycją tekstu.

Dolny Command Line pozostaje zawsze jednym wierszem. Obszar diagnostyczny po prawej stronie ma stale zarezerwowane miejsce, więc pojawienie się błędu nie zmienia szerokości pola wejściowego i nie powoduje przeskoku Viewera. Długi komunikat jest wizualnie skracany w tym wierszu, a pełna treść jest dostępna jako tooltip.

### Powtórzenie ostatniej komendy

Podczas jednego aktywnego Sketch Edit SimpleSolid pamięta ostatnią poprawnie uruchomioną komendę z zestawu `Line / Circle / Arc / Move / Copy / Rotate / Scale / Mirror`.

Gdy jesteś w zwykłym `Select` i focus ma viewport, **Enter** albo **Space** uruchamia tę komendę ponownie.

Powtórzenie zaczyna świeżą komendę. Nie odtwarza poprzednich Base/Reference/axis/placement points, preview ani wcześniejszego selection snapshotu. Używa aktualnego selection zgodnie z normalną gramatyką narzędzia — np. MOVE z aktualnym selection działa selection-first, a bez selection przechodzi do Select objects. Powtórzone COPY ponownie prosi o Base Point i nie używa wcześniejszych placementów.

`Select`, zwykłe zaznaczanie, Delete, grip manipulation, Undo/Redo i Esc nie zastępują zapamiętanej komendy. Pamięć jest czyszczona po zakończeniu Sketch Edit i nie przechodzi do innego Sketchu, Dokumentu ani po ponownym otwarciu pliku.

Istniejące znaczenie klawiszy ma pierwszeństwo: Enter nadal zatwierdza aktywną manipulację/transformację, Enter/Space nadal kończy Select objects w aktywnej transformacji, a Space przy focusie pola tekstowego pozostaje zwykłą spacją.


Tworzenie Line/Circle/Arc i zwykłe selection zachowują dotychczasową gramatykę. Zaznaczone edytowalne entities pokazują grips kodujące stan: pusty kwadrat w idle, pusty cyan na hover i pełny żółty dla aktywnego/captured gripa.

### Tryby edycji gripów

Center grips dla Line/Circle/Arc są **Move-only**. Kliknięcie center gripa przesuwa cały zamrożony mieszany selection, używając położenia gripa ze startu sesji jako implicit base.

Line Start/End, cztery Circle quadrant grips oraz Arc Start/End/Mid domyślnie uruchamiają **Reshape** właściciela. Podczas aktywnej manipulacji możesz nacisnąć **Space**, aby przełączać:

```text
Reshape ↔ Move
```

W `Reshape` zmienia się tylko prymityw będący właścicielem aktywnego gripa. W `Move` przesuwa się cały selection zamrożony w chwili rozpoczęcia manipulacji. Ten sam aktywny grip, pivot, selection oraz bieżące położenie kursora pozostają zachowane przy przełączaniu.

Preview po Space jest liczone ponownie z geometrii ze startu sesji, a nie z poprzedniego preview. Samo przełączenie nie zmienia dokumentu, nie tworzy revision ani kroku Undo. Operations pokazuje bieżący tryb jako `Grip — Reshape` albo `Grip — Move`.

LMB albo Enter zatwierdza aktualny tryb. Esc anuluje całą niezatwierdzoną manipulację i zachowuje selection. Na center gripach Space niczego nie przełącza — pozostają w Move.

Space podczas aktywnej manipulacji gripem ma pierwszeństwo przed Repeat Last Command. Space przy focusie Command Line/pola tekstowego pozostaje zwykłą spacją.

Podczas grip Reshape albo grip Move możesz także użyć Direct Distance: ustaw kursorem kierunek od położenia gripa ze startu sesji, wpisz odległość w Command Line i naciśnij Enter. Dla Move rozwiązuje to punkt docelowy dokładnie w zadanej odległości od pivotu; dla Reshape ten sam resolved point trafia do zwykłej semantyki reshape danego gripa.

### Grip Copy

Podczas aktywnej manipulacji gripem wpisz `C` i naciśnij Enter, aby włączyć **Grip Copy**. Copy jest modyfikatorem bieżącego trybu Reshape/Move, a nie trzecim trybem edycji.

W **Reshape + Copy** każdy zaakceptowany placement tworzy jedną kopię ze świeżym EntityId tylko prymitywu będącego właścicielem aktywnego gripa; oryginalny właściciel i wszystkie pozostałe zaznaczone encje pozostają bez zmian. W **Move + Copy** każdy placement kopiuje cały selection zamrożony przy rozpoczęciu manipulacji gripem. Oryginały pozostają selection/reference set, a utworzone kopie nie przejmują zaznaczenia.

Placement możesz zaakceptować LMB albo istniejącym Direct Distance: włącz Copy, ustaw kierunek kursorem, wpisz odległość i naciśnij Enter. Po udanym placement Grip Copy pozostaje aktywny dla następnej kopii z tego samego źródła i pivotu ze startu interakcji, ale poprzedni kierunek kursora jest czyszczony; przed kolejnym numerycznym placementem ponownie przesuń kursor.

Placement bez rzeczywistej zmiany jest czystym no-op. Space zmienia Reshape↔Move tam, gdzie jest obsługiwane, i wyłącza Grip Copy, więc dla nowego trybu trzeba ponownie wysłać `C`. Esc kończy bieżącą sesję gripu i zachowuje wcześniej zatwierdzone kopie. Undo podczas aktywnego Grip Copy najpierw kończy transient session, a następnie cofa tylko ostatni zatwierdzony krok historii.

### Wybór obiektów dla Modify

Move, Copy, Rotate, Scale i Mirror działają na dowolnym mieszanym selection Line/Circle/Arc.

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

Destination możesz wskazać LMB albo podać przez Direct Distance: po Base Point ustaw kursorem kierunek, wpisz odległość w Command Line i naciśnij Enter. Przesunięcie o zero jest czystym no-op: nie zmienia authored state i nie tworzy kroku Undo.

### Copy

COPY używa tego samego modelu wyboru obiektów co pozostałe narzędzia Modify. Po zamrożeniu source selection wskaż **Base Point**, a następnie placement point. Placement możesz wskazać LMB albo rozwiązać przez Direct Distance z bieżącego kierunku kursora.

Preview pokazuje translację oryginalnego source selection o `placement - base`. Przy poprawnym niezerowym placement:

- oryginalne Line/Circle/Arc pozostają bez zmian i pozostają zaznaczone;
- każda nowa kopia dostaje świeże, niealiasujące EntityId;
- jeden placement jest jedną atomową zmianą i jednym krokiem Undo;
- COPY pozostaje aktywne, więc możesz wskazać kolejne placement points bez ponownego wyboru obiektów i Base Point.

Każdy kolejny placement jest liczony od tego samego oryginalnego source snapshotu i Base Point, a nie od poprzednio utworzonej kopii.

Placement dokładnie w Base Point nie tworzy niewidocznej nakładającej się kopii. To czysty no-op: nie powstają entities, nie są zużywane EntityId i nie powstaje krok Undo.

Esc kończy bieżącą sesję COPY i zachowuje source selection. Kopie zatwierdzone wcześniej w tej samej sesji pozostają w Sketchu. Undo/Redo działa na poszczególnych zatwierdzonych placementach; Redo przywraca te same EntityId kopii, a nowe COPY po Undo nie wykorzystuje ponownie ID wcześniej zatwierdzonej i cofniętej kopii.

Undo nie cofa session-local high-water identyfikatorów, więc nowy COPY w tej samej sesji nie wykorzystuje ponownie ID cofniętej kopii. Jeżeli Undo przywróci dokładnie ostatnio zapisany authored state, dokument pozostaje clean jak wcześniej. Gdy późniejsza zatwierdzona kopia zostanie normalnie zapisana, istniejący format Part utrwala także aktualny `next_entity_id`.

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

Preview Move/Copy/Rotate/Scale/Mirror jest tylko runtime. Move/Rotate/Scale/Mirror edytują istniejące entities i zachowują ich EntityId. COPY tworzy nowe entities ze świeżymi EntityId wyłącznie przy zatwierdzonym placement.

LMB na końcowym etapie albo Enter zatwierdza bieżący poprawny preview/placement. Esc anuluje niezatwierdzony stan i zachowuje właściwy selection.

Obecna wersja obsługuje **Direct Distance** w pięciu klasach wejścia punktowego: drugi/kolejny punkt LINE, grip Reshape, grip Move, destination normalnego MOVE oraz placement normalnego COPY. Grip Copy używa istniejącego PointRequest gripa Reshape/Move zamiast tworzyć nową ścieżkę numeryczną; gdy Copy jest ON, rozwiązany punkt gripa zatwierdza kopię zamiast edycji oryginału. Najpierw musi istnieć base/pivot, następnie ustaw kursorem niezerowy kierunek, wpisz w Command Line skończoną nieujemną odległość i naciśnij Enter. Separator `.` jest zawsze akceptowany; akceptowany jest również bieżący separator dziesiętny locale, np. `,` w polskiej konfiguracji.

Po numerycznym placement COPY pozostaje aktywne dla następnej kopii, ale poprzedni kierunek nie jest używany ponownie po cichu — przesuń kursor, aby ustalić kierunek kolejnego Direct Distance. W LINE kolejny segment analogicznie wymaga nowego kierunku od nowego endpointu. Wartość `0` przechodzi przez resolver, po czym zwykła semantyka narzędzia decyduje o no-op/rejection: LINE nie tworzy odcinka zerowego, MOVE jest no-op, a COPY nie tworzy nakładającej się kopii.

Nie są jeszcze obsługiwane: numeryczny kąt Rotate, współczynnik Scale, współrzędne absolutne/względne, zapis polarny, sufiksy/jednostki, Dynamic Input, Ortho/Polar ani snapping/tracking. Drukowalny tekst wpisywany przy normalnym focusie viewportu CAD trafia do globalnego bufora Command Line; znaczenie liczby nadal należy do aktywnego PointRequest/narzędzia i nie jest zgadywane przez globalny router.

Space wpisany przy focusie pola tekstowego pozostaje znakiem tekstowym i nie uruchamia akcji CAD.

Narzędzia tworzenia zachowują wcześniejsze selection, ale ukrywają/dezaktywują grips podczas działania; nowa geometria nie jest automatycznie zaznaczana. `Finish Sketch` kończy edycję. Support Sketchu jest obecnie ograniczony do trzech płaszczyzn Origin.

Copy połączone z Rotate/Scale/Mirror, ordinary-Select RMB context, clipboard/cross-Sketch Copy, Ortho/Polar, snapping/tracking/inference, coordinate/Dynamic Input, numeryczne Rotate/Scale, constraints/solver, authored dimensions, płaszczyzny Construction/Datum i płaskie ściany modelu pozostają późniejszymi etapami.

<!-- section-id: product.parts.measure -->
## Inspect — Measure

Podczas edycji Sketchu użyj **Inspect → Measure**, aby sprawdzić jedną linię, okrąg albo łuk bez zmiany modelu.

Jeżeli w chwili uruchomienia Measure dokładnie jedna encja Sketchu jest już zaznaczona, od razu staje się początkowym celem pomiaru. Przy pustym zaznaczeniu kliknij encję. Przy zaznaczeniu wielu encji Measure celowo nie zgaduje, którą masz na myśli — kliknij obiekt, który chcesz sprawdzić. Cel Measure jest oddzielony od zwykłego selection, więc pomiar innej encji nie zastępuje istniejącego zaznaczenia.

Operations pokazuje:

- **Line:** Length, Delta U, Delta V oraz skierowany Angle +U od Start do End;
- **Circle:** Radius, Diameter, Circumference i Area;
- **Arc:** Radius, Start Angle, End Angle, Signed Sweep i Arc Length.

Można mierzyć zarówno geometrię Regular, jak i Construction. Pomiar korzysta z semantycznej geometrii Sketchu, a nie z widocznej tessellacji Viewera.

Measure możesz także uruchomić wpisując `MEASURE` w Command Line. Narzędzie pozostaje aktywne, więc można kolejno klikać różne encje. Kliknięcie pustego tła czyści bieżący wynik pomiaru, ale nie zmienia normalnego zaznaczenia. **Esc** kończy Measure i wraca do Select.

Measure jest read-only: samo uruchomienie ani zmiana celu nie powodują dirty Dokumentu, nie zmieniają revision, nie zużywają identyfikatorów i nie tworzą kroku Undo. Measure nie wchodzi do Repeat Last Command.

Obecne wartości liniowe i pola są pokazywane w skali współrzędnych Sketchu bez etykiety jednostki fizycznej. Kąty są prezentowane w stopniach. Jawne jednostki Dokumentu i konwersja należą do późniejszego etapu Precision/Units.

R8A mierzy tylko całe encje. Pomiar relacyjny point-to-point/perpendicular/line-to-line, trwałe referencje do sub-elementów, viewportowe overlaye wymiarowe oraz authored dimensions/constraints nie należą do bieżącego narzędzia.

<!-- section-id: product.parts.profiles -->
## Construction i Profile

Każda linia, okrąg i łuk w Sketchu może mieć rolę **Regular** albo **Construction**. Zaznacz geometrię w zwykłym Select i użyj `Regular` lub `Construction` w Operations. Construction pozostaje zapisaną geometrią pomocniczą, ale nie zamyka ani nie dzieli regionów używanych przez Profile. W viewporcie geometria Construction jest pokazywana linią przerywaną; jest to wyłącznie wskazówka prezentacyjna, a trwałą prawdą pozostaje authored rola encji.

Narzędzie **Profile** działa w aktywnym Sketchu. Przesuwanie kursora nad zamkniętą geometrią pokazuje półprzezroczysty wynik regionu; kliknięcie przyjmuje kandydata do bieżącego draftu i Status przypomina, że dopiero **Finish Profile** wykonuje trwały commit. O tym, co jest regionem, decyduje dokładna semantyka Line/Circle/Arc, nie tessellation Viewera.

Operations udostępnia **Add Area**, **Subtract Area**, **Detect Islands**, **Highlight on Hover**, **Show Region Boundaries**, **Show Problems**, **Find All Regions**, **Finish Profile** i **Cancel**. Find All Regions jest tylko diagnostyczne i nie tworzy Profile automatycznie. Podczas Subtract wynikowy draft pozostaje błękitny, a aktualnie wskazany region do odjęcia dostaje osobne czerwono-pomarańczowe podświetlenie.

Otwarty łańcuch pozostaje otwarty: SimpleSolid nie domyka małej szczeliny ukrytą tolerancją ani nie naprawia jej automatycznie. Kliknięcie w miejscu, gdzie nie istnieje bounded region, zgłasza diagnostykę otwartej granicy, jeżeli analiza ją wykryła. Bez snapping/OSNAP dwa punkty, które tylko wyglądają na pokrywające się na ekranie, nie są automatycznie zrównywane. Rozłączony wynik Add, Subtract rozcinający materiał albo niejednoznaczna topologia są odrzucane bez zmiany Dokumentu.

Finish tworzy Part-owned Profile ze stabilnym `ProfileId`. Profile przechowuje semantyczne odwołania do geometrii źródłowego Sketchu zamiast kopii widocznego wypełnienia. Edycja źródła może pozostawić Profile **Valid**, uczynić go **Invalid**, a późniejsza naprawa może przywrócić Valid bez zmiany ProfileId. SimpleSolid nie przepina automatycznie Invalid Profile do podobnej ani najbliższej nowej geometrii.

Profile jest dzieckiem źródłowego Sketchu w Document Tree. Properties pokazuje Name, ProfileId, Source Sketch, Status, diagnostykę, Area, Perimeter, Holes i Visibility. Area/Perimeter/Holes są pochodne i dla Invalid stają się niedostępne zamiast pokazywać stare wartości. Name i Visibility są authored.

Po **Finish Sketch** widoczny i Valid Profile pozostaje płaskim zaznaczalnym obiektem Part na płaszczyźnie źródłowego Sketchu. Widoczność Profile jest niezależna od widoczności geometrii Sketchu. Zaznaczenie wybiera Profile, nie jego krzywe źródłowe. **Delete Profile** usuwa tylko Profile i pozostawia geometrię Sketchu.

Aby zmienić region istniejącego Profile, zaznacz dokładnie jeden Profile i użyj **Edit Profile** albo wpisz `EDITPROFILE`. Edycja zachowuje ProfileId. `PROFILE` rozpoczyna nowy draft.

Command Line i Operations sterują tym samym stanem Profile. Komendy kontekstowe to `ADD`, `SUBTRACT`, `FIND`, `FINISH`, `CANCEL` oraz `ISLANDS ON|OFF`, `BOUNDARIES ON|OFF`, `PROBLEMS ON|OFF`.

Profile jest semantyką 2D/Part. **Nie wykonuje Extrude i nie tworzy bryły.** Modelowane operacje bryłowe wymagają osobnej funkcji Part.

<!-- section-id: product.parts.navigation -->
## Nawigacja 3D i Navigation Cube

Workbench Parta udostępnia Pan środkowym przyciskiem myszy, Orbit przez Shift + środkowy przycisk myszy, Zoom kółkiem, Fit oraz projekcję Orthographic/Perspective.

W prawym górnym rogu viewportu 3D znajduje się przestrzenny Navigation Cube, który podąża za aktualną orientacją kamery. Kliknij opisaną ścianę, aby przejść do Front, Back, Left, Right, Top albo Bottom; krawędź wybiera odpowiadający widok dwuosiowy, a narożnik — odpowiadający widok izometryczny. Przejścia orientacji są animowane.

Gdy widok jest wyrównany do ściany, kontrolki wokół Cube umożliwiają dokładne przejścia o 90° do sąsiednich widoków oraz roll o dokładnie 90° zgodnie lub przeciwnie do ruchu wskazówek zegara. Home ustawia Top-Front-Right ISO i wykonuje Fit All.

Projekcja jest niezależna od orientacji Cube. Kontrolka `ORTHO/PERSP` obok Cube przełącza wyłącznie Orthographic/Perspective. Jeśli model jest w Perspective, kliknięcie ściany, krawędzi, narożnika albo Home zachowuje Perspective; analogicznie akcje te zachowują Orthographic, gdy jest ono aktywne.

Zmiany nawigacji i kamery są tymczasowym stanem widoku. Nie dirty'ują Parta, nie wymagają Save i nie tworzą wpisów CAD Undo.

<!-- section-id: product.parts.presentation-recovery -->
## Awaria prezentacji 3D i ponowienie

Jeżeli operacja CAD zostanie poprawnie zatwierdzona w modelu, ale Viewer 3D nie zdoła odświeżyć prezentacji, zmiana modelu pozostaje zatwierdzona. Revision, stan wymagający Save oraz historia Undo/Redo nie są cofane z powodu błędu wyświetlania.

Workbench pokazuje wtedy komunikat o nieudanym odświeżeniu prezentacji 3D. Podczas tego komunikatu widocznej sceny nie należy traktować jako zsynchronizowanej z bieżącym modelem.

Następne normalne pełne odświeżenie Viewera automatycznie ponawia budowę prezentacji z aktualnego authored state. Po poprawnym odświeżeniu stan błędu i komunikat znikają. Recovery nie odtwarza modelu z obiektów Viewera i nie tworzy dodatkowego wpisu Undo.

<!-- section-id: product.parts.save-close -->
## Save i zamykanie

`Save` zapisuje bieżący authored state Parta, w tym widoczność Origin oraz utworzone Sketche z trwałą tożsamością/supportem/placementem, geometrią Line/Circle/Arc i stabilnymi EntityId, do pliku `.ss2part`.

Zwykły Save jest warunkowy względem dokładnej wersji pliku natywnego, którą ta sesja otworzyła albo ostatnio poprawnie zapisała. Jeżeli target został usunięty, podmieniony, zmieniony na dysku, zawiera już inny DocumentId albo jest aktualnie chroniony przez inny współpracujący Save SS2, SimpleSolid zgłasza konflikt Save zamiast po cichu nadpisać plik. Part w pamięci pozostaje otwarty i zachowuje lokalne zmiany/historię Undo.

Po poprawnym Save sesja przyjmuje nową opublikowaną wersję pliku, więc kolejny niezmieniony Save działa normalnie. Ścisła gwarancja braku lost-update dotyczy współpracujących instancji SS2; SimpleSolid nie deklaruje ogólnego atomowego compare-and-swap wobec każdego obcego programu zapisującego plik.

Przy zamykaniu Parta z niezapisanymi zmianami program wymaga decyzji `Save`, `Discard` albo `Cancel`.

Przy zamykaniu całego Projektu lub aplikacji, gdy jakikolwiek Part jest dirty, dostępne są `Save All`, `Discard` i `Cancel`.

Jeżeli zapis się nie powiedzie albo zgłosi konflikt, Dokument/Projekt pozostaje otwarty. Zamknięcie ostatniego otwartego Parta pozostawia Projekt otwarty i pokazuje Project Workspace Dashboard. Samo użycie `Workspace` nigdy nie zamyka Parta.

<!-- section-id: product.parts.restart -->
## Restart i ponowne otwarcie

Po restarcie aplikacji otwórz ten sam Projekt.

SimpleSolid ponownie skanuje Workspace. Zapisany Part zostaje odnaleziony z tym samym DocumentId, zapisanymi właściwościami, zapisaną widocznością Origin oraz zapisanymi Sketches.

Historia Undo/Redo, aktywne zaznaczenie, aktywny tryb edycji Sketchu i stan kamery nie są zapisywane do pliku i po ponownym otwarciu zaczynają się od nowa.

<!-- section-id: product.parts.conflicts -->
## Konflikty Dokumentu i Save

Jeżeli dwa pliki `.ss2part` w jednym Workspace zawierają ten sam DocumentId, `Open…` pokazuje wpis konfliktu tożsamości oraz informacje o wykrytych lokalizacjach. Takiego wpisu nie można otworzyć przez DocumentId, dopóki konflikt nie zostanie usunięty. SimpleSolid nie wybiera arbitralnie jednej kopii i nie nadaje automatycznie nowego ID.

Uszkodzony albo nieobsługiwany natywny Part jest pokazywany jako niepoprawny wpis zamiast być traktowany jako poprawny Dokument.

Konflikt Save jest czymś innym niż konflikt discovery w Workspace: oznacza, że otwarta sesja nie ma już uprawnienia do zastąpienia wersji pliku znajdującej się obecnie pod jej ścieżką. SimpleSolid pozostawia dokument w pamięci otwarty i nie wykonuje po cichu force-overwrite ani nie odtwarza usuniętego targetu. Save As / Force Overwrite nie są obecnie dostępne jako ścieżka recovery.

<!-- section-id: product.parts.current-limits -->
## Aktualne ograniczenia Parta

Obecny Part zapewnia tożsamość/właściwości Dokumentu, wbudowany Origin, trwałą widoczność referencji, fundament Workbench/Viewer 3D oraz trwałe Sketche na płaszczyznach Origin z authored geometrią Line/Circle/Arc.

Bieżący Sketch UI zapewnia Select oraz grupy Create, Modify i Inspect z Line/Circle/Arc, Move/Copy/Rotate/Scale/Mirror oraz read-only Measure, addytywne point/Window/Crossing selection, semantyczne primary i hover, kwadratowe grips kodujące stan, selection-first i command-first common transforms, normalne COPY z repeated placement i świeżymi EntityId, Grip Copy przez aktywne `C` dla owner-only Reshape albo frozen-selection Move, Repeat Last Command przez Enter/Space w zwykłym Select, Space CycleEditMode między owner-only Reshape i frozen-selection Move na wspieranych non-center grips, Move-only center grips, atomowy mieszany Delete, Undo/Redo, Operations oraz keyboard-first Command Line.

Direct Distance jest dostępny dla obsługiwanych etapów PointRequest opisanych wyżej, również dla placementów gripu po włączeniu Grip Copy. Profile nie jest operacją bryłową. Nadal brakuje Rotate/Scale/Mirror+Copy, ordinary-Select RMB context, clipboard/cross-Sketch Copy, numerycznego kąta Rotate i współczynnika Scale, współrzędnych absolutnych/względnych/polarnych, unit expressions i Dynamic Input, snapping/inference, constraintów/solvera, authored dimensions, supportu Sketchu na Datum/płaskiej ścianie modelu, Bodies, Features/Extrude, modelowanej geometrii bryłowej, Material oraz narzędzi Assembly/Drawing.

Bardzo wczesne testowe pliki `.ss2part` sprzed obecnego natywnego formatu nie są obsługiwanym formatem danych i nie są automatycznie migrowane.

