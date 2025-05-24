/**
 * ESP32_CYD_RS485_HAUS_BUS.ino - Version 1.50 mit Button-basiertem Service-Touch
 * 
 * Features: 
 * - 5-Sekunden Service-Touch über langen Button-Press
 * - Kein Display-Flackern mehr
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

// Timing für Hintergrundbeleuchtungs-Status
unsigned long lastBacklightStatusTime = 0;

void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println("\n=== ESP32 ST7789 Touch-Panel - v1.50 + Button-Service-Touch ===");
  Serial.println("Firmware: Button-basierte Service-Aktivierung (5s langer Press)");
  Serial.println("Features: Service-Manager + CSMA/CD + Touch-Interface");
  Serial.println("Hardware: ESP32-CYD mit RS485 Interface");
  
  // Service-Manager initialisieren (lädt gespeicherte Konfiguration)
  setupServiceManager();
  
  
  // *** ORIENTIERUNGS-SYNCHRONISATION BEIM START ***
  int currentRotation = tft.getRotation();
  int actualOrientation = (currentRotation == 1) ? LANDSCAPE : PORTRAIT;
  
  if (serviceManager.getOrientation() != actualOrientation) {
    Serial.print("INFO: Orientierungs-Synchronisation - Display: ");
    Serial.print(actualOrientation == LANDSCAPE ? "LANDSCAPE" : "PORTRAIT");
    Serial.print(", Service-Manager: ");
    Serial.println(serviceManager.getOrientation() == LANDSCAPE ? "LANDSCAPE" : "PORTRAIT");
    
    // Service-Manager-Orientierung an Display anpassen
    serviceManager.setOrientation(actualOrientation);
  }
  // Orientierung anwenden falls gespeichert
  if (serviceManager.getOrientation() != SCREEN_ORIENTATION) {
    Serial.println("INFO: Wende gespeicherte Orientierung an");
  }
  
  // Kommunikation initialisieren (CSMA/CD)
  setupCommunication();
  
  // RGB-LED initialisieren
  setupLed();
  
  // Hintergrundbeleuchtung initialisieren
  setupBacklight();
  setBacklight(DEFAULT_BACKLIGHT);
  
  // Touchscreen initialisieren
  setupTouch();
  
  // Display initialisieren
  setupDisplay();
  
  // Button-Konfiguration
  initializeButtons();
  
  // Startup-Screen anzeigen
  showStartupScreen();
  
  // Erstes Backlight-Status senden
  sendBacklightStatus();
  lastBacklightStatusTime = millis();
  
  delay(2000);
  
  // Hauptmenü anzeigen
  showMenu();
  
  Serial.println("INFO: System vollständig initialisiert");
  Serial.println("INFO: Langer Button-Press (5s) aktiviert Service-Manager");
}

void initializeButtons() {
  // Button-Konfiguration wird in menu.cpp/initButtons() gesetzt
  Serial.println("\n=== Button-LED-Zuordnung ===");
  Serial.println("Button 1 (Index 0) → BTN.17 ↔ LED.49");
  Serial.println("Button 2 (Index 1) → BTN.18 ↔ LED.50");
  Serial.println("Button 3 (Index 2) → BTN.19 ↔ LED.51");
  Serial.println("Button 4 (Index 3) → BTN.20 ↔ LED.52");
  Serial.println("Button 5 (Index 4) → BTN.21 ↔ LED.53");
  Serial.println("Button 6 (Index 5) → BTN.22 ↔ LED.54");
  Serial.println("============================");
  Serial.println("INFO: Langer Button-Press (5s) = Service-Manager");
}

void showStartupScreen() {
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  
  // Header
  tft.drawCentreString("ESP32 Touch Panel", SCREEN_WIDTH/2, 20, 2);
  tft.drawCentreString("v1.50 + Button-Service", SCREEN_WIDTH/2, 45, 2);
  
  // Status-Informationen
  tft.drawCentreString("Initialisierung...", SCREEN_WIDTH/2, SCREEN_HEIGHT/2 - 20, 2);
  tft.drawCentreString("CSMA/CD Kommunikation", SCREEN_WIDTH/2, SCREEN_HEIGHT/2, 1);
  tft.drawCentreString("Service-Manager aktiv", SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 15, 1);
  
  // Device-Info
  String deviceInfo = "Device ID: " + serviceManager.getDeviceID();
  tft.drawCentreString(deviceInfo, SCREEN_WIDTH/2, SCREEN_HEIGHT - 60, 1);
  
  String orientInfo = "Orientierung: " + String(serviceManager.getOrientation() == LANDSCAPE ? "Landscape" : "Portrait");
  tft.drawCentreString(orientInfo, SCREEN_WIDTH/2, SCREEN_HEIGHT - 45, 1);
  
  // Service-Hinweis
  tft.drawCentreString("Langer Button-Press (5s) = Service", SCREEN_WIDTH/2, SCREEN_HEIGHT - 20, 1);
}

void loop() {
  // LED-Status aktualisieren
  updateLedStatus();
  
  // Kommunikation mit CSMA/CD verwalten
  updateCommunication();
  
  // Service-Manager Update (für Progress-Bar etc.)
  updateServiceManager();
  
  // Prüfen, ob ein Statusupdate für die Hintergrundbeleuchtung fällig ist
  unsigned long currentMillis = millis();
  if (currentMillis - lastBacklightStatusTime >= BACKLIGHT_STATUS_INTERVAL) {
    sendBacklightStatus();
    lastBacklightStatusTime = currentMillis;
  }
  
  // *** TOUCH-VERARBEITUNG: Service-Manager vs. normale Buttons ***
  if (touchscreen.tirqTouched() && touchscreen.touched()) {
    delay(50); // Entprellung
    
    int x, y;
    getTouchPoint(&x, &y);
    
    // *** SERVICE-MANAGER IST BEREITS AKTIV → Service hat absolute Priorität ***
    if (serviceManager.isServiceMode()) {
      handleServiceTouch(x, y, true);
    } 
    // *** SERVICE-MANAGER IST INAKTIV → Button-basierte Verarbeitung ***
    else {
      // Test-Button prüfen
      if (x >= SCREEN_WIDTH - 60 && x <= SCREEN_WIDTH - 5 && y >= 5 && y <= 35) {
        #if DB_INFO == 1
          Serial.println("DEBUG: Test-Button gedrückt");
        #endif
        testTouch();
      } else {
        // Normale Button-Verarbeitung mit Service-Touch-Integration
        handleButtonWithServiceOption(x, y);
      }
    }
  } else {
    // *** TOUCH LOSGELASSEN ***
    handleServiceTouch(0, 0, false);
  }
  
  // Statistiken alle 60 Sekunden ausgeben
  static unsigned long lastStatsOutput = 0;
  if (millis() - lastStatsOutput > 60000) {
    printCommunicationStats();
    lastStatsOutput = millis();
  }
}

// *** NEUE FUNKTION: Button mit Service-Option ***
void handleButtonWithServiceOption(int x, int y) {
  int buttonPressed = checkButtonPress(x, y);
  
  if (buttonPressed >= 0) {
    #if DB_INFO == 1
      Serial.print("DEBUG: Button ");
      Serial.print(buttonPressed + 1);
      Serial.print(" (");
      Serial.print(buttons[buttonPressed].label);
      Serial.println(") berührt - starte Timing");
    #endif
    
    // *** TIMING-SYSTEM: Kurz vs. Lang ***
    unsigned long touchStartTime = millis();
    bool serviceActivationStarted = false;
    bool serviceManagerActivated = false;
    
    // Button visuell als gedrückt anzeigen
    bool wasActive = buttons[buttonPressed].isActive;
    setButtonActive(buttonPressed, true);
    
    // *** TOUCH-SCHLEIFE mit Timing ***
    while (touchscreen.touched()) {
      unsigned long elapsed = millis() - touchStartTime;
      
      // *** PHASE 1: 0-2s = Normale Button-Vorbereitung ***
      if (elapsed < 2000) {
        // Noch keine Aktion, warten
        delay(10);
        updateCommunication();
      }
      // *** PHASE 2: 2-5s = Service-Aktivierung läuft ***
      else if (elapsed >= 2000 && elapsed < 5000) {
        if (!serviceActivationStarted) {
          // Service-Aktivierung starten
          serviceActivationStarted = true;
          
          #if DB_INFO == 1
            Serial.println("DEBUG: 2s erreicht - Progress-Bar wird gestartet");
          #endif
          
          // Progress-Bar manuell zeichnen
          drawServiceProgressBar(elapsed - 2000, 3000); // 0-3000ms Progress
        } else {
          // Progress-Bar aktualisieren
          drawServiceProgressBar(elapsed - 2000, 3000);
        }
        
        delay(10);
        updateCommunication();
      }
      // *** PHASE 3: 5s+ = Service-Manager aktivieren ***
      else if (elapsed >= 5000 && !serviceManagerActivated) {
        #if DB_INFO == 1
          Serial.println("DEBUG: 5s erreicht - Service-Manager wird DIREKT aktiviert!");
        #endif
        
        // *** DIREKTE SERVICE-MANAGER AKTIVIERUNG ***
        serviceManager.enterServiceMode();
        serviceManagerActivated = true;
        
        // Warten bis Touch losgelassen
        while (touchscreen.touched()) {
          delay(10);
          updateCommunication();
        }
        
        #if DB_INFO == 1
          Serial.println("DEBUG: Touch losgelassen - Service-Manager ist aktiv");
        #endif
        
        return; // Service-Manager übernimmt vollständig
      } else if (elapsed >= 5000) {
        // Service-Manager ist bereits aktiviert, weiter warten
        delay(10);
        updateCommunication();
      }
    }
    
    // *** TOUCH LOSGELASSEN - Entscheidung treffen ***
    unsigned long totalTime = millis() - touchStartTime;
    
    if (totalTime < 2000) {
      // *** KURZER TOUCH (< 2s): Normale Button-Funktion ***
      #if DB_INFO == 1
        Serial.print("DEBUG: Kurzer Touch (");
        Serial.print(totalTime);
        Serial.println("ms) - normale Button-Funktion");
      #endif
      
      // Normale Button-Verarbeitung
      buttons[buttonPressed].pressed = true;
      sendTelegram("BTN", buttons[buttonPressed].instanceID, "STATUS", "1");
      
      delay(100); // Kurze visuelle Bestätigung
      
      if (!wasActive) {
        setButtonActive(buttonPressed, false);
      }
      
      // Test-Button neu zeichnen
      tft.fillRect(SCREEN_WIDTH - 60, 5, 55, 30, TFT_BLUE);
      tft.setTextColor(TFT_WHITE);
      tft.drawCentreString("TEST", SCREEN_WIDTH - 32, 15, 1);
      
      buttons[buttonPressed].pressed = false;
      sendTelegram("BTN", buttons[buttonPressed].instanceID, "STATUS", "0");
      
    } else if (totalTime >= 2000 && totalTime < 5000) {
      // *** MITTLERER TOUCH (2-5s): Service-Aktivierung abgebrochen ***
      #if DB_INFO == 1
        Serial.print("DEBUG: Mittlerer Touch (");
        Serial.print(totalTime);
        Serial.println("ms) - Service-Aktivierung abgebrochen");
      #endif
      
      // Button wieder normal
      if (!wasActive) {
        setButtonActive(buttonPressed, false);
      }
      
      // Progress-Bar entfernen und zurück zum Menü
      showMenu();
    }
    // *** LANGER TOUCH (5s+): Service-Manager wurde bereits aktiviert ***
    // Nichts mehr zu tun, Service-Manager ist aktiv
    
  } else {
    #if DB_INFO == 1
      Serial.println("DEBUG: Touch außerhalb aller Buttons");
    #endif
  }
}


void drawServiceProgressBar(unsigned long elapsed, unsigned long duration) {
  int percent = (elapsed * 100) / duration;
  percent = constrain(percent, 0, 100);
  
  int barWidth = SCREEN_WIDTH - 20;
  int barHeight = 20;
  int barX = 10;
  int barY = SCREEN_HEIGHT - 30;
  
  // Hintergrund löschen
  tft.fillRect(0, barY - 20, SCREEN_WIDTH, 50, TFT_WHITE);
  
  // Text
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString("Service-Modus aktivieren...", SCREEN_WIDTH/2, barY - 15, 1);
  
  // Progress-Bar Rahmen
  tft.drawRect(barX, barY, barWidth, barHeight, TFT_BLACK);
  
  // Progress-Bar Füllung
  int fillWidth = (barWidth - 2) * percent / 100;
  if (fillWidth > 0) {
    uint16_t color = TFT_GREEN;
    if (percent < 50) color = TFT_YELLOW;
    if (percent < 25) color = TFT_RED;
    
    tft.fillRect(barX + 1, barY + 1, fillWidth, barHeight - 2, color);
  }
  
  // Zeit-Anzeige
  int secondsRemaining = (duration - elapsed) / 1000 + 1;
  secondsRemaining = max(secondsRemaining, 0);
  
  String timeText = String(secondsRemaining) + "s";
  tft.drawCentreString(timeText, SCREEN_WIDTH/2, barY + 5, 1);
  
  #if DB_INFO == 1
    static int lastPercent = -1;
    if (percent != lastPercent && percent % 20 == 0) { // Alle 20% loggen
      Serial.print("DEBUG: Progress: ");
      Serial.print(percent);
      Serial.print("% (");
      Serial.print(elapsed);
      Serial.print("/");
      Serial.print(duration);
      Serial.println("ms)");
      lastPercent = percent;
    }
  #endif
}

// *** VEREINFACHTE handleNormalTouch() - Backup-Funktion ***
void handleNormalTouch(int x, int y) {
  // Fallback-Funktion falls handleButtonWithServiceOption() nicht funktioniert
  
  // Test-Button prüfen
  if (x >= SCREEN_WIDTH - 60 && x <= SCREEN_WIDTH - 5 && y >= 5 && y <= 35) {
    testTouch();
    return;
  }
  
  // Button-Press prüfen
  int buttonPressed = checkButtonPress(x, y);
  
  if (buttonPressed >= 0) {
    #if DB_INFO == 1
      Serial.print("DEBUG: Fallback - Normaler Button gedrückt: ");
      Serial.print(buttons[buttonPressed].label);
    #endif
    
    // Einfache Button-Verarbeitung ohne Service-Touch
    buttons[buttonPressed].pressed = true;
    sendTelegram("BTN", buttons[buttonPressed].instanceID, "STATUS", "1");
    
    bool wasActive = buttons[buttonPressed].isActive;
    setButtonActive(buttonPressed, true);
    
    // Warte bis Touch losgelassen
    while (touchscreen.touched()) {
      delay(10);
      updateCommunication();
    }
    
    if (!wasActive) {
      setButtonActive(buttonPressed, false);
    }
    
    // UI-Elemente neu zeichnen
    redrawUIElements();
    
    buttons[buttonPressed].pressed = false;
    sendTelegram("BTN", buttons[buttonPressed].instanceID, "STATUS", "0");
  }
}

void redrawUIElements() {
  // Test-Button neu zeichnen
  tft.fillRect(SCREEN_WIDTH - 60, 5, 55, 30, TFT_BLUE);
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString("TEST", SCREEN_WIDTH - 32, 15, 1);
  
  // Helligkeit-Anzeige neu zeichnen
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.fillRect(10, SCREEN_HEIGHT - 25, 200, 20, TFT_WHITE);
  tft.drawString("Helligkeit: " + String(currentBacklight) + "%", 10, SCREEN_HEIGHT - 20, 1);
}