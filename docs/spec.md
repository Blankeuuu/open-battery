# Specyfikacja robocza Open Battery

Status: trwa wywiad; implementacja niezatwierdzona.

## Decyzje zatwierdzone

2026-11-22: użytkownik zaakceptował zakres opisany w README: zasobnik z procentem, AJ179 Pro przez 3151:402D, normalny odczyt co 60 sekund, stan ładowania i jedno ostrzeżenie, minimalny narzut bez stałego okna i sieci. Repozytorium publiczne, początkowo dokumentacja.


### Druga runda — zatwierdzone

- W pierwszej wersji wskaźnik pokazuje samą liczbę, np. 87, bez znaku % i rysunku baterii. Docelowa estetyka będzie dopracowana później. Po najechaniu: nazwa myszy, procent i czas ostatniego odczytu.
- Ostrzeżenie przy poziomie 20% lub mniej, raz; ponowne uzbrojenie po odczycie powyżej 25%.
- Domyślny start z Windows. Menu: Odśwież teraz, Uruchamiaj z Windows, Zakończ; bez osobnego okna ustawień na początek.
- Przy braku wiarygodnego stanu ładowania pokazujemy wyłącznie procent. Wzrost procentu nie oznacza automatycznie ładowania.
- Q7 pozostaje otwarte: odpowiedź użytkownika „nie pokazuje ze mysz spi” wymaga doprecyzowania. Nie zaakceptowano reguły dwóch nieudanych odczytów ani zastępowania procentu znakiem ? po pięciu minutach.

## Otwarte decyzje

- Docelowy wygląd wskaźnika: późniejszy etap; obecnie sama liczba bez znaku % i rysunku baterii.
- Prezentacja po utracie odczytu, uśpieniu myszy i odłączeniu odbiornika.
- Zachowanie ostrzeżenia po ponownym uruchomieniu programu.
- Mierzalny budżet RAM, CPU i pracy w tle.
- Weryfikacja protokołu baterii i współpraca z oprogramowaniem producenta.
- Technologia, dystrybucja, wspierane wersje Windows i licencja.

## Fakty i ograniczenia

W poprzedniej diagnostyce Windows wykrywał odbiornik 3151:402D jako sprawny. Halo Battery 1.17.0 wyklucza ten identyfikator z obsługi AJAZZ z powodu niepotwierdzonej wymiany odczytu baterii. Nie wykonano testu protokołu dla Open Battery. Nazwa modelu przypisana do wspólnego PID w cudzym projekcie nie rozstrzyga modelu myszy użytkownika.

Źródło: https://github.com/HeyOkay/HaloBattery/blob/main/docs/protocols.md
