# SimpleSolid 2.0 — prompt wznowienia nowego kontekstu

Kontynuuj projekt **SimpleSolid 2.0 (SS2)**. Pracuj po polsku, inżyniersko, krytycznie i w granicach faktycznej autoryzacji.

## Zasada aktualności — żadnego zamrożonego checkpointu

Repozytorium `MechanicalCave/SimpleSolid2` i obowiązujące governance są źródłami prawdy. **Ten prompt jest instrukcją odczytu, nie rejestrem aktualnego stanu projektu.** Nie wpisuj tutaj na stałe bieżącej gałęzi, HEAD/SHA, numerów PR/CI, wersji roadmap, nazwy aktywnego etapu, datowanego statusu ani następnego kamienia milowego. Te informacje szybko się dezaktualizują.

W **każdym** nowym kontekście pobierz świeży stan GitHub (chroniony `main`, otwarte/scalone PR-y, workflow runs i wyniki). Jeśli pracujesz na lokalnym Windows, sprawdź również rzeczywisty `git branch --show-current`, `git rev-parse HEAD` i `git status --short`; nie utożsamiaj lokalnej gałęzi z `origin/main` bez weryfikacji. Rozróżniaj statusy DRAFT, zaakceptowany projekt, zakończona implementacja, Owner FINAL, MERGED i powdrożeniowy readback.

## Obowiązkowa rekonstrukcja autorytetu

Czytaj w tej kolejności, kierując się **aktualną zawartością** wskazanych źródeł:

1. `AGENTS.md`, `governance/CONSTITUTION.md`, `governance/FOUNDATION.md`, `governance/ARCHITECTURE.md`.
2. Istotne **zaakceptowane** ADR-y, w tym ADR-y wymienione w bieżącym `work/ACTIVE.yaml`.
3. `work/ACTIVE.yaml`: `active_work`, `program_roadmap`, `program_roadmap_version`, `accepted_adrs` i bieżąca historia `next`. Plik `ACTIVE` jest wskaźnikiem obowiązującej pracy, ale **sam wskaźnik nie dowodzi, że wskazany etap pozostaje niezakończony**.
4. **Dokładny** plik `program_roadmap` z `ACTIVE`, w zadeklarowanej tam wersji; pozostałe roadmapy tylko wtedy, gdy dotyczą wybranego obszaru. W historycznych nagłówkach roadmap i dawnych propozycji mogą pozostać nieaktualne informacje o PR-ach — rozstrzygaj je na podstawie aktualnego GitHuba.
5. **Dokładny** `active_work` i jego skutecznie zaakceptowane, scalone na chronionym `main` poprawki D2/D3; uwzględnij niezmienione obowiązujące decyzje i jawne odroczenia.
6. `governance/DOCUMENTATION.md`, aktualną dokumentację as-built `docs/internal/` oraz — zależnie od zadania — `docs/product/pl/`, `docs/product/en/`.
7. Aktualny HEAD/PR/diff, potwierdzone testy **na ich dokładnych SHA**, decyzje Ownera oraz brakujące bramki.

Przy sprzecznościach nie wybieraj automatycznie nowszej daty ani tekstu z Draft PR. Ustal, co zostało rzeczywiście zaakceptowane, scalone i odczytane z chronionego `main`; zgłoś nierozstrzygniętą sprzeczność jako **STOP**.

## Rozpoznanie etapu i prawo do działania

- Jeżeli `ACTIVE` nadal wskazuje etap **już odebrany i scalony**, odnotuj to jako brak formalnego przejścia i zaproponuj **odrębny, ograniczony kontrakt następnej fazy oraz osobny ACTIVE-only switch**. Nie reaktywuj zakończonego etapu i nie uznawaj planu roadmapy za aktywację.
- Wcześniejsze Owner **D3** dotyczące kolejnych funkcji nie oznacza automatycznie akceptacji ich konkretnego kontraktu **D2**, listy plików ani implementacji.
- Przy braku skutecznie zaakceptowanego i aktywowanego nowego Work Contract dozwolony jest audyt, readback, projekt i propozycja; **nie zaczynaj trwałych zmian produktu**.
- D0/D1 wyłącznie w obecnym zatwierdzonym zakresie. D2/D3 i każdy nowy dokładny path allowlist wymagają wyraźnej decyzji Ownera.
- **Akceptacja kontraktu**, **zgoda na chroniony guarded-squash merge**, **Owner manual/technical FINAL**, **aktywacja przez `ACTIVE.yaml`** i **prawo do dystrybucji/licencji** są odrębnymi bramkami. Nie wywódź jednej z drugiej ani z ogólnego „kontynuuj”.
- Nie przenoś uprawnień, statusów PASS lub testów z poprzedniego PR, projektu-dawcy, commita, etapu headless/DEV albo historycznego pliku na nową implementację.

## Stałe reguły inżynierskie

- Konstytucja → Foundation/Architecture → zaakceptowane ADR → roadmapa → aktywny kontrakt → implementacja.
- Trwała mutacja: Command → Validation → Transaction → owning Domain Document → Evaluation. Authored intent i semantyczne ID pozostają autorytatywne; UI/Viewer/Qt/OCCT, cache, współrzędne ekranu i indeksy topologii nie są trwałą tożsamością CAD.
- Brak zgadywania brakujących/stale referencji, cichego rebind, niezatwierdzonych globalnych tolerancji, częściowych commitów lub pozornego PASS. Nie osłabiaj testów, aby uzyskać zieloną bramkę.
- Nie zmieniaj bezpośrednio chronionego `main`; używaj zatwierdzonej procedury PR, dokładnego HEAD, odpowiedniego Windows CLOSURE/FULL, oddzielnej zgody Ownera na merge i powdrożeniowego readback.
- Jeśli zmiana wpływa na dokumentację, stosuj DOC-01: kanoniczny Markdown, parytet PL/EN, generowany wyłącznie przez `.\\ss2.ps1 docs` plik `docs/browser/index.html` i `.\\ss2.ps1 verify`. Nie poprawiaj generowanego HTML ręcznie.
- Nie domniemaj gotowości do release z testu DEV ani z faktu posiadania oryginalnych bajtów zależności; odrębne prawa/licencja/packaging wymagają własnego rozstrzygnięcia.

## Pierwszy raport po wznowieniu

Krótko podaj: **(1)** repo / chroniony `main` / lokalny HEAD i stan drzewa (jeśli dostępne); **(2)** rzeczywisty `ACTIVE`, roadmapę i obowiązujący Work Contract; **(3)** ostatnią **faktycznie zakończoną** fazę z dowodem Owner FINAL, chronionym merge i readback; **(4)** aktualne otwarte PR-y istotne dla celu, z odróżnieniem Draft od zaakceptowanej pracy; **(5)** dokładne, właściwe dla aktualnego SHA dowody Windows/Docs i ich ograniczenia; **(6)** pierwszy **dozwolony** kolejny krok i osobne decyzje D2/D3/STOP.

Nie zaczynaj implementacji ani scalenia na podstawie samego promptu wznowienia. Jeśli opis użytkownika lub stary dokument koliduje z aktualnym repo, najpierw wyjaśnij różnicę, zamiast kopiować przestarzały checkpoint.
