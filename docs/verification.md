# Weryfikacja Open Battery 0.1.0

Data: 2026-11-22. Windows 11 x64, rzeczywisty odbiornik 3151:402D.

## Wyniki

| Kontrola | Wynik i dowód |
| --- | --- |
| Model i parser | PASS: 18 kontroli przez build/model-tests.exe |
| Sprzęt | Trzy kolejne odczyty build/battery-status.exe: reading, 100%, Win32 error 0 |
| Widoczność | Użytkownik potwierdził widoczne 100 w zasobniku |
| Ikony | Wyrenderowano i obejrzano 100 oraz Zzz; 1000 tworzonych i niszczonych ikon: GDI 6 -> 6 |
| Logi | Tryb normalny nie utworzył logu; debug rotuje do 5 plików, każdy <=1 000 000 bajtów |
| Instalacja | msiexec /i /qn: exit 0, instalacja per-user |
| Aktualizacja | Test 0.1.0 -> testowe 0.1.1: exit 0; wartość kontrolna i Initialized zachowane |
| Odinstalowanie | msiexec /x /qn: exit 0; binarka, ustawienia HKCU, autostart i logi usunięte; proces zamknięty |
| Instalacja końcowa | Ponownie zainstalowano 0.1.0: exit 0; uruchomiono z AppData/Local/Programs/Open Battery |
| Autostart | Wpis HKCU Run wskazuje zainstalowane OpenBattery.exe, nie katalog build |
| Rozmiar | EXE 501 248 bajtów; MSI 548 864 bajty |
| Formatowanie | git diff --check: PASS |

MSI 0.1.0 SHA256:
`A9A8D44DD6B36963A209F92AE4F263DE4D6931B68F5446B496E90FDCD4FAB798`

Testowe MSI 0.1.1 służyło tylko sprawdzeniu aktualizacji; nie jest wydaniem produktu. Po teście je odinstalowano.

## Budżet zasobów

Pomiar działającego builda normalnego, debug wyłączony, z rzeczywistym odczytem co 60 sekund. Start po 2 sekundach inicjalizacji. Czas pomiaru: 627,681 s. Odczyty przez System.Diagnostics.Process; CPU = przyrost TotalProcessorTime / czas ścienny × 100, bez dzielenia przez liczbę procesorów logicznych.

- Średnie CPU względem jednego rdzenia: 0,03734% — PASS względem 0,1%.
- Pamięć prywatna na końcu: 2 834 432 bajty.
- Szczyt PeakPagedMemorySize64 (peak private commit): 2 908 160 bajtów — PASS względem 10 000 000.
- Pamięć robocza procesu jest inną metryką i nie jest objęta limitem 10 MB pamięci prywatnej.

Artefakty lokalne (ignorowane w Git): build/perf-start.json, build/perf-result.json, build/uninstall-result.json i logi msiexec. Nie są automatycznie wysyłane.

## Co poprawiono podczas testów

MSI WindowsBuild raportował na tej maszynie wartość zgodności 9600, mimo Windows 11. Warunek zmieniono na odczyt rzeczywistego CurrentBuildNumber z 64-bitowego rejestru; instalacja następnie przeszła.

Pierwszy test GDI porównywał pusty proces z procesem po inicjalizacji GDI. Dodano rozgrzanie renderera przed pomiarem; kolejne 1000 cykli utrzymało stałe 6 uchwytów.

Skrypt weryfikacji odinstalowania początkowo błędnie traktował brak wartości autostartu jako błąd samego testu. Ponowny niezależny odczyt potwierdził usunięcie wszystkich danych.

## Ograniczenia i testy manualne

Nie potwierdzono pełnego cyklu rozładowania, fizycznego ładowania, kontrolowanego odłączenia/wybudzenia, startu po ponownym logowaniu i restartu Explorer. Parser i ostrzeżenia mają testy automatyczne, ale rzeczywiste powiadomienie przy niskiej baterii wymaga naturalnego spadku poziomu lub osobnego testu manualnego.

Web app użytkownika pokazała pełną ikonę baterii, zgodną z raportowanym 100%; to nie jest kalibracja poziomu ogniwa. Ładowanie pozostaje unknown; brak zgadywania.

MSI i EXE nie są podpisane. Nie opublikowano GitHub release.

## Ikony i aktualizacja 0.1.1 — 2026-11-23

- Podłączono oryginalne app.svg, sleep_32.svg, error_32.svg i disconnected_mouse_x.svg. Pozostałe trzy warianty rozłączenia zachowano.
- Podglądy jasne/ciemne obejrzano w rozmiarach 16, 24, 32 i 48 px. Zasoby obejmują też 20, 40 i 64 px; aplikacja dodatkowo 128 i 256 px.
- Test ładowania z natywnych zasobów: 56 przypadków (4 rodzaje × 2 motywy × 7 rozmiarów), każdy z kontrolą wymiarów — PASS. Ikona app jest ta sama dla obu motywów.
- 1000 cykli tworzenia/usuwania ikon: GDI 6 -> 6. Testy 18 kontroli modelu oraz rotacji logów nadal PASS.
- MSI 0.1.1 zainstalowane jako aktualizacja 0.1.0: exit 0. Wartości Initialized/Debug/Warned i autostart porównano przed i po — zachowane.
- Końcowa aplikacja uruchomiona z LocalAppData/Programs/Open Battery, FileVersion 0.1.1. EXE 593 920 bajtów; MSI 696 320 bajtów.
- MSI SHA256: 3A383DEF9F71352D48BB007284772D316A269240A58152A214264BE80B723146.
- Smoke test zasobów: 68,635 s, peak private 2 068 480 bajtów. Przyrost TotalProcessorTime był zerowy w rozdzielczości licznika; nie oznacza to absolutnego braku CPU. Nie powtarzano pełnego 10-minutowego pomiaru z sekcji 0.1.0.
- Renderer SVG i Node są wyłącznie narzędziami builda. Aplikacja ładuje gotowe ICO z zasobów EXE; zachowano cel lekkiego runtime.
- Nie powtarzano fizycznego testu odłączenia/uśpienia ani pełnego odinstalowania po zmianie samych ikon. Nie zmieniano HID ani reguł ostrzegania. Nie opublikowano release.

## Kolory i JetBrains Mono — 0.1.2, 2026-11-23

- Nowe progi: 70–100% zielony, 30–69% pomarańczowy, 0–29% czerwony. Kolor wynika z procentu, nie z flagi ładowania. Próg ostrzeżenia 20% jest osobną regułą.
- Osadzono niezmieniony JetBrains Mono Bold 2.304 na OFL 1.1. Font jest prywatny dla procesu, bez instalacji systemowej. Licencję dołączono do MSI.
- Renderowanie 4× i redukcja pokrycia do premultiplikowanego alfa; stała wysokość cyfr i dopasowanie szerokości 100. Obejrzano podglądy rzeczywistego kodu GDI na jasnym i ciemnym tle dla 16/20/24/32 px.
- PASS: 24 kontrole modelu, w tym 0/29/30/69/70/100. Test GDI potwierdził rodzinę JetBrains Mono, właściwe pasma kolorów w obu motywach, widoczny antyaliasing i przezroczysty margines cyfr bez obcinania. Testy zasobów i rotacji logów nadal PASS. 1000 cykli: GDI 5 -> 5.
- W testowej kompilacji ANSI wykryto różnicę typu RT_RCDATA; użyto jawnego MAKEINTRESOURCEW(10) i ponownie wykonano cały test oraz build.
- Aktualizacja MSI 0.1.1 -> 0.1.2: exit 0; Initialized/Debug/Warned i autostart zachowane. Zainstalowana wersja FileVersion 0.1.2 jest uruchomiona.
- EXE 876 544 bajty; MSI 823 296 bajtów. SHA256 MSI: B6714672B2E704923388A2E48B7FDD0141EF2B7010437E89471C810F67A1E6C7.
- Smoke procesu: 126,772 s, peak private 2 043 904 bajty, zerowy przyrost TotalProcessorTime w rozdzielczości licznika. Odbiornik zgłaszał inactive; nie jest to pełny 10-minutowy test aktywnego wyświetlania cyfr z nowym fontem. Renderowanie fontu sprawdzono oddzielnie w testach i narzędziu podglądu.
- Podglądy: build/numbers/preview-dark.png i preview-light.png, odtwarzane przez scripts/preview-numbers.ps1.
- Nie zmieniano HID ani reguł powiadomień. Nie opublikowano release i nie powtarzano pełnego odinstalowania ani pełnego cyklu sprzętowego.

## Czytelność i wybudzanie — 0.1.3, 2026-11-23

- Osadzony JetBrains Mono ExtraBold 2.304; większe cyfry i pełna szerokość 100. Obejrzano podglądy obu motywów z kodu produkcyjnego.
- Test regresji przed poprawką: FAIL, aktywność przy inactive nie uruchamiała odczytu (0 prób). Po poprawce PASS: natychmiastowy odczyt, scalanie 100 zdarzeń, stare odpowiedzi odbiornika, odczyt już w toku, ograniczenie prób oraz filtrowanie urządzenia. Test używa kontrolowanego źródła danych, bez sprzętu.
- scripts/build.ps1 i scripts/test.ps1: PASS; 24 kontrole modelu, rzeczywista rodzina ExtraBold i alfa cyfr, 56 przypadków zasobów, rotacja logów; 1000 cykli ikon GDI 5 -> 5.
- Subskrypcja Raw Input rzeczywistego odbiornika: armed, error=0. Dwa fizyczne przejścia inactive -> reading po aktywności AJ179: około 47 ms od obsługi zdarzenia do odczytu. Użytkownik potwierdził natychmiastowy powrót procentu. To obserwacja lokalna, nie gwarancja czasu systemowego.
- Pasywna identyfikacja urządzenia używa wyłącznie nagłówka Raw Input. Brak odczytu pozycji/przycisków i logowania ruchów. Subskrypcja tylko przy inactive/error, wyłączana po pierwszym zdarzeniu; najwyżej 4 odczyty i 15 s przerwy po niepowodzeniu.
- Aktualizacja MSI 0.1.2 -> 0.1.3: exit 0, ustawienia zachowane, zainstalowana 0.1.3 uruchomiona. Tymczasowe logowanie stanów po próbie wyłączono, przywracając poprzedni brak wartości Debug.
- Krótki pomiar 95,132 s z dwoma wybudzeniami i tymczasowym debug: CPU 0,01642% jednego rdzenia, peak private 2 121 728 bajtów. Nie zastępuje pełnego testu 10-minutowego.
- EXE 883 200 bajtów; MSI 823 296 bajtów. SHA256 MSI: DC5F07FF6A400F25602EDAC4C2A654DFA060A9523CA5C8941912804DE857E8EE.
- Bez zmian protokołu HID, palety i ostrzeżeń. Nie powtarzano pełnego odinstalowania ani testu rozładowania. Bez publikacji release.

## Wiele urządzeń i Bluetooth — 0.2.0–0.2.2, 2026-11-25

- Zaimplementowano Devices, niezależne wybory/nazwy/ostrzeżenia, tożsamości i historię odczytów. Po próbach A i C użytkownik wybrał procent w indywidualnie kolorowanym okręgu. Kolor wybiera natywny dialog Ring color; ustawienie przetrwa ponowne uruchomienie.
- scripts/build.ps1 oraz scripts/test.ps1 dla 0.2.2: PASS. 24 kontrole AJ179; Sony USB/BT/CRC i dopełnienie Windows; historia/tożsamości i osobne ostrzeżenia; regresja wybudzania AJ179; prywatny font, alfa, 56 zasobów, GDI i rotacja logów. Natywny test Devices używa izolowanego klucza HKCU, bez zmiany parowania i ustawień użytkownika. 1000 okręgów GDI 16 -> 16.
- Pad: rzeczywisty błąd odtworzony przez devices-status --require-bt-controller (exit 1). Przyczyny: błędna klasyfikacja USB, 547-bajtowe dopełnienie raportu BT i tryb 0x01 bez baterii. Test 547-bajtowego raportu najpierw FAIL, po naprawie PASS. Odczyt wejściowy GET 0x11 został odrzucony (87).
- Po jawnej zgodzie użytkownika dodano i włączono ExtendedReports dla jego DualShock 4: GET kalibracji 0x02, bez zapisu kalibracji/LED/wibracji. Rzeczywisty wynik: Bluetooth, około 75%, charging=no, error=0; devices-status --require-bt-controller exit 0. Po późniejszym rozłączeniu prawidłowo zwrócono Disconnected; nie jest to regresja parsera. DualSense ma testy formatów, lecz nie był fizycznie dostępny.
- JBL TUNE700BT: profil Hands-Free ma właściwość battery=10 i ten sam kontener co urządzenie. W czasie odczytów pozostawał rozłączony, więc aplikacja nie pokazywała starego procentu jako bieżącego. Świeży poziom przy aktywnym połączeniu czeka na test użytkownika.
- AJ179: użytkownik potwierdził zdjęcie i ponowne odłożenie na stację. F7 w obu sytuacjach identyczny, 70%, active; brak osobnej flagi ładowania w pozostałych bajtach, brak dodatkowej właściwości Windows. Wcześniejszy inactive nie odróżnia snu i stacji. Nie wprowadzono zgadywanego ładowania.
- 0.2.1: aktualizacja MSI exit 0, ustawienia zachowane. Pomiar 611,044 s: CPU 0,01279% jednego rdzenia; private 2 781 184, peak private 2 859 008 bajtów. Scope: AJ179 i DS4 wybrane, JBL rozłączony/niewybrany; nie dowodzi pełnego testu trzech połączonych urządzeń. Wcześniejszy pomiar 0.2.0 z interakcjami: CPU 0,1432%, peak 4 653 056; nie jest kontrolowanym pomiarem bezczynności.
- 0.2.2: MSI exit 0, ustawienia zachowane, proces uruchomiony. EXE 959 488 bajtów; MSI 851 968 bajtów. SHA256 MSI: 2A8E9F38DFB724CAA73340DA05A977536C303A020C070A98B01B46D6994EA052. Pomiar zasobów tej wersji trwa, wynik będzie dopisany po zakończeniu.
- Obejrzano natywne bitmapy okręgów obu motywów w rozmiarach 16–32 px. Natywne Devices wyrenderowano poza ekranem; komputerowe sterowanie pulpitem było niedostępne (pipe), zapasowe zgłaszało USER_CONTROL. Nie przejmowano pulpitu. Wizualna kontrola prawdziwego paska pozostaje po stronie użytkownika.
- Nie powtarzano pełnego odinstalowania ani rozładowania ogniw. Brak release, podpisu i merge do gałęzi domyślnej.
