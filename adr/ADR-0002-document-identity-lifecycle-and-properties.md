# ADR-0002 — Tożsamość, lifecycle i właściwości Dokumentów CAD

**Status:** ACCEPTED  
**Owner acceptance:** 2026-09-23  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Zakres:** Document identity / portability / discovery / DocumentSession / persistence / properties / export identity semantics

## Kontekst

Foundation definiuje `Document` jako niezależnie persystowany semantyczny element CAD ze stabilnym `DocumentId`, niezależnym od nazwy pliku, ścieżki i display name.

SS2 rozpoznaje trzy trwałe rodzaje Dokumentów:

```text
PartDocument
AssemblyDocument
DrawingDocument
```

Project i Document mają różne zakresy tożsamości.

Project zapewnia kontekst Workspace, discovery/resolution oraz runtime `ProjectSession`, ale nie jest właścicielem tożsamości Dokumentu.

W praktycznym workflow CAD Dokumenty muszą być przenośne pomiędzy Projektami. Użytkownik może skopiować Part lub Assembly do innego Workspace, może przechowywać kopie robocze, eksportować całą maszynę lub jej fragment oraz reorganizować strukturę katalogów bez zmiany technicznej tożsamości Dokumentu.

Jednocześnie zwykłe operacje filesystemu mogą utworzyć kilka plików deklarujących ten sam `DocumentId` wewnątrz jednego Workspace.

SS2 musi rozróżniać:

```text
identity conflict
```

od:

```text
filename/path conflict
```

oraz od zwykłego istnienia tej samej logicznej tożsamości w dwóch niezależnych Workspace.

Dokumenty potrzebują również trwałych właściwości inżynierskich używanych później przez Drawing, BOM, eksport, wyszukiwanie i automatyzację. Właściwości te nie mogą być utożsamione z filename, path ani technicznym `DocumentRevision`.

## Decyzja

### 1. Document jest przenośną tożsamością niezależną od Project

`DocumentId` określa trwałą logiczną tożsamość Dokumentu niezależnie od:

```text
ProjectId
Workspace path
directory
filename
display name
```

PartDocument i AssemblyDocument nie posiadają obligatoryjnego `owner ProjectId`.

Skopiowanie Dokumentu do innego Workspace nie wymaga zmiany jego `DocumentId`.

Przyjęcie istniejącego Dokumentu przez inny Project oznacza jego discovery/resolution w nowym Workspace, a nie przejęcie jego tożsamości przez Project.

W szczególności poprawny jest stan:

```text
Project A
    Parts/Gear.ss2part -> DocumentId X

Project B
    Imported/Gear.ss2part -> DocumentId X
```

Obie lokalizacje reprezentują kopie tej samej logicznej tożsamości Dokumentu.

`DocumentId` nie oznacza globalnego singletonu i nie zapewnia synchronizacji rozbieżnych filesystemowych kopii.

### 2. Copy zachowuje tożsamość, jawne Duplicate tworzy nową

Semantyka Dokumentów odpowiada zasadzie przyjętej dla Project identity w ADR-0001.

```text
filesystem copy
→ ten sam DocumentId

move / rename
→ ten sam DocumentId

Save As
→ ten sam DocumentId
→ nowa aktywna lokalizacja

explicit Duplicate / Save Copy As / Make Independent
→ nowy DocumentId
```

SS2 nie zmienia `DocumentId` tylko dlatego, że plik został przeniesiony, przemianowany albo skopiowany.

Powstanie nowej tożsamości wymaga jawnej operacji użytkownika.

### 3. Jeden DocumentId musi rozwiązywać się jednoznacznie wewnątrz Workspace

W ramach jednego Workspace obowiązuje:

```text
DocumentId -> 0 locations
    Missing

DocumentId -> 1 native Document
    Resolved

DocumentId -> N native Documents
    IdentityConflict
```

Jeżeli kilka odkrytych Dokumentów w jednym Workspace deklaruje ten sam `DocumentId`, system nie może samowolnie wybrać jednego z nich.

Resolution po tym `DocumentId` failuje jawnie i fail-closed.

SS2 pokazuje użytkownikowi wszystkie wykryte lokalizacje konfliktu w postaci ścieżek względnych względem Workspace.

Dodatkowe informacje diagnostyczne, takie jak nazwa Dokumentu, typ, schema version, timestamp, rozmiar lub fingerprint zawartości, mogą pomagać użytkownikowi w decyzji, ale nie zastępują `DocumentId` i nie stanowią trwałej tożsamości.

### 4. Rozwiązanie konfliktu tożsamości jest jawne

Użytkownik może wskazać, który Dokument zachowuje istniejący `DocumentId`.

Pozostałe skonfliktowane pliki mogą zostać usunięte, przeniesione poza rozwiązywany Workspace albo otrzymać nowe `DocumentId` przez jawną operację zmiany tożsamości.

Zmiana `DocumentId` nigdy nie odbywa się po cichu.

Sam wybór jednej kopii do inspekcji nie oznacza rozwiązania konfliktu, jeżeli pozostałe Dokumenty z tym samym ID pozostają w aktywnym zakresie discovery Workspace.

Po poprawnym rozwiązaniu konfliktu stary `DocumentId` wskazuje Dokument wybrany do zachowania tej tożsamości.

Istniejące semantyczne referencje do starego `DocumentId` pozostają referencjami do tej tożsamości.

Jeżeli użytkownik chce, aby określona referencja wskazywała Dokument, któremu nadano nowe ID, wymagane jest jawne retargetowanie tej referencji.

SS2 nie zgaduje historycznej intencji na podstawie ścieżki, timestampu, nazwy lub podobieństwa zawartości.

### 5. Project-level discovery/index jest mechanizmem resolution, nie źródłem tożsamości

Filesystem Workspace oraz embedded identities Dokumentów pozostają autorytatywne dla fizycznej zawartości Projectu zgodnie z Foundation.

ProjectSession może utrzymywać rebuildable:

```text
Document Registry
Document Index
Discovery State
```

mapujące przykładowo:

```text
DocumentId -> workspace-relative path
```

w celu szybkiego resolution, wykrywania konfliktów i obsługi przyszłych referencji Assembly/Drawing.

Taki index/catalog jest pochodny i odbudowywalny.

Nie staje się ukrytą trwałą bazą własności Dokumentów i nie zastępuje `DocumentId` zapisanego w samym Dokumencie.

### 6. Document nie wymaga wspólnej mutowalnej klasy bazowej

Architektura rozróżnia wspólne mechanizmy Dokumentu od semantyki konkretnych domen.

Preferowany kształt to:

```text
shared/core contracts
    DocumentId
    DocumentKind
    DocumentRevision
    common document properties
    persistence/version primitives

domain aggregates
    PartDocument
    AssemblyDocument
    DrawingDocument
```

ADR nie wymaga wspólnego mutowalnego C++ base class `Document`.

Concrete domain Document pozostaje właścicielem swojej authored semantics.

Ewentualny wspólny interfejs runtime ma być mały i nie może stać się drugim właścicielem modelu domenowego.

### 7. DocumentSession jest runtime wrapperem jednego otwartego Dokumentu

Otwarcie trwałego Dokumentu tworzy runtime `DocumentSession`.

DocumentSession może koordynować:

```text
loaded domain Document
current physical location
save checkpoint
Undo / Redo
active transaction/edit context
evaluation state/cache
runtime diagnostics
selection/presentation bindings
```

DocumentSession nie jest trwałą design intent i nie posiada drugiej kopii semantycznego modelu.

W jednym `ProjectSession` jeden jednoznacznie rozwiązany `DocumentId` posiada co najwyżej jedną kanoniczną aktywną DocumentSession.

Ponowne żądanie otwarcia tego samego rozwiązanego Dokumentu powinno prowadzić do istniejącej sesji, a nie tworzyć konkurencyjną drugą sesję mutującą tę samą logiczną instancję.

### 8. Save, Save As i Save Copy As mają różną semantykę

`Save` zapisuje bieżący Dokument w jego aktualnej lokalizacji i zachowuje `DocumentId`.

`Save As` relokuje bieżący Dokument do nowej lokalizacji i zachowuje jego `DocumentId`.

`Save Copy As` / `Duplicate` tworzy niezależny Dokument z nowym `DocumentId` i nie zmienia aktywnej tożsamości ani save checkpointu bieżącej DocumentSession.

Publikacja trwałego pliku używa bezpiecznej/atomowej semantyki persistence.

Save checkpoint może zostać przesunięty dopiero po potwierdzonym sukcesie trwałego zapisu.

Nieudany zapis nie może oznaczyć Dokumentu jako saved/clean.

### 9. Persistujemy authored semantics, nie derived geometry

Native Document przechowuje trwałą tożsamość, rozpoznawalny rodzaj/schema Dokumentu oraz authored semantics należącą do domeny.

Persistence nie zapisuje jako źródła prawdy:

```text
OCCT handles
TopoDS identity
evaluated B-Rep identity
viewer objects
tessellation cache
runtime dependency graph
DocumentSession state
Undo/Redo history
```

Part persistence należy semantycznie do Part domain.

Shared persistence posiada jedynie wspólne mechanizmy takie jak format/version recognition, atomic replace, helpers i migration dispatch.

### 10. Document Properties są trwałą authored semantics

PartDocument, AssemblyDocument i DrawingDocument posiadają trwałe właściwości dokumentowe niezależne od filename/path.

Architektura przewiduje wspólne pojęcia takie jak:

```text
document number
title / engineering name
description
engineering revision
```

oraz przyszły mechanizm custom properties.

Dokładne nazwy serialized keys, typy API i UI edytora właściwości nie są zamrażane przez ten ADR.

Właściwości te mogą być używane przez Drawing, BOM, export, automation oraz wyszukiwanie.

Przykładowo źródłowy plik:

```text
Part001.ss2part
```

może posiadać:

```text
Number: 12-04-117
Title:  Wał napędowy
```

i podczas eksportu zostać opublikowany jako:

```text
12-04-117 - Wał napędowy.ss2part
```

bez zmiany `DocumentId` i bez zmiany źródłowej nazwy pliku.

### 11. Engineering revision i DocumentRevision są różnymi pojęciami

Techniczne:

```text
DocumentRevision
```

służy runtime consistency, Commands, Transactions, Undo/Redo i synchronizacji evaluated state.

Użytkowe:

```text
EngineeringRevision
```

jest authored property Dokumentu, przykładowo:

```text
A
B
C
01
02
```

Te dwa pojęcia nie mogą być utożsamione.

### 12. Właściwości domenowe pozostają własnością domeny

Nie wszystkie wartości widoczne w tabeli Properties są common Document Properties.

Przykładowo materiał Parta jest semantyką Part:

```text
PartDocument
    material assignment
```

a nie fundamentalnym:

```text
CommonDocumentProperties.material = string
```

Part może udostępniać efektywny materiał jako właściwość odczytywaną przez Drawing, BOM lub export.

Pozwala to później rozwinąć materiał o density, mass properties, manufacturing semantics lub multi-body assignment bez migracji z przypadkowego płaskiego string property.

Analogicznie quantity nie jest właściwością PartDocument. Ilość wynika z wystąpień `OccurrenceId` w Assembly i z kontekstu BOM.

### 13. Property mutation używa wspólnej ścieżki Commands/Transactions

Zmiana trwałej właściwości Dokumentu jest semantic mutation.

Obowiązuje:

```text
UI / Script / AI
        ↓
Semantic Command
        ↓
Validation
        ↓
Transaction
        ↓
Owning Domain Document
```

Property editor nie mutuje bezpośrednio authored state poza tą ścieżką.

Udany semantic commit zmienia trwały stan i odpowiednio przesuwa techniczny `DocumentRevision`.

No-op nie tworzy fałszywej zmiany.

Odrzucona lub nieudana operacja nie może częściowo zmienić Dokumentu ani historii.

Undo/Redo reprezentują nowe semantic mutations; techniczny revision timeline pozostaje monotoniczny zamiast cofać numer rewizji do historycznej wartości.

### 14. Save state i evaluation state są niezależne

SS2 rozdziela:

```text
authored state / needsSave
```

od:

```text
evaluation validity / needsRecompute
```

Poprawna authored mutation może zostać committed nawet wtedy, gdy jej późniejsza evaluation zakończy się błędem.

Dokument z nieudaną evaluation może zostać zapisany.

Kernel/provider nie jest arbitrem tego, czy użytkownik może zachować swoją authored design intent.

Po restarcie derived evaluation jest odbudowywana z persistent authored state.

### 15. Invalidation zależy od znaczenia zmiany

Nie każda authored mutation wymaga recompute całej geometrii.

Przykładowo:

```text
title/name change
→ persistence dirty
→ geometry może pozostać valid

material change
→ persistence dirty
→ B-Rep może pozostać valid
→ derived physical properties mogą wymagać odświeżenia

feature geometry change
→ persistence dirty
→ Part geometry invalid
```

ADR nie wymaga globalnego dependency graphu ani mechanizmu invalidation na poziomie wszystkich przyszłych subsystemów.

Każda domena pozostaje właścicielem swoich evaluation semantics.

### 16. OCCT pozostaje wyłącznie za Kernel API

Part, Assembly, Drawing, persistence, Document Properties i durable references nie zależą od typów OCCT.

Runtime evaluator może otrzymywać provider-neutral handles do derived geometry przez Kernel API.

Żaden `TopoDS_*`, provider handle, topology ordinal lub native OCCT identity nie może stać się `DocumentId`, authored reference ani persistent property identity.

### 17. Filename conflict podczas exportu nie jest identity conflict

Export może publikować wiele różnych Dokumentów o różnych `DocumentId`, których źródłowe lub wygenerowane nazwy plików są identyczne.

Przykład:

```text
GearboxA/shaft.ss2part -> DocumentId A
GearboxB/shaft.ss2part -> DocumentId B
```

Przy publikacji do jednego katalogu powstaje filename conflict, ale nie identity conflict.

SS2 rozwiązuje taki konflikt przez wybór unikalnych export-relative paths / filenames.

Nie zmienia z tego powodu `DocumentId`.

### 18. Native model export działa na tożsamościach i zależnościach, nie na przypadkowej strukturze folderów

Przyszły eksport całej maszyny lub fragmentu modelu powinien rozpoczynać się od wybranego Document/DocumentId i obliczać wymagany closure trwałych referencji.

Filesystem hierarchy nie definiuje hierarchy modelu.

Konceptualnie:

```text
selected root Document(s)
        ↓
resolve Document references
        ↓
compute export closure
        ↓
assign export-relative locations
        ↓
publish Documents
```

Eksport może zachować strukturę źródłową albo świadomie ją spłaszczyć.

Przy spłaszczaniu kolidujące nazwy otrzymują deterministyczne unikalne nazwy wynikowe.

Sam export nie zmienia tożsamości Dokumentów, chyba że użytkownik jawnie wybierze osobną operację tworzenia nowych niezależnych tożsamości.

Pakiet eksportowy może zawierać manifest mapujący:

```text
DocumentId -> exported relative path
```

Manifest opisuje konkretną publikację/export.

Nie zastępuje embedded `DocumentId` i nie staje się źródłem CAD identity.

### 19. Drawing, BOM i export konsumują właściwości bez kopiowania źródła prawdy

Przyszły Drawing może odczytywać właściwości własnego DrawingDocument oraz właściwości semantycznego source Document.

Nie zakłada się, że:

```text
Drawing number == Part number
```

zawsze.

Drawing ma własną tożsamość i może mieć własne właściwości.

Title block, BOM i export korzystają ze stabilnego znaczenia właściwości, nie z przetłumaczonych etykiet UI.

Dokładny binding/template language pozostaje deferred.

## Pierwsza sekwencja implementacyjna

Pierwszy work contract dla Part lifecycle powinien udowodnić platformowy lifecycle Dokumentu bez wprowadzania jeszcze modelowania geometrycznego.

Oczekiwany przepływ:

```text
opened Project
    ↓
create empty PartDocument
    ↓
assign stable DocumentId
    ↓
set basic Document Properties
    ↓
open/own DocumentSession
    ↓
semantic property mutation through Command/Transaction
    ↓
Undo / Redo
    ↓
Save
    ↓
Close
    ↓
restart application
    ↓
reopen same Project
    ↓
discover/open same PartDocument
    ↓
same DocumentId + same authored properties
```

Pierwszy slice nie potrzebuje Sketch, Extrude, realnego Part feature modelu ani OCCT modeling.

Evaluation boundary ma pozostać zgodny z architekturą, ale dla pustego Part może być trywialny.

## Konsekwencje

Model zapewnia trwałą tożsamość Dokumentów niezależną od struktury katalogów i pozwala naturalnie kopiować Part/Assembly między Projektami.

Assembly i Drawing mogą później referencjonować źródła przez `DocumentId`, a nie filename/path.

Filesystemowe kopie pozostają wiernymi kopiami logicznej tożsamości.

Konflikt kilku kopii tej samej tożsamości wewnątrz jednego Workspace jest jawny i nie prowadzi do niedeterministycznego resolution.

Document Properties tworzą stabilną podstawę dla późniejszego Drawing, BOM, exportu i automatyzacji bez wiązania tych funkcji z filename.

Rozdzielenie authored state, save state i evaluation state pozwala zachować nieukończoną lub chwilowo geometrycznie niepoprawną pracę bez uzależniania persistence od kernela.

Kosztem jest konieczność jawnego conflict-resolution, utrzymywania rebuildable document discovery/index oraz zachowania ścisłego rozdziału pomiędzy identity, location, user properties i derived state.

## Poza zakresem tej decyzji

ADR nie zamraża dokładnego binarnego/tekstowego formatu native files, rozszerzeń plików, serialized encoding `DocumentId`, C++ class layout, finalnego property API, pełnego systemu custom-property types, materiałowej bazy danych, BOM engine, Drawing title-block language, semantic diff/merge Dokumentów, synchronizacji filesystemowych klonów, konfiguracji Assembly, globalnego dependency scheduler, persistent naming algorithm Parta ani konkretnego feature/history modelu.

Te elementy wymagają osobnych późniejszych decyzji lub work contracts wtedy, gdy pojawi się konkretny wymóg produktu.

## Relacja do Foundation

ADR uszczegóławia istniejące CORE zasady Foundation dotyczące `DocumentId`, filesystem authority, ProjectSession/DocumentSession, Commands/Transactions, domain-owned persistence, derived evaluation oraz provider-neutral Kernel API.

Nie redefiniuje Project/Document modelu, nie przenosi własności semantyki pomiędzy domenami i nie wymaga Foundation Amendment.
