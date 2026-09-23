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
- tworzenie i otwieranie natywnych dokumentów Part `.ss2part`;
- stabilny `DocumentId` niezależny od nazwy i ścieżki pliku;
- edycję właściwości Number, Title, Description i Engineering Revision;
- Undo / Redo dla tych zmian;
- Save i bezpieczne zamykanie z ostrzeżeniem o niezapisanych zmianach;
- ponowne odnajdywanie Partów po restarcie;
- wykrywanie konfliktu, gdy dwa pliki w jednym Workspace mają ten sam DocumentId.

Obecny Part jest dokumentem trwałym, ale nie zawiera jeszcze modelowania geometrycznego.

Sketch, Bodies/Features, Assembly, Drawing i Viewer nie są jeszcze częścią dostępnej powierzchni produktu.

<!-- section-id: product.overview.browser -->
## Product Browser

Product Browser jest generowany z dokumentów Markdown znajdujących się w repozytorium.

Wersje polska i angielska są utrzymywane jako równorzędne źródła. Browser domyślnie otwiera dokumentację po polsku i pozwala przełączyć ją na English.
