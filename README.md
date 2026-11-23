# Open Battery

Lekki, natywny wskaźnik baterii AJAZZ AJ179 Pro dla Windows 11 x64. Interfejs po angielsku, dokumentacja po polsku. Licencja [MIT](LICENSE).

## Działanie

- Procent w zasobniku: JetBrains Mono ExtraBold; 70–100% zielony, 30–69% pomarańczowy, 0–29% czerwony. Odczyt co 60 sekund, ręczne Refresh now i natychmiastowe odświeżenie po aktywności AJ179 podczas wyświetlania snu lub błędu.
- Zzz dla nieaktywnej myszy, przekreślona mysz dla odłączonego odbiornika, wykrzyknik dla błędu.
- Jedno ostrzeżenie przy 20% lub mniej, pamiętane po restarcie; ponowne uzbrojenie po przekroczeniu 25%.
- Start with Windows, Copy diagnostics, Debug mode, Open logs folder i Exit w menu.
- Bez sieci, usługi i dodatkowego runtime. Logi wyłączone domyślnie; w debug mode maksymalnie 5 × 1 MB.
- Odbiornik 3151:402D, kolekcja FFFF:0002. Wiarygodny stan ładowania tego modelu nie jest znany i nie jest zgadywany.

## Instalacja

Uruchom lokalnie zbudowany `build/OpenBattery-0.1.3-x64.msi`, następnie Open Battery z menu Start. Instalacja dla bieżącego użytkownika. Ikona może trafić pod strzałkę ukrytych ikon Windows.

Aktualizacja nowszym MSI zachowuje ustawienia. Odinstalowanie przez Windows usuwa ustawienia, autostart i logi programu. MSI nie jest obecnie podpisane ani opublikowane jako release.

## Budowanie i testy

Wymagane: Windows x64, PowerShell, Node.js z npm i .NET SDK do narzędzia WiX. Sam program nie wymaga .NET.

```powershell
.\scripts\bootstrap.ps1
.\scripts\build.ps1
.\scripts\test.ps1
.\build\battery-status.exe
```

Bootstrap pobiera przypięty Zig 0.15.2 jako kompilator C++ i lokalny WiX 4.0.6. Zig jest weryfikowany SHA256. Narzędzia i artefakty pozostają w ignorowanych katalogach .tools, .wix i build; nie zmieniają globalnego PATH. Szczegóły licencji narzędzi: [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

Przed przebudową zamknij Open Battery. Testy parsera, ostrzeżeń, ikon i logów nie potrzebują myszy. Narzędzie battery-status wykonuje rzeczywisty odczyt HID.

## Dokumentacja

- [Specyfikacja](docs/spec.md)
- [Protokół i dowody sprzętowe](docs/protocol.md)
- [Weryfikacja](docs/verification.md)
- [Ikony i podglądy](icons/README.md)
- [Słownik](GLOSSARY.md)
