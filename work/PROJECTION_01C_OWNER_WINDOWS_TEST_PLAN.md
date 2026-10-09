# PG-01C — instrukcja testów Ownera w Windows (wersja robocza)

**Status:** DRAFT — NIE uruchamiać testu akceptacyjnego przed exact-head Windows FULL PASS i potwierdzeniem przez implementację aktualnego commit SHA.
**Kontrakt:** `work/PROJECTION_01C_PROJECT_EDGE_UI_CONTRACT.md` (Owner D2-A–D2-E).
**PR:** https://github.com/MechanicalCave/SimpleSolid2/pull/304 — bez merge do `main`.
**Cel:** z jednej aplikacji przetestować Project Geometry (Edge) + panel Operations + globalną Command Line + natywny OCCT + cykl życia linków, bez mylenia go z Face/PG-01D.

## Uruchamianie — po oficjalnym statusie „GOTOWE DO TESTÓW”

Wykonaj na **Windows**, w lokalnym repozytorium SimpleSolid2, po zapisaniu lub zabezpieczeniu własnych zmian. Do testów używaj SHA wskazanego w wiadomości potwierdzającej gotowość; sam branch PR może później zmienić HEAD.

```powershell
git status --short
git fetch origin
git switch --detach <DOKLADNY_POTWIERDZONY_SHA>
```

W katalogu projektu uruchom `START_SS2.cmd` — otwiera standardową sesję **SS2 Shell** z konfiguracją Qt/OCCT. W otwartej konsoli:

```powershell
ss2-run
```

Powyższe konfiguruje, buduje bieżący program `simplesolid2` i uruchamia `SimpleSolid2.exe`. Równoważny dispatcher to `./ss2.ps1 run`. Dla dokumentacji `ss2-docs` / `./ss2.ps1 docs`, a Browser jest w `docs/browser/index.html`. Nie używaj samej aplikacji ze starego `main` ani przypadkowego wcześniejszego `build` bez aktualnego buildu. Jeśli lokalna maszyna nie ma wymaganego środowiska Qt/OCCT, zgłoś komunikat `ss2-status` zamiast ręcznie kopiować DLL.

## Scenariusz 1 — pierwsza projekcja dwóch material Edge

1. W Project Workspace otwórz lub utwórz testowy **Part** (oddzielny od danych produkcyjnych). Utwórz **Sketch** na **XY Plane**, narysuj prostokąt `40 × 30 mm` i zakończ Sketch.
2. Z prostokąta utwórz **Profile** i **Extrude Add** o wysokości `20 mm`, z pełnym aktualnym Body.
3. Utwórz **drugi Sketch** na XY Plane i wejdź w **Sketch Edit**. Z grupy Sketch **Modify** wybierz **Project Geometry**, alternatywnie w Command Line wpisz `PROJECT`.
4. Po prawej stronie, w **Operations**, sprawdź czy widoczny jest poprawny `Source stage` i licznik `Material Edges selected: 0`. Wskaż rzeczywistą material Edge na krawędzi prostopadłościanu. Następną różną krawędź dodaj przez **Ctrl + lewy klik**. Panel musi pokazać 2, a nie dwa razy ten sam Edge.
5. Przełącz **Regular / Construction**. Podgląd musi być aktualny i zmieniać styl zgodnie z rolą; dokument nie może być dirty wyłącznie od podglądu. Zatwierdź **Finish Project Geometry**. Powinny powstać **dwie** skojarzone encje w **jednym** Undo.
6. `Undo` usuwa obie, `Redo` przywraca te same EntityIds, role i źródła. Wybór linked entity pokazuje status/source Feature. Regular linked nadaje się do poprawnego Profile, Construction pozostaje referencją.

## Scenariusz 2 — Command Line, Esc, Enter, Cancel

- Ponownie zacznij `PROJECT` z globalnej Command Line, wskaż Edge, przetestuj `REGULAR`, `CONSTRUCTION`, `REMOVE`, `CLEAR` i `FINISH`.
- Wpisać część słowa w Command Line, np. `CONSTR`, nacisnąć **Esc**: znika tylko tekst, staged źródła pozostają. Kolejne Esc przy aktywnym wyborze czyści staged Edge, następne Esc wychodzi do **Select**.
- `CANCEL` i przycisk **Cancel** wychodzą od razu, bez Undo/dirty i bez zostawionego podglądu. Pusty lub błędny Finish nie może zmieniać modelu; pozostawia możliwość poprawienia wyboru.
- Zweryfikować osobno Finish z prawego panelu, przez tekst `FINISH` oraz przez **Enter z fokusem w Viewporcie**. Nie mylić pustego Enter w zwykłym Sketch Select (Repeat Last) z Finish aktywnego Project.
- Edytor właściwości, okna modalne i skróty Ctrl+Z/Ctrl+S nie powinny przekazywać wpisywanego tekstu do CAD Command Line; zmiana Sketch/Dokumentu anuluje rozpoczęty `PROJECT`.

## Scenariusz 3 — link, zmiana źródła i trwałość

1. Po utworzeniu linked Edge kliknij go w Sketch Select. Sprawdź odrębne oznaczenie linku (niebieska linia bazowa), źródłowy Feature/status oraz zgodność Regular/Construction.
2. Wybierz **Break Link**: bieżąca pozycja i identyfikator geometrii pozostają, zależność znika; Undo przywraca link, Redo ponownie odłącza. Sprawdź również zwykły Delete i Undo.
3. Edytuj **upstream** bazowy prostokąt z 40×30 na 50×40, nie edytując linked entity. Linked Sketch i snap/Measure powinny pokazywać nową rzeczywistą projekcję, bez zachowania starej seed geometry. Profile z linked geometry nie może być błędnie wypełnione starym obrysem.
4. Stłum/wyłącz źródłowy Extrude. Linked geometria z niedostępnym źródłem nie może udawać poprawnej ani umożliwiać Break Link na starym seed. Przywróć Extrude; ten sam link ma wrócić bez ręcznego przepinania.
5. Wykonaj **Save**, **Close Part**, ponownie **Open** z aktualnego `.ss2part` i, jeśli to możliwe, zrestartuj aplikację. Sprawdź źródła linked Edge, geometrię, status, rolę i zachowanie Undo/Redo. Powtórz z dwoma otwartymi Partami i przełączeniem Document Tabs.

## Raport Ownera

Podaj: testowany SHA, system Windows, wynik PASS/FAIL poszczególnych scenariuszy, pierwsze powtarzalne kroki odtworzenia, pełny komunikat/status z Command Line i — w razie błędu — zrzut Workbench (Viewport + prawa Operations/Properties). Nie zatwierdzaj PG-01D/Face ani merge PG-01C przez samą gotowość buildu; końcowy Owner PASS pozostaje odrębną decyzją.

**Niezależne ograniczenie:** issue #302 (warm-cache FAST registry mismatch) jest śledzone osobno; wyniki FAST i FULL należy raportować oddzielnie. Nie wolno osłabiać testów PG-01C z tego powodu.
