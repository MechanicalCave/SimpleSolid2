# PART-01 — Trwały lifecycle pustego PartDocument

**Status:** ACCEPTED  
**Owner acceptance:** 2026-09-23  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Powiązany ADR:** `ADR-0002-document-identity-lifecycle-and-properties.md`

## 1. Cel

Zaimplementować pierwszy rzeczywisty lifecycle CAD Document w SS2 poprzez minimalny `PartDocument`, bez modelowania geometrycznego.

Kontrakt ma udowodnić:

```text
Opened Project
→ Create Part
→ stable DocumentId
→ persistent empty PartDocument
→ DocumentSession
→ edit common Document Properties
→ Command / Transaction
→ Undo / Redo
→ Save
→ Close
→ restart application
→ reopen same Project
→ discover same Part
→ reopen same DocumentId
→ same authored properties
```

PART-01 buduje platformowy fundament przyszłych Part, Assembly i Drawing, ale implementuje wyłącznie domenę Part w minimalnym zakresie potrzebnym do udowodnienia lifecycle.

Nie implementuje jeszcze modelowania 3D.

## 2. Zakres IN

PART-01 obejmuje:

- silnie typowany `DocumentId`, odrębny od `ProjectId`;
- `DocumentKind` potrzebny do rozpoznania `PartDocument`;
- techniczny `DocumentRevision`;
- wspólne trwałe `DocumentProperties`;
- minimalny `PartDocument`;
- minimalną transakcję domenową Part;
- semantyczną ścieżkę Commands / Transactions;
- runtime `DocumentSession`;
- runtime Undo / Redo;
- save checkpoint i `needsSave`;
- trwały natywny plik Part;
- bezpieczny/atomowy zapis;
- discovery Part documents wewnątrz bieżącego Workspace;
- runtime index/resolution `DocumentId → path`;
- wykrywanie konfliktów wielu plików z tym samym `DocumentId`;
- minimalny user flow Create / Open / Edit Properties / Undo / Redo / Save / Close;
- ponowne odkrycie i otwarcie Dokumentu po restarcie aplikacji;
- automatyczne testy lifecycle, identity, persistence i failure behavior;
- aktualizację wymaganej dokumentacji internal oraz PL/EN product docs.

## 3. Natywny Part Document

PART-01 przyjmuje natywne rozszerzenie:

```text
.ss2part
```

Rozszerzenie identyfikuje natywny top-level `PartDocument` SS2.

Przykład:

```text
Part001.ss2part
```

Powód użycia `.ss2part`:

- jednoznacznie wskazuje format SS2;
- nie sugeruje kompatybilności binarnej z SS1;
- jest czytelne dla użytkownika i filesystemu;
- pozwala później wprowadzić odpowiednio odrębne formaty Assembly i Drawing.

Dokładna wewnętrzna reprezentacja tekstowa/binarna nie jest publiczną semantyką tego kontraktu.

Format musi jednak posiadać rozpoznawalną wersję/schema i umożliwiać kontrolowane odrzucenie nieobsługiwanej wersji.

## 4. DocumentId

`DocumentId`:

- jest własnym typem semantycznym;
- nie jest aliasem `ProjectId`;
- jest generowany przy utworzeniu nowego Parta;
- pozostaje stabilny przez Save, Close i restart;
- jest niezależny od path, filename i properties;
- nie zawiera `ProjectId`;
- jest zapisany wewnątrz natywnego Dokumentu;
- nie jest OCCT/Qt/Viewer/runtime handle;
- nie zmienia się po filesystemowym rename/move/copy.

Dokładny generator i serialized encoding mogą pozostać implementacyjnym szczegółem wersjonowanego formatu.

Publiczny kontrakt nie może uzależnić się od konkretnej tekstowej reprezentacji ID.

## 5. Początkowe tworzenie Parta

`Create Part` działa wyłącznie w obrębie aktualnie otwartego Project Workspace.

Minimalny przepływ:

```text
New Part
→ wybór lokalizacji/nazwy wewnątrz Workspace
→ default filename suggestion
→ generate fresh DocumentId
→ create valid empty PartDocument
→ atomically publish native file
→ create canonical DocumentSession
```

UI może proponować nazwę:

```text
Part001.ss2part
Part002.ss2part
...
```

jako wygodny default.

Nazwa ta jest tylko filename.

Nie staje się trwałą semantyczną tożsamością ani automatycznym `document.number`.

Istniejący plik nie może zostać cicho nadpisany.

PART-01 nie wymaga jeszcze lifecycle dokumentu „Untitled” bez fizycznej lokalizacji. Nowy Part otrzymuje docelową lokalizację podczas tworzenia.

## 6. Document Properties

Minimalny trwały zestaw common properties:

```text
Number
Title
Description
EngineeringRevision
```

Wartości mogą początkowo być puste.

Properties:

- należą do authored Document semantics;
- są niezależne od filename/path;
- przeżywają Save / Close / restart;
- są edytowane wyłącznie poprzez semantic Commands;
- uczestniczą w Undo/Redo;
- powodują `needsSave`, jeżeli zmieniają trwały stan.

PART-01 nie implementuje jeszcze:

```text
Material
Custom Properties
BOM
Drawing title block bindings
export filename templates
```

W szczególności `Material` nie może zostać zasymulowany jako przypadkowy common string property.

## 7. DocumentRevision

`DocumentRevision` jest technicznym licznikiem spójności modelu.

W obrębie aktywnego lifecycle Dokumentu:

```text
successful semantic mutation
→ DocumentRevision increases

Undo
→ semantic mutation
→ DocumentRevision increases

Redo
→ semantic mutation
→ DocumentRevision increases
```

Revision nigdy nie cofa się do historycznej wartości tylko dlatego, że Undo przywróciło wcześniejszy authored state.

No-op:

```text
→ no authored change
→ no new Undo entry
→ no revision increment
```

Rejected/failed command:

```text
→ no partial mutation
→ no revision increment
→ existing Undo/Redo state preserved
```

PART-01 nie ustanawia publicznej gwarancji konkretnej liczbowej wartości `DocumentRevision` po restarcie aplikacji.

`EngineeringRevision` pozostaje całkowicie odrębnym pojęciem.

## 8. PartDocument

Minimalny `PartDocument` posiada authored state niezbędny do tego kontraktu:

```text
DocumentId
DocumentRevision
DocumentProperties
```

oraz pusty Part-domain state przygotowany do późniejszego rozszerzenia.

PART-01 nie tworzy jeszcze:

```text
Sketch
Body
Feature
Extrude
Chamfer
Datum
Sheet Metal
feature history
general dependency graph
```

Nie należy kopiować struktury feature collections z SS1 tylko po to, aby „przygotować się” na przyszłość.

## 9. Mutation path

Każda trwała zmiana properties przechodzi przez:

```text
UI / Script / AI
        ↓
Semantic Command
        ↓
current-context validation
        ↓
PartDocument Transaction
        ↓
atomic PartDocument commit
```

UI nie otrzymuje publicznego mechanizmu mutowania trwałego `PartDocument` bezpośrednio.

Pierwszy kontrakt może użyć wąskiego zestawu Commands potrzebnego do properties.

Dokładne prywatne klasy są D0/D1, o ile zachowana jest powyższa granica.

## 10. Transaction semantics

Part transaction:

- pracuje na staged state;
- nie ujawnia częściowej trwałej mutacji przed commit;
- waliduje wynikowy authored state przed commit;
- commit jest atomowy z punktu widzenia domeny;
- rollback/abandon pozostawia Dokument bez zmiany;
- successful no-op nie tworzy sztucznej rewizji.

PART-01 nie tworzy uniwersalnego frameworku transakcji dla wszystkich możliwych domen ponad to, czego wymaga istniejąca Foundation i ten konkretny use case.

## 11. DocumentSession

Otwarcie Parta tworzy runtime `DocumentSession`.

DocumentSession koordynuje co najmniej:

```text
DocumentId
current physical path
loaded PartDocument
save checkpoint
Undo / Redo history
needsSave
runtime diagnostics
```

Jest runtime-only.

Nie jest serializowany jako authored Part state.

W jednym `ProjectSession`:

```text
one resolved DocumentId
→ at most one canonical active DocumentSession
```

Ponowne żądanie otwarcia tego samego resolved `DocumentId` aktywuje istniejącą sesję zamiast tworzyć drugiego niezależnego mutatora tego samego Dokumentu.

## 12. Undo / Redo i Save checkpoint

Undo/Redo history jest runtime state.

Nie jest zapisywana do `.ss2part`.

Po Close/Reopen:

```text
authored state survives
Undo/Redo history does not
```

Save checkpoint śledzi semantic state zapisany skutecznie na dysku.

Przykładowo:

```text
Save
→ clean

edit Title
→ dirty

Undo back to saved authored state
→ clean

Redo
→ dirty
```

Techniczny `DocumentRevision` może przy tym nadal rosnąć monotonnie.

Dlatego dirty state nie może być naiwnie definiowany jako:

```text
current DocumentRevision != saved numeric revision
```

jeżeli semantyczny history cursor/checkpoint wskazuje dokładnie zapisany authored state.

## 13. Persistence

Native Part persistence przechowuje wyłącznie trwałą semantykę i niezbędne version/identity metadata.

Logicznie plik musi umożliwić odzyskanie:

```text
native document kind
supported schema/format version
DocumentId
DocumentProperties
Part authored state
```

Nie przechowuje jako authoritative state:

```text
Qt objects
Viewer state
OCCT objects / TopoDS handles
evaluated B-Rep
mesh/tessellation
Undo/Redo history
DocumentSession
runtime discovery index
```

Shared persistence może dostarczać mechanizm atomic write/replace, format recognition i normalized errors.

Part domain pozostaje właścicielem znaczenia swojego schema.

## 14. Save

`Save`:

```text
validated authored state
→ serialize to temporary/staged output
→ durable atomic publish/replace
→ only after success update save checkpoint
```

W przypadku failure:

- dotychczasowy poprawny plik nie może zostać częściowo uszkodzony;
- DocumentSession pozostaje dirty;
- użytkownik otrzymuje strukturalną diagnostykę;
- Undo/Redo i authored in-memory state pozostają dostępne.

PART-01 implementuje `Save`.

ADR-0002 definiuje semantykę przyszłych `Save As` i `Save Copy As`, ale ich pełny user flow nie należy do PART-01.

## 15. Workspace discovery

`ProjectSession` otrzymuje minimalny runtime discovery/resolution mechanism dla native Part files w aktualnym Workspace.

Dla PART-01 może to być odbudowywalny scan/index.

Nie wymaga trwałego hidden database.

Discovery opiera się na realnym filesystemie i embedded `DocumentId`.

Przeniesienie lub rename `.ss2part` wewnątrz Workspace:

```text
→ path changes
→ DocumentId does not
```

Po ponownym discovery Dokument jest znajdowany pod nową lokalizacją jako ta sama tożsamość.

Ciągły filesystem watcher nie jest wymagany przez PART-01. Jawne odświeżenie/reopen może odbudować stan discovery.

## 16. IdentityConflict

Jeżeli discovery znajdzie:

```text
A/Gear.ss2part -> DocumentId X
B/Gear.ss2part -> DocumentId X
```

stan dla `X` jest:

```text
IdentityConflict
```

SS2:

- nie wybiera pierwszego pliku;
- nie wybiera najnowszego pliku;
- nie zmienia automatycznie żadnego ID;
- nie używa filename jako tie-breaker;
- pokazuje wszystkie znalezione workspace-relative paths;
- blokuje resolution/open po `DocumentId = X`;
- pozwala nadal korzystać z niezależnych, jednoznacznie rozwiązanych Dokumentów.

Byte-for-byte identyczne kopie również stanowią konflikt resolution.

PART-01 wymaga wykrywania i jawnego raportowania konfliktu.

Pełny conflict-resolution workflow:

```text
choose keeper
assign fresh IDs
retarget references
batch repair
```

pozostaje poza PART-01.

## 17. Invalid native documents

Plik z natywnym rozszerzeniem, którego:

- format jest uszkodzony;
- brakuje wymaganej tożsamości;
- schema version jest nieobsługiwana;
- DocumentId jest niepoprawny;
- declared kind nie odpowiada formatowi;

nie może zostać cicho zignorowany jako poprawny Part.

Discovery zapisuje jawny diagnostic/unavailable state.

Uszkodzenie jednego Parta nie powinno automatycznie uniemożliwiać otwarcia całego Projectu ani niezależnych poprawnych Dokumentów, jeżeli ich resolution pozostaje deterministyczne.

## 18. Minimalny UI lifecycle

Workspace Shell otrzymuje tylko tyle UI, ile potrzeba do udowodnienia lifecycle:

```text
New Part
Open discovered Part
Edit common properties
Undo
Redo
Save
Close Part
```

Dopuszczalny jest minimalny document chooser/list pokazujący co najmniej:

```text
display Title if available
filename / relative path
DocumentId diagnostic identity where useful
conflict / invalid state
```

Nie tworzymy w PART-01 pełnego Project Browsera, Assembly Tree ani CAD feature tree.

## 19. Dirty close

SS2 nie może cicho utracić niezapisanych authored changes.

Próba Close Part / Close Project / exit przy dirty DocumentSession wymaga jawnej decyzji:

```text
Save
Discard
Cancel
```

`Save`:

- próbuje zapisać;
- przy failure nie zamyka dokumentu.

`Discard`:

- zamyka runtime session;
- nie zapisuje zmian.

`Cancel`:

- pozostawia dokument/session otwarte.

## 20. Evaluation / OCCT boundary

PART-01 nie implementuje geometrycznego Part evaluation.

Nie dodaje modelowania OCCT tylko po to, aby udowodnić lifecycle Dokumentu.

Architektura pozostaje zgodna z:

```text
Part authored state
→ future Part evaluation
→ Kernel API
→ OCCT provider
```

Common property mutation nie może wymagać OCCT.

W żadnym pliku PART-01 nie może pojawić się trwała provider-native geometry identity jako część authored Part semantics.

## 21. SS1 donor audit

SS1 jest donor/reference, nie authority.

Wartościowe do przeniesienia jako invariants/tests lub oczyszczone mechanizmy:

```text
DocumentId strong identity
DocumentRevision monotonicity
DocumentSession lifecycle
PartDocument transaction behavior
Command history invariants
save checkpoint semantics
atomic persistence patterns
restart persistence tests
Kernel API boundary tests
```

Szczególnie zachowujemy scenariusze:

- staged mutation invisible before commit;
- rejected command preserves state/history;
- no-op does not create false history;
- Undo/Redo are semantic mutations;
- saved checkpoint może zostać osiągnięty przez Undo;
- Close/Reopen destroys runtime history;
- persisted authored state survives restart.

Nie kopiujemy:

- SS1 `SimpleSolid_Next` jako modułu;
- konkretnego feature-history architecture;
- rozbudowanego recompute dependency graphu;
- historycznych Part collections tylko dlatego, że istnieją w donorze.

## 22. Zakres OUT

PART-01 jawnie nie obejmuje:

```text
Sketch / Shared 2D editing
Bodies
Features
Extrude
Chamfer
Datum geometry
real Part evaluation
OCCT modeling
3D Viewer integration
topology selection
persistent naming
Material model
mass properties
Assembly
Drawing
BOM
custom properties
export model
Save As UI
Save Copy As / Duplicate UI
automatic IdentityConflict repair
retargeting references
external-document editing outside current Project Workspace
continuous filesystem watching
semantic merge of diverged document clones
```

## 23. Testy akceptacyjne

Automatyczne testy muszą udowodnić co najmniej:

1. Create Part generuje nowy poprawny `DocumentId`.
2. `DocumentId` jest odrębnym typem od `ProjectId`.
3. Native Part nie wymaga persisted owner `ProjectId`.
4. Create publikuje poprawny natywny Part w wybranej lokalizacji Workspace.
5. Reopen zwraca ten sam `DocumentId`.
6. Filename/path nie definiuje tożsamości.
7. Rename/move pliku i ponowne discovery zachowuje `DocumentId`.
8. Basic Document Properties przeżywają Save/Close/restart.
9. Property mutation odbywa się przez Command/Transaction.
10. Successful mutation tworzy Undo state i zwiększa technical revision.
11. Undo/Redo zwiększają technical revision monotonnie.
12. No-op nie zwiększa revision i nie tworzy historii.
13. Failed/rejected command nie zmienia authored state ani istniejącej historii.
14. Po Save session jest clean.
15. Edit po Save powoduje dirty.
16. Undo do saved authored checkpoint może przywrócić clean state.
17. Save failure nie oznacza dokumentu jako clean.
18. Failed atomic save nie uszkadza poprzedniej poprawnej wersji pliku.
19. Close/Reopen usuwa runtime Undo/Redo history.
20. Po restarcie aplikacji ten sam Part jest ponownie discoverable i otwierany z tym samym `DocumentId`.
21. Jeden resolved `DocumentId` nie tworzy dwóch aktywnych kanonicznych DocumentSessions w jednym ProjectSession.
22. Filesystem copy z tym samym `DocumentId` tworzy `IdentityConflict`.
23. Conflict diagnostic zawiera wszystkie znalezione relative paths.
24. Conflict nie jest rozwiązywany arbitralnym wyborem jednej kopii.
25. Conflict jednego ID nie blokuje niezależnych poprawnych Partów.
26. Invalid/unsupported native Part daje strukturalną diagnostykę.
27. Common property lifecycle nie wymaga OCCT ani Viewer.
28. Persistence nie zawiera runtime DocumentSession/Undo/Viewer/OCCT identity.
29. Dirty close nie traci zmian bez jawnego Save/Discard.
30. Cały repository verify/build/test pozostaje PASS.

## Documentation impact

Internal docs: required  
User/Product docs: required

Reason:

PART-01 wprowadza pierwszy rzeczywisty CAD Document lifecycle, durable identity/persistence, DocumentSession, discovery/conflict behavior oraz nowe user-visible workflow Create/Open/Edit/Save/Close/Reopen.

Internal as-built docs muszą opisać rzeczywistą zaimplementowaną ownership/lifecycle/persistence strukturę.

Product docs PL/EN muszą opisać użytkownikowi minimalny Part workflow, znaczenie filename vs properties oraz widoczne failure/conflict behavior.

Generated Product Browser musi zostać odświeżony i pozostać Git-clean po ponownej generacji.

## 25. Architecture impact

PART-01 implementuje zaakceptowaną Foundation 1.0 i ADR-0002.

Nie autoryzuje nowych D2/D3 zmian.

Jeżeli implementacja wymaga zmiany między innymi:

```text
Document identity semantics
Project/Document ownership
DocumentSession ownership
persistence meaning
Commands/Transactions boundary
IdentityConflict semantics
OCCT boundary
```

praca zatrzymuje się do czasu jawnej decyzji Ownera.

## 26. Kryterium ukończenia

PART-01 jest ukończony, gdy rzeczywista aplikacja i automatyczne testy udowadniają:

```text
Project
→ Create persistent Part
→ stable DocumentId
→ DocumentSession
→ edit authored properties
→ Undo / Redo
→ Save
→ Close
→ Restart
→ Discover
→ Reopen
→ same DocumentId + same authored state
```

oraz gdy konflikt dwóch plików o tym samym `DocumentId` jest wykrywany i raportowany fail-closed zamiast rozstrzygany przez zgadywanie.

Nie rozpoczynamy automatycznie Sketch/geometry/OCCT po ukończeniu PART-01. Następny zakres wymaga osobnego zaakceptowanego kontraktu.
