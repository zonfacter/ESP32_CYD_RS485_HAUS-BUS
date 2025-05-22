#include "backlight.h"
#include "communication.h"
#include <Arduino.h>

// Einige Definitionen für die direkte Register-Manipulation
#include "driver/ledc.h"

// Aktuelle Hintergrundbeleuchtung
int currentBacklight = DEFAULT_BACKLIGHT;
int backlightValue = DEFAULT_BACKLIGHT;

// Initialisiere die Hintergrundbeleuchtung mit direktem ESP-IDF API-Aufruf
void setupBacklight() {
  setBacklight(DEFAULT_BACKLIGHT);
}


// PWM-Steuerung für die Hintergrundbeleuchtung (0-100%)
// Initialisierung und Helligkeit setzen
void setBacklight(int percent) {
  percent = constrain(percent, 0, 100);
  currentBacklight = percent;

  // Map auf 8-Bit PWM-Wert
  int duty = map(percent, 0, 100, 0, 255);

  // LEDC mit neuer API (Arduino-ESP32 3.x)
  static bool pwmAttached = false;
  if (!pwmAttached) {
    ledcAttach(TFT_BL_PIN, PWM_FREQ, PWM_RESOLUTION);
    pwmAttached = true;
  }

  ledcWrite(TFT_BL_PIN, duty);

  Serial.print("Hintergrundbeleuchtung: ");
  Serial.print(percent);
  Serial.print("% (PWM: ");
  Serial.print(duty);
  Serial.println(")");
}



void setBacklightPWM(int value) {
  // Einfach die vorhandene setBacklight-Funktion nutzen
  setBacklight(value);
}

// Sendet den aktuellen Status der Hintergrundbeleuchtung
void sendBacklightStatus() {
  sendTelegram("LBN", "16", "STATUS", String(currentBacklight));
}