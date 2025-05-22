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
  // Konfiguriere den Pin als Ausgang (für Fallback-Modus)
  pinMode(TFT_BL_PIN, OUTPUT);
  
  // LEDC Timer-Konfiguration
  ledc_timer_config_t ledc_timer = {
    .speed_mode       = LEDC_HIGH_SPEED_MODE,
    .duty_resolution  = (ledc_timer_bit_t)PWM_RESOLUTION,
    .timer_num        = LEDC_TIMER_0,
    .freq_hz          = PWM_FREQ,
    .clk_cfg          = LEDC_AUTO_CLK
  };
  
  // LEDC Timer initialisieren
  if (ledc_timer_config(&ledc_timer) == ESP_OK) {
    // LEDC Channel-Konfiguration
    ledc_channel_config_t ledc_channel = {
      .gpio_num       = TFT_BL_PIN,
      .speed_mode     = LEDC_HIGH_SPEED_MODE,
      .channel        = (ledc_channel_t)TFT_BL_CHANNEL,
      .intr_type      = LEDC_INTR_DISABLE,
      .timer_sel      = LEDC_TIMER_0,
      .duty           = 0,
      .hpoint         = 0
    };
    
    // LEDC Channel initialisieren
    if (ledc_channel_config(&ledc_channel) == ESP_OK) {
      Serial.println("ESP-IDF LEDC für Hintergrundbeleuchtung initialisiert");
      setBacklight(DEFAULT_BACKLIGHT);
      return;
    }
  }
  
  // Fallback bei Fehlern: Einfaches digitalWrite
  Serial.println("PWM-Initialisierung fehlgeschlagen, verwende einfaches Ein/Aus");
  digitalWrite(TFT_BL_PIN, currentBacklight > 0 ? HIGH : LOW);
}

// PWM-Steuerung für die Hintergrundbeleuchtung (0-100%)
void setBacklight(int percent) {
  // Begrenze den Wert auf 0-100%
  percent = constrain(percent, 0, 100);
  currentBacklight = percent;
  
  // Berechne den Duty-Cycle (0 bis 2^resolution-1)
  uint32_t maxDuty = (1 << PWM_RESOLUTION) - 1;
  uint32_t duty = map(percent, 0, 100, 0, maxDuty);
  
  // Setze den Duty-Cycle über ESP-IDF API
  if (ledc_set_duty(LEDC_HIGH_SPEED_MODE, (ledc_channel_t)TFT_BL_CHANNEL, duty) != ESP_OK ||
      ledc_update_duty(LEDC_HIGH_SPEED_MODE, (ledc_channel_t)TFT_BL_CHANNEL) != ESP_OK) {
    // Fallback bei Fehler
    digitalWrite(TFT_BL_PIN, percent > 0 ? HIGH : LOW);
  }
  
  Serial.print("Hintergrundbeleuchtung: ");
  Serial.print(percent);
  Serial.print("% (Duty: ");
  Serial.print(duty);
  Serial.print("/");
  Serial.print(maxDuty);
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