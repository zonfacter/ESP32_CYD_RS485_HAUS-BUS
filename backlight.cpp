#include "backlight.h"
#include "communication.h"
#include "service_manager.h"  // ← NEU: ServiceManager Include hinzufügen
#include <Arduino.h>

// Einige Definitionen für die direkte Register-Manipulation
#include "driver/ledc.h"

// Aktuelle Hintergrundbeleuchtung
int currentBacklight = DEFAULT_BACKLIGHT;
int backlightValue = DEFAULT_BACKLIGHT;

// PWM-Initialisierung Flag
static bool pwmInitialized = false;


// Initialisiere die Hintergrundbeleuchtung - KORRIGIERT: PWM sofort initialisieren
void setupBacklight() {
  Serial.println("DEBUG: Initialisiere Hintergrundbeleuchtung...");
  
  // PWM sofort initialisieren (wie im funktionierenden Test-Sketch)
  ledcAttach(TFT_BL_PIN, PWM_FREQ, PWM_RESOLUTION);
  pwmInitialized = true;
  
  Serial.print("DEBUG: PWM initialisiert - Pin: ");
  Serial.print(TFT_BL_PIN);
  Serial.print(", Frequenz: ");
  Serial.print(PWM_FREQ);
  Serial.print("Hz, Auflösung: ");
  Serial.print(PWM_RESOLUTION);
  Serial.println(" Bit");
  
  // Standardhelligkeit setzen
  setBacklight(DEFAULT_BACKLIGHT);
  
  Serial.print("DEBUG: Standardhelligkeit gesetzt: ");
  Serial.print(DEFAULT_BACKLIGHT);
  Serial.println("%");
}

// PWM-Steuerung für die Hintergrundbeleuchtung (0-100%) - KORRIGIERT
void setBacklight(int percent) {
  percent = constrain(percent, 0, 100);
  currentBacklight = percent;

  // Map auf 8-Bit PWM-Wert
  int duty = map(percent, 0, 100, 0, 255);

  // Sicherheitsprüfung: PWM muss initialisiert sein
  if (!pwmInitialized) {
    Serial.println("ERROR: PWM nicht initialisiert! Rufe setupBacklight() auf.");
    setupBacklight();
    return;
  }

  // PWM-Wert setzen
  ledcWrite(TFT_BL_PIN, duty);

  // Detaillierte Debug-Ausgabe
  Serial.print("DEBUG: Hintergrundbeleuchtung gesetzt - ");
  Serial.print(percent);
  Serial.print("% → PWM-Wert: ");
  Serial.print(duty);
  Serial.print(" (Pin ");
  Serial.print(TFT_BL_PIN);
  Serial.println(")");
  
  // Zusätzliche Verifikation: PWM-Wert zurücklesen (falls möglich)
  Serial.print("DEBUG: PWM-Kanal aktiv, Duty Cycle gesetzt auf ");
  Serial.println(duty);
}

void setBacklightPWM(int value) {
  // Einfach die vorhandene setBacklight-Funktion nutzen
  setBacklight(value);
}

// Sendet den aktuellen Status der Hintergrundbeleuchtung
void sendBacklightStatus() {
  // *** KORRIGIERT: Device ID vom ServiceManager holen ***
  String currentDeviceID = serviceManager.getDeviceID();
  
  // Telegramm manuell aufbauen um sicherzustellen, dass die richtige Device ID verwendet wird
  String telegram = String((char)START_BYTE) + currentDeviceID + ".LBN.16.STATUS." + String(currentBacklight) + String((char)END_BYTE);
  
  #if DB_TX_INFO == 1
    Serial.print("DEBUG: Sende Backlight-Status mit Device ID ");
    Serial.print(currentDeviceID);
    Serial.print(": ");
    Serial.print(currentBacklight);
    Serial.print("% - Telegramm: ");
    Serial.println(telegram);
  #endif
  
  // Direkt über CSMA/CD senden
  if (!addToSendQueue(telegram, 7, false)) {  // Niedrige Priorität für Status
    #if DB_TX_INFO == 1
      Serial.println("DEBUG: Konnte Backlight-Status nicht senden");
    #endif
  }
}