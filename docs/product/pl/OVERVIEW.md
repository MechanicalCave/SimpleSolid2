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
- trwały Origin, planarne Face Body oraz Offset Datum Planes jako semantyczne podpory Sketch;
- grupa Reference Geometry z trwałymi Offset Datum Planes, Edit/Show/Hide i preview/intersection w Viewporcie;
- Shared-2D Line/Circle/Arc/Rectangle, Regular/Construction, grip editing i Move/Copy/Rotate/Scale/Mirror;
- precision input z jednostkami, Polar, Dynamic Input, OSNAP/Tracking/Inference;
- Trim/Extend oraz read-only Measure;
- Part-owned live-reference Profiles z Add/Subtract;
- Part-owned Sketch-Line Axes oraz bezpośredni Origin X/Y/Z AxisReference;
- dokładnie jeden trwały Body i uporządkowane Extrude/Revolve Features;
- Extrude Add/Cut z OneSide Forward/Reverse i Midplane;
- Revolve Add/Cut z One Side/Midplane, 0 < Angle <= 360° i Reverse dla One Side;
- dynamiczny preview, Edit Extrude/Revolve, naprawa Axis, Feature status, Suppress/Unsuppress/Delete oraz Undo/Redo;
- Save z ochroną konfliktu oraz Save/Close/Reopen z cold rebuildem Body, Datumów, Axes i ich semantycznych zależności.

Part jest obecnie jedynym zaimplementowanym typem głównego Dokumentu CAD. Assembly i Drawing nie są jeszcze dostępne.

Bieżący solid modeling jest celowo ograniczony do Extrude Add/Cut i Revolve Add/Cut w udokumentowanych wariantach OneSide/Midplane. Datum Axis, Datum Point, dodatkowe konstruktory Datum Plane, konstruktory Axis z Body-Edge/Curve, Projection, Fillet/Chamfer, wieloobrotowy Revolve i multi-body pozostają późniejszym zakresem.

<!-- section-id: product.overview.browser -->
## Product Browser

Product Browser jest generowany z dokumentów Markdown znajdujących się w repozytorium.

Wersje polska i angielska są utrzymywane jako równorzędne źródła. Browser domyślnie otwiera dokumentację po polsku i pozwala przełączyć ją na English.
