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
