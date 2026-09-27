# Dokumentacja produktu SimpleSolid 2.0

<!-- doc-id: product.overview -->
<!-- document-kind: product -->

<!-- section-id: product.overview.purpose -->
## O tej dokumentacji

Ta dokumentacja opisuje **aktualny, zaakceptowany stan produktu** SimpleSolid 2.0.

Jest przeznaczona do zrozumienia i używania programu. Nie opisuje historii implementacji, aktywnych prac, roadmapy, klas C++ ani dowodów CI.

<!-- section-id: product.overview.current -->
## Co jest obecnie dostępne

Obecny produkt udostępnia:

- tworzenie i otwieranie Projektów;
- stabilną tożsamość Projektu oraz Recent Projects;
- przenoszenie Projektu i odzyskiwanie lokalizacji przez `Locate…`;
- tworzenie natywnych Partów `.ss2part` przez wybór folderu wewnątrz Workspace;
- tworzenie nowego folderu bezpośrednio z dialogu New Part;
- otwieranie wykrytych Dokumentów przez neutralny dialog `Open…`;
- stabilny `DocumentId` niezależny od nazwy i ścieżki pliku;
- otwieranie kilku Partów i przełączanie ich dolnymi Document Tabs;
- wspólny CAD Workbench z Document Tree, Properties, Operations i Status;
- viewport 3D oparty o OCCT z siatką odniesienia i responsywnym ViewCube;
- Pan, Orbit, Zoom, Fit, widoki standardowe/narożne oraz Orthographic/Perspective;
- wbudowany Origin Parta: punkt, osie i płaszczyzny;
- zsynchronizowane zaznaczenie Tree/Viewport z czyszczeniem zaznaczenia na pustym tle i primary selection;
- trwałe Show/Hide referencji Origin z Undo/Redo;
- edycję Number, Title, Description i Engineering Revision;
- tworzenie i edycję Sketchy na płaszczyznach Origin z geometrią Line/Circle/Arc, zaznaczeniem, gripami oraz Move/Copy/Rotate/Scale/Mirror;
- keyboard-first Command Line z Direct Distance w obsługiwanych wejściach punktowych;
- Save z ochroną przed konfliktem zmienionego pliku oraz bezpieczne zamykanie z ochroną niezapisanych zmian;
- ponowne odnajdywanie Partów po restarcie;
- wykrywanie konfliktu, gdy dwa pliki w jednym Workspace mają ten sam DocumentId.

Dialog `Open…` jest już dokumentowo-neutralny, ale jedynym zaimplementowanym typem głównego Dokumentu CAD pozostaje obecnie Part.

Obecny Part jest trwałym dokumentem CAD ze wspólnym środowiskiem pracy 3D oraz trwałymi Sketchami na płaszczyznach Origin. Nie zawiera jeszcze modelowanej geometrii bryłowej Body/Feature.

Powierzchnie dokumentów/narzędzi Assembly i Drawing nie są jeszcze dostępne.

<!-- section-id: product.overview.browser -->
## Product Browser

Product Browser jest generowany z dokumentów Markdown znajdujących się w repozytorium.

Wersje polska i angielska są utrzymywane jako równorzędne źródła. Browser domyślnie otwiera dokumentację po polsku i pozwala przełączyć ją na English.
