# Specyfikacja robocza Open Battery

Status: trwa wywiad; implementacja niezatwierdzona.

## Decyzje zatwierdzone

2026-11-22: użytkownik zaakceptował zakres opisany w README: zasobnik z procentem, AJ179 Pro przez 3151:402D, normalny odczyt co 60 sekund, stan ładowania i jedno ostrzeżenie, minimalny narzut bez stałego okna i sieci. Repozytorium publiczne, początkowo dokumentacja.


### Druga runda — zatwierdzone

- W pierwszej wersji wskaźnik pokazuje samą liczbę, np. 87, bez znaku % i rysunku baterii. Docelowa estetyka będzie dopracowana później. Po najechaniu: nazwa myszy, procent i czas ostatniego odczytu.
- Ostrzeżenie przy poziomie 20% lub mniej, raz; ponowne uzbrojenie po odczycie powyżej 25%.
- Domyślny start z Windows. Menu: Refresh now, Start with Windows, Exit; bez osobnego okna ustawień na początek.
- Przy braku wiarygodnego stanu ładowania pokazujemy wyłącznie procent. Wzrost procentu nie oznacza automatycznie ładowania.
- Nieaktywna mysz przy podłączonym odbiorniku: kreskówkowa ikona Zzz zamiast procentu; obejmuje sen i wyłączenie, jeśli protokół ich nie rozróżnia. Dymek: Mouse inactive, ostatni procent i czas odczytu. Błąd komunikacji nie oznacza automatycznie snu. Techniczne warunki rozpoznania stanu wymagają weryfikacji protokołu.

### Trzecia runda — zatwierdzone

- Cel zasobów: do 10 MB pamięci prywatnej i średnio poniżej 0,1% CPU podczas 10 minut bezczynności na komputerze użytkownika. Bez regularnych zapisów na dysk. Są to kryteria do pomiaru, a nie osiągnięte wyniki.
- Stan wysłanego ostrzeżenia jest pamiętany po restarcie. Ponowne uzbrojenie dopiero po wiarygodnym odczycie powyżej 25%; mały zapis przy zmianie stanu jest dopuszczony.
- Po zatwierdzeniu pełnej specyfikacji pierwszym etapem będzie weryfikacja rzeczywistego odczytu baterii AJ179 Pro. Budowa wskaźnika nastąpi po potwierdzeniu odczytu. Żadnego etapu implementacji jeszcze nie rozpoczęto.

### Czwarta runda — zatwierdzone

- Odłączenie odbiornika: szara ikona przekreślonej myszy; dymek Receiver disconnected.
- Akceptowane opóźnienie powrotu procentu po wybudzeniu: do 60 sekund. Dostępne ręczne Odśwież teraz. Szybszy powrót po zdarzeniu urządzenia jest dopuszczony, jeśli nie wymaga częstszego odpytywania.
- Dystrybucja przez instalator MSI. Wariant przenośny nie jest planowany w pierwszej wersji.

### Piąta runda — zatwierdzone

- MSI instaluje dla aktualnego użytkownika, bez uprawnień administratora i bez usługi systemowej. Autostart po zalogowaniu użytkownika.
- Aktualizacja ręczna przez nowszy MSI, z zachowaniem ustawień. Program nie sprawdza aktualizacji przez sieć.
- Błąd odczytu przy obecnym odbiorniku: szary wykrzyknik, opis Could not read battery, ostatni znany procent i czas odczytu. Przed pierwszym poprawnym odczytem opis No reading, bez wymyślonej wartości.
- Pierwsza wersja wspiera Windows 11 x64.

### Szósta runda — zatwierdzone

- Cały interfejs programu i instalatora wyłącznie po angielsku. Dokumentacja robocza pozostaje po polsku.
- Potwierdzone ładowanie: zielony procent i dymek Charging — 42% (wartość przykładowa). Bez animacji; ikony Zzz, odłączenia i błędu również statyczne.
- Domyślnie brak zapisu logów. Menu Copy diagnostics kopiuje wersję programu, identyfikator odbiornika, stan i ostatni błąd; bez numerów seryjnych i danych osobowych. Brak wysyłania danych.
- Osobny debug mode automatycznie zapisuje logi w jednym miejscu. Jest świadomym wyjątkiem od braku regularnych zapisów w trybie normalnym. Szczegóły trybu wymagają uzgodnienia.
- Pełne odinstalowanie usuwa autostart, ustawienia, zapamiętane ostrzeżenie i dane programu. Aktualizacja MSI zachowuje ustawienia.

## Otwarte decyzje

- Docelowy wygląd wskaźnika: późniejszy etap; obecnie sama liczba bez znaku % i rysunku baterii.
- Weryfikacja protokołu baterii i współpraca z oprogramowaniem producenta.
- Technologia i licencja; limity, lokalizacja i czas działania debug mode; kolory procentu podczas pracy na baterii.

## Fakty i ograniczenia

W poprzedniej diagnostyce Windows wykrywał odbiornik 3151:402D jako sprawny. Halo Battery 1.17.0 wyklucza ten identyfikator z obsługi AJAZZ z powodu niepotwierdzonej wymiany odczytu baterii. Nie wykonano testu protokołu dla Open Battery. Nazwa modelu przypisana do wspólnego PID w cudzym projekcie nie rozstrzyga modelu myszy użytkownika.

Źródło: https://github.com/HeyOkay/HaloBattery/blob/main/docs/protocols.md
