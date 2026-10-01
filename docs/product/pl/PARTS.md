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
- Engineering revision;
- jednostkę długości wejścia/wyświetlania: mm, cm, m, in albo ft.

Zmiana jednostki Parta zmienia interpretację bezjednostkowych wartości Length i sposób prezentacji wartości fizycznych. Istniejąca geometria nie jest przeskalowywana. Jednostka jest authored state Dokumentu, uczestniczy w Undo/Redo, dirty'uje Part przy zmianie i staje się trwała po `Save`. Starsze natywne Party otwierają się jako mm.

`Apply Properties` wprowadza pozostałe zmiany do otwartej sesji Dokumentu. Nie są trwałe na dysku, dopóki nie wykonasz `Save`.

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
  Rectangle

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

W aktywnym Sketchu obsługiwane są słowa `SELECT`, `LINE`, `CIRCLE`, `ARC`, `RECTANGLE`, `MEASURE`, `MOVE`, `COPY`, `ROTATE`, `SCALE` i `MIRROR`. Command Line jest kontekstowy: jeżeli aktywny etap oczekuje punktu, bieżący semantic PointRequest ma pierwszeństwo przed uruchomieniem nowej komendy. W etapach obsługujących Direct Distance samodzielna wartość liczbowa jest interpretowana jako odległość. Podczas aktywnej manipulacji gripem `C` + Enter jest lokalnym słowem kluczowym włączającym Grip Copy; poza aktywnym gripem `C` nie staje się globalnym aliasem komendy. Enter konsumuje wpisany token także wtedy, gdy jest błędny: model oraz aktywne narzędzie/etap pozostają bez zmian, edytowane pole jest czyszczone, a diagnostic informuje o błędzie bez zmuszania użytkownika do ręcznego kasowania odrzuconego tekstu.

Prawdziwe pola tekstowe — np. edycja właściwości — zachowują własny focus i wpisywanie do nich nie zasila Command Line. Skróty aplikacji z Ctrl/Alt/Meta również nie są zamieniane na tekst CAD. Menu, popup, okno modalne, inne okno aplikacji oraz drugi widoczny Workspace SS2 zachowują własność klawiatury i nie zasilają Command Line działającego w tle.

Częściowo wpisany token CAD należy do semantycznego requestu, w którym rozpoczęto wpisywanie. Zmiana requestu/narzędzia/Sketcha/Dokumentu czyści taki token, zanim mógłby wykonać się w nowym kontekście. Zwykły ruch kursora zmieniający kierunek w ramach tego samego PointRequest nie czyści wpisu. Zmiana Dokumentu lub przejście do Workspace również usuwa częściowy input, więc nie może on trafić do ukrytego Dokumentu.

Przy focusie viewportu i niepustym buforze Command Line klawisz Delete należy do aktywnego inputu i nie usuwa zaznaczonej geometrii. Ostatni znak usuwa Backspace. Gdy bufor jest pusty, zwykłe Delete Selection działa jak wcześniej. Jeżeli focus ma sam Command Line albo inne prawdziwe pole tekstowe, Delete pozostaje normalną edycją tekstu.

Dolny Command Line pozostaje zawsze jednym wierszem. Obszar diagnostyczny po prawej stronie ma stale zarezerwowane miejsce, więc pojawienie się błędu nie zmienia szerokości pola wejściowego i nie powoduje przeskoku Viewera. Długi komunikat jest wizualnie skracany w tym wierszu, a pełna treść jest dostępna jako tooltip.

### Powtórzenie ostatniej komendy

Podczas jednego aktywnego Sketch Edit SimpleSolid pamięta ostatnią poprawnie uruchomioną komendę z zestawu `Line / Circle / Arc / Rectangle / Move / Copy / Rotate / Scale / Mirror`.

Gdy jesteś w zwykłym `Select` i focus ma viewport, **Enter** albo **Space** uruchamia tę komendę ponownie.

Powtórzenie zaczyna świeżą komendę. Nie odtwarza poprzednich Base/Reference/axis/placement points, preview ani wcześniejszego selection snapshotu. Używa aktualnego selection zgodnie z normalną gramatyką narzędzia — np. MOVE z aktualnym selection działa selection-first, a bez selection przechodzi do Select objects. Powtórzone COPY ponownie prosi o Base Point i nie używa wcześniejszych placementów.

`Select`, zwykłe zaznaczanie, Delete, grip manipulation, Undo/Redo i Esc nie zastępują zapamiętanej komendy. Pamięć jest czyszczona po zakończeniu Sketch Edit i nie przechodzi do innego Sketchu, Dokumentu ani po ponownym otwarciu pliku.

Istniejące znaczenie klawiszy ma pierwszeństwo: Enter nadal zatwierdza aktywną manipulację/transformację, Enter/Space nadal kończy Select objects w aktywnej transformacji, a Space przy focusie pola tekstowego pozostaje zwykłą spacją.


### Rectangle i rola tworzenia

**Rectangle** używa dwóch kliknięć: **First Corner**, a następnie **Opposite Corner**. Prostokąt jest wyrównany do osi U/V aktywnego Sketchu. Oba wymiary U i V muszą być niezerowe. Poprawne zatwierdzenie tworzy cztery zwykłe encje Line w jednej atomowej operacji i jednym kroku Undo, po czym Rectangle pozostaje aktywne dla kolejnego prostokąta.

Rectangle nie jest trwałym specjalnym prymitywem. Po utworzeniu jego cztery krawędzie są niezależnymi zwykłymi Lines z niezależnymi EntityId. Edycja jednej krawędzi może otworzyć albo zdeformować prostokąt; SimpleSolid nie dodaje ukrytych constraintów Coincident, Horizontal/Vertical ani zachowania prostokątności.

Prawy panel **Operations** pokazuje checkowalną opcję **Construction**, gdy aktywne jest Line, Circle, Arc albo Rectangle. Ustawia ona rolę geometrii tworzonej **od tego momentu**. Jej przełączenie nie modyfikuje już zaznaczonej geometrii i nie tworzy kroku Undo. Dla Rectangle ten sam panel Operations pokazuje dodatkowo **Draw Diagonals**; opcja ta jest ukryta dla pozostałych narzędzi tworzenia. Do zmiany istniejącej geometrii służą osobne kontrolki Regular/Construction pokazywane przez Operations podczas zwykłego Select. Creation Role zaczyna każdą nową sesję Sketch Edit jako Regular i nie jest zapisywana.

**Draw Diagonals** jest runtime-only opcją Rectangle i w każdej nowej sesji Sketch Edit zaczyna jako OFF. Gdy jest ON, ten sam commit Rectangle dodaje obie przekątne jako zwykłe linie **Construction**. Regular Rectangle tworzy więc cztery Regular krawędzie i dwie Construction przekątne; Construction Rectangle tworzy sześć Construction Lines. Punkt przecięcia nie tworzy center point, RectangleId, grupy ani constraintu.

Preview Rectangle używa tych samych reguł krawędzi/przekątnych. Preview Construction jest kreskowane wyłącznie jako wskazówka wizualna. Finalny commit jest związany z DocumentRevision zapamiętanym przy First Corner; jeżeli Dokument zmieni się przed Opposite Corner, oczekujący prostokąt zostaje odrzucony fail-closed zamiast zostać po cichu przeliczony na nowy stan.

Na etapie Opposite Corner precision input przyjmuje także `Width;Height`. Width i Height są dodatnimi wartościami Length, a bieżący kwadrant kursora określa orientację lewo/prawo i góra/dół. To gramatyka specyficzna dla Rectangle i nie jest reinterpretowana jako ogólne `U;V`.

Tworzenie Line/Circle/Arc i zwykłe selection zachowują dotychczasową gramatykę. Zaznaczone edytowalne entities pokazują grips kodujące stan: pusty kwadrat w idle, pusty cyan na hover i pełny żółty dla aktywnego/captured gripa.

### Tryby edycji gripów

Center grips przechodzą cyklem:

```text
Move → Rotate → Scale → Mirror → Move
```

Obsługiwane non-center grips Line/Circle/Arc zaczynają w owner-only **Reshape** i przechodzą cyklem:

```text
Reshape → Move → Rotate → Scale → Mirror → Reshape
```

Move/Rotate/Scale/Mirror działają na całym selection zamrożonym przy rozpoczęciu manipulacji; Reshape edytuje tylko właściciela aktywnego gripa. Każdy preview jest liczony od geometrii ze startu interakcji. Rotate przechwytuje świeży kierunek odniesienia, Scale świeży promień odniesienia, a Mirror używa aktywnego gripa jako pierwszego punktu osi.

Możesz wpisać dokładną wartość: Angle dla Rotate, dodatni Factor dla Scale i Axis Angle dla Mirror. Po zaakceptowaniu dokładna wartość ma pierwszeństwo przed dalszym ruchem kursora.

Space przełącza tryb i nie tworzy authored change. LMB albo Enter zatwierdza, Esc anuluje niezatwierdzoną manipulację. Grip Copy działa tylko w Reshape i Move i wyłącza się po przejściu do innego trybu.

### Grip Copy

Podczas aktywnej manipulacji gripem wpisz `C` i naciśnij Enter, aby włączyć **Grip Copy**. Copy jest modyfikatorem commit dostępnym tylko w Reshape i Move.

W **Reshape + Copy** placement tworzy fresh-ID kopię wyłącznie właściciela aktywnego gripa. W **Move + Copy** placement kopiuje cały zamrożony selection. Oryginały pozostają bez zmian i zaznaczone.

Placement może użyć LMB albo tego samego precision PointRequest. Po udanej kopii source/pivot ze startu interakcji pozostają dla kolejnego placementu, ale request-local pointer candidates i numeric locks są czyszczone. Placement bez zmiany jest no-op. Space przełącza tryb i wyłącza Copy; po powrocie do Reshape albo Move trzeba ponownie wysłać `C`. Esc kończy transient session i zachowuje committed copies; Undo działa potem normalnie na ostatnim zatwierdzonym kroku historii.

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

### Zatwierdzanie i precision input

Preview Move/Copy/Rotate/Scale/Mirror jest runtime-only. Move/Rotate/Scale/Mirror zachowują EntityId, a COPY tworzy świeże EntityId wyłącznie dla zaakceptowanych placementów. LMB na końcowym etapie albo Enter zatwierdza bieżący poprawny request.

Wspólna gramatyka precision input przyjmuje:

```text
Punkt absolutny        U;V
Względny kartezjański  @dU;dV
Względny polarny       @Distance<Angle
```

Bezjednostkowy Length używa bieżącej jednostki Parta. Jawny suffix `mm`, `cm`, `m`, `in` albo `ft` ją nadpisuje. Kropka dziesiętna jest zawsze akceptowana, a bieżący przecinek dziesiętny jest obsługiwany tam, gdzie ma zastosowanie. Ograniczona arytmetyka obsługuje `+`, `-`, `*`, `/` i nawiasy z walidacją wymiarów. Bare Angle oznacza stopnie; `deg` i `rad` są jawnymi suffixami kąta. Notacja stopy/cale przez cudzysłowy/apostrofy jest odrzucana.

Line używa exact point input dla pierwszego i kolejnych punktów. Circle ma sekwencję **Center → Size**; Size domyślnie oznacza **Diameter** w każdej sesji Sketch Edit, a lokalne `D`/`R` przełączają Diameter/Radius. Arc ma sekwencję **Start → End → Arc Point / Radius**. Radius musi być co najmniej połową chordu; strona kursora wybiera bulge, Radius tworzy minor/semicircle, a jawny trzeci Arc Point może utworzyć major arc. Rectangle przyjmuje `Width;Height`.

Rotate przyjmuje exact signed **Angle**, Scale exact positive **Factor**. Grip Rotate/Scale/Mirror wystawiają **Angle**, **Factor** i **Axis Angle**.

### Polar i Dynamic Input

**Polar** jest attraction aid, nie snappingiem. Domyślnie startuje jako ON z 45° (`360/8`), Reference **Absolute** i bez Additional Angles. Operations konfiguruje Step, Absolute/Relative i opcjonalne pojedyncze Additional Angles. Relative wymaga poprawnej referencji semantycznej i nie przechodzi po cichu na Absolute. **F10** przełącza Polar. Ustawienia przeżywają wyjście/wejście do Sketchy w bieżącej sesji aplikacji i resetują się po restarcie.

**Dynamic Input (DYN)** domyślnie jest OFF i przełącza się przez **F12** albo Operations. Gdy jest ON, overlay przy kursorze pokazuje pola tego samego requestu i używa tego samego live tokenu co Command Line. Dla based point kolejność to **Distance → Angle → dU → dV**; punkt bez base używa **U → V**; Rectangle używa **Width → Height**; Circle pokazuje Diameter/Radius; Rotate/Scale/grip Mirror pokazują pojedyncze wartości semantyczne.

**Tab** blokuje poprawną wartość bieżącego pola i przechodzi dalej; pusty Tab tylko przechodzi dalej; **Shift+Tab** cofa. **Enter** zatwierdza request z locków i pozostałych pointer/Polar values. **Esc** najpierw czyści live text, potem request-local locks, a następnie zwykły stage narzędzia. Locked Angle jest absolutny od Sketch +U i ma pierwszeństwo przed Polar; locked Distance może łączyć się z captured Polar direction. Konfliktujące rodziny locków są odrzucane fail-closed.

Polar/DYN nie tworzą Document revision, dirty ani Undo i nie są zapisywane do Parta.

Space wpisany przy focusie pola tekstowego pozostaje znakiem tekstowym i nie uruchamia akcji CAD.

Narzędzia tworzenia zachowują wcześniejsze selection, ale ukrywają/dezaktywują grips podczas działania; nowa geometria nie jest automatycznie zaznaczana. `Finish Sketch` kończy edycję. Support Sketchu jest obecnie ograniczony do trzech płaszczyzn Origin.

Copy połączone z Rotate/Scale/Mirror, ordinary-Select RMB context, clipboard/cross-Sketch Copy, Grid Snap, constraints/solver, authored dimensions, płaszczyzny Datum i płaskie ściany modelu pozostają późniejszymi etapami. Nie ma osobnego trybu Ortho; do przyciągania wyłącznie ortogonalnego użyj Polar ze Step=90°.

<!-- section-id: product.parts.structural-editing -->
## Edycja strukturalna — Trim i Extend

Podczas Sketch Edit grupa **Modify** udostępnia **Trim**, **Extend** i **Extend Both**. Te same narzędzia można uruchomić z Command Line przez `TRIM`, `EXTEND` albo `EXTEND BOTH`.

### Trim

Domyślny workflow jest tool-first: uruchom **Trim**, kliknij jedną lub więcej skończonych encji Line/Arc/Circle, aby zebrać boundaries cięcia, a następnie naciśnij **Enter** albo **PPM**, aby zatwierdzić ten zestaw. Po zatwierdzeniu klikaj fragmenty targetów do usunięcia; Trim pozostaje aktywny dla kolejnych targetów.

Preselection pozostaje skrótem: jeżeli poprawne boundaries są już zaznaczone przed uruchomieniem **Trim**, zostają zaakceptowane od razu i narzędzie przechodzi bezpośrednio do etapu wyboru fragmentów targetu. Geometria Regular i Construction może być zarówno celem, jak i boundary.

Trim celowo dopuszcza tylko jeden wynik:

- można usunąć końcowy fragment **Line**, pozostawiając jedną Line;
- można usunąć końcowy fragment **Arc**, pozostawiając jeden Arc;
- **Circle** można przyciąć, gdy wybrane skończone boundaries dają co najmniej dwa różne exact cut locations. Kliknięty lokalny fragment Circle jest usuwany, a połączone dopełnienie staje się jednym Arc.

Usunięcie środkowego fragmentu Line/Arc, które pozostawiłoby dwa elementy, jest odrzucane. Split i Join nie należą do tego zestawu narzędzi. Tangent Circle Trim z tylko jednym przecięciem, coincident/overlap ambiguity albo wynik, którego nie da się reprezentować jako jednej obsługiwanej prymitywy, jest odrzucany bez zmiany Sketchu.

Circle→Arc jest rzeczywistą zmianą identity: stary Circle zostaje wycofany, a nowy Arc dostaje świeże identity. Istniejące Profile **nie są** automatycznie przepinane do nowego Arc. Profile zależny od wycofanego Circle może stać się Invalid, dopóki Undo nie przywróci poprzedniej geometrii.

### Extend

Domyślny workflow jest tool-first: uruchom **Extend**, kliknij jedną lub więcej skończonych encji Line/Arc/Circle, aby zebrać finite boundaries, a następnie naciśnij **Enter** albo **PPM**, aby je zatwierdzić. Po zatwierdzeniu klikaj w pobliżu końca każdej docelowej Line lub Arc, która ma być przedłużona; Extend pozostaje aktywny dla kolejnych targetów.

Preselection pozostaje skrótem: poprawne boundaries zaznaczone przed uruchomieniem **Extend** są akceptowane od razu, więc narzędzie przechodzi bezpośrednio do etapu wyboru końca targetu. Wybrany koniec jest przedłużany do najbliższego poprawnego exact intersection w tym kierunku.

Standardowy Extend korzysta wyłącznie ze **skończonej authored boundary geometry**. Nie używa niewidocznego wirtualnego przedłużenia boundary. Circle nie jest celem Extend.

### Extend Both

**Extend Both** działa wyłącznie dla dokładnie dwóch Lines. Uruchom narzędzie, wybierz pierwszą Line, potem drugą. Gdy obie skończone Lines wymagają przedłużenia, ich nieskończone proste nośne służą do wyznaczenia jednego wspólnego virtual intersection i obie Lines są przedłużane do niego atomowo.

Lines równoległe lub coincident, przypadki gdy intersection już leży na którejkolwiek skończonej Line oraz przypadki wymagające przedłużenia tylko jednej Line są odrzucane. Gdy tylko jeden cel ma dojść do istniejącej skończonej boundary, użyj zwykłego Extend.

### Preview, historia i zachowanie przy błędzie

Przed zatwierdzeniem viewport pokazuje preview proponowanego usuwanego fragmentu albo dodawanego przedłużenia. Preview jest runtime-only i nie jest wybieralną geometrią.

Każdy zaakceptowany Trim albo Extend tworzy jeden krok Undo. Extend Both zmienia obie Lines atomowo w jednym kroku Undo. **Esc** anuluje tylko transient tool state; wcześniej zatwierdzone zmiany pozostają zwykłą historią. Undo/Redo zachowuje zaakceptowane identity encji, a Save/Close/Reopen zachowuje wynikową geometrię i identity.

Prawdą strukturalną jest exact geometria Sketchu. Object Snap, Tracking, Polar i Dynamic Input mogą pomagać w akwizycji inputu, ale nie tworzą constraints, nie zgrzewają małych szczelin i nie zmieniają near miss w intersection. Przypadki ambiguous, unsupported, stale albo non-finite są odrzucane fail-closed bez częściowej edycji.

<!-- section-id: product.parts.object-snap -->
## Object Snap, Tracking i temporary overrides

Podczas Sketch Edit panel **Operations** udostępnia **Object Snap**, **Modes**, **Temporary** i **Tracking** obok Polar i Dynamic Input.

Object Snap jest exact snappingiem semantycznym. Nie przesuwa istniejącej geometrii i nigdy nie tworzy automatycznie authored constraintu Coincident, Tangent, Perpendicular ani innego. Bieżący snap pokazuje screen-space marker/label.

Tryby trwałe:

- **END** — endpointy Line/Arc;
- **MID** — midpoint Line i midpoint Arc po authored sweep;
- **CEN** — center Circle/Arc;
- **QUAD** — quadranty Circle ±U/±V i tylko quadranty Arc leżące na authored sweep;
- **INT** — exact skończone dyskretne przecięcia pobliskiej geometrii Line/Circle/Arc;
- **ORG** — intrinsic Sketch Origin `(0,0)`;
- **PER** — request-relative wynik prostopadły, gdy PointRequest ma wymagany base;
- **TAN** — request-relative tangent do Circle/Arc oraz deferred/common-tangent flow dla Line;
- **NEA** — ciągła nearest projection na skończoną krzywą źródłową; jest fallbackiem poniżej bardziej szczegółowych snapów;
- **EXT** — jawny dodatni promień przedłużenia Line poza acquired endpoint; nie działa na skończonym segmencie Line.

Domyślnie **END/MID/CEN/QUAD/INT/ORG są ON**, a **PER/TAN/NEA/EXT są OFF**. Master Object Snap, trwały zestaw trybów i master Tracking są preferencjami application/user i przeżywają restart aplikacji. Nie są zapisywane do Parta i nie tworzą Document dirty ani kroku Undo.

Selektor **Temporary** stosuje dokładnie jeden override do następnego requestu punktu: `END`, `MID`, `CEN`, `QUAD`, `INT`, `PER`, `TAN`, `NEA`, `ORG`, `EXT` albo `NONE`. Czyści się po jednym zaakceptowanym punkcie, Esc, zastąpieniu requestu/narzędzia albo wyjściu ze Sketchu bez zmiany trwałych trybów. `NONE` dla tego punktu wyłącza object-derived OSNAP, OTRACK oraz pomoc EXT/TAN/PER, ale jawny input liczbowy, numeric locks, ogólna bazowa inferencja U/V, Polar i raw pointer pozostają dostępne. Niedostępna żądana rodzina jest odrzucana fail-closed zamiast przejścia na inny snap.

Object Snap używa tego samego finalnego point-resolution path co Dynamic Input i Polar. Pełny jawny input liczbowy ma najwyższy priorytet. Numeric locks pozostają authoritative; snap może wspomóc częściowo zablokowany request tylko wtedy, gdy wynik po lockach nadal jest dokładnie reklamowanym punktem snap. Zgodny OSNAP ma następnie pierwszeństwo przed Tracking/inference, te przed Polar, a Polar przed raw pointer.

**Tracking** (OTRACK) domyślnie jest OFF. Gdy jest włączony, zatrzymaj kursor przez około **0,4 s** nad kwalifikującym się exact snapem `END/MID/CEN/QUAD/INT/ORG`, aby acquired tracking anchor. Zachowywane są maksymalnie dwa anchory; trzeci nie zastępuje po cichu wcześniejszego. Odsuń kursor, a następnie ponownie wykonaj deliberate dwell nad tym samym acquired snapem, aby usunąć anchor. Zaakceptowany punkt, Esc, zastąpienie requestu/narzędzia i wyjście ze Sketchu czyszczą request-local anchory.

Każdy anchor może wystawić exact guide'y Sketch U/V. Gdy Polar jest ON, jego zaakceptowane kierunki mogą także uczestniczyć. Guide intersection z dwóch anchorów ma pierwszeństwo przed pojedynczą zgodną guide projection i Polar. Markery Tracking i guide'y są runtime-only, non-selectable i non-authored. Tracking OFF albo `NONE` usypia object-derived guides bez zmiany zapisanych preferencji.

Obecnie Object Snap i Tracking nie mają osobnych bindingów F3/F11; używaj widocznych kontrolek Operations. Polar pozostaje pod **F10**, a Dynamic Input pod **F12**.
<!-- section-id: product.parts.measure -->
## Inspect — Measure

Podczas edycji Sketchu użyj **Inspect → Measure**, aby sprawdzać geometrię bez zmiany modelu. Measure ma dwa tryby read-only: domyślną inspekcję całej encji oraz relacyjny **Between**.

### Quick Measure

Jeżeli przy uruchomieniu Measure zaznaczona jest dokładnie jedna Line, Circle albo Arc, staje się początkowym celem. Przy pustym selection kliknij encję. Przy wielu zaznaczonych encjach Measure nie zgaduje. Cel Measure jest oddzielony od zwykłego selection.

Operations pokazuje dla Line Length/Delta U/Delta V/Angle +U, dla Circle Radius/Diameter/Circumference/Area, a dla Arc Radius/Start Angle/End Angle/Signed Sweep/Arc Length. Można mierzyć geometrię Regular i Construction. Measure można też uruchomić przez `MEASURE` w Command Line.

### Measure Between

Gdy Measure jest aktywne, kliknij **Between** w Operations albo wpisz `BETWEEN`. Between prosi o **Target A** i **Target B**, zachowując zwykłe selection Sketchu.

Semantyczne markery punktów są normalnie ukryte. Zbliż kursor do obsługiwanego punktu, a jego runtime marker się ujawni. Po odsunięciu kursora niezaakceptowany marker znika. Sama bliskość nigdy nie wybiera celu, nie snapuje i nie przesuwa punktu CAD: **proximity tylko ujawnia; nie przechwytuje celu**. Punkt wybierasz klikając widoczny marker. Zaakceptowany marker pozostaje przypięty do czasu zastąpienia lub wyczyszczenia relacji.

Dostępne role punktów:

- **Line:** Start, Midpoint, End;
- **Circle:** Center i cztery Quadrants ±U/±V;
- **Arc:** Center, Start, End, Midpoint wzdłuż authored sweep.

Można też wskazać **Line body**. Cały Circle ani Arc nie jest celem relacyjnym; trzeba wybrać jego jawny marker punktu.

Between obsługuje dokładnie:

- **Point ↔ Point:** Distance, Delta U, Delta V i skierowany Angle +U od A do B;
- **Point ↔ Line:** odległość prostopadłą do **nieskończonej prostej nośnej** Line;
- **Line ↔ Line:** mniejszy nieskierowany kąt od 0° do 90°.

Dla Point↔Line stopa prostopadłej nie jest ograniczana do skończonego odcinka. Gdy wypada poza Start/End, viewport pokazuje odrębne przedłużenie prostej nośnej. Point↔Point pokazuje tymczasowy segment łączący cele. Line↔Line podświetla obie Lines, ale nie rysuje klasycznego łuku ani tekstu wymiaru kątowego.

Nakładające się markery o różnych współrzędnych semantycznych są odrzucane fail-closed zamiast nearest-wins lub candidate cycling. Markery o tych samych współrzędnych są równoważne pomiarowo. To nie jest OSNAP i nie tworzy constraints.

Po wyniku kolejny zaakceptowany cel rozpoczyna następną relację. Kliknięcie pustego tła czyści bieżący stan relacyjny, ale pozostaje w Between. **Esc** raz wraca do zwykłego Measure; **Esc** drugi raz wraca do Select.

Measure i Between są read-only: nie powodują dirty, nie zmieniają revision, nie zużywają identyfikatorów i nie tworzą Undo. Runtime measurement references nie są zapisywane, a Measure nie wchodzi do Repeat Last Command.

Wartości liniowe Measure są pokazywane w bieżącej jednostce długości Parta, a pole Circle w kwadracie tej jednostki; kąty pozostają w stopniach. Zmiana jednostki aktualizuje prezentację bez zmiany mierzonej geometrii. Ogólne minimum distance curve-to-curve, OSNAP/tracking/inference, trwałe referencje sub-elementów, authored dimensions i constraints pozostają poza bieżącym Measure. Szersze wyświetlanie wymiarów w viewporcie jest odłożone i ma zostać rozważone razem z przyszłymi authored/parametric dimensions i constraints, zamiast jako osobny subsystem overlay R8C.

<!-- section-id: product.parts.profiles -->
## Construction i Profile

Każda linia, okrąg i łuk w Sketchu może mieć rolę **Regular** albo **Construction**. Zaznacz geometrię w zwykłym Select i użyj `Regular` lub `Construction` w Operations. Construction pozostaje zapisaną geometrią pomocniczą, ale nie zamyka ani nie dzieli regionów używanych przez Profile. W viewporcie geometria Construction jest pokazywana linią przerywaną; jest to wyłącznie wskazówka prezentacyjna, a trwałą prawdą pozostaje authored rola encji. Cadence dash/gap jest screen-space presentation: krótkie i długie Construction Lines mają ten sam rytm wizualny, zoom nie modyfikuje authored geometry, a preview i committed Construction używają tej samej polityki. Regular Rectangle uczestniczy w analizie Profile tylko przez cztery Regular krawędzie; opcjonalne Construction przekątne nie dzielą regionu materiału. Construction Rectangle nie wnosi żadnej granicy materiału.

Narzędzie **Profile** działa w aktywnym Sketchu. Przesuwanie kursora nad zamkniętą geometrią pokazuje półprzezroczysty wynik regionu; kliknięcie przyjmuje kandydata do bieżącego draftu i Status przypomina, że dopiero **Finish Profile** wykonuje trwały commit. O tym, co jest regionem, decyduje dokładna semantyka Line/Circle/Arc, nie tessellation Viewera.

Operations udostępnia **Add Area**, **Subtract Area**, **Detect Islands**, **Highlight on Hover**, **Show Region Boundaries**, **Show Problems**, **Find All Regions**, **Finish Profile** i **Cancel**. Find All Regions jest tylko diagnostyczne i nie tworzy Profile automatycznie. Podczas Subtract wynikowy draft pozostaje błękitny, a aktualnie wskazany region do odjęcia dostaje osobne czerwono-pomarańczowe podświetlenie.

Otwarty łańcuch pozostaje otwarty: SimpleSolid nie domyka małej szczeliny ukrytą tolerancją ani nie naprawia jej automatycznie. Kliknięcie w miejscu, gdzie nie istnieje bounded region, zgłasza diagnostykę otwartej granicy, jeżeli analiza ją wykryła. Bez snapping/OSNAP dwa punkty, które tylko wyglądają na pokrywające się na ekranie, nie są automatycznie zrównywane. Rozłączony wynik Add, Subtract rozcinający materiał albo niejednoznaczna topologia są odrzucane bez zmiany Dokumentu.

Finish tworzy Part-owned Profile ze stabilnym `ProfileId`. Profile przechowuje semantyczne odwołania do geometrii źródłowego Sketchu zamiast kopii widocznego wypełnienia. Edycja źródła może pozostawić Profile **Valid**, uczynić go **Invalid**, a późniejsza naprawa może przywrócić Valid bez zmiany ProfileId. Wspólne Move, Rotate, dodatni Scale albo Mirror całej połączonej granicy Line/Arc zachowuje jej istniejącą topologię endpointów; jest to zachowanie już ustanowionego kontaktu, a nie domykanie szczeliny na podstawie bliskości. Przesunięcie tylko części granicy może celowo utworzyć prawdziwą szczelinę, która pozostaje otwarta do czasu rzeczywistego ponownego domknięcia authored geometry. SimpleSolid nie przepina automatycznie Invalid Profile do podobnej ani najbliższej nowej geometrii.

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

Obecny Part zapewnia tożsamość/właściwości Dokumentu, trwałą jednostkę długości wejścia/wyświetlania, wbudowany Origin, trwałą widoczność referencji, fundament Workbench/Viewer 3D, trwałe Sketche na płaszczyznach Origin z authored geometrią Line/Circle/Arc i rolami Regular/Construction, authoring Rectangle rozkładany na zwykłe Lines oraz Part-owned Profiles z live RegionIntent.

Bieżący Sketch UI obejmuje keyboard-first precision input, wartości mm/cm/m/in/ft i ograniczone expressions, absolutne/względne współrzędne kartezjańskie i polarne, exact Circle/Arc/Rectangle input, numeryczny Rotate/Scale i grip transforms, Polar attraction, Dynamic Input z request-local locks, Measure w bieżących jednostkach fizycznych, obsługiwany Grip edit cycle, Grip Copy w Reshape/Move, repeated COPY, Repeat Last Command, Undo/Redo oraz Save/Reopen.

Profile nie jest operacją bryłową. Nadal brakuje Rotate/Scale/Mirror+Copy, ordinary-Select RMB context, clipboard/cross-Sketch Copy, OSNAP/tracking/inference, Grid Snap, authored constraints/solver, authored dimensions, supportu Sketchu na Datum/płaskiej ścianie modelu, Bodies, Features/Extrude, modelowanej geometrii bryłowej, Material oraz narzędzi Assembly/Drawing.

Bardzo wczesne testowe pliki `.ss2part` sprzed obecnego natywnego formatu nie są obsługiwanym formatem danych i nie są automatycznie migrowane.
