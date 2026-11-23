# Narzędzia i źródła

Kod Open Battery: MIT, copyright 2026 Blankeuuu. Nie skopiowano kodu innych aplikacji do odczytu baterii; źródła wiedzy o protokole wskazano w docs/protocol.md.

## Kompilator

[Zig 0.15.2](https://ziglang.org/download/0.15.2/release-notes.html) i dostarczane biblioteki kompilatora mają własne licencje (m.in. MIT, Apache-2.0 i licencje nagłówków MinGW). Pełne teksty i noty są w pobranym archiwum narzędzia, w tym LICENSE oraz lib/libcxx/LICENSE.TXT i plikach licencyjnych MinGW. Archiwum kompilatora nie jest częścią instalatora Open Battery.

## Instalator

[WiX Toolset 4.0.6](https://github.com/wixtoolset/wix/tree/v4.0.6), copyright .NET Foundation and contributors, licencja [Microsoft Reciprocal License (MS-RL)](https://github.com/wixtoolset/wix/blob/v4.0.6/LICENSE.TXT). Rozszerzenie WixToolset.UI.wixext 4.0.6 dostarcza standardowy interfejs instalatora. Kod źródłowy tych elementów jest dostępny w wskazanym repozytorium WiX; własne źródła instalatora w installer/Package.wxs.

WiX jest lokalnym narzędziem budowania, nie runtime aplikacji. Przypięta wersja 4.0.6 poprzedza wprowadzenie Open Source Maintenance Fee w WiX 6. Zmiana wersji wymaga ponownej oceny warunków. Nie korzystamy z płatnych rozszerzeń FireGiant.

## Generowanie ikon podczas builda

@resvg/resvg-js 2.6.2: MPL-2.0, https://github.com/thx/resvg-js/tree/v2.6.2.
pngjs 7.0.0: MIT, https://github.com/pngjs/pngjs/tree/v7.0.0.
Są używane wyłącznie jako lokalne narzędzia w .tools/icon-tools, z przypiętym package-lock.json. Nie są linkowane ani dołączane do aplikacji i MSI. Oryginalne SVG dostarczył użytkownik projektu; konwersja zachowuje je bez zmian.
