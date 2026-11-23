# Ikony Open Battery

Oryginalne SVG dostarczył użytkownik. Pliki źródłowe pozostają niezmienione.

## Wybrany zestaw

| Zastosowanie | Źródło |
| --- | --- |
| Aplikacja, EXE, menu Start, lista zainstalowanych programów | app.svg |
| Nieaktywna mysz | sleep_32.svg |
| Odłączony odbiornik | disconnected_mouse_x.svg |
| Błąd / brak odczytu | error_32.svg |

Pozostałe warianty rozłączenia są zachowane jako alternatywy: disconnected_mouse_signal.svg, disconnected_broken_link.svg i disconnected_no_signal.svg. Domyślny mouse_x odpowiada uzgodnionemu symbolowi myszy z przekreśleniem.

Procent baterii nadal jest renderowany dynamicznie. Kolory stanu normalnego, niskiej baterii i ładowania nie wymagają zestawu 101 bitmap. Brak wiarygodnego stanu ładowania AJ179 Pro pozostaje bez zmian.

## Budowanie

scripts/render-icons.mjs przetwarza SVG za pomocą lokalnych, przypiętych @resvg/resvg-js 2.6.2 i pngjs 7.0.0. Narzędzia są potrzebne tylko do builda.

- Zasobnik: 16, 20, 24, 32, 40, 48 i 64 px.
- Ikona aplikacji: dodatkowo 128 i 256 px.
- Symbole stanów: szarość #B8B8B8 dla ciemnego motywu i #666666 dla jasnego.
- Barwiona jest gotowa bitmapa, z zachowaniem alfa; maski i wycięcia SVG nie są modyfikowane.
- ICO i zasoby COFF powstają w build/icons i build/icons.obj, po czym są dołączane do EXE. Runtime korzysta z Win32 LoadImage; bez SVG, Node i bibliotek renderera.
- Zmiana motywu odświeża ikonę; wybierany jest odpowiedni wariant jasny/ciemny.
- Brak zasobu uruchamia dotychczasowy renderer awaryjny.

Podglądy po buildzie: build/icons/preview-dark.png i build/icons/preview-light.png. Zawierają też zachowane alternatywy rozłączenia. Oryginalna ikona app.svg jest kolorowa w obu motywach.

## Procent i typografia — 0.1.3

Na polecenie użytkownika kolor cyfr odpowiada wyłącznie poziomowi baterii: 70–100% zielony, 30–69% pomarańczowy, 0–29% czerwony. Zielony nie jest już zarezerwowany dla ładowania. Powiadomienie o niskiej baterii nadal ma osobny próg 20%.

Cyfry używają osadzonego JetBrains Mono ExtraBold 2.304, ładowanego prywatnie przez AddFontMemResourceEx. Nie instalujemy fontu w Windows. Renderer rysuje w czterokrotnie większej skali i redukuje obraz przez uśrednianie pokrycia do premultiplikowanego kanału alfa. Wysokość cyfr jest stała; trzycyfrowe 100 wykorzystuje pełną szerokość ikony. Waga ExtraBold i korekta pokrycia alfa 0,85 wzmacniają czytelność w 16 px, zachowując przezroczyste tło.

Kolory dla ciemnego / jasnego motywu:
- zielony #55D98B / #168A45,
- pomarańczowy #FFB44B / #B46200,
- czerwony #FF6969 / #C82D37.

Podgląd rzeczywistego renderera: scripts/preview-numbers.ps1 po buildzie. Wyniki: build/numbers/preview-dark.png i preview-light.png (16, 20, 24, 32 px i powiększenie ×4). Menu i pozostałe ikony zachowują swój styl.
