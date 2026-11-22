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
