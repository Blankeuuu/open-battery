# AJ179 Pro — odczyt baterii przez 2.4 GHz

## Zakres

Tylko odbiornik 3151:402D i kolekcja producenta FFFF:0002 z 65-bajtowym raportem feature w Windows. Kolekcje ruchu myszy i klawiatury nie są otwierane do komunikacji.

Zapytanie: 65 bajtów, ID raportu 0 w bajcie 0, komenda F7 w bajcie 1, reszta zera. Po 50 ms odczyt feature report ID 0. Nie wysyłamy komend ustawień, firmware, resetu ani parowania.

## Format lokalnie zaobserwowany

Pierwsze osiem bajtów aktywnej myszy: `00 00 00 64 01 00 01 02`.
Wcześniejszy odczyt nieaktywności: `00 00 00 64 01 01 01 02`.

Windows zachowuje bajt ID raportu:
- Bajt 0: ID raportu, 0.
- Bajt 1: gotowość odczytu, 0/1.
- Bajt 3: poziom baterii 0..100.
- Bajt 5: mysz aktywna przy 0, nieaktywna przy 1.
- Bajt 6: gotowość wysyłania, 0/1.

Parser sprawdza długość, ID, zakres procentu i pól statusu; odrzuca pusty raport. Raport nieaktywności nie odświeża procentu ani czasu poprawnego pomiaru. Brak komunikacji jest błędem, nie snem. Stan ładowania nie jest ustalony i pozostaje nieznany.

## Dowody i granice

Trzy kolejne odczyty lokalnego odbiornika zwróciły aktywną mysz, 100% i brak błędu. Użytkownik potwierdził działanie myszy poza stacją przez 2.4 GHz oraz pokazał web app z pełną ikoną baterii. Nie jest to kalibracja rzeczywistego naładowania ogniwa. Nie testowano jeszcze całego cyklu rozładowania, ładowania ani kontrolowanego wyłączenia/wybudzenia.

Pierwsza diagnostyka odczytywała niewłaściwie skonstruowaną komendę F7 i zwracała podobny status. Zastąpiono ją sekwencją report ID 0 + F7 + odczyt report ID 0; narzędzie produkcyjne używa tej sekwencji. Nie przyjęto ogólnej reguły „dowolny bajt 0..100 jest baterią”.

## Źródła

Implementacja Open Battery została napisana niezależnie na podstawie udokumentowanego formatu i lokalnych pomiarów; nie kopiowano kodu sterowników innych projektów.

- [Aj179PStat — opis sprzętu i ograniczeń](https://github.com/GetTheNya/Aj179PStat/tree/4b0edefa692a02e7c29c618660862ba0796ba315).
- [OpenMouse — AJ179 Pro](https://github.com/OpenMouse-Project/mouse-protocol/blob/93c4cc601d448ea481eb59d7b785eec75ae16ec2/docs/ajazz-aj179-pro.md).
- [OpenMouse — GearHub HID](https://github.com/OpenMouse-Project/mouse-protocol/blob/93c4cc601d448ea481eb59d7b785eec75ae16ec2/src/drivers/gearhub/hid.ts).
- [HID API Microsoft](https://learn.microsoft.com/en-us/windows-hardware/drivers/hid/hid-api).
