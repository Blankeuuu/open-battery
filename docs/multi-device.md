# Obsługa wielu urządzeń — 0.2.4

Uzgodnienia Q1–Q28 z 2026-11-25. Kanoniczne wymagania i historia decyzji: `wiki/projects/open-battery.md` w Markdown Vault.

## Wskaźniki

Od 0.2.4 cyfry są około 11% wyższe, a obwódka o 25% grubsza. Sprawdzono natywne podglądy w 16/20/24/32 px, w tym wartość 100.

Ostatecznie użytkownik zastąpił warianty A/C kolorowym okręgiem wokół procentu: jedno miejsce w zasobniku na urządzenie. Kolor okręgu jest tożsamością urządzenia, kolor cyfr nadal oznacza poziom baterii. W Devices przycisk Ring color otwiera natywny wybór koloru. Wybór jest zapamiętywany osobno dla każdego egzemplarza. Domyślne kolory: mysz #F4565E, kontroler #B182FF, słuchawki #43C6D2. JetBrains Mono ExtraBold jest prywatnie osadzony w EXE. Progi: 70–100 zielony, 30–69 pomarańczowy, 0–29 czerwony. Opcjonalny numer jest teraz etykietą w dymku, bez dodatkowego znaku w ikonie. Błyskawica przy dolnej krawędzi oznacza wyłącznie potwierdzone ładowanie; nie zmienia koloru tożsamości okręgu ani progów procentu. Przybliżenie poziomu jest opisane w dymku i Devices, bez znaku ≈ w ikonie.

Rozłączenie: x w okręgu, bez procentu. Nieaktywna mysz: z. Brak obsługiwanego poziomu: ?. Błąd odczytu: !. Kolor okręgu pozostaje przypisany do urządzenia także w tych stanach. Ostatni poprawny procent i jego czas pozostają historyczne w dymku. Sam upływ czasu nie zmienia poprawnego wyniku w błąd; nieudana próba odczytu już tak. Domyślna ikona Open Battery pozostaje, gdy wszystkie urządzenia są wyłączone.

Lewy klik otwiera Devices z wybranym urządzeniem; prawy udostępnia odświeżenie urządzenia, Devices i ustawienia aplikacji. Kolejnością i ukrywaniem ikon zarządza Windows.

## Devices

Lista zawiera wykryte i zapamiętane urządzenia. Nowe urządzenia są domyślnie wyłączone; dotychczasowa mysz zachowuje widoczność podczas migracji. Szczegóły pokazują typ, połączenie, poziom, stan, czas sprawdzenia, źródło i błąd. Można ustawić własną nazwę, numer dymka, kolor okręgu oraz niezależne miganie niskiej baterii. Zmiany nazwy/numeru zatwierdza Save; przełączniki działają od razu.

Forget device usuwa ustawienia Open Battery, nie parowanie Windows. Po ponownym wykryciu urządzenie wraca z domyślną nazwą i wyłączonym wskaźnikiem. AJ179 zachowuje wyłączony wpis migracyjny, aby ponowny start nie włączył go automatycznie. Ponowne połączenie zachowuje ustawienia urządzeń, których nie zapomniano. Okno nie ma aktywnego timera w tle; po zamknięciu jego kontrolki i font są zwalniane. Interfejs i MSI są po angielsku.

## Ostrzeżenia i ustawienia

Od 0.2.3 użytkownik zastąpił wszystkie powiadomienia o niskiej baterii miganiem okręgu. Przy poprawnym aktualnym odczycie 0–20% okrąg naprzemiennie świeci normalnie i przygasa co 1 s (pełny cykl 2 s). Cyfry nie migają. Miganie trwa, dopóki urządzenie jest wybrane, opcja Blink at 20% or less włączona, poziom ≤20% i brak potwierdzonego ładowania. Odczyt 21%, rozłączenie, nieaktywność, błąd albo brak danych zatrzymują miganie; historyczny niski procent nie wystarcza.

BlinkLow jest niezależnym ustawieniem per urządzenie, domyślnie włączonym. Dawne Warned oraz Notifications nie blokują nowego efektu; te wartości zachowano wyłącznie dla zgodności ustawień. Stara reguła jednego popupu i ponownego uzbrojenia powyżej 25% została zastąpiona. Wyłączenie migania w Devices jest zapamiętywane.

Renderowane są tylko dwie klatki po zmianie danych/koloru. Jeden wspólny timer działa wyłącznie, gdy co najmniej jedno urządzenie wymaga migania. Tick aktualizuje tylko takie urządzenia przez NIF_ICON, bez renderowania fontu, wątku HID, logowania lub zapisów ustawień. Po ustaniu niskiego stanu timer jest usuwany. Przy nieznanym ładowaniu AJ179 efekt nie może być wyłączony na podstawie domniemanej stacji.

Ustawienia znajdują się w HKCU `Software\Open Battery\Devices`, osobno dla tożsamości urządzeń. Nazwa wyświetlana nie jest tożsamością. Sony przez USB/Bluetooth scala się tylko przy pasującym standardowym identyfikatorze fizycznym; bez niego używany jest kontener Windows, bez zgadywania po nazwie. Identyfikatory nie trafiają do logów ani schowka diagnostycznego; używany jest lokalny skrót tożsamości. Ustawienia i logi pozostają lokalne. Aktualizacja MSI zachowuje ustawienia; pełne odinstalowanie czyści je istniejącym mechanizmem.

## Odczyty i lekkość

AJ179 zachowuje odczyt przez odbiornik 3151:402D i szybkie wybudzanie. Kontrolowany test zdjęcia i ponownego odłożenia na stację wykazał identyczny 65-bajtowy raport F7 (70%, active). Wcześniejszy inactive występuje również podczas snu; nie jest flagą ładowania. Nie wykryto dodatkowej właściwości ładowania Windows. Stan ładowania stacji pozostaje unknown, nie jest wnioskowany ze wzrostu poziomu. Po sprawdzeniu qmk.top/v4 i raportów GET 0/5 użytkownik zdecydował pominąć wykrywanie ładowania na stacji; odrzucił też ręczne oznaczenie. Nie twierdzimy, że zbadano wszystkie możliwe komendy firmware. Surowy strumień myszy nie jest stale obsługiwany podczas aktywnego odczytu. Nowe funkcje nie wysyłają ustawień myszy ani kontrolera.

Bluetooth klasyczny: wyliczane są urządzenia znane Windows, **bez aktywnego inquiry ani prób połączenia**. Poziom jest odczytywany z właściwości Windows, również z węzłów profilu Hands-Free połączonych kontenerem z urządzeniem. Właściwość HFP `{104EA319-6EE2-4701-BD47-8DDBF425BBE5}, 2` jest nieudokumentowana publicznie; odczyt ma kontrolę typu i zakresu. Jej brak oznacza brak dostępnego poziomu. System może zwracać wartość z pamięci podręcznej: czas w dymku jest czasem sprawdzenia danych w Windows, nie gwarantowanym czasem fizycznego pomiaru ogniwa. Po rozłączeniu wartość nie jest wyświetlana jako bieżąca.

Sony: DualShock 4 054C:05C4/09CC oraz DualSense 054C:0CE6/0DF2. Pasywny odczyt jednego wejściowego raportu HID z ograniczeniem czasu, wyłącznie dla wybranych urządzeń. Domyślnie brak zapisu, przełączania trybu raportów, zmian LED/wibracji i stałego strumienia. Wyjątek to jawna opcja ExtendedReports opisana poniżej. USB i pełne Bluetooth mają osobne formaty; Bluetooth wymaga poprawnego CRC. Krótkie raporty BT nie mają baterii i dają ?. Windows może dopełniać raport do 547 bajtów; parser sprawdza format i CRC właściwych 78 bajtów, bez mylenia dopełnienia ze skróconym raportem USB. Stopnie baterii są przybliżeniem, nie pomiarem co 1%. Dane przycisków, czujników i dotyku są odrzucane, nie logowane. Stan błędu ładowania nie jest udawanym 0%.

**Opcja pełnych raportów DualShock 4:** domyślnie wyłączona, osobno dla urządzenia w Devices. Na wyraźną zgodę użytkownika włączono ją dla jego podłączonego pada. Gdy aktywny DS4 przez BT nadaje wyłącznie raport 0x01 bez baterii, odczyt kalibracji GET_FEATURE 0x02 przełącza pełne raporty do ponownego połączenia. Nie zapisuje kalibracji ani nie wysyła LED/wibracji. Może zmienić zgodność starszych gier. Wyłączenie opcji zatrzymuje przyszłe żądania; powrót do podstawowego trybu wymaga ponownego połączenia kontrolera. Odczyt nadal jest okresowy, bez stałego strumienia w aplikacji. Rzeczywisty test: Bluetooth, około 75%, charging=no, error=0.

Odczyt kontrolny co 60 s, aktualizacja po zdarzeniach urządzenia i po wznowieniu Windows. Zdarzenia urządzeń są scalane. Nie budzimy sprzętu poleceniami dla samej baterii. Zamknięte Devices nie odświeża ukrytych kontrolek; ikony są renderowane ponownie tylko po zmianie wyglądu. Logi pozostają domyślnie wyłączone, z rotacją 5 × 1 MB po włączeniu debug.

Cel akceptacji: ≤10 MB pamięci prywatnej, średnio <0,1% jednego rdzenia przez 10 min z trzema urządzeniami i zamkniętym Devices. Osobno sprawdzamy otwieranie okna, ponowne połączenia i dłuższą pracę. Cele wymagają pomiaru bieżącej wersji; stare wyniki 0.1.x ich nie zastępują. Brak sieci, usługi i dodatkowego runtime pozostaje wymaganiem. Nie deklarujemy obsługi wszystkich modeli Bluetooth ani aktywnego skanowania BLE/GATT w tej wersji.

## Weryfikacja

Test migania: tests/blink-regression.cpp przechwytuje rzeczywiste wywołania Shell_NotifyIcon i korzysta z prawdziwego timera ukrytego okna. Przed poprawką FAIL przy 19%; po poprawce PASS. Sprawdza brak NIF_INFO, niezmienne cyfry, granice 20/21, stary Warned, brak odczytów HID i alokacji GDI na 1000 ticków oraz stop/opt-out/persistencję.

`scripts/test.ps1`: model AJ179, parsery Sony/CRC, historia i tożsamość urządzeń, niezależne ostrzeżenia, istniejące wybudzanie AJ179, font/zasoby/logi oraz natywne testy Devices i ikon w izolowanym kluczu testowym. `build/devices-status.exe` wykonuje rzeczywiste odczyty bez zapisów i nie drukuje adresów Bluetooth ani numerów seryjnych. Wyniki sprzętowe i pomiary: `docs/verification.md`.

Źródła formatów i API:

- [Sony/Linux hid-playstation — formaty raportów i statusy baterii](https://raw.githubusercontent.com/torvalds/linux/master/drivers/hid/hid-playstation.c) — wykorzystano informacje o protokole, bez kopiowania implementacji sterownika.
- [Microsoft BluetoothFindFirstDevice](https://learn.microsoft.com/en-us/windows/win32/api/bluetoothapis/nf-bluetoothapis-bluetoothfindfirstdevice) — klasyczne Bluetooth; nie wylicza BLE.
- [Microsoft SetupDiGetDevicePropertyW](https://learn.microsoft.com/en-us/windows/win32/api/setupapi/nf-setupapi-setupdigetdevicepropertyw).
- [Microsoft CM_Get_DevNode_PropertyW](https://learn.microsoft.com/en-us/windows/win32/api/cfgmgr32/nf-cfgmgr32-cm_get_devnode_propertyw).
- [Oficjalny SDL — DualShock 4, pełne raporty i odczyt kalibracji](https://github.com/libsdl-org/SDL/blob/main/src/joystick/hidapi/SDL_hidapi_ps4.c).
- [Microsoft ReadFile — wejście overlapped](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-readfile).
