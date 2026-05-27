# Reflex Tester — Gra na refleks z Arduino UNO

[![License: CC0](https://img.shields.io/badge/License-CC0_1.0-lightgrey.svg)](https://creativecommons.org/publicdomain/zero/1.0/)

> Sprawdź swój refleks! Zapal LED losowo zapala się – wciśnij odpowiedni przycisk jak najszybciej.  
> Czas reakcji wyświetla się na LCD i pokazuje servo jak wskazówka tachometru.

**[Film demonstracyjny na YouTube »](https://youtube.com/shorts/jfGQCqqiEYI?is=CBfYQlIFeNpK-fi8)**

---

## Co robi projekt

- HC-SR04 wykrywa zbliżającą się rękę i startuje grę po **losowym czasie 1–3 sekund** (nie wiadomo kiedy zaskoczy — to sedno gry)
- Losowo zapala się jedna z diod LED
- Gracz wciśka odpowiadający przycisk jak najszybciej
- LCD pokazuje **czas reakcji w milisekundach** i aktualny **rekord**
- **Servo 1** obraca się jak wskazówka tachometru — im szybciej, tym wyżej
- **Servo 2** wskazuje aktualny poziom trudności (1–4)
- **Enkoder obrotowy** zmienia poziom trudności (więcej LED-ek, krótszy czas)
- **Potencjometr** reguluje czułość wykrywania ręki (10–30 cm)
- Zły przycisk lub timeout → reset punktów, servo opada

---

## Poziomy trudności

| Poziom | Aktywne LED/przyciski | Czas na reakcję |
| :----: | :-------------------: | :-------------: |
|   1    |           2           |     2000 ms     |
|   2    |           3           |     1500 ms     |
|   3    |           4           |     1000 ms     |
|   4    |           4           |     750 ms      |

---

## Lista elementów

| Element                  | Ilość | Uwagi                      |
| ------------------------ | :---: | -------------------------- |
| Arduino UNO              |   1   |                            |
| Wyświetlacz LCD I2C 16x2 |   1   | Adres 0x27 lub 0x3F        |
| Czujnik HC-SR04          |   1   |                            |
| Dioda LED                |   4   | Różne kolory jeśli możliwe |
| Tact switch              |   4   |                            |
| Micro servo SG90         |   2   |                            |
| Enkoder obrotowy         |   1   | Waveshare SKU 9533         |
| Potencjometr             |   1   |                            |
| Rezystor 1k Ω            |   4   | Do każdej LED              |
| Płytka stykowa + kable   |   –   |                            |

---

## Schemat podłączenia

| Element                | Pin Arduino |
| ---------------------- | ----------- |
| LED 1                  | D2          |
| LED 2                  | D3          |
| LED 3                  | D4          |
| LED 4                  | D5          |
| Przycisk 1             | D6          |
| Przycisk 2             | D7          |
| Przycisk 3             | D8          |
| Przycisk 4             | D9          |
| Servo "gauge" (czas)   | D10         |
| Servo "level" (poziom) | D11         |
| HC-SR04 TRIG           | D12         |
| HC-SR04 ECHO           | D13         |
| Potencjometr (środek)  | A0          |
| Enkoder SIA (CLK)      | A1          |
| Enkoder SIB (DT)       | A2          |
| Enkoder SW             | A3          |
| LCD SDA                | A4          |
| LCD SCL                | A5          |

**LED:** długa nóżka → rezystor 1k Ω → pin Arduino; krótka nóżka → GND  
**Przyciski:** jeden pin → Arduino (INPUT_PULLUP), drugi pin → GND  
**Potencjometr:** lewy → GND, środek → A0, prawy → 5V

---

## Instrukcja wykonania krok po kroku

### Krok 1 — Zainstaluj biblioteki

Otwórz Arduino IDE → **Szkic → Dołącz bibliotekę → Zarządzaj bibliotekami**

Wyszukaj i zainstaluj:

- `LiquidCrystal_I2C` (autor: Frank de Brabander)
- `Servo` – wbudowana, nie trzeba instalować

---

### Krok 2 — Podłącz wyświetlacz LCD

LCD ma 4 piny: GND, VCC, SDA, SCL

```
LCD GND  →  Arduino GND
LCD VCC  →  Arduino 5V
LCD SDA  →  Arduino A4
LCD SCL  →  Arduino A5
```

> Jeśli LCD nic nie pokazuje po uruchomieniu — pokręć małym śrubokrętem potencjometr kontrastu z tyłu modułu I2C.

---

### Krok 3 — Podłącz czujnik HC-SR04

```
HC-SR04 VCC   →  5V
HC-SR04 GND   →  GND
HC-SR04 TRIG  →  D12
HC-SR04 ECHO  →  D13
```

---

### Krok 4 — Podłącz diody LED

Każda dioda wymaga rezystora 1k Ω (chroni przed spaleniem).

```
D2 → rezystor 1kΩ → długa nóżka LED1 → krótka nóżka → GND
D3 → rezystor 1kΩ → długa nóżka LED2 → krótka nóżka → GND
D4 → rezystor 1kΩ → długa nóżka LED3 → krótka nóżka → GND
D5 → rezystor 1kΩ → długa nóżka LED4 → krótka nóżka → GND
```

> Długa nóżka (+) = anoda, krótka nóżka (-) = katoda.

---

### Krok 5 — Podłącz przyciski tact switch

Przyciski używają INPUT_PULLUP — nie potrzeba zewnętrznych rezystorów.

```
Przycisk 1: pin 1 → D6,  pin 2 → GND
Przycisk 2: pin 1 → D7,  pin 2 → GND
Przycisk 3: pin 1 → D8,  pin 2 → GND
Przycisk 4: pin 1 → D9,  pin 2 → GND
```

> Ważne: każdy przycisk musi być umieszczony **dokładnie naprzeciwko** swojej LED.

> Tact switch ma 4 nóżki, ale są parami — nóżki po tej samej stronie są zawsze zwarte. Podłącz kable po **przeciwnych stronach** przycisku (góra-dół), nie po tej samej (lewo-lewo). Najprościej: obróć przycisk na płytce o 90° jeśli nie działa.

---

### Krok 6 — Podłącz serwomechanizmy SG90

Każde servo ma 3 przewody: brązowy (GND), czerwony (5V), pomarańczowy (sygnał).

```
Servo 1 (wskazówka czasu): sygnał → D10
Servo 2 (wskazówka poziomu): sygnał → D11
Oba servo: czerwony → 5V, brązowy → GND
```

> Jeśli servo drży lub Arduino się resetuje — podłącz 5V serwomechanizmów bezpośrednio do zewnętrznego zasilacza 5V.

---

### Krok 7 — Podłącz enkoder obrotowy

```
Enkoder SIA (CLK)  →  A1
Enkoder SIB (DT)   →  A2
Enkoder SW         →  A3
Enkoder +          →  3.3V lub 5V
Enkoder GND        →  GND
```

---

### Krok 8 — Podłącz potencjometr

```
Lewy pin    →  GND
Środkowy    →  A0
Prawy pin   →  5V
```

---

### Krok 9 — Wgraj kod

1. Otwórz plik `reflex_tester.ino` w Arduino IDE
2. Wybierz **Narzędzia → Płytka → Arduino UNO**
3. Wybierz właściwy port COM
4. Kliknij **Wgraj** (strzałka →)
5. Po wgraniu LCD powinien pokazać `Reflex Tester`

---

### Krok 10 — Pierwsze uruchomienie

1. Pokręć enkoderem — servo poziomu powinno się ruszyć, na LCD zmieni się cyfra
2. Zbliż rękę do HC-SR04 na odległość ~15 cm — LCD pokaże `Gotowy? Czekaj...`
3. Po losowym czasie (1–3 sekundy) zapali się jedna z LED — wciśnij odpowiadający przycisk jak najszybciej!
4. LCD pokaże Twój czas reakcji w milisekundach i rekord sesji
5. Potencjometrem możesz regulować na jaką odległość reaguje czujnik (10–30 cm)
6. Wciśnięcie gałki enkodera resetuje rekord i punkty

---

## Rozwiązywanie problemów

| Problem                         | Możliwa przyczyna                    | Rozwiązanie                                                               |
| ------------------------------- | ------------------------------------ | ------------------------------------------------------------------------- |
| LCD nic nie pokazuje            | Zły adres I2C                        | Zmień 0x27 na 0x3F w kodzie                                               |
| LCD pokazuje kwadraty           | Za niski kontrast                    | Pokręć potencjometr z tyłu modułu                                         |
| Servo drży                      | Za mały prąd z USB                   | Zasil serwomechanizmy zewnętrznie                                         |
| Czujnik nie reaguje             | Zbyt duża odległość                  | Pokręć potencjometrem w prawo                                             |
| Przycisk działa bez naciśnięcia | Kable po tej samej stronie przycisku | Podłącz kable po przeciwnych stronach (góra-dół) lub obróć przycisk o 90° |
| Tylko jeden przycisk działa     | Pozostałe źle zorientowane           | Sprawdź orientację każdego przycisku osobno                               |

---

## Licencja

Projekt udostępniony na licencji **Creative Commons Zero (CC0 1.0)**.  
Możesz go używać, modyfikować i rozpowszechniać bez ograniczeń.

[![CC0](https://licensebuttons.net/p/zero/1.0/88x31.png)](https://creativecommons.org/publicdomain/zero/1.0/)
