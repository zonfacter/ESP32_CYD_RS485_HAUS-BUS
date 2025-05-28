/**
 * ESP32_CYD_RS485_HAUS_BUS.ino - Version 1.60 mit korrigierter Button-Verarbeitung
 * 
 * Features: 
 * - Timing-basierte Button-Verarbeitung (50ms Verzögerung für STATUS.1)
 * - 10-Sekunden Timeout mit automatischem STATUS.0
 * - Service-Touch über Service-Icon
 * - CSMA/CD + Touch-Interface
 */

#include "config.h"
#include "touch.h"
#include "display.h"
#include "menu.h"
#include "communication.h"
#include "backlight.h"
#include "led.h"
#include "service_manager.h"
#include <EEPROM.h>
#include "header_display.h"

// *** NEU: Display-Kalibrierung (nur für Inbetriebnahme) ***
#include "display_calibration.h"

#include "converter_web_service.h"  // *** NEU ***

// *** NUR DAS TFT-OBJEKT DEFINIEREN ***
TFT_eSPI tft = TFT_eSPI();

// Timing für Hintergrundbeleuchtungs-Status  
unsigned long lastBacklightStatusTime = 0;

// *** NEU: Button-Timing Variablen ***
struct ButtonTiming {
  bool touchActive;
  unsigned long touchStartTime;
  bool status1Sent;
  int activeButtonIndex;
};

ButtonTiming buttonTiming = {false, 0, false, -1};

// *** NEU: Externe Funktion aus communication.cpp ***
extern void applyAllPendingLedStates();

// Timing-Konstanten
const unsigned long BUTTON_CONFIRM_DELAY = 50;    // 50ms Verzögerung für STATUS.1
const unsigned long BUTTON_MAX_TIMEOUT = 10000;   // 10 Sekunden maximale Druckzeit

void setup() {
  Serial.begin(115200);
  delay(100);
  
  #if DB_INFO == 1
    Serial.println("\nESP32 ST7789 Touch-Menü mit CSMA/CD + Header-Display - Start");
    Serial.println("Firmware-Version: 1.60");
    Serial.println("Datum: Mai 2025");
    Serial.println("Hardware: Separate UART2 RS485 mit CSMA/CD");
    Serial.println("LED-Button-Zuordnung: LED 49-54 → Button 1-6");
    Serial.println("NEU: Header-Display mit Zeit/Datum/Device ID/Service-Icon");
    Serial.println("NEU: Timing-basierte Button-Verarbeitung (50ms + 10s Timeout)");
  #endif
  
  // *** DEBUG: Config-Werte prüfen ***
  Serial.println("=== CONFIG DEBUG ===");
  Serial.print("SCREEN_ORIENTATION: "); Serial.println(SCREEN_ORIENTATION);
  Serial.print("SCREEN_WIDTH: "); Serial.println(SCREEN_WIDTH);
  Serial.print("SCREEN_HEIGHT: "); Serial.println(SCREEN_HEIGHT);
  
  // *** NEU: DISPLAY-KALIBRIERUNG (vor allem anderen!) ***
  #ifdef DISPLAY_CALIBRATION_H
    startDisplayCalibration();
    
    // Optional: Warten auf Bestätigung vor normalem Start
    Serial.println("Drücken Sie Enter um mit normalem Betrieb fortzufahren...");
    while (!Serial.available()) {
      delay(100);
    }
    Serial.readString(); // Input lesen
  #endif

  // *** NEU: Service-Manager initialisieren (lädt gespeicherte Konfiguration) ***
  setupServiceManager();
  
  // *** NEU: Header-Display initialisieren ***
  setupHeaderDisplay();
  
// *** NEU: Converter Web Service initialisieren ***
  Serial.println("🔄 Initialisiere Converter Web Service...");
  if (!webConverter.begin()) {
    Serial.println("❌ Converter Web Service konnte nicht initialisiert werden!");
  } else {
    Serial.println("✅ Converter Web Service erfolgreich initialisiert");
    
    // Callback für Konfigurationsänderungen setzen
    webConverter.setConfigChangedCallback([](String configType) {
      Serial.println("📡 Konfiguration geändert: " + configType);
      
      if (configType == "buttons") {
        Serial.println("🔄 Aktualisiere Button-Display...");
        drawButtons();
      } else if (configType == "system") {
        Serial.println("🔄 Aktualisiere System-Konfiguration...");
        // Header neu zeichnen falls Device ID geändert
        if (!serviceManager.isServiceMode()) {
          drawHeader();
        }
      }
    });
    
    // Gespeicherte Konfiguration laden und anwenden
    Serial.println("📥 Lade gespeicherte Button-Konfiguration...");
    // webConverter.loadAll() wird bereits in begin() aufgerufen
    webConverter.begin();
    webConverter.printStatus();
  }

  // *** NEU: Gespeicherte Orientierung anwenden ***
  if (serviceManager.getOrientation() != SCREEN_ORIENTATION) {
    Serial.println("DEBUG: Wende gespeicherte Orientierung an");
    serviceManager.setOrientation(serviceManager.getOrientation());
  }
  
  // Kommunikation initialisieren (mit CSMA/CD)
  setupCommunication();
  
  // Initialisiere die RGB-LED
  setupLed();
  
  // Initialisiere Hintergrundbeleuchtung
  setupBacklight();
  setBacklight(DEFAULT_BACKLIGHT);
  
  // Initialisiere den Touchscreen
  setupTouch();
  
  // Initialisiere die Button-IDs
  initializeButtons();
  
  // Initialisiere das Display (nur falls Kalibrierung nicht lief)
  #ifndef DISPLAY_CALIBRATION_H
    setupDisplay();
  #endif
  
  // *** ERZWINGE PORTRAIT NACH ALLEM ***
  Serial.println("=== ERZWINGE PORTRAIT ===");
  tft.setRotation(SCREEN_ORIENTATION);
  Serial.print("Nach setRotation - TFT Rotation: "); 
  Serial.println(tft.getRotation());
  Serial.print("TFT Größe: "); 
  Serial.print(tft.width()); 
  Serial.print("x"); 
  Serial.println(tft.height());

  // Anzeige einiger Infos
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString("ESP32 ST7789 mit Header-Display", SCREEN_WIDTH/2, 40, 2);
  tft.drawCentreString("Version 1.60 + Timing-Buttons", SCREEN_WIDTH/2, SCREEN_HEIGHT/2, 2);
  tft.drawCentreString("Initialisierung...", SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 20, 1);
  tft.drawCentreString("Device ID: " + serviceManager.getDeviceID(), SCREEN_WIDTH/2, SCREEN_HEIGHT - 50, 1);
  tft.drawCentreString("Button: 50ms + 10s Timeout", SCREEN_WIDTH/2, SCREEN_HEIGHT - 30, 1);
  
  // Erste Statusmeldung der Hintergrundbeleuchtung
  sendBacklightStatus();
  lastBacklightStatusTime = millis();
  
  delay(3000);  // Längere Anzeige für neue Infos
  
  // Gehe direkt zum Menü
  showMenu();
}

void loop() {
  // LED-Status aktualisieren
  updateLedStatus();
  
  // Service-Manager Update
  updateServiceManager();
  
  // *** NEU: Button-Timing verwalten ***
  updateButtonTiming();
  
  // Header-Zeit aktualisieren (alle 1000ms)
  static unsigned long lastHeaderUpdate = 0;
  if (millis() - lastHeaderUpdate > 1000) {
    if (!serviceManager.isServiceMode()) {  // Nur im Hauptmenü
      updateHeaderTime();
    }
    lastHeaderUpdate = millis();
  }
  
  // Kommunikation mit CSMA/CD verwalten
  updateCommunication();
  
  // Prüfen, ob ein Statusupdate für die Hintergrundbeleuchtung fällig ist
  unsigned long currentMillis = millis();
  if (currentMillis - lastBacklightStatusTime >= BACKLIGHT_STATUS_INTERVAL) {
    sendBacklightStatus();
    lastBacklightStatusTime = currentMillis;
  }
  
  // Touch-Eingaben verarbeiten
  if (touchscreen.tirqTouched() && touchscreen.touched()) {
    delay(50); // Entprellung
    
    int x, y;
    getTouchPoint(&x, &y);
    
    #if DB_INFO == 1
      Serial.print("DEBUG: Touch erkannt - X: ");
      Serial.print(x);
      Serial.print(", Y: ");
      Serial.print(y);
      Serial.print(" (Service-Mode: ");
      Serial.print(serviceManager.isServiceMode() ? "JA" : "NEIN");
      Serial.println(")");
    #endif
    
    // Service-Manager Touch-Handling zuerst prüfen
    if (serviceManager.isServiceMode()) {
      handleServiceTouch(x, y, true);
      return;  // Service-Modus hat Vorrang
    }
    
    // Service-Icon Touch prüfen (nur im Hauptmenü)
    if (checkServiceIconTouch(x, y)) {
      #if DB_INFO == 1
        Serial.println("DEBUG: Service-Icon berührt - aktiviere Service-Modus");
      #endif
      
      // Service-Icon visuell als gedrückt anzeigen
      drawServiceIcon(true);
      delay(200);  // Kurzes visuelles Feedback
      
      // Service-Modus aktivieren
      serviceManager.enterServiceMode();
      return;
    }
    
    // *** NEUE TIMING-BASIERTE BUTTON-VERARBEITUNG ***
    handleButtonTouch(x, y);
    
  } else {
    // *** Touch nicht aktiv - prüfe ob Button-Timing läuft ***
    if (buttonTiming.touchActive) {
      // Touch wurde losgelassen
      handleButtonRelease();
    }
    
    // Auch ohne Touch Service-Manager updaten
    if (serviceManager.isServiceMode()) {
      handleServiceTouch(0, 0, false);  // Touch = false
    }
  }
  
  // Statistiken alle 60 Sekunden ausgeben (optional)
  static unsigned long lastStatsOutput = 0;
  if (millis() - lastStatsOutput > 60000) {
    printCommunicationStats();
    lastStatsOutput = millis();
  }
}

// *** NEUE FUNKTION: Timing-basierte Button-Touch-Verarbeitung ***
void handleButtonTouch(int x, int y) {
  int buttonPressed = checkButtonPress(x, y);
  
  if (buttonPressed >= 0) {
    // Prüfen, ob es ein neuer Button-Touch ist
    if (!buttonTiming.touchActive || buttonTiming.activeButtonIndex != buttonPressed) {
      
      #if DB_INFO == 1
        Serial.print("DEBUG: Button ");
        Serial.print(buttonPressed + 1);
        Serial.print(" (");
        Serial.print(buttons[buttonPressed].label);
        Serial.print(" - ID: ");
        Serial.print(buttons[buttonPressed].instanceID);
        Serial.println(") berührt - starte Timing");
      #endif
      
      // Neuer Button-Touch
      buttonTiming.touchActive = true;
      buttonTiming.touchStartTime = millis();
      buttonTiming.status1Sent = false;
      buttonTiming.activeButtonIndex = buttonPressed;
      
      // Button sofort visuell als aktiv anzeigen (grün)
      setButtonActive(buttonPressed, true);
      
      #if DB_INFO == 1
        Serial.println("DEBUG: Button visuell aktiviert, warte 50ms für STATUS.1");
      #endif
    }
  }
}

// *** NEUE FUNKTION: Button-Release-Verarbeitung ***
void handleButtonRelease() {
  if (buttonTiming.touchActive && buttonTiming.activeButtonIndex >= 0) {
    
    #if DB_INFO == 1
      Serial.print("DEBUG: Button ");
      Serial.print(buttonTiming.activeButtonIndex + 1);
      Serial.println(" losgelassen");
    #endif
    
    // FALLENDE FLANKE: STATUS.0 senden (nur wenn STATUS.1 gesendet wurde)
    if (buttonTiming.status1Sent) {
      sendTelegram("BTN", buttons[buttonTiming.activeButtonIndex].instanceID, "STATUS", "0");
      
      #if DB_INFO == 1
        Serial.println("DEBUG: FALLENDE FLANKE - Telegramm STATUS.0 gesendet");
      #endif
    }
    
    // Button visuell deaktivieren (grau)
    setButtonActive(buttonTiming.activeButtonIndex, false);
    
    // Button-Status zurücksetzen
    buttons[buttonTiming.activeButtonIndex].pressed = false;
    
    #if DB_INFO == 1
      Serial.print("DEBUG: Button ");
      Serial.print(buttonTiming.activeButtonIndex + 1);
      Serial.println(" deaktiviert und zurückgesetzt");
    #endif
    
    // *** NEU: Pending LED States anwenden ***
    applyAllPendingLedStates();
    
    // Timing zurücksetzen
    resetButtonTiming();
  }
}

// *** NEUE FUNKTION: Button-Timing Update (in loop() aufgerufen) ***
void updateButtonTiming() {
  if (!buttonTiming.touchActive) {
    return; // Kein aktiver Button-Touch
  }
  
  unsigned long elapsed = millis() - buttonTiming.touchStartTime;
  
  // *** PHASE 1: Nach 50ms STATUS.1 senden ***
  if (elapsed >= BUTTON_CONFIRM_DELAY && !buttonTiming.status1Sent) {
    
    #if DB_INFO == 1
      Serial.println("DEBUG: 50ms erreicht - sende STATUS.1");
    #endif
    
    // STEIGENDE FLANKE: STATUS.1 senden
    sendTelegram("BTN", buttons[buttonTiming.activeButtonIndex].instanceID, "STATUS", "1");
    
    // Button als gedrückt markieren
    buttons[buttonTiming.activeButtonIndex].pressed = true;
    buttonTiming.status1Sent = true;
    
    #if DB_INFO == 1
      Serial.println("DEBUG: STEIGENDE FLANKE - Telegramm STATUS.1 gesendet");
    #endif
  }
  
  // *** PHASE 2: Nach 10 Sekunden Timeout ***
  if (elapsed >= BUTTON_MAX_TIMEOUT) {
    
    #if DB_INFO == 1
      Serial.println("DEBUG: 10-Sekunden Timeout erreicht - forciere STATUS.0");
    #endif
    
    // Timeout erreicht - forciere STATUS.0
    if (buttonTiming.status1Sent) {
      sendTelegram("BTN", buttons[buttonTiming.activeButtonIndex].instanceID, "STATUS", "0");
      
      #if DB_INFO == 1
        Serial.println("DEBUG: TIMEOUT - Telegramm STATUS.0 gesendet");
      #endif
    }
    
    // Button visuell deaktivieren
    setButtonActive(buttonTiming.activeButtonIndex, false);
    buttons[buttonTiming.activeButtonIndex].pressed = false;
    
    #if DB_INFO == 1
      Serial.print("DEBUG: Button ");
      Serial.print(buttonTiming.activeButtonIndex + 1);
      Serial.println(" nach Timeout zurückgesetzt");
    #endif
    
    // *** NEU: Pending LED States anwenden ***
    applyAllPendingLedStates();
    
    // Timeout-Warnung anzeigen (optional)
    showTimeoutWarning();
    
    // Timing zurücksetzen
    resetButtonTiming();
  }
}

// *** NEUE FUNKTION: Button-Timing zurücksetzen ***
void resetButtonTiming() {
  buttonTiming.touchActive = false;
  buttonTiming.touchStartTime = 0;
  buttonTiming.status1Sent = false;
  buttonTiming.activeButtonIndex = -1;
  
  #if DB_INFO == 1
    Serial.println("DEBUG: Button-Timing zurückgesetzt");
  #endif
}

// *** NEUE FUNKTION: Timeout-Warnung anzeigen ***
void showTimeoutWarning() {
  #if DB_INFO == 1
    Serial.println("DEBUG: Zeige Timeout-Warnung");
  #endif
  
  // Kurze visuelle Warnung am unteren Bildschirmrand
  tft.fillRect(0, SCREEN_HEIGHT - 30, SCREEN_WIDTH, 30, TFT_ORANGE);
  tft.setTextColor(TFT_BLACK);
  tft.drawCentreString("Button-Timeout (10s erreicht)", SCREEN_WIDTH/2, SCREEN_HEIGHT - 20, 1);
  
  delay(1000); // 1 Sekunde anzeigen
  
  // Warnung entfernen - zurück zum normalen Menü
  showMenu();
}

void initializeButtons() {
  // Button-Konfiguration wird in menu.cpp/initButtons() gesetzt
  Serial.println("\n=== ORIENTIERUNGS-TEST SETUP ===");
  Serial.print("TFT-Rotation beim Start: ");
  Serial.println(tft.getRotation());
  Serial.print("Bildschirmgröße: ");
  Serial.print(tft.width());
  Serial.print(" x ");
  Serial.println(tft.height());
  
  Serial.println("\n=== Button-LED-Zuordnung ===");
  Serial.println("Button 1 (Index 0) → BTN.17 ↔ LED.49");
  Serial.println("Button 2 (Index 1) → BTN.18 ↔ LED.50");
  Serial.println("Button 3 (Index 2) → BTN.19 ↔ LED.51");
  Serial.println("Button 4 (Index 3) → BTN.20 ↔ LED.52");
  Serial.println("Button 5 (Index 4) → BTN.21 ↔ LED.53");
  Serial.println("Button 6 (Index 5) → BTN.22 ↔ LED.54");
  Serial.println("============================");
  Serial.println("INFO: Service-Icon Touch (rechts oben) = Service-Menü");
  Serial.println("INFO: Button-Timing: 50ms Verzögerung + 10s Timeout");
}

void showStartupScreen() {
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  
  // Header
  tft.drawCentreString("ESP32 Touch Panel", SCREEN_WIDTH/2, 20, 2);
  tft.drawCentreString("v1.60 + Timing-Buttons", SCREEN_WIDTH/2, 45, 2);
  
  // Status-Informationen
  tft.drawCentreString("Initialisierung...", SCREEN_WIDTH/2, SCREEN_HEIGHT/2 - 20, 2);
  tft.drawCentreString("CSMA/CD Kommunikation", SCREEN_WIDTH/2, SCREEN_HEIGHT/2, 1);
  tft.drawCentreString("Service-Manager aktiv", SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 15, 1);
  
  // Device-Info
  String deviceInfo = "Device ID: " + serviceManager.getDeviceID();
  tft.drawCentreString(deviceInfo, SCREEN_WIDTH/2, SCREEN_HEIGHT - 60, 1);
  
  String orientInfo = "Orientierung: " + String(serviceManager.getOrientation() == LANDSCAPE ? "Landscape" : "Portrait");
  tft.drawCentreString(orientInfo, SCREEN_WIDTH/2, SCREEN_HEIGHT - 45, 1);
  
  // Button-Timing Info
  tft.drawCentreString("Button: 50ms Delay + 10s Timeout", SCREEN_WIDTH/2, SCREEN_HEIGHT - 20, 1);
}

// *** ALTE FUNKTIONEN ENTFERNT ***
// handleButtonWithServiceOption() - nicht mehr benötigt
// drawServiceProgressBar() - nicht mehr benötigt  
// handleNormalTouch() - nicht mehr benötigt

void redrawUIElements() {
  // Test-Button neu zeichnen (falls vorhanden)
  if (SCREEN_WIDTH > 240) { // Nur bei ausreichender Breite
    tft.fillRect(SCREEN_WIDTH - 60, 5, 55, 30, TFT_BLUE);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("TEST", SCREEN_WIDTH - 32, 15, 1);
  }
  
  // Helligkeit-Anzeige neu zeichnen
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.fillRect(10, SCREEN_HEIGHT - 25, 200, 20, TFT_WHITE);
  tft.drawString("Helligkeit: " + String(currentBacklight) + "%", 10, SCREEN_HEIGHT - 20, 1);
}