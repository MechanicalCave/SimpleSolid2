# WB-01A — Workbench Stabilization & Document Dialog UX

**Status:** ACCEPTED — REOPENED FOR FOLLOW-UP FIXES  
**Owner acceptance:** 2026-09-23  
**Owner follow-up acceptance:** 2026-09-24  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Powiązane ADR:** ADR-0002, ADR-0003  
**Parent work:** WB-01

## 1. Cel

Ustabilizować ukończony WB-01 przed rozpoczęciem Sketch/Body/Feature work oraz usunąć ujawnione przez realne użycie luki UX w tworzeniu/otwieraniu Dokumentów i responsywności nawigacji.

WB-01A ma udowodnić:

```text
open Project
→ create/open Part through document-oriented dialogs
→ use shared CAD Workbench
→ repeatedly click/select in native 3D Viewport
→ repeatedly Show/Hide Origin references
→ resize Workbench aggressively
→ ViewCube remains usable and contained
→ no process crash / desktop exit
→ existing authored state and runtime-state rules remain intact
```

WB-01A jest stabilizacją i dopracowaniem istniejącego Workbencha. Nie aktywuje nowej domeny CAD.

## 2. Priorytety

Realizacja jest podzielona na dwa poziomy:

### P0 — stabilność

P0 musi być zakończone i zweryfikowane przed pracami P1:

- crash przy klikaniu w przestrzeń 3D;
- crash przy powtarzanym Show/Hide Origin;
- lifecycle provider-native detected/hover/selection objects podczas wymiany sceny;
- bezpieczna granica błędów/wyjątków providera;
- natywne stress/regression tests.

### P1 — UX

Po zielonym P0:

- responsywny ViewCube;
- Workspace Location Picker dla tworzenia Dokumentów;
- neutralne `Open…` zamiast `Open Part…`;
- dokumentacja as-built PL/EN.

## 3. Zakres IN — stabilność Viewportu

WB-01A obejmuje:

- deterministyczny native Windows stress test realnego Qt/OCCT Viewera;
- powtarzane sekwencje:
  - viewport click / empty click;
  - selection replace;
  - selection toggle;
  - scene/reference refresh;
  - Show/Hide Origin;
  - Undo/Redo visibility;
  - camera navigation;
- provider-native state cleanup przy usuwaniu/zastępowaniu presentation objects;
- gwarancję, że usunięte presentation objects nie pozostają jako aktywne detected/hovered/selected native handles;
- fail-closed containment recoverable provider/kernel failures na granicy Viewera;
- brak silent corruption selection authority lub persistent visibility po błędzie providera;
- brak modalnego debugger/runtime dialogu jako oczekiwanego mechanizmu test failure.

Dokładny mechanizm OCCT cleanup/exception handling jest D1, o ile zachowuje powyższe invariants i provider boundary ADR-0003.

## 4. Zamrożony input grammar Viewportu

WB-01A zamraża podstawową semantykę myszy:

```text
Left click                 → replace semantic selection
Ctrl + Left click          → toggle semantic selection member
Left click on empty space  → clear semantic selection
Middle drag                → Pan
Shift + Middle drag        → Orbit
Mouse wheel                → Zoom
Right click                → reserved for context menu
```

W WB-01A sam prawy klik:

- nie zmienia semantic selection;
- nie zmienia kamery;
- nie wykonuje ukrytej operacji modelowej;
- nie może powodować crasha.

Jeżeli context menu dla danego miejsca nie istnieje, prawy klik może być no-op.

Hover/detection pozostaje runtime presentation state i nie jest semantic selection.

## 5. Scene replacement safety

Zmiana persistent Origin visibility może wymagać aktualizacji sceny providera, ale:

- semantic Origin references nie zmieniają identity;
- Tree selection authority nie może zostać utracona przypadkiem;
- provider musi przed usunięciem/zastąpieniem native presentation objects unieważnić lub bezpiecznie odłączyć zależny detected/hover/selection state;
- scene update nie może pozostawiać dangling provider-native references;
- repeated update musi być idempotentny względem authored state;
- failure providera nie może zmieniać Part authored visibility poza już zaakceptowaną semantic transaction.

WB-01A nie przenosi OCCT selection ownership do domeny ani UI.

## 6. Responsywny ViewCube

Obecny wieloprzyciskowy układ ViewCube zostaje zastąpiony/skomponowany jako kompaktowy CAD navigation widget.

Wymagania:

- widget pozostaje overlayem Editor Surface;
- nie może narzucać dużego minimum width na Viewport;
- nie może nachodzić na Properties/Operations ani wychodzić poza Editor Surface;
- resizing splittera i całego okna nie może powodować overlap/corruption/crash;
- przy małej szerokości przechodzi w kompaktowy wariant zamiast ściskać pełną siatkę;
- zachowuje dostęp do:
  - Front / Back / Left / Right / Top / Bottom;
  - Isometric / corner orientations;
  - Fit;
  - Orthographic / Perspective;
- steruje wyłącznie przez provider-neutral `IDocumentViewport`;
- kamera pozostaje runtime-only i nie dirty'uje Dokumentu.

Dokładny pixel threshold, styling i finalne ikony są D1.

## 7. Workspace Location Picker — New Document

Tworzenie Parta ma używać wspólnego, dokumentowo-neutralnego Workspace Location Picker zamiast ręcznego tekstowego wpisywania ścieżki.

Picker musi:

- pokazywać hierarchię katalogów bieżącego Workspace;
- pozwalać wybrać katalog docelowy;
- pozwalać wpisać nazwę nowego Dokumentu;
- udostępniać `Create Folder`;
- tworzyć folder wyłącznie wewnątrz bieżącego Workspace;
- nie pozwalać wyjść poza Workspace przez absolute path, `..` lub równoważne obejście;
- nie eksponować `.simplesolid` jako zwykłego miejsca zapisu Dokumentu;
- failować closed, gdy target file już istnieje;
- nie tworzyć ukrytych katalogów nadrzędnych bez jawnej akcji użytkownika;
- zachować current Part lifecycle i atomic publication.

W WB-01A picker jest używany przez `New Part…`, ale jego kontrakt UI nie może zależeć od PartDocument semantics.

## 8. Open Document UX

Workbench action zostaje nazwana:

```text
Open…
```

lub równoważnie `Open Document…` w miejscu, gdzie pełna nazwa poprawia czytelność.

Dialog ma być dokumentowo-neutralny:

- pokazuje rodzaj Dokumentu;
- pokazuje nazwę/tytuł i lokalizację;
- current implementation może dostarczać wyłącznie `Part`, ponieważ Assembly/Drawing nie istnieją;
- invalid native files i identity conflicts pozostają widoczne, ale nieopenable;
- otwarcie resolved DocumentId nadal reuse'uje canonical DocumentSession/tab;
- dialog nie tworzy fake Assembly/Drawing entries;
- backend może pozostać Part-only tam, gdzie nie ma jeszcze wspólnego multi-kind discovery contract.

WB-01A nie wprowadza AssemblyDocument ani DrawingDocument.

## 9. Granice architektury

WB-01A nie zmienia:

- ProjectId / DocumentId semantics;
- ProjectSession / DocumentSession ownership;
- Part authored-state ownership;
- persistent Origin identity/visibility meaning;
- selected set + primary selection authority;
- provider-neutral Viewer boundary;
- Workbench Shell responsibility;
- `.ss2part` schema meaning poza ewentualną naprawą błędu kompatybilną z obecnym v2.

Jeżeli naprawa crasha wymaga zmiany którejkolwiek z tych reguł, praca zatrzymuje się do jawnej decyzji Ownera.

## 10. Zakres OUT

WB-01A nie obejmuje:

```text
Sketch Core
Sketch Edit
Sketch entities
constraint solver
Body
Feature
Extrude
modeled Part B-Rep
geometry Kernel API expansion
topology naming
AssemblyDocument
DrawingDocument
BOM
Material
Save As / Save Copy As
identity-conflict repair
multi-document-kind persistence
camera persistence across restart
final ribbon/menu system
```

## 11. Testy akceptacyjne — P0

P0 musi udowodnić co najmniej:

1. Native Windows Qt/OCCT Viewer tworzy się i zamyka bez crasha.
2. Repeated left-click selection nie crashuje.
3. Repeated empty-space click/clear nie crashuje.
4. Repeated Ctrl+left toggle nie crashuje.
5. Repeated right-click/no-op path nie crashuje.
6. Repeated Show/Hide jednego Origin reference nie crashuje.
7. Repeated batch Show/Hide wielu Origin references nie crashuje.
8. Show/Hide przeplatane Viewport pickiem nie crashuje.
9. Show/Hide przeplatane Pan/Orbit/Zoom nie crashuje.
10. Undo/Redo przeplatane scene refresh nie crashuje.
11. Stress sequence wykonuje wielokrotne iteracje, nie tylko pojedynczy happy path.
12. Selection authority po stress sequence pozostaje valid.
13. Authored visibility po stress sequence odpowiada semantic commands.
14. Provider failure nie powoduje silent authored-state mutation.
15. Wszystkie istniejące WB-01/PART-01 testy pozostają PASS.

Domyślny native stress gate powinien wykonywać co najmniej 100 iteracji reprezentatywnej sekwencji, chyba że udokumentowany limit środowiska testowego wymaga równoważnego deterministycznego rozwiązania.

## 12. Testy akceptacyjne — P1

P1 musi udowodnić co najmniej:

1. ViewCube pozostaje wewnątrz Editor Surface podczas resize.
2. Bardzo wąski Viewport nie powoduje overlap ani crasha.
3. Compact ViewCube nadal umożliwia podstawową orientację, Fit i projection switch.
4. Navigation nadal nie dirty'uje Document.
5. New Part pokazuje folder tree bieżącego Workspace.
6. `Create Folder` tworzy katalog pod Workspace.
7. Próba zapisu poza Workspace jest odrzucona.
8. `.simplesolid` nie jest zwykłym targetem Document creation.
9. Existing target file failuje closed.
10. New Part nadal tworzy prawidłowy canonical DocumentSession.
11. Workbench eksponuje `Open…`, nie Part-specific top-level Open action.
12. Open dialog pokazuje current Document kind = Part.
13. Invalid/conflict entries pozostają visible but disabled.
14. Existing canonical session/tab jest reuse'owany.
15. PL/EN product docs i internal as-built docs odpowiadają finalnemu UX.
16. Generated Product Browser pozostaje Git-clean.
17. Pełny exact-head docs/verify/build/CTest gate jest PASS.

## Documentation impact

Internal docs: required  
User/Product docs: required  
Reason: WB-01A materialnie zmienia zachowanie Viewport input, ViewCube responsywność oraz tworzenie/otwieranie Dokumentów.

Internal docs muszą opisać provider hardening/stress coverage, input grammar, responsive ViewCube oraz shared Workspace Location Picker/Open Document UX.

Product docs PL/EN muszą opisać finalne kliknięcia/nawigację, tworzenie Parta z folder tree/Create Folder oraz neutralne `Open…`.

Generated Product Browser musi zostać odświeżony i pozostać Git-clean.

## 13. Sekwencja realizacji

WB-01A może być realizowany kilkoma PR-ami pod jednym kontraktem.

Rekomendowana kolejność:

```text
Slice A
native crash reproduction + stress gate + provider hardening

Slice B
responsive compact ViewCube + resize regression tests

Slice C
Workspace Location Picker + Create Folder + Open… dialog

Slice D
as-built docs + PL/EN + Product Browser + completion
```

P1 nie zaczyna się przed zielonym P0.

## 14. Kryterium ukończenia

WB-01A jest ukończony, gdy użytkownik może:

```text
open Project
→ create Part in a chosen Workspace folder
→ optionally create that folder
→ open Documents through Open…
→ resize the Workbench aggressively
→ use a compact usable ViewCube
→ repeatedly click/select in the native 3D Viewport
→ repeatedly Show/Hide Origin references
→ Undo/Redo
→ navigate
→ continue working without process crash
```

przy zachowaniu:

```text
no Sketch
no Body/Feature
no modeled solid
no Assembly
no Drawing
no OCCT leakage into domain/UI contracts
no change to Document identity semantics
```

Ukończenie WB-01A nie uruchamia automatycznie następnego etapu CAD.


## 15. Owner-approved follow-up — 2026-09-24

Po finalnym ręcznym teście Owner ponownie otworzył WB-01A wyłącznie dla dwóch defektów mieszczących się w pierwotnym zakresie P1:

1. **ViewCube native repaint regression**
   - podczas zmiany szerokości prawego panelu / Editor Surface poprzedni obszar ViewCube może pozostawiać artefakty nad natywnym Qt/OCCT Viewportem;
   - naprawa ma zapewnić poprawne odświeżenie starego i nowego obszaru overlayu również nad realnym native Viewerem;
   - nie zmienia provider-neutral Viewer API ani camera semantics.

2. **Open Document interactive columns**
   - kolumny `Kind / Name / Location / Status` mają być ręcznie resizable przez użytkownika;
   - domyślnie `Name` ma być wyraźnie węższe niż obecnie, a `Location` szersze;
   - szerokości nie są w tym follow-upie persystowane między restartami.

Jawnie **poza zakresem follow-upu** pozostają:
- tile/icon view;
- thumbnails / preview generation;
- thumbnail cache;
- zmiany native Document format/persistence.

Te tematy nie są częścią WB-01A. Następnym planowanym większym krokiem po ponownym zamknięciu WB-01A jest osobny D2 contract dotyczący architektury natywnych formatów/persistence Part/Assembly/Drawing przed implementacją Sketchera.

### Follow-up acceptance

Follow-up jest ukończony, gdy:
- ręczne/native resize nie pozostawia artefaktów ViewCube;
- automatyczne testy obejmują repaint/invalidation path możliwy do zweryfikowania mechanicznie oraz dotychczasową geometry containment regression;
- nagłówki Open Document są interaktywne i mają zaakceptowane domyślne proporcje;
- dokumentacja as-built/product pozostaje zgodna;
- exact-head docs/verify/build/CTest gate jest PASS.
