


/*
 *  Projekt:  Światło dla Mamy – Sapiens Annotata v1.07a
 *  Platforma: Arduino Nano
 *
 *  Opis:
 *  Program do sterowania czterema kanałami LED poprzez czujnik pojemnościowy.
 *  Zawiera automatyczną kalibrację czujnika, dynamiczne progi dotyku,
 *  efekt FLASH przy maksymalnej jasności oraz funkcję resetu poprzez długie przytrzymanie.
 *
 *  Autorzy:  Alea & Jarosław "zeb’ER" Rochalski
 *  Wersja:   1.07a / 2025
 *  Cytat:   "Światło, które uczy — linijka po linijce."
 */

#include <CapacitiveSensor.h>   // biblioteka do obsługi czujnika pojemnościowego
#include <Timers.h>              // biblioteka do prostych timerów programowych
#include "TimersCompat.h"      // kompatybilność: udostępnia Timer i SECS()
#include <avr/wdt.h>             // biblioteka do obsługi Watchdog Timer (WDT)

// --- KONFIGURACJA PINÓW ---
#define TOUCH_SEND 2             // pin wysyłający sygnał dla czujnika pojemnościowego
#define TOUCH_RECEIVE 4          // pin odbierający sygnał z czujnika (linia pomiarowa)

#define PWM3 6                   // wyjście PWM kanału 1 LED
#define PWM4 9                   // wyjście PWM kanału 2 LED
#define PWM5 10                  // wyjście PWM kanału 3 LED
#define PWM6 11                  // wyjście PWM kanału 4 LED

// --- OBIEKTY I TIMER-y ---
CapacitiveSensor cs_2_4 = CapacitiveSensor(TOUCH_SEND, TOUCH_RECEIVE); // tworzy obiekt czujnika pojemnościowego między pinami 2 i 4
Timer ledBlinkTimer;          // timer do migania wbudowaną diodą LED (sygnalizacja „życia”)
Timer holdTimer;              // timer do wykrywania długiego dotyku
Timer flashTimer;             // timer do efektu FLASH przy maksymalnej jasności
Timer resetTimer;             // timer do nadzoru resetu (3 sekundy dotyku przed Watchdogiem)

// --- PARAMETRY JASNOŚCI ---
int step = 2;                 // krok zmiany jasności (im większy, tym szybsze rozjaśnianie/ściemnianie)
int brightness = 0;           // aktualna wartość jasności (0–255)
int flashCycles = 1;          // liczba błysków efektu FLASH przy maksymalnej jasności

// --- PROGI DOTYKU (dynamiczne, kalibrowane) ---
float touchFactor = 1.9;      // współczynnik progu dotyku (ile razy powyżej tła uznać za dotyk)
float releaseFactor = 1.3;    // współczynnik progu puszczenia dotyku
long baseline = 0;            // wartość tła (średni odczyt czujnika bez dotyku)
long delta = 0;               // różnica pomiędzy aktualnym odczytem a tłem

// --- ZMIENNE STANU SYSTEMU ---
bool touchState = false;      // aktualny stan czujnika dotyku (true = dotyk, false = brak)
bool prevTouchState = false;  // poprzedni stan dotyku (do wykrywania zmian)
bool longTouch = false;       // flaga długiego przytrzymania
bool directionUp = true;      // kierunek regulacji jasności (true = rozjaśnianie, false = ściemnianie)
bool flashActive = false;     // flaga aktywnego efektu FLASH
bool flashDown = false;       // flaga kierunku błysku (true = przygaszenie, false = rozbłysk)
bool resetTimerActive = false;// flaga informująca, że licznik resetu jest aktywny
bool flashUsed = false;       // flaga zaznaczająca, że efekt Flasch ma się wykonać tylko raz

int flashCounter = 0;         // licznik wykonanych błysków FLASH

// --- FUNKCJE POMOCNICZE ---

// Ustawia jasność wszystkich czterech kanałów LED
void setBrightness(int val) {
  val = constrain(val, 0, 255);    // ogranicza wartość jasności do zakresu 0–255
  analogWrite(PWM3, val);          // zapis PWM dla kanału 1
  analogWrite(PWM4, val);          // zapis PWM dla kanału 2
  analogWrite(PWM5, val);          // zapis PWM dla kanału 3
  analogWrite(PWM6, val);          // zapis PWM dla kanału 4
}

// Uruchamia efekt FLASH przy maksymalnej jasności
void startFlash() {
  flashActive = true;              // aktywuje efekt błysku
  flashTimer.restart();            // resetuje licznik czasu błysku
  flashDown = true;                // ustawia kierunek na „przygaszenie”
  flashCounter = 0;                // zeruje licznik błysków
}


// --- FUNKCJA AUTOKALIBRACJI ---
// Mierzy środowisko w momencie włączenia układu i dobiera progi dotyku
void autoCalibrate() {
  Serial.println(F("Kalibracja środowiska..."));   // informacja w monitorze szeregowym

  long minVal = 999999;            // minimalny pomiar w trakcie kalibracji
  long maxVal = 0;                 // maksymalny pomiar w trakcie kalibracji
  long total = 0;                  // suma wszystkich pomiarów (do wyliczenia średniej)

  // wykonuje 100 pomiarów w odstępach 30 ms, by zebrać próbkę środowiska
  for (int i = 0; i < 100; i++) {
    long val = cs_2_4.capacitiveSensor(30); // pomiar z czujnika pojemnościowego (30 próbek wewnętrznych)
    total += val;                            // akumulacja do średniej
    if (val < minVal) minVal = val;          // aktualizacja wartości minimalnej
    if (val > maxVal) maxVal = val;          // aktualizacja wartości maksymalnej
    delay(30);                               // krótka przerwa dla stabilności pomiaru
  }

  baseline = total / 100;                   // wyliczenie średniej wartości tła
  long range = maxVal - minVal;             // obliczenie fluktuacji tła
  if (range < 10) range = 10;               // minimalna wartość różnicy (uniknięcie dzielenia przez zero)

  // wyliczenie współczynnika progu dotyku na podstawie zakresu fluktuacji
  touchFactor = 1.0 + (float)range / (float)baseline * 5.0;  
  if (touchFactor < 1.5) touchFactor = 1.5; // ograniczenie minimalnego współczynnika
  if (touchFactor > 3.0) touchFactor = 3.0; // ograniczenie maksymalnego współczynnika
  releaseFactor = touchFactor - 0.3;        // histereza (próg zwolnienia)

  // komunikaty diagnostyczne
  Serial.print(F("Baseline: ")); Serial.println(baseline);      
  Serial.print(F("Zakres fluktuacji: ")); Serial.println(range);
  Serial.print(F("touchFactor: ")); Serial.println(touchFactor, 2);
  Serial.print(F("releaseFactor: ")); Serial.println(releaseFactor, 2);
  Serial.println(F("Kalibracja zakończona.\n"));   // zakończenie kalibracji

}






// --- FUNKCJA SETUP ---
// Konfiguracja pinów, uruchomienie timerów i kalibracja środowiska
void setup() {
  pinMode(PWM3, OUTPUT);               // ustawienie kanału PWM3 jako wyjście
  pinMode(PWM4, OUTPUT);               // ustawienie kanału PWM4 jako wyjście
  pinMode(PWM5, OUTPUT);               // ustawienie kanału PWM5 jako wyjście
  pinMode(PWM6, OUTPUT);               // ustawienie kanału PWM6 jako wyjście
  pinMode(LED_BUILTIN, OUTPUT);        // wbudowana dioda LED jako wskaźnik działania

  Serial.begin(115200);                  // uruchomienie komunikacji szeregowej 115200 bps
  cs_2_4.set_CS_AutocaL_Millis(0xFFFFFFFF);  // wyłączenie automatycznej kalibracji biblioteki (sterujemy ręcznie)

  ledBlinkTimer.begin(SECS(0.4));      // konfiguracja migania diody LED co 0.4 s
  flashTimer.begin(SECS(0.2));         // interwał błysku FLASH = 200 ms
  holdTimer.begin(SECS(0.15));         // czas detekcji długiego dotyku = 150 ms
  resetTimer.begin(SECS(3));           // czas podtrzymania dotyku przed resetem = 3 s

  wdt_enable(WDTO_8S);                 // aktywacja Watchdog Timer (reset po 8 s braku odświeżenia)
  autoCalibrate();                     // automatyczna kalibracja czujnika środowiska

  Serial.println(F("=== Światło dla Mamy v1.07a 'Sapiens Annotata' uruchomione ===")); // komunikat startowy




}
// --- PĘTLA GŁÓWNA PROGRAMU ---
// Odpowiada za: 
// - cykliczne odczyty czujnika pojemnościowego,
// - adaptację tła (baseline),
// - wykrywanie dotyku i długiego przytrzymania,
// - sterowanie jasnością LED,
// - efekt FLASH przy maksymalnej jasności,
// - nadzór czasu (reset po długim dotyku).
void loop() {

  // --- Miganie wbudowaną diodą LED jako sygnał „życia” systemu ---
  if (ledBlinkTimer.available()) {          // jeśli minął czas interwału migania
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); // zmiana stanu diody LED
    ledBlinkTimer.restart();                // restart timera
  }

  // --- Odczyt wartości z czujnika pojemnościowego ---
  long total = cs_2_4.capacitiveSensor(30); // pobranie aktualnego pomiaru (30 prób dla stabilności)
  delta = total - baseline;                  // obliczenie różnicy pomiędzy bieżącym sygnałem a tłem

  // --- Adaptacja wartości bazowej (baseline) ---
  if (!touchState)                           // adaptacja tylko, gdy brak dotyku (żeby nie „rozjechać” poziomu)
    baseline = (baseline * 0.995) + (total * 0.005); // filtr dolnoprzepustowy: 0.5% nowej wartości, 99.5% starej

  // --- Dynamiczne progi względem tła ---
  if (total > baseline * touchFactor)        // jeśli sygnał przekroczył określony współczynnik tła
    touchState = true;                       // uznaj dotyk za aktywny
  else if (total < baseline * releaseFactor) // jeśli spadł poniżej progu zwolnienia
    touchState = false;                      // uznaj, że dotyk zakończony

  // --- Obsługa długiego dotyku ---
  if (touchState && !prevTouchState) {  
      holdTimer.restart();          // restart JEDNORAZOWO, przy rozpoczęciu dotyku
  }
  if (holdTimer.available() && touchState) {  
      longTouch = true;             // po ~150 ms mamy długi dotyk
  }

  // --- Regulacja jasności LED ---
  if (touchState && longTouch && !flashActive) {  // działa tylko przy aktywnym dotyku i braku efektu FLASH
    if (directionUp) brightness += step;          // jeśli kierunek w górę — zwiększ jasność
    else brightness -= step;                      // jeśli kierunek w dół — zmniejsz jasność
    brightness = constrain(brightness, 0, 255);   // utrzymanie zakresu 0–255

if (brightness >= 255 && !flashUsed) {            //  jeśli osiągnięto maksimum jasności
      brightness = 255;                           // ustalenie sztywnego maksimum
      startFlash();                               // uruchomienie efektu FLASH
      flashUsed = true;       // blokada kolejnych FLASH
}

  }

  // --- Zmiana kierunku po puszczeniu dotyku ---
  if (prevTouchState && !touchState) {            // jeśli poprzednio był dotyk, a teraz go brak
    directionUp = !directionUp;                   // zmień kierunek regulacji (rozjaśnianie ↔ ściemnianie)
    longTouch = false;                            // reset flagi długiego dotyku
    flashUsed = false;                            // erset flagi po puszczeniu przycisku

  }
  prevTouchState = touchState;                    // zapamiętaj aktualny stan jako poprzedni

  // --- Efekt FLASH przy maksymalnej jasności ---
  if (flashActive && flashTimer.available()) {    // jeśli aktywny efekt FLASH i minął czas interwału
    flashTimer.restart();                         // restart timera FLASH
    setBrightness(flashDown ? 100 : 255);         // przełącz między 100 a 255 (przygaszenie ↔ pełna jasność)
    flashDown = !flashDown;                       // odwróć kierunek następnego błysku
    flashCounter++;                               // zwiększ licznik błysków

    if (flashCounter >= flashCycles * 2) {        // jeśli wykonano pełną liczbę cykli (np. 2 × góra/dół)
      flashActive = false;                        // zakończ efekt FLASH
      flashCounter = 0;                           // wyzeruj licznik
      setBrightness(255);                         // pozostaw LED na pełnej jasności
    }
  }

  // --- Aktualizacja jasności LED (jeśli FLASH nieaktywny) ---
  if (!flashActive) setBrightness(brightness);    // ustaw aktualny poziom PWM na wszystkich kanałach

  // --- Nadzór czasu dotyku i reset systemu (Watchdog Timer) ---
  if (touchState) {                               // jeśli palec na czujniku
    if (!resetTimerActive) {                      // jeśli licznik resetu jeszcze nieaktywny
      resetTimer.restart();                       // rozpocznij odliczanie 3 s
      resetTimerActive = true;                    // ustaw flagę aktywnego timera resetu
    }
    if (!resetTimer.available()) {                // dopóki licznik nie osiągnął 3 s
      wdt_reset();                                // regularnie resetuj WDT (nie pozwól na restart)
    }
  } else {                                        // jeśli brak dotyku
    resetTimer.restart();                         // resetuj licznik resetu
    resetTimerActive = false;                     // flaga nieaktywna
    wdt_reset();                                  // regularne odświeżenie WDT
  }

  // --- Sekcja diagnostyczna (wysyłanie danych do monitora szeregowego) ---
  
  
// --- SOFT-PAD DELUX+DELUX ---

// Total
Serial.print(" Total:");
if (total < 10) Serial.print("   ");
else if (total < 100) Serial.print("  ");
else if (total < 1000) Serial.print(" ");
Serial.print(total);
Serial.print("\t");

// Base
Serial.print("  Base:");
if (baseline < 10) Serial.print("   ");
else if (baseline < 100) Serial.print("  ");
else if (baseline < 1000) Serial.print(" ");
Serial.print(baseline);
Serial.print("\t");

// Delta
Serial.print("     Δ:");
if (delta < 10) Serial.print("   ");
else if (delta < 100) Serial.print("  ");
else if (delta < 1000) Serial.print(" ");
Serial.print(delta);
Serial.print("\t");

// Touch
Serial.print(" Touch:");
Serial.print(" ");
Serial.print(touchState);
Serial.print("\t");

// Brt
Serial.print("   Brt:");
if (brightness < 10) Serial.print("   ");
else if (brightness < 100) Serial.print("  ");
else Serial.print(" ");
Serial.print(brightness);
Serial.print("\t");

// Fact
Serial.print("  Fact:");
Serial.print(" ");
Serial.print(touchFactor, 2);
Serial.print("\t");

// RST
Serial.print("   RST:");
Serial.println(resetTimer.available() ? " expired" : " running");





}
