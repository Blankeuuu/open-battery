# Specyfikacja robocza Open Battery

Status: trwa wywiad; implementacja niezatwierdzona.

## Decyzje zatwierdzone

2026-11-22: użytkownik zaakceptował zakres opisany w README: zasobnik z procentem, AJ179 Pro przez 3151:402D, normalny odczyt co 60 sekund, stan ładowania i jedno ostrzeżenie, minimalny narzut bez stałego okna i sieci. Repozytorium publiczne, początkowo dokumentacja.


### Druga runda — zatwierdzone

- W pierwszej wersji wskaźnik pokazuje samą liczbę, np. 87, bez znaku % i rysunku baterii. Docelowa estetyka będzie dopracowana później. Po najechaniu: nazwa myszy, procent i czas ostatniego odczytu.
- Ostrzeżenie przy poziomie 20% lub mniej, raz; ponowne uzbrojenie po odczycie powyżej 25%.
- Domyślny start z Windows. Menu: Odśwież teraz, Uruchamiaj z Windows, Zakończ; bez osobnego okna ustawień na początek.
- Przy braku wiarygodnego stanu ładowania pokazujemy wyłącznie procent. Wzrost procentu nie oznacza automatycznie ładowania.
- Uśpienie myszy ma być prezentowane ikoną Zzz w kreskówkowym stylu, zamiast procentu. Warunki rozpoznawania tego stanu i odróżnienia od wyłączenia lub braku odpowiedzi pozostają do ustalenia po sprawdzeniu protokołu.

### Trzecia runda — zatwierdzone

- Cel zasobów: do 10 MB pamięci prywatnej i średnio poniżej 0,1% CPU podczas 10 minut bezczynności na komputerze użytkownika. Bez regularnych zapisów na dysk. Są to kryteria do pomiaru, a nie osiągnięte wyniki.
- Stan wysłanego ostrzeżenia jest pamiętany po restarcie. Ponowne uzbrojenie dopiero po wiarygodnym odczycie powyżej 25%; mały zapis przy zmianie stanu jest dopuszczony.
- Po zatwierdzeniu pełnej specyfikacji pierwszym etapem będzie weryfikacja rzeczywistego odczytu baterii AJ179 Pro. Budowa wskaźnika nastąpi po potwierdzeniu odczytu. Żadnego etapu implementacji jeszcze nie rozpoczęto.

## Otwarte decyzje

- Docelowy wygląd wskaźnika: późniejszy etap; obecnie sama liczba bez znaku % i rysunku baterii.
- Prezentacja po utracie odczytu, uśpieniu myszy i odłączeniu odbiornika.
- Weryfikacja protokołu baterii i współpraca z oprogramowaniem producenta.
- Technologia, dystrybucja, wspierane wersje Windows i licencja.

## Fakty i ograniczenia

W poprzedniej diagnostyce Windows wykrywał odbiornik 3151:402D jako sprawny. Halo Battery 1.17.0 wyklucza ten identyfikator z obsługi AJAZZ z powodu niepotwierdzonej wymiany odczytu baterii. Nie wykonano testu protokołu dla Open Battery. Nazwa modelu przypisana do wspólnego PID w cudzym projekcie nie rozstrzyga modelu myszy użytkownika.

Źródło: https://github.com/HeyOkay/HaloBattery/blob/main/docs/protocols.md
