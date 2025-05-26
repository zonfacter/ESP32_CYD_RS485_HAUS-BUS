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
#include "header_display.h"

// Timing für Hintergrundbeleuchtungs-Status
unsigned long lastBacklightStatusTime = 0;

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
  #endif
  
  // *** NEU: Service-Manager initialisieren (lädt gespeicherte Konfiguration) ***
  setupServiceManager();
  
  // *** NEU: Header-Display initialisieren ***
  setupHeaderDisplay();
  
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
  
  // Initialisiere das Display
  setupDisplay();

  // Anzeige einiger Infos
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString("ESP32 ST7789 mit Header-Display", SCREEN_WIDTH/2, 40, 2);
  tft.drawCentreString("Version 1.60 + Zeit/Datum/Service", SCREEN_WIDTH/2, SCREEN_HEIGHT/2, 2);
  tft.drawCentreString("Initialisierung...", SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 20, 1);
  tft.drawCentreString("Device ID: " + serviceManager.getDeviceID(), SCREEN_WIDTH/2, SCREEN_HEIGHT - 50, 1);
  tft.drawCentreString("Service: Header-Icon Touch", SCREEN_WIDTH/2, SCREEN_HEIGHT - 30, 1);
  
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
    
    // *** ENTFERNT: Test-Button Touch-Handling (jetzt im Service-Menü) ***
    
 int buttonPressed = checkButtonPress(x, y);
    
    if (buttonPressed >= 0) {
      #if DB_INFO == 1
        Serial.print("DEBUG: Button gedrückt: ");
        Serial.print(buttons[buttonPressed].label);
        Serial.print(" (ID: ");
        Serial.print(buttons[buttonPressed].instanceID);
        Serial.println(")");
      #endif
      
      // Sofort bei Berührung senden (Steigende Flanke)
      if (!buttons[buttonPressed].pressed) {  // Nur wenn vorher nicht gedrückt
        buttons[buttonPressed].pressed = true;
        
        // STEIGENDE FLANKE: Sofort senden
        sendTelegram("BTN", buttons[buttonPressed].instanceID, "STATUS", "1");
        
        // Button visuell als gedrückt anzeigen (grün hinterlegen)
        setButtonActive(buttonPressed, true);
        
        #if DB_INFO == 1
          Serial.println("DEBUG: STEIGENDE FLANKE - Telegramm STATUS.1 gesendet");
        #endif
      }
      
      // Warte, bis der Touchscreen losgelassen wird
      while (touchscreen.touched()) {
        delay(10);
        
        // Während des Wartens die Kommunikation weiter verwalten
        updateCommunication();
        updateServiceManager();
      }
      
      // FALLENDE FLANKE: Beim Loslassen senden
      if (buttons[buttonPressed].pressed) {  // Nur wenn vorher gedrückt
        buttons[buttonPressed].pressed = false;
        
        // FALLENDE FLANKE: Beim Loslassen senden
        sendTelegram("BTN", buttons[buttonPressed].instanceID, "STATUS", "0");
        
        #if DB_INFO == 1
          Serial.println("DEBUG: FALLENDE FLANKE - Telegramm STATUS.0 gesendet");
        #endif
        
        // *** EINFACH: Button immer auf inaktiv (grau) setzen nach Loslassen ***
        // (LED-Telegramme können ihn später wieder aktivieren)
        setButtonActive(buttonPressed, false);
        
        #if DB_INFO == 1
          Serial.print("DEBUG: Button ");
          Serial.print(buttonPressed);
          Serial.println(" nach Loslassen auf grau gesetzt");
        #endif
      }
     }
    
  } else {
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