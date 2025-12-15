Światło dla Mamy — Sapiens Annotata
Plik: Swiatlo_Sapiens_Tabela_Ideal.ino
Serial: 115200

## Uwaga: wgrywanie przez bootloader (upload_speed)

- Jeśli podczas uploadu pojawia się błąd `avrdude: stk500_getsync() not in sync`, przyczyną może być niezgodna prędkość bootloadera lub problem z auto-resetem.
- Arduino IDE ma ustawienie "Default" które często dobiera odpowiednią prędkość bootloadera dla wybranej płytki. PlatformIO używa wartości `upload_speed` z pliku `platformio.ini` (jeśli jest ustawiona) albo domyślnej prędkości przypisanej do płytki.
- VSCode + PlatformIO IDE nie skanuje wszystkich możliwych prędkości — stosuje konfigurację projektu. Jeśli Twój bootloader działa na 115200 (np. nowszy Optiboot), dodaj w sekcji środowiska `upload_speed = 115200` w `platformio.ini`.
- Szybkie kroki przy błędzie sync:
  1. Zamknij monitor szeregowy (jeśli otwarty).
  2. Spróbuj upload z wymuszeniem portu: `platformio run -e nanoatmega328 --target upload --upload-port /dev/ttyUSB0 -v`.
  3. Jeśli nadal nie działa, podczas próby uploadu naciśnij krótko przycisk RESET na płytce, aby wymusić wejście do bootloadera.
- Komendy w terminalu PlatformiO .:
-        pio run -e nano_115200 -t upload
-        pio run -e nano_57600 -t upload
- 115200  - to NOWY, szybszy Uploader, natomiast 57600 - to STARY , wolniejszy .
- Jak nie jeden, to drugi i powinno zadziałać. 

