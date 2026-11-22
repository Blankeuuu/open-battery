# Specyfikacja robocza Open Battery

Status: trwa wywiad; implementacja niezatwierdzona.

## Decyzje zatwierdzone

2026-11-22: użytkownik zaakceptował zakres opisany w README: zasobnik z procentem, AJ179 Pro przez 3151:402D, normalny odczyt co 60 sekund, stan ładowania i jedno ostrzeżenie, minimalny narzut bez stałego okna i sieci. Repozytorium publiczne, początkowo dokumentacja.

## Otwarte decyzje

- Sposób prezentacji procentu i czytelność ikony.
- Prezentacja po utracie odczytu, uśpieniu myszy i odłączeniu odbiornika.
- Próg ostrzeżenia i warunek jego ponownego uzbrojenia.
- Uruchamianie z Windows oraz minimalna obsługa przez menu.
- Mierzalny budżet RAM, CPU i pracy w tle.
- Obsługa braku wiarygodnej informacji o ładowaniu.
- Weryfikacja protokołu baterii i współpraca z oprogramowaniem producenta.
- Technologia, dystrybucja, wspierane wersje Windows i licencja.

## Fakty i ograniczenia

W poprzedniej diagnostyce Windows wykrywał odbiornik 3151:402D jako sprawny. Halo Battery 1.17.0 wyklucza ten identyfikator z obsługi AJAZZ z powodu niepotwierdzonej wymiany odczytu baterii. Nie wykonano testu protokołu dla Open Battery. Nazwa modelu przypisana do wspólnego PID w cudzym projekcie nie rozstrzyga modelu myszy użytkownika.

Źródło: https://github.com/HeyOkay/HaloBattery/blob/main/docs/protocols.md
