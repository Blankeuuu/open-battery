# Specyfikacja Open Battery — zatwierdzona

Status: pełna specyfikacja zatwierdzona przez użytkownika 2026-11-22. Użytkownik polecił rozpocząć implementację 2026-11-22; prace rozpoczęto od weryfikacji protokołu.

## Cel i zakres

Minimalny wskaźnik baterii AJAZZ AJ179 Pro przez odbiornik 2.4 GHz `3151:402D` dla Windows 11 x64. Pierwsza wersja obsługuje ten egzemplarz; odczyt baterii pozostaje niezweryfikowany. Interfejs programu i instalatora wyłącznie po angielsku. Dokumentacja robocza po polsku. Licencja MIT.

Bez stałego okna, usługi systemowej i aktywności sieciowej programu. Bez historii baterii, wykresów i prognoz czasu pracy. Docelowa estetyka wskaźnika będzie rozwijana później.

## Wskaźnik i stany

Ikona w zasobniku systemowym obok zegara; na początek sama liczba, bez znaku %. Wszystkie ikony statyczne.

| Stan | Wskaźnik | Dymek |
| --- | --- | --- |
| Poprawny odczyt, praca na baterii | Procent JetBrains Mono ExtraBold: 70–100% zielony, 30–69% pomarańczowy, 0–29% czerwony | Nazwa myszy, procent, czas odczytu |
| Potwierdzone ładowanie | Kolor procentu według poziomu baterii | Charging — 42% (wartość przykładowa), czas odczytu |
| Nieaktywna mysz, odbiornik obecny | Księżyc z Z zamiast procentu (sleep_32.svg) | Mouse inactive, ostatni procent i czas odczytu, jeśli dostępne |
| Odbiornik odłączony | Szara mysz z X (disconnected_mouse_x.svg) | Receiver disconnected |
| Błąd odczytu, odbiornik obecny | Szary trójkąt z wykrzyknikiem (error_32.svg) | Could not read battery, ostatni procent i czas odczytu, jeśli dostępne |
| Brak pierwszego poprawnego odczytu | Szary trójkąt z wykrzyknikiem (error_32.svg) | No reading |

Zzz może obejmować sen i wyłączenie myszy, jeśli protokół ich nie rozróżnia. Błąd komunikacji nie jest automatycznie snem. Techniczne rozpoznawanie stanów wymaga sprawdzenia protokołu. Brak wiarygodnej flagi ładowania oznacza pokazywanie samego procentu, bez oznaczania ładowania; sam wzrost procentu nie wystarcza.

## Odczyt i współpraca z urządzeniem

- Normalne odpytywanie co 60 sekund; ręczne Refresh now.
- Od wersji 0.1.3 aktywność AJ179 podczas snu lub błędu uruchamia natychmiastowy odczyt. Tymczasowy Raw Input identyfikuje odbiornik po nagłówku; nie odczytuje pozycji ani przycisków. Po poprawnym odczycie subskrypcja jest wyłączana.
- Stary stan odbiornika: najwyżej 4 próby, dodatkowe po 250/750/1500 ms; po niepowodzeniu 15 s przerwy. Zdarzenie urządzenia również odświeża stan. Brak odpytywania przy każdym ruchu aktywnej myszy.
- Jeśli aplikacja AJAZZ blokuje odbiornik: stan błędu i próba co 60 sekund, bez zamykania innych programów.
- Możliwy konflikt można opisać w diagnostyce; pewna przyczyna tylko przy wiarygodnym rozpoznaniu.
- Nie pokazujemy zmyślonych wartości. Ostatni odczyt przy błędzie lub nieaktywności jest opisany jako historyczny.

## Ostrzeżenie o niskiej baterii

- Jedno ostrzeżenie przy ≤20%, bez powtarzania po wybudzeniu lub restarcie.
- Ponowne uzbrojenie po poprawnym odczycie >25%.
- Stan ostrzeżenia przechowywany między uruchomieniami; mały zapis przy zmianie stanu dopuszczony.
- Przy potwierdzonym ładowaniu brak ostrzeżenia.
- Po ustaniu ładowania przy ≤20% ostrzeżenie może zostać wysłane, jeśli nie było go w tym cyklu.

## Menu i ustawienia

- Refresh now
- Start with Windows — domyślnie włączone, start po zalogowaniu
- Copy diagnostics
- Debug mode — domyślnie wyłączone
- Open logs folder
- Exit

Bez osobnego okna ustawień na początek.

## Diagnostyka

Domyślnie brak zapisu logów na dysku. Copy diagnostics kopiuje wersję programu, identyfikator odbiornika, stan i ostatni błąd; bez numerów seryjnych i danych osobowych. Nic nie jest wysyłane.

Debug mode automatycznie zapisuje czas, próby odczytu, wyniki i błędy dotyczące myszy do `%LOCALAPPDATA%\Open Battery\Logs`. Ustawienie jest pamiętane po restarcie aż do wyłączenia. Rotacja maksymalnie 5 plików po 1 MB; najstarsze zastępowane. Bez śledzenia ruchów, kliknięć, numerów seryjnych i danych innych urządzeń.

## Budżet zasobów

- Cel: do 10 MB pamięci prywatnej.
- Cel: średnio <0,1% CPU podczas 10 minut bezczynności na komputerze użytkownika.
- Bez regularnych zapisów w normalnym trybie; dopuszczone zapisy ustawień i stanu ostrzeżenia przy ich zmianie.
- Debug mode jest świadomym wyjątkiem dopuszczającym zapis logów.

To kryteria do pomiaru, nie osiągnięte wyniki. Szczegółową procedurę pomiaru trzeba opisać przed raportowaniem wyników.

## Technologia i instalacja

Zatwierdzone: C++ z bezpośrednim Win32 i systemowym HID API; instalator MSI przez WiX. Wybrano małą liczbę warstw pod cel niskiego zużycia zasobów. Wersje narzędzi i warunki użycia WiX należy sprawdzić przed instalacją; narzędzi C++/WiX jeszcze nie instalowano.

MSI instaluje dla aktualnego użytkownika, bez administratora i usługi systemowej. Aktualizacja ręczna nowszym MSI zachowuje ustawienia; program nie sprawdza aktualizacji przez sieć. Pełne odinstalowanie usuwa autostart, ustawienia, stan ostrzeżenia i dane programu, w tym logi. Brak wariantu portable w pierwszej wersji.

## Kolejność dalszych prac i granica zgody

1. Końcowy przegląd i akceptacja specyfikacji: zakończone 2026-11-22. Polecenie rozpoczęcia prac otrzymano 2026-11-22.
2. Po osobnym poleceniu rozpoczęcia: weryfikacja rzeczywistego odczytu baterii tego egzemplarza.
3. Dopiero po potwierdzeniu odczytu: budowa wskaźnika i MSI oraz testy funkcjonalne i zasobów.

Akceptacja poszczególnych decyzji nie jest poleceniem rozpoczęcia implementacji. Szczegóły protokołu pozostają do zbadania. Pierwszy zestaw SVG użytkownika podłączono 2026-11-23; mapowanie i pipeline opisano w icons/README.md.

## Podstawa i ograniczenia

W poprzedniej diagnostyce Windows wykrywał odbiornik 3151:402D jako sprawny. Halo Battery 1.17.0 wykluczał ten identyfikator z obsługi AJAZZ ze względu na brak potwierdzonej wymiany baterii. Sprawne HID nie potwierdza telemetrii; nazwa modelu przypisana do współdzielonego PID w innym projekcie nie rozstrzyga modelu użytkownika.

Źródła:
- [Halo Battery — protokoły](https://github.com/HeyOkay/HaloBattery/blob/main/docs/protocols.md)
- [Ikona zasobnika Win32](https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shell_notifyiconw)
- [Windows HID API](https://learn.microsoft.com/en-us/windows-hardware/drivers/hid/hid-api)
- [MSI installation context](https://learn.microsoft.com/en-us/windows/win32/msi/installation-context)
- [WiX Package](https://docs.firegiant.com/wix/schema/wxs/package/)

## Znany efekt sprawdzania środowiska

Podczas badania narzędzi polecenie `dotnet tool list --global` uruchomiło inicjalizację .NET, która zgłosiła instalację developerskiego certyfikatu HTTPS. Nie uruchomiono nadawania zaufania. Bez wcześniejszej inwentaryzacji nie potwierdzono, czy certyfikat był nowy. Nie jest to zależność Open Battery; w tamtym etapie nie uruchamiano implementacji ani buildów.
