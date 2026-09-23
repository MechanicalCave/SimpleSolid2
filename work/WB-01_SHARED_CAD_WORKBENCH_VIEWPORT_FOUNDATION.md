# WB-01 — Shared CAD Workbench & 3D Viewport Foundation

**Status:** ACCEPTED  
**Owner acceptance:** 2026-09-23  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Powiązane ADR:** ADR-0002, ADR-0003

## 1. Cel

Zaimplementować pierwszy wspólny CAD Workbench SS2, który będzie później używany przez Part, Assembly i Sketch edit context.

WB-01 ma udowodnić:

```text
Open Project
→ Open several PartDocuments
→ one shared CAD Workbench
→ bottom Document Tabs
→ one ActiveDocumentSession
→ Document Tree
→ shared Properties
→ shared Operations surface
→ shared OCCT-backed 3D Viewport
→ Document Origin
→ reference grid
→ ViewCube
→ pan / orbit / zoom
→ standard views
→ ortho / perspective
→ Tree selection
→ persistent Show / Hide of Origin references
→ Save
→ restart
→ visibility survives
```

WB-01 nie implementuje jeszcze właściwego modelowania geometrycznego.

## 2. Zakres IN

WB-01 obejmuje:

- wspólny CAD Workbench Shell;
- runtime active-document/view context;
- dolne Document Tabs;
- Status / Diagnostics;
- generic Document Tree infrastructure;
- selected set + primary selection;
- Tree multi-selection i context actions;
- built-in Part Document Origin;
- persistent presentation visibility dla Origin references;
- wspólny Properties panel;
- wspólny Operations panel;
- provider-neutral Viewer API;
- Qt/OCCT Viewer provider;
- reference grid;
- ViewCube;
- podstawową nawigację kamery;
- migrację trwałego formatu Part potrzebną dla persistent visibility;
- automatyczne testy architektury, lifecycle i UX;
- aktualizację dokumentacji internal oraz PL/EN product docs.

## 3. Docelowy layout

Po otwarciu Dokumentu Workspace prezentuje:

```text
┌──────────────────┬──────────────────────────────┬──────────────────────┐
│                  │                              │      Properties      │
│  Document Tree   │        3D Viewport           ├──────────────────────│
│                  │                              │      Operations      │
│                  │                              │                      │
├──────────────────┴──────────────────────────────┴──────────────────────┤
│ [ Part A ] [ Part B ] [ Part C ]                                     │
├───────────────────────────────────────────────────────────────────────┤
│ Status / diagnostics / context hints                                  │
└───────────────────────────────────────────────────────────────────────┘
```

Górna część głównego okna pozostaje przeznaczona dla globalnych/menu/view actions.

WB-01 nie implementuje finalnego ribbon/menu systemu.

## 4. Migracja z PART-01 UI

Istniejący PART-01 `PartWorkspacePanel` był poprawnym lifecycle slice, ale nie jest docelowym CAD layoutem.

WB-01 przenosi istniejące możliwości:

```text
Document Properties
Undo
Redo
Save
Close
New Part
Open Part
```

do nowego Workbench bez zmiany ich semantyki domenowej.

UI migration nie może stworzyć drugiej ścieżki mutacji `PartDocument`.

## 5. New/Open Part

`New Part…` i `Open Part…` pozostają document-lifecycle actions, a nie narzędziami modelowania w Operations.

Open korzysta z istniejącego Workspace discovery i nadal pokazuje resolved documents, invalid native Parts oraz IdentityConflict.

Otwarcie już otwartego DocumentId aktywuje istniejącą zakładkę zamiast tworzyć drugi DocumentSession.

## 6. Document Tabs i ActiveDocumentSession

Każdy otwarty DocumentSession ma jedną zakładkę Workbench.

Kliknięcie zakładki zmienia `ActiveDocumentSession`, ale nie otwiera ponownie pliku i nie tworzy nowej sesji.

Zmiana aktywnego Dokumentu przełącza:

```text
Document Tree
Viewport/view state
Properties
Operations
Selection
Status / Diagnostics context
```

Zamknięcie zakładki oznacza Close Document i respektuje istniejącą semantykę Save / Discard / Cancel.

## 7. Runtime Document View State

Każdy otwarty Dokument może posiadać runtime-only view state:

```text
camera
projection
selection
primary selection
Tree expansion
active editor context
runtime diagnostics
```

Przełączenie A → B → A powinno w tej samej sesji aplikacji odtworzyć runtime view context A.

Ten stan nie jest częścią `.ss2part` w WB-01.

Kamera nie przeżywa restartu aplikacji.

## 8. Minimalny Document Tree

Minimalny Tree pustego Parta:

```text
<Part Document>
└── Origin
    ├── XY Plane
    ├── XZ Plane
    ├── YZ Plane
    ├── X Axis
    ├── Y Axis
    ├── Z Axis
    └── Origin Point
```

Root Part reprezentuje semantic Document.

`Origin` jest grouping node.

Jego dzieci reprezentują realne built-in semantic references.

WB-01 nie dodaje pustych future-folderów Bodies / Features / Sketches / Datums.

## 9. Built-in Document Origin

Built-in role semantics:

```text
OriginPoint
XAxis
YAxis
ZAxis
XYPlane
XZPlane
YZPlane
```

Identity wynika deterministycznie z roli.

Nie otrzymują losowych DocumentObjectId.

Nie istnieje Delete dla built-in Origin references.

Hide nie wpływa na ich istnienie ani semantic referencability.

## 10. Origin presentation i grid

Viewport prezentuje Origin poprzez provider-neutral reference presentation.

Exact visual style nie jest kontraktem domenowym.

Reference grid posiada co najmniej:

```text
frame
spacing
major/minor presentation
visibility
```

Grid jest runtime presentation aid, nie Datum Plane, Sketch entity ani persistent CAD object.

WB-01 nie zamraża adaptive grid/snapping dla Sketchera.

## 11. Persistent visibility

Widoczność built-in Origin references jest persistent presentation semantics.

Po Save / Close / restart / reopen stan musi zostać odtworzony.

Typowy impact:

```text
needsSave = true
viewer refresh = true
geometry recompute = false
```

WB-01 nie implementuje jeszcze globalnego visibility frameworku dla nieistniejących Features/Sketches/Occurrences, ale nie może blokować rozszerzenia tego konceptu.

## 12. Part persistence schema migration

PART-01 schema v1 nie posiada persistent presentation state.

WB-01 wprowadza następną rozpoznawalną wersję native Part schema.

Wymagane zachowanie:

```text
.ss2part v1
    → remains readable

load v1
    → deterministic default presentation state

Open v1
    → does not rewrite file

successful later Save
    → publishes current supported schema

current schema
    → preserves Origin visibility
```

Unsupported future schema nadal failuje closed.

Builtin Origin identities nie wymagają random IDs w pliku; persisted state odwołuje się do deterministycznych ról.

## 13. Visibility mutation i Undo/Redo

Persistent visibility przechodzi przez controlled Command / Transaction path.

Logicznie:

```text
Tree context menu
→ Set Visibility intent
→ validation
→ Part transaction
→ persistent presentation state
→ DocumentRevision
→ save-dirty
→ viewer refresh
```

Batch Show/Hide dla kilku targetów jest jednym commit i jednym Undo entry.

Successful batch mutation zwiększa technical DocumentRevision dokładnie raz.

No-op nie tworzy historii ani revision increment.

Undo batch operacji przywraca indywidualny previous visibility state każdego targetu.

## 14. Tree selection

Tree obsługuje click, Ctrl+click, Shift+click i Ctrl+Shift where applicable.

Shift range używa widocznego porządku Tree.

Workbench posiada jeden document-scoped:

```text
selected set
primary selection
```

Selection jest runtime-only i nie powoduje `needsSave`.

## 15. Context actions i capabilities

Origin reference context menu udostępnia co najmniej Show / Hide.

Origin grouping node może oferować Show All / Hide All jako batch operation.

Operacja pojawia się dla multi-selection tylko, jeśli jest poprawna dla całego selected set:

```text
available actions
=
intersection of capabilities of selected items
```

WB-01 nie implementuje polityki "apply where possible".

Mixed visibility nie powoduje toggle: Show ustawia true, Hide ustawia false.

## 16. Synchronizacja Tree i Viewport

Wspólna Selection authority synchronizuje Tree i Viewer.

Co najmniej:

```text
select visible XY Plane in Tree
→ highlight XY Plane in Viewport

pick visible XY Plane in Viewport
→ select XY Plane in Tree
→ make XY Plane primary selection
```

Grid nie jest selectable semantic object.

ViewCube nie uczestniczy w Document selection.

## 17. Properties

Gdy primary selection jest root PartDocument, Properties nadal edytuje:

```text
Number
Title
Description
Engineering Revision
```

przez istniejący Command/Transaction lifecycle.

Gdy primary selection jest built-in Origin reference, Properties może prezentować jego role/type i sensowne read-only informacje.

WB-01 nie wprowadza masowego generic property editing.

## 18. Operations

Operations jest wspólną powierzchnią przyszłych Tool contributions.

WB-01 nie dodaje fałszywych modeling tools.

Dopuszczalny stan pustego Parta:

```text
Operations
    No modeling operations are available in this scope.
```

Panel musi jednak umożliwiać późniejsze Tool Palette → Active Tool Task → Accept / Cancel bez wymiany Workbench.

## 19. Provider-neutral Viewer API

WB-01 wprowadza osobną publiczną granicę Viewera bez zależności na Qt i OCCT.

Musi reprezentować pojęcia potrzebne dla:

```text
CameraState
Projection
standard orientations
Scene / presentation entries
Transform
reference presentation
selection token
navigation intents
```

Dokładne nazwy klas/containerów są D1, o ile granica pozostaje provider-neutral.

SS1 Viewer API jest donorem designu i testów, nie frozen SS2 API.

## 20. OCCT Viewer provider

Implementacja 3D Viewport używa OCCT po providerowej stronie.

Typy:

```text
V3d_*
AIS_*
Graphic3d_*
TopoDS_*
```

nie mogą przeciekać do publicznego Viewer API, PartDocument, persistence, Commands ani common Workbench contracts.

WB-01 używa OCCT do prezentacji i nawigacji, nie do Part feature modeling.

## 21. OCCT jako build dependency

Obecny bootstrap potrafi wykrywać machine-local OCCT.

Po WB-01 pełny build aplikacji wymaga poprawnie wykrytego Qt i OCCT.

`ss2 setup`, `verify` i CMake messaging muszą przestać opisywać brak OCCT jako poprawny stan dla pełnego produktu.

Konfiguracja nadal korzysta z istniejącego machine-local `CMAKE_PREFIX_PATH`; nie powstaje drugi mechanizm konfiguracji OCCT.

## 22. Boundary protection

Automatyczna weryfikacja chroni provider-neutral boundary.

Publiczne nagłówki Viewera nie mogą zawierać lub wymagać OCCT types.

Part/Application common domain nie może include'ować Viewer provider internals.

SS1 boundary scanning jest donor pattern.

## 23. CameraState i navigation

CameraState reprezentuje provider-neutral znaczenie potrzebne do odtworzenia widoku w aktywnej sesji, co najmniej:

```text
eye / position
target
up
projection mode
orthographic scale or perspective equivalent
```

Camera state jest runtime-only.

WB-01 implementuje co najmniej:

```text
Pan
Orbit
Zoom
Fit All

Front
Back
Left
Right
Top
Bottom
Isometric

Orthographic
Perspective
```

Exact keyboard/mouse bindings nie są D2 kontraktem, ale pierwsza implementacja ma zapewnić normalną obsługę myszy i udokumentować rzeczywiste gesty.

SS1 navigation mapping i Windows fixes są donor/reference.

## 24. ViewCube

Viewport posiada ViewCube w prawym górnym obszarze Editor Surface.

ViewCube:

- odzwierciedla orientację kamery;
- umożliwia standardowe orientacje;
- wspiera standardowe corner/isometric orientations;
- komunikuje się przez neutralny Viewer Navigation API;
- nie zna bezpośrednio `V3d_View`.

Wygląd jest własną prezentacją SS2, nie kopią konkretnego produktu.

## 25. Projection i Fit All

Orthographic / Perspective są runtime view state i nie dirty'ują Dokumentu.

Tab switch w tej samej sesji przywraca projection danego Dokumentu.

Fit All musi zachowywać się poprawnie także w reference-only scene bez modeled solid.

## 26. Brak model geometry

Viewport WB-01 wyświetla jedynie:

```text
grid
Origin references
ViewCube/navigation overlays
selection/highlight
```

Nie tworzymy testowego BoxFeature ani trwałego solidu tylko dla Viewera.

Pierwsza evaluated CAD geometry będzie osobnym kontraktem.

## 27. Sketch-aware architecture bez Sketch implementacji

WB-01 nie implementuje Sketch Core ani Sketch Edit mode.

Workbench/Viewer nie może jednak wymagać później osobnego Sketch window/canvas universe.

Publiczne granice muszą umożliwiać przyszły flow:

```text
same Workbench
same Document Tree
same Properties
same Operations
same Selection authority
same Document Viewport

Part context
    ↓ Start Sketch
Sketch edit context
    ↓ Finish Sketch
Part context
```

## 28. Workbench Shell nie zna przyszłych domain tools

Shell nie może zawierać przyszłych switchy/enumów typu Extrude, Chamfer, SketchLine, Mate czy AssemblyPattern.

Workbench zna presentation contracts i tool contribution infrastructure.

Domena dostarczy konkretne Operations później.

## 29. SS1 donor policy

Warto ponownie wykorzystać koncepcje oraz małe oczyszczone implementacje z:

```text
Viewer API CameraState
Viewer scene/presentation contracts
selection transport concepts
navigation mapping
Qt/OCCT viewer plumbing
Windows input fixes
presentation roles
ToolTaskPanel anatomy
```

Nie kopiujemy wprost:

```text
M16ShellWindow
milestone compatibility seams
Part/Sketch/Assembly knowledge embedded in Shell
SketchEditorWidget toolbar architecture
feature-specific viewer code
historical preview/topology machinery
```

Każdy donor fragment podlega Foundation/ADR-0003 SS2.

## 30. Test strategy

Deterministyczne rzeczy są testowane bez realnego GPU framebuffer, m.in.:

```text
CameraState
standard-view mapping
projection switching
scene/reference semantics
selection transport
navigation mapping
Tree selection
capability intersection
visibility command/history
schema migration
tab/active-document lifecycle
```

Qt Workbench interaction jest testowany offscreen tam, gdzie nie wymaga natywnego renderowania.

OCCT provider otrzymuje osobne tests/smoke dla konstrukcji i navigation adapterów.

WB-01 nie wymaga screenshot pixel comparison.

Realny native Windows application smoke Viewera pozostaje wymaganym dowodem ukończenia.

## 31. Zakres OUT

WB-01 nie obejmuje:

```text
Sketch Core
Sketch entities
constraint solver
Sketch Edit mode
Bodies
Features
Extrude
Chamfer
Part evaluated B-Rep
geometry Kernel API use
topology selection
persistent topology naming
AssemblyDocument
Assembly occurrences
mates/constraints
DrawingDocument
BOM
Material
custom properties
Save As
Save Copy As
identity-conflict repair
multiple simultaneous split viewports
camera persistence across restart
final ribbon/menu design
```

## 32. Testy akceptacyjne

Weryfikacja musi udowodnić co najmniej:

1. Viewer public API kompiluje się bez Qt/OCCT types.
2. OCCT provider internals nie przeciekają przez publiczne Viewer headers.
3. Full product configure/build wymaga wykrytego OCCT.
4. Wszystkie testy PART-01 pozostają PASS.
5. Opening Part tworzy jedną Document tab.
6. Opening second Part tworzy drugą tab.
7. Opening already-open DocumentId reuses existing tab/session.
8. Tab switching zmienia ActiveDocumentSession bez zamykania dokumentów.
9. Runtime camera/selection state jest izolowany per document.
10. Closing dirty tab respektuje Save/Discard/Cancel.
11. Empty Part Tree zawiera root + Origin grouping.
12. Origin eksponuje siedem deterministic built-in roles.
13. Built-in refs nie są random DocumentObjectIds.
14. Origin grouping nie jest drugim visibility ownerem.
15. click/Ctrl/Shift selection działa zgodnie z ADR-0003.
16. selected set i primary selection są rozdzielone.
17. Mixed selected capabilities pokazują tylko wspólne actions.
18. Hide one Origin ref jest jedną semantic mutation.
19. Batch Hide jest jednym commit i jednym Undo entry.
20. Batch Undo odtwarza indywidualny prior state.
21. Visibility no-op nie tworzy revision/history.
22. Visibility mutation ustawia needsSave.
23. Visibility mutation nie wymaga geometry recompute.
24. Save/restart zachowuje visibility.
25. Legacy .ss2part v1 pozostaje czytelny.
26. Open v1 nie przepisuje pliku.
27. Later successful Save migruje do current schema.
28. Future unsupported schema nadal failuje closed.
29. Tree selection Origin ref highlightuje Viewport.
30. Viewport pick aktualizuje Tree/primary selection.
31. Grid nie jest selectable semantic geometry.
32. Pan działa.
33. Orbit działa.
34. Zoom działa.
35. Fit All działa dla reference-only scene.
36. Standard views działają deterministycznie.
37. Isometric działa.
38. Ortho/Perspective działa.
39. Navigation nie dirty'uje Document.
40. ViewCube śledzi camera orientation.
41. ViewCube steruje kamerą przez neutralny navigation contract.
42. Part root pokazuje istniejące Document Properties.
43. Property editing zachowuje Command/Transaction/Undo/Redo.
44. Origin primary selection zmienia Properties context bez direct mutation.
45. Operations istnieje bez fake modeling tools.
46. Shell nie zna future Part/Assembly/Sketch tool semantics.
47. Existing dirty-close guards działają z wieloma tabs.
48. Restart rediscoveruje Parts bez persistence camera/selection.
49. Native Windows application otwiera funkcjonalny OCCT-backed viewport.
50. Pełny docs/verify/build/test gate jest PASS.

## Documentation impact

Internal docs: required  
User/Product docs PL/EN: required

WB-01 materialnie zmienia Workspace i wprowadza pierwszy realny CAD Viewport.

Internal docs muszą opisać Workbench ownership, ActiveDocumentSession, runtime DocumentViewState, Selection authority, Origin semantic references, persistent presentation state, Part schema migration, Viewer API/provider boundary, OCCT build dependency i navigation architecture.

Product docs PL/EN muszą opisać Document Tabs, Tree/Properties/Operations layout, Origin, Show/Hide, multi-selection, 3D navigation, standard views, ViewCube i Orthographic/Perspective.

Generated Product Browser musi zostać odświeżony i pozostać Git-clean.

## 33. Architecture impact

WB-01 implementuje Foundation 1.0 oraz ADR-0002/ADR-0003.

Jeżeli implementacja wymaga zmiany m.in. Document identity, DocumentSession ownership, Origin identity semantics, persistent visibility meaning, Workbench layout responsibility, Sketch integration direction, Viewer provider boundary lub selection authority, praca zatrzymuje się do jawnej decyzji Ownera.

## 34. Sekwencja realizacji

WB-01 może być realizowany kilkoma PR-ami pod jednym kontraktem.

Rekomendowana kolejność:

```text
Slice A
Viewer API + OCCT provider + deterministic navigation tests

Slice B
Part Origin semantics + persistent visibility + schema migration

Slice C
Workbench Shell + Document Tabs + ActiveDocumentSession

Slice D
Tree + selection + context actions + Properties/Operations integration

Slice E
ViewCube + reference presentation + final UX/docs/native smoke
```

## 35. Kryterium ukończenia

WB-01 jest ukończony, gdy użytkownik może:

```text
open Project
→ open two Parts
→ switch them with bottom Document Tabs
→ see each Part in the same common CAD Workbench
→ use Document Tree
→ select Origin references singly or in groups
→ Show / Hide them
→ Undo / Redo visibility
→ Save
→ navigate shared 3D Viewport
→ use ViewCube and standard views
→ switch ortho/perspective
→ close/restart
→ recover persistent visibility
```

przy zachowaniu:

```text
no Sketch implementation
no Body/Feature
no modeled solid
no Assembly
no Drawing
no persistent topology naming
no OCCT leakage into domain
```

Ukończenie WB-01 nie uruchamia automatycznie następnego etapu.
