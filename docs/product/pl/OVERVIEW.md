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

- tworzenie/otwieranie Projektów, Recent Projects i recovery lokalizacji;
- natywne Dokumenty Part `.ss2part` ze stabilnym `DocumentId`;
- wiele otwartych Partów z Document Tabs;
- wspólny CAD Workbench z Tree, Properties, Operations, Status i keyboard-first Command Line;
- OCCT-backed Viewport 3D, ViewCube, Pan/Orbit/Zoom/Fit i Ortho/Perspective;
- trwały Origin oraz Sketche na płaszczyznach XY/XZ/YZ;
- Shared-2D Line/Circle/Arc/Rectangle, Regular/Construction, grip editing i Move/Copy/Rotate/Scale/Mirror;
- precision input z jednostkami, Polar, Dynamic Input, OSNAP/Tracking/Inference;
- Trim/Extend oraz read-only Measure;
- Part-owned live-reference Profiles z Add/Subtract;
- dokładnie jeden trwały Body i uporządkowane Extrude Features;
- Extrude Add/Cut z OneSide Forward/Reverse i Midplane;
- dynamiczny preview, Edit Extrude, Feature status, Suppress/Unsuppress/Delete oraz Undo/Redo;
- Save z ochroną konfliktu oraz Save/Close/Reopen z cold rebuildem modelu bryłowego.

Part jest obecnie jedynym zaimplementowanym typem głównego Dokumentu CAD. Assembly i Drawing nie są jeszcze dostępne.

Bieżący solid modeling jest celowo ograniczony do PM-01 Extrude; Datum/planar-face support, topology picking, Revolve, Fillet/Chamfer i multi-body pozostają późniejszym zakresem.

<!-- section-id: product.overview.browser -->
## Product Browser

Product Browser jest generowany z dokumentów Markdown znajdujących się w repozytorium.

Wersje polska i angielska są utrzymywane jako równorzędne źródła. Browser domyślnie otwiera dokumentację po polsku i pozwala przełączyć ją na English.
