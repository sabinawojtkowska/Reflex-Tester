/*
 * ============================================================
 *  REFLEX TESTER — gra na refleks
 * ============================================================
 *  Sprzet:
 *    Arduino UNO
 *    LCD I2C 16x2      SDA=A4, SCL=A5
 *    HC-SR04           TRIG=D12, ECHO=D13
 *    4x LED            D2, D3, D4, D5
 *    4x tact switch    D6, D7, D8, D9  (INPUT_PULLUP)
 *    Servo gauge       D10  (wskazowka czasu reakcji)
 *    Servo level       D11  (wskazowka poziomu trudnosci)
 *    Enkoder CLK=A1, DT=A2, SW=A3
 *    Potencjometr      A0  (czulosc wykrywania reki)
 *
 *  Biblioteki: LiquidCrystal_I2C, Servo (wbudowana)
 * ============================================================
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

// Piny
const int LED_PINS[4]  = {2, 3, 4, 5};
const int BTN_PINS[4]  = {6, 7, 8, 9};
const int SERVO_GAUGE  = 10;
const int SERVO_LEVEL  = 11;
const int HC_TRIG      = 12;
const int HC_ECHO      = 13;
const int POT_PIN      = A0;
const int ENC_CLK      = A1;
const int ENC_DT       = A2;
const int ENC_SW       = A3;

// Obiekty
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo servoGauge;
Servo servoLevel;

// Maszyna stanow gry
enum GameState {
  STATE_IDLE,
  STATE_COUNTDOWN,
  STATE_PLAYING,
  STATE_RESULT,
  STATE_WRONG
};

GameState gameState = STATE_IDLE;

// Zmienne gry
int  difficulty  = 1;
int  activeLed   = -1;
long reactionMs  = 0;
long bestTime    = 9999;
int  score       = 0;

// Parametry poziomow
const int LEVEL_LEDS[4]    = {2, 3, 4, 4};
const int LEVEL_TIMEOUT[4] = {2000, 1500, 1000, 750};

// Zmienne czasowe
unsigned long stateStart = 0;
unsigned long ledOnAt    = 0;
unsigned long randomWait = 0;  // losowy czas oczekiwania przed startem [ms]

// Zmienne enkodera
int encLastClk = HIGH;
int encLastSw  = HIGH;

// =======================================================
void setup() {
  Serial.begin(9600);

  for (int i = 0; i < 4; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
    pinMode(BTN_PINS[i], INPUT_PULLUP);
  }

  pinMode(HC_TRIG, OUTPUT);
  pinMode(HC_ECHO, INPUT);

  pinMode(ENC_CLK, INPUT);
  pinMode(ENC_DT,  INPUT);
  pinMode(ENC_SW,  INPUT_PULLUP);

  servoGauge.attach(SERVO_GAUGE);
  servoLevel.attach(SERVO_LEVEL);
  servoGauge.write(0);
  servoLevel.write(0);

  lcd.init();
  lcd.backlight();

  randomSeed(analogRead(A0));

  showIdleScreen();
}

// =======================================================
void loop() {
  handleEncoder();

  switch (gameState) {
    case STATE_IDLE:      handleIdle();      break;
    case STATE_COUNTDOWN: handleCountdown(); break;
    case STATE_PLAYING:   handlePlaying();   break;
    case STATE_RESULT:    handleResult();    break;
    case STATE_WRONG:     handleWrong();     break;
  }
}

// Enkoder - zmiana poziomu trudnosci
void handleEncoder() {
  int clk = digitalRead(ENC_CLK);
  int sw  = digitalRead(ENC_SW);

  if (clk != encLastClk && clk == LOW) {
    int dt = digitalRead(ENC_DT);
    if (dt != clk) {
      difficulty = min(4, difficulty + 1);
    } else {
      difficulty = max(1, difficulty - 1);
    }
    servoLevel.write(map(difficulty, 1, 4, 0, 180));
    Serial.print("Poziom: ");
    Serial.println(difficulty);
    if (gameState == STATE_IDLE) showIdleScreen();
  }
  encLastClk = clk;

  if (sw == LOW && encLastSw == HIGH) {
    bestTime = 9999;
    score    = 0;
    if (gameState == STATE_IDLE) showIdleScreen();
    Serial.println("Reset rekordu!");
  }
  encLastSw = sw;
}

// Pomiar odleglosci HC-SR04
float measureDistance() {
  digitalWrite(HC_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(HC_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(HC_TRIG, LOW);
  long dur = pulseIn(HC_ECHO, HIGH, 20000);
  if (dur == 0) return 999;
  return dur * 0.0343 / 2.0;
}

// Ekran glowny
void showIdleScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Reflex Tester   ");
  lcd.setCursor(0, 1);
  lcd.print("Lvl:");
  lcd.print(difficulty);
  if (bestTime < 9999) {
    lcd.print(" Rek:");
    lcd.print(bestTime);
    lcd.print("ms  ");
  } else {
    lcd.print(" Zbliz reke!  ");
  }
}

// Stan: IDLE - czekaj az gracz zbliza reke
void handleIdle() {
  int potVal = analogRead(POT_PIN);
  float triggerDist = map(potVal, 0, 1023, 10, 30);

  float dist = measureDistance();

  if (dist > 2 && dist < triggerDist) {
    // Wylosuj czas oczekiwania 1000-3000ms
    randomWait = random(1000, 3001);

    gameState  = STATE_COUNTDOWN;
    stateStart = millis();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("    Gotowy?     ");
    lcd.setCursor(0, 1);
    lcd.print("   Czekaj...    ");

    Serial.print("Losowy czas oczekiwania: ");
    Serial.print(randomWait);
    Serial.println("ms");
  }
}

// Stan: COUNTDOWN - czekaj losowy czas, potem start
void handleCountdown() {
  if (millis() - stateStart >= randomWait) {
    startRound();
  }
}

// Zapal losowy LED i przejdz do STATE_PLAYING
void startRound() {
  int numLeds = LEVEL_LEDS[difficulty - 1];
  activeLed   = random(0, numLeds);

  for (int i = 0; i < 4; i++) digitalWrite(LED_PINS[i], LOW);
  digitalWrite(LED_PINS[activeLed], HIGH);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("    NACISKAJ!   ");
  lcd.setCursor(0, 1);
  lcd.print(">>> Przycisk ");
  lcd.print(activeLed + 1);
  lcd.print(" <<<");

  ledOnAt   = millis();
  gameState = STATE_PLAYING;

  Serial.print("LED: ");
  Serial.println(activeLed + 1);
}

// Stan: PLAYING - czekaj na wcisnięcie przycisku
void handlePlaying() {
  unsigned long elapsed = millis() - ledOnAt;
  int numLeds = LEVEL_LEDS[difficulty - 1];
  int timeout = LEVEL_TIMEOUT[difficulty - 1];

  for (int i = 0; i < numLeds; i++) {
    if (digitalRead(BTN_PINS[i]) == LOW) {
      digitalWrite(LED_PINS[activeLed], LOW);

      if (i == activeLed) {
        // Poprawny przycisk
        reactionMs = elapsed;
        if (reactionMs < bestTime) bestTime = reactionMs;
        score++;

        int angle = map(constrain(reactionMs, 0, 2000), 0, 2000, 180, 0);
        servoGauge.write(angle);

        gameState  = STATE_RESULT;
        stateStart = millis();
        showResult();
      } else {
        // Zly przycisk
        score      = 0;
        gameState  = STATE_WRONG;
        stateStart = millis();
        servoGauge.write(0);

        for (int j = 0; j < 4; j++) digitalWrite(LED_PINS[j], LOW);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("  ZLY PRZYCISK! ");
        lcd.setCursor(0, 1);
        lcd.print("  Punkty reset  ");
      }
      return;
    }
  }

  // Timeout
  if (elapsed > timeout) {
    score      = 0;
    gameState  = STATE_WRONG;
    stateStart = millis();
    servoGauge.write(0);

    for (int i = 0; i < 4; i++) digitalWrite(LED_PINS[i], LOW);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("   ZA WOLNO!    ");
    lcd.setCursor(0, 1);
    lcd.print(">");
    lcd.print(timeout);
    lcd.print("ms  Pkt:0   ");
  }
}

// Wyswietl ekran wyniku
void showResult() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Czas:");
  lcd.print(reactionMs);
  lcd.print("ms      ");
  lcd.setCursor(0, 1);
  if (reactionMs == bestTime) {
    lcd.print("** NOWY REKORD!*");
  } else {
    lcd.print("Rek:");
    lcd.print(bestTime);
    lcd.print("ms Pkt:");
    lcd.print(score);
    lcd.print("  ");
  }

  Serial.print("Czas reakcji: ");
  Serial.print(reactionMs);
  Serial.println("ms");
}

// Stan: RESULT - czekaj 3s i wróc do IDLE
void handleResult() {
  if (millis() - stateStart >= 3000) {
    gameState = STATE_IDLE;
    showIdleScreen();
  }
}

// Stan: WRONG - czekaj 2s i wróc do IDLE
void handleWrong() {
  if (millis() - stateStart >= 2000) {
    gameState = STATE_IDLE;
    servoGauge.write(0);
    showIdleScreen();
  }
}
