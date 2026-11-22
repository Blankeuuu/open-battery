# Open Battery

Lekki wskaźnik baterii myszy dla Windows. Projekt jest na etapie uzgadniania działania; implementacja nie została rozpoczęta.

## Uzgodniony zakres

- Ikona w zasobniku systemowym obok zegara z widocznym procentem baterii.
- Pierwsze urządzenie: AJAZZ AJ179 Pro przez odbiornik 2.4 GHz `3151:402D`.
- Odczyt co 60 sekund podczas normalnej pracy.
- Wskazanie poziomu baterii i stanu ładowania oraz jedno ostrzeżenie o niskiej baterii.
- Minimalny narzut, bez stałego okna i bez aktywności sieciowej programu.
- Bez historii, wykresów i prognozy pozostałych godzin w pierwszej wersji.

Obsługa odczytu tego egzemplarza nie jest jeszcze potwierdzona. Działanie myszy jako HID nie potwierdza obsługi telemetrii baterii.

## Dokumentacja

- [Specyfikacja robocza](docs/spec.md)
- [Słownik](GLOSSARY.md)

Publiczne repozytorium na licencji [MIT](LICENSE). Implementacja wymaga zatwierdzenia pełnej specyfikacji przez użytkownika.
