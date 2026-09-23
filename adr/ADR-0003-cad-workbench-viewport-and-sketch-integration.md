# ADR-0003 — Wspólny CAD Workbench, Viewport i integracja Sketch

**Status:** ACCEPTED  
**Owner acceptance:** 2026-09-23  
**Foundation:** 1.0 (foundation-v1.0)  
**Powiązane decyzje:** ADR-0002  
**Zakres:** CAD Workbench / Document Tree / Properties / Operations / Viewport / Selection / Origin / Sketch integration

## Kontekst

Po PART-01 SS2 posiada trwały lifecycle PartDocument, DocumentSession, Document Properties, Undo/Redo, persistence oraz Workspace discovery.

Przed właściwym modelowaniem Part/Assembly potrzebna jest wspólna powierzchnia pracy CAD. Part, przyszły Assembly, Sketch oraz częściowo Drawing będą potrzebowały wspólnych elementów UX: Document Tree, Properties, narzędzi tworzenia i edycji, selection, viewport/editor surface, navigation, diagnostics i active-document context.

SS1 jest wartościowym donorem dla Shell, Viewer API, Qt/OCCT Viewer, presentation roles, ToolTaskPanel i Sketch Core. Nie jest jednak wzorcem integracji Sketchera jako osobnej aplikacji wewnątrz CAD. SS2 ma tego nie powtarzać.

## Decyzja

### 1. Jeden wspólny CAD Workbench Shell

Podstawowy layout otwartego Dokumentu CAD jest wspólny dla Part, Assembly, Sketch edit context oraz przyszłego Drawing:

    LEFT                  CENTER                         RIGHT
    Document Tree         Editor Surface                 Properties
                                                         Operations

    BOTTOM
    Document Tabs

    BELOW TABS
    Status / Diagnostics

Workbench Shell nie posiada semantyki Part, Assembly, Sketch ani Drawing. Konkretna domena lub aktywny editor context dostarcza dane prezentacyjne i intents dla tych powierzchni.

### 2. Open documents i ActiveDocumentSession

ProjectSession może posiadać wiele otwartych DocumentSession, ale Workbench ma jeden ActiveDocumentSession dla aktualnej powierzchni edycji.

Zmiana aktywnej zakładki przełącza Document Tree, Editor Surface, Properties, Operations, Selection oraz status/diagnostics context bez zamykania pozostałych DocumentSessions.

### 3. Document Tabs i status bar

Document Tabs znajdują się na dole Workbench, podobnie do sprawdzonego układu w profesjonalnych systemach CAD. Górna część interfejsu pozostaje dla menu i głównych narzędzi.

Pod Document Tabs znajduje się Status / Diagnostics Bar przeznaczony m.in. dla tool hints, validation state, selection hints, coordinates, runtime diagnostics i operation progress.

Dokładny wygląd tabów, overflow, ikony i przyszłe split views pozostają szczegółem późniejszego UX contract.

### 4. Document Tree jest presentation surface, nie modelem CAD

Tree zapewnia hierarchy, expand/collapse, selection, multi-selection, visibility presentation, context actions, diagnostics i synchronizację z viewportem.

Tree node nie staje się trwałym CAD object tylko dlatego, że występuje w Tree.

Tree rozróżnia semantic nodes od presentation/group nodes. Folder organizacyjny nie otrzymuje automatycznie CAD identity ani authored state.

### 5. Multi-selection w Tree

Podstawowa gramatyka:

    click          -> replace selection
    Ctrl + click   -> toggle item
    Shift + click  -> visible-range selection
    Ctrl + Shift   -> extend range where applicable

Zakres Shift jest liczony według aktualnej widocznej kolejności Tree. Collapsed descendants nie są niejawnie zaznaczane.

### 6. Selected set i primary selection

Document-scoped selection authority posiada selected set oraz primary selection.

Selected set służy operacjom grupowym. Primary selection jest domyślnym kontekstem dla Properties i operacji wymagających jednego aktywnego obiektu.

Tree, Viewport, Properties i Tools nie tworzą konkurencyjnych selection models.

### 7. Multi-selection actions

Context menu dla multi-selection pokazuje tylko operacje poprawne dla całego zaznaczenia.

Logicznie:

    available actions = intersection of capabilities of selected items

SS2 nie wykonuje domyślnie operacji tylko na tym podzbiorze zaznaczenia, dla którego przypadkiem jest ona możliwa.

### 8. Group operations są jedną logiczną operacją

Operacja grupowa jest jednym Command/Transaction i jednym Undo entry.

Przykład: zaznaczenie XY Plane, XZ Plane i YZ Plane, a następnie Hide, jest jedną operacją.

### 9. Properties

Properties jest wspólną powierzchnią kontekstową dla Document, Part object, Sketch entity/constraint, Assembly occurrence i przyszłych Drawing objects.

Wspólny panel nie oznacza jednego płaskiego PropertyBag. Znaczenie i ownership właściwości pozostaje domenowe, a trwałe zmiany przechodzą przez Commands/Transactions.

### 10. Operations

Prawy panel Operations jest wspólnym systemem narzędzi dla Part, Assembly, Sketch i przyszłego Drawing.

Ma dwa podstawowe stany:

- Tool Palette — dostępne operacje aktywnego kontekstu;
- Active Tool Task — Tool Header, tool-specific inputs, validation/diagnostics, Cancel/Accept.

SS1 ToolTaskPanel i jego wspólne komponenty są donorami koncepcji, nie frozen SS2 API.

### 11. Context menu zamiast zewnętrznych przełączników

Operacje dotyczące konkretnego elementu modelu, w szczególności Show/Hide, preferują context menu Tree zamiast trwałych zewnętrznych przycisków zajmujących miejsce interfejsu.

### 12. Sketch nie posiada własnego systemu toolbarów

Sketch tools są kontrybuowane do wspólnego Operations. Wejście w Sketch zmienia aktywny editor/tool context, nie aplikację.

Document Tree pozostaje obecny podczas Sketch Edit. Aktywny Sketch może być wyróżniony w zwykłym Document Tree.

### 13. Editor Surface

Centralna powierzchnia to Editor Surface.

Part i Assembly używają wspólnego Document Viewport. Sketch osadzony w Part/Assembly pozostaje w tym samym Document Viewport. Przyszły Drawing może używać własnego 2D page renderer/editor w tym samym Workbench Shell.

### 14. Sketch Edit odbywa się naturalnie w przestrzeni 3D

Sketch posiada lokalną przestrzeń 2D U/V oraz hostowe SketchPlacement do przestrzeni Dokumentu.

Podczas rozpoczęcia Sketch Edit kamera ustawia się względem płaszczyzny szkicu, reference grid przechodzi do frame szkicu, aktywny Sketch staje się edytowalny, a pozostały model pozostaje widoczny jako kontekst.

SS2 nie zamyka Part/Assembly UI i nie otwiera osobnej aplikacji Sketcher.

### 15. Sketch pozostaje elementem sceny po zakończeniu edycji

Po Finish Sketch authored geometry i placement Sketch pozostają w Dokumencie. Sketch może nadal być prezentowany w przestrzeni 3D; edit mode jest runtime state.

### 16. Wspólny Sketch Core

Architektura przewiduje:

    2D Foundation
        ↓
    Sketch Core
        ↓
    Host Integration

2D Foundation zawiera neutralne primitives matematyczne i geometrii 2D.

Sketch Core odpowiada za entities, constraints, driving dimensions, construction geometry, solver integration, inference/snapping concepts i local validation.

Sketch Core nie zna PartDocument, AssemblyDocument, DrawingDocument, OCCT, Qt ani filesystem paths.

### 17. Sketch Host

Part, Assembly i Drawing mogą hostować wspólny Sketch Core.

Host odpowiada za placement, support/reference semantics, persistence ownership, external references, Commands/Transactions integration, tree placement, scene presentation i host-specific lifecycle.

Part jest pierwszym naturalnym hostem.

Assembly może później używać tego samego Core dla layout/skeleton/reference sketches.

Drawing może posiadać prawdziwy DrawingSketch, ale Drawing View, Drawing Dimension, Leader, Text, Symbol, Table, Border i Title Block zachowują własną semantykę. Drawing jako całość nie jest Sketchem.

### 18. Wspólny Document Viewport

SS2 nie tworzy osobnych systemów PartViewer i AssemblyViewer.

Neutralna scena Viewera od początku dopuszcza scene item/asset, transform, visibility/presentation i selection token, ponieważ przyszły Assembly potrzebuje wielu instancji geometrii z różnymi transformacjami.

### 19. Viewer API jest provider-neutral

Publiczna granica Viewera nie ujawnia V3d_View, AIS_InteractiveContext, AIS_Shape, TopoDS_Shape ani provider identity.

Neutralny Viewer API może posiadać koncepty CameraState, Scene, SceneItem, Transform, PresentationRole, SelectionToken i ReferencePresentation.

SS1 Viewer API jest ważnym dawcą granicy, ale nie jest kopiowany automatycznie jako frozen SS2 API.

### 20. ViewCube

Viewport posiada wspólny orientation/navigation widget funkcjonalnie podobny do sprawdzonych CAD ViewCube solutions.

Obsługuje co najmniej Front, Back, Left, Right, Top, Bottom oraz standardowe narożniki/isometric orientations.

ViewCube komunikuje się przez neutralny Viewer Navigation API i nie wywołuje bezpośrednio OCCT.

### 21. Kamera i nawigacja

Wspólny 3D Viewport zapewnia co najmniej Pan, Orbit, Zoom, Fit All, standard views, Isometric oraz Orthographic/Perspective.

Camera/navigation state jest runtime presentation state i nie powoduje DocumentRevision, needsSave ani CAD Undo entry.

Sketch i przyszły Drawing współdzielą z 3D możliwie spójne Pan/Zoom/Fit. Orbit jest 3D-only.

SS1 native Windows navigation fixes, zoom-around-cursor i Fit behavior są donor knowledge wymagającym ponownego wykorzystania lub ponownego testowania.

### 22. Grid jest presentation primitive

Reference grid ma frame, spacing, major/minor steps, visibility i presentation style.

W zwykłym 3D może reprezentować globalną płaszczyznę odniesienia. Podczas Sketch Edit może przejść do local Sketch frame.

Grid nie jest Datum Plane, Sketch entity ani trwałym CAD object.

### 23. Part i Assembly mają wspólny built-in Document Origin

Part i Assembly używają tego samego fundamentalnego Document Origin / Document Coordinate System.

Built-in semantic references:

    Origin Point
    X Axis
    Y Axis
    Z Axis
    XY Plane
    XZ Plane
    YZ Plane

są prawdziwymi semantic references, do których inne authored objects mogą trwale się odwoływać.

### 24. Deterministyczne identities Origin

Built-in Origin references mają identity wynikającą z jednoznacznej roli, logicznie np.:

    BuiltinReference::OriginPoint
    BuiltinReference::XAxis
    BuiltinReference::YAxis
    BuiltinReference::ZAxis
    BuiltinReference::XYPlane
    BuiltinReference::XZPlane
    BuiltinReference::YZPlane

Nie wymagają losowo generowanych zwykłych DocumentObjectId. Dokładny C++ encoding pozostaje implementation detail.

### 25. Istnienie i widoczność Origin są niezależne

Built-in Origin references zawsze istnieją semantycznie.

Hide zmienia presentation, ale nie usuwa, nie dezaktywuje i nie zmienia identity reference object.

Origin w Tree jest grouping node. Może oferować Show All / Hide All jako grupową operację na dzieciach, ale sam nie posiada niezależnego nadrzędnego visibility flag.

### 26. Persistent model-object visibility

Jawna decyzja użytkownika o widoczności odpowiednich obiektów modelowych przeżywa Save/Close/restart.

Dotyczy m.in.:

    Sketch
    Origin plane / axis / point where applicable
    future Datum/reference objects
    Assembly occurrence

Visibility jest persistent presentation semantics, odrębną od authored engineering geometry.

Typowy impact:

    needsSave = true
    viewer refresh = true
    geometry recompute = false

### 27. Brak zwykłego Hide finalnego Parta we własnym PartDocument

Główna wynikowa geometria Parta w swoim własnym PartDocument nie otrzymuje zwykłej user-facing operacji Hide Part.

Sketch/reference/datum objects mogą być ukrywane.

Ten sam Part użyty jako Assembly occurrence może być Show/Hide w kontekście Assembly.

### 28. Show/Hide jest jawnie ustawianym stanem

Dla multi-selection context menu oferuje Show i Hide, nie niejednoznaczny per-item toggle.

Przy mixed visibility Hide ustawia cały obsługiwany zaznaczony set na hidden, a Show na visible.

### 29. Persistent visibility przechodzi przez Command/Transaction

Tree nie mutuje visibility bezpośrednio.

Logicznie:

    Tree context menu
        ↓
    SetVisibilityCommand
        ↓
    Document transaction
        ↓
    persistent presentation state

Multi-selection jest jednym commandem i jednym Undo entry. Zmiana visibility jest Undo/Redo-able.

### 30. Runtime presentation state

Camera, projection mode, pan/zoom/orbit, hover, temporary highlight, active editor context, active selection, temporary Tool preview i temporary navigation/grid state są runtime-only i same nie dirty'ują Dokumentu.

Persistent model-object visibility została zdefiniowana osobno jako trwała presentation semantics.

### 31. Wspólny Selection subsystem

Tree, Viewport, Properties i Tools używają wspólnej document-scoped selection authority.

Tool może posiadać chwilowy input/candidate/preview state, ale nie staje się on drugim trwałym źródłem Document selection.

Provider-native Viewer selection tokens są transient i są mapowane na semantic/application context.

Sketch używa tego samego selection model. Wejście w Sketch nie tworzy odrębnego selection universe.

## Wnioski z audytu SS1

### Zachować jako donor concepts

Wartościowe elementy SS1:

    Viewer API oddzielony od Qt/OCCT
    CameraState
    neutral Scene / SceneInstance / Transform
    presentation roles
    selection transport boundary
    ToolTaskPanel anatomy
    GeometryInputRow
    QuantityEditor
    ToolValidationMessage
    ToolActionBar
    shared PresentationQt visual grammar
    SketchController / Sketch Core separation
    SketcherQt as presentation/input adapter
    native Windows navigation fixes

### Nie kopiować jako architektury SS2

Nie przenosimy wprost:

    M16ShellWindow jako centralnego monolitu
    Shell znającego konkretne Features / Assembly operations
    SketchEditorWidget-own toolbar
    osobnego Sketch canvas jako aplikacji w aplikacji
    milestone append-only compatibility seams
    historycznych frozen presentation aggregates

## Konsekwencje

Workbench UX pozostaje spójny między Part, Assembly i Sketch.

Sketch staje się naturalnym trybem edycji w przestrzeni 3D Dokumentu i pozostaje elementem sceny po zakończeniu edycji.

Part i Assembly współdzielą Document Origin i mogą używać jego stabilnych built-in references.

Part i Assembly współdzielą Viewport bez wiązania Viewera z domeną.

Sketch Core może być używany przez Part, Assembly i prawdziwe Drawing Sketches bez modelowania całego Drawing jako Sketch.

Persistent visibility może działać przez Tree/Commands/Undo bez mieszania jej z geometry recompute.

## Świadomie deferred

ADR nie zamraża jeszcze:

    docelowej hierarchii Part Tree
    Body/Feature model
    Assembly occurrence model
    Sketch persistence schema
    future topology-based Sketch support/reference semantics
    persistent topology naming
    Drawing domain model
    Drawing annotation model
    konkretnego constraint solvera
    dokładnego C++ API Scene
    dokładnego C++ API Selection
    dokładnego C++ API Tool contribution
    dokładnego zestawu ikon
    finalnego menu/ribbon layout
    dokładnych skrótów klawiszowych
    finalnych gestów myszy
    kolorystyki i theme
    split views / multiple simultaneous viewports
    camera persistence between application runs
    future Datum object model

## Następny work contract

Pierwszy kontrakt implementacyjny po ADR-0003 powinien dotyczyć wyłącznie fundamentu wspólnego Workbench/Viewera.

Powinien udowodnić:

    open PartDocument
        ↓
    CAD Workbench

    LEFT
        generic Document Tree

    CENTER
        shared OCCT-backed 3D Viewport
        ViewCube
        Document Origin presentation
        reference grid
        pan / orbit / zoom
        standard views
        ortho / perspective

    RIGHT
        Properties
        Operations infrastructure

    BOTTOM
        Document Tabs

    BELOW
        Status / Diagnostics

oraz przygotować wspólne granice dla selection set + primary selection, Tree multi-selection, context actions, persistent visibility, ActiveDocumentSession i przyszłego Sketch editor context.

Kontrakt nie powinien jeszcze implementować Sketch Core, Sketch entities, Body, Feature, Extrude, Assembly, Drawing ani persistent topology naming.
