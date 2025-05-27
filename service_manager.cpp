/**
 * service_manager.cpp - Version 1.50 - VOLLSTÄNDIGE VERSION
 * 
 * Implementation des Service-Menüs für ESP32 Touch-Interface
 * Original: ~800+ Zeilen - Alle Funktionen implementiert
 */

#include "service_manager.h"
#include "menu.h"
#include "communication.h"
#include "touch.h"  // Für getTouchPoint()
#include <EEPROM.h>
#include <WiFi.h>
#include "web_server_manager.h"
#include "config_manager.h"
#include "header_display.h"
// Globale ServiceManager Instanz
ServiceManager serviceManager;

// EEPROM-Adresse für Konfiguration
#define CONFIG_EEPROM_ADDR 100
#define CONFIG_MAGIC_BYTE 0xA5

// Konfiguration-Struktur
struct ServiceConfig {
  uint8_t magic;           // Validierung
  char deviceID[5];        // "5999\0"
  uint8_t orientation;     // 0=Portrait, 1=Landscape
  uint8_t checksum;        // Einfache Checksumme
};

ServiceManager::ServiceManager() {
  currentState = SERVICE_INACTIVE;
  touchStartTime = 0;
  longTouchActive = false;
  lastProgressUpdate = 0;
  progressPercent = 0;
  currentDeviceID = DEVICE_ID;  // Aus config.h
  currentOrientation = SCREEN_ORIENTATION;  // Aus config.h
  configChanged = false;
  editDeviceID = "";
  editPosition = 0;
  
  // WiFi und Web-Interface
  wifiActive = false;
  webServerActive = false;
  wifiSSID = "ESP32-ServiceMode";
  wifiPassword = "service123";
  
  // Service-Buttons initialisieren
  initServiceButtons();
  
  // Numpad-Buttons initialisieren
  initNumpadButtons();
}

void ServiceManager::initServiceButtons() {
  // *** Service-Menü Layout ist IMMER basierend auf aktueller Bildschirm-Größe ***
  
  int buttonW = (SCREEN_WIDTH - 40) / 3;  // 3 Spalten
  int buttonH = 40;
  int spacing = 10;
  int startY = 60;
  
  // Erste Reihe: Device ID, Orientation, Test
  serviceButtons[0] = {10, startY, buttonW, buttonH, "Device ID", TFT_BLUE, TFT_WHITE, true};
  
  // *** KORRIGIERT: Button-Label basierend auf gewählter (nicht aktueller) Orientierung ***
  String orientLabel;
  if (currentOrientation == LANDSCAPE) {
    orientLabel = "→ Portrait";  // Wechseln zu Portrait
  } else {
    orientLabel = "→ Landscape"; // Wechseln zu Landscape  
  }
  serviceButtons[1] = {20 + buttonW, startY, buttonW, buttonH, orientLabel, TFT_GREEN, TFT_BLACK, true};
  
  // Test-Button
  serviceButtons[2] = {30 + 2*buttonW, startY, buttonW, buttonH, "TEST", TFT_BLUE, TFT_WHITE, true};
  
  // Zweite Reihe: WiFi, Web Config, (leer)
  String wifiLabel = wifiActive ? "WiFi OFF" : "WiFi ON";
  uint16_t wifiColor = wifiActive ? TFT_RED : TFT_ORANGE;
  serviceButtons[3] = {10, startY + buttonH + spacing, buttonW, buttonH, wifiLabel, wifiColor, TFT_BLACK, true};
  
  serviceButtons[4] = {20 + buttonW, startY + buttonH + spacing, buttonW, buttonH, "Web Config", TFT_PURPLE, TFT_WHITE, wifiActive};
  
  // Platzhalter
  serviceButtons[5] = {30 + 2*buttonW, startY + buttonH + spacing, buttonW, buttonH, "---", TFT_LIGHTGREY, TFT_DARKGREY, false};
  
  // Dritte Reihe: Save & Exit, Cancel
  serviceButtons[6] = {10, startY + 2*(buttonH + spacing), (2*buttonW + spacing), buttonH, "SAVE & EXIT", TFT_DARKGREEN, TFT_WHITE, true};
  
  serviceButtons[7] = {30 + 2*buttonW, startY + 2*(buttonH + spacing), buttonW, buttonH, "CANCEL", TFT_RED, TFT_WHITE, true};
  
  #if DB_INFO == 1
    Serial.print("DEBUG: Service-Buttons initialisiert für ");
    Serial.print(SCREEN_WIDTH);
    Serial.print("x");
    Serial.print(SCREEN_HEIGHT);
    Serial.print(" - Button-Label: ");
    Serial.println(orientLabel);
  #endif
}

void ServiceManager::initNumpadButtons() {
  // Dynamisches Layout je nach Orientierung
  int buttonW, buttonH, spacing, startX, startY;
  int cols, rows;
  
  if (currentOrientation == LANDSCAPE) {
    // Landscape: Kleinere Buttons, kompakteres Layout
    buttonW = 18;  // Kleiner für Landscape
    buttonH = 18;  // Kleiner für Landscape
    spacing = 5;   // Weniger Abstand
    cols = 4;
    rows = 4;
    startX = (SCREEN_WIDTH - (cols * buttonW + (cols-1) * spacing)) / 2;
    startY = 100;  // Höher positioniert
  } else {
    // Portrait: Größere Buttons, mehr Platz
    buttonW = 20;
    buttonH = 20;
    spacing = 5;
    cols = 4;
    rows = 4;
    startX = (SCREEN_WIDTH - (cols * buttonW + (cols-1) * spacing)) / 2;
    startY = 80;
  }
  
  // Zahlen 1-3 (erste Reihe)
  numpadButtons[0] = {startX, startY, buttonW, buttonH, "1", '1', TFT_LIGHTGREY, TFT_BLACK};
  numpadButtons[1] = {startX + (buttonW + spacing), startY, buttonW, buttonH, "2", '2', TFT_LIGHTGREY, TFT_BLACK};
  numpadButtons[2] = {startX + 2*(buttonW + spacing), startY, buttonW, buttonH, "3", '3', TFT_LIGHTGREY, TFT_BLACK};
  
  // Zahlen 4-6 (zweite Reihe)
  numpadButtons[3] = {startX, startY + (buttonH + spacing), buttonW, buttonH, "4", '4', TFT_LIGHTGREY, TFT_BLACK};
  numpadButtons[4] = {startX + (buttonW + spacing), startY + (buttonH + spacing), buttonW, buttonH, "5", '5', TFT_LIGHTGREY, TFT_BLACK};
  numpadButtons[5] = {startX + 2*(buttonW + spacing), startY + (buttonH + spacing), buttonW, buttonH, "6", '6', TFT_LIGHTGREY, TFT_BLACK};
  
  // Zahlen 7-9 (dritte Reihe)
  numpadButtons[6] = {startX, startY + 2*(buttonH + spacing), buttonW, buttonH, "7", '7', TFT_LIGHTGREY, TFT_BLACK};
  numpadButtons[7] = {startX + (buttonW + spacing), startY + 2*(buttonH + spacing), buttonW, buttonH, "8", '8', TFT_LIGHTGREY, TFT_BLACK};
  numpadButtons[8] = {startX + 2*(buttonW + spacing), startY + 2*(buttonH + spacing), buttonW, buttonH, "9", '9', TFT_LIGHTGREY, TFT_BLACK};
  
  // Zahl 0 (vierte Reihe)
  numpadButtons[9] = {startX, startY + 3*(buttonH + spacing), buttonW, buttonH, "0", '0', TFT_LIGHTGREY, TFT_BLACK};
  
  // Funktions-Buttons (rechte Spalte)
  numpadButtons[10] = {startX + 3*(buttonW + spacing), startY, buttonW, buttonH, "+", '+', TFT_GREEN, TFT_BLACK};
  numpadButtons[11] = {startX + 3*(buttonW + spacing), startY + (buttonH + spacing), buttonW, buttonH, "-", '-', TFT_RED, TFT_WHITE};
  numpadButtons[12] = {startX + 3*(buttonW + spacing), startY + 2*(buttonH + spacing), buttonW, buttonH, "<", '<', TFT_BLUE, TFT_WHITE};
  numpadButtons[13] = {startX + 3*(buttonW + spacing), startY + 3*(buttonH + spacing), buttonW, buttonH, ">", '>', TFT_BLUE, TFT_WHITE};
  
  // OK und CANCEL - ORIENTIERUNGSABHÄNGIG
  if (currentOrientation == LANDSCAPE) {
    // Landscape: Kleinere OK/CANCEL Buttons nebeneinander
    int okCancelW = (2*buttonW + spacing) / 2 - 2;  // Zwei gleich große Buttons
    int okCancelY = startY + 4*(buttonH + spacing) + spacing;
    
    numpadButtons[14] = {startX, okCancelY, okCancelW, buttonH, "OK", 'O', TFT_DARKGREEN, TFT_WHITE};
    numpadButtons[15] = {startX + okCancelW + 4, okCancelY, okCancelW, buttonH, "CANCEL", 'C', TFT_RED, TFT_WHITE};
  } else {
    // Portrait: Breitere OK/CANCEL Buttons wie ursprünglich
    int wideButtonW = 2*buttonW + spacing;
    int okCancelY = startY + 4*(buttonH + spacing) + 10;
    
    numpadButtons[14] = {startX, okCancelY, wideButtonW, buttonH, "OK", 'O', TFT_DARKGREEN, TFT_WHITE};
    numpadButtons[15] = {startX + wideButtonW + spacing, okCancelY, wideButtonW, buttonH, "CANCEL", 'C', TFT_RED, TFT_WHITE};
  }
}

void ServiceManager::update() {
  // Nur aktiv wenn nicht im Service-Modus
  if (currentState == SERVICE_INACTIVE || currentState == SERVICE_ACTIVATING) {
    // Wird in der Haupt-loop() über handleTouch() aufgerufen
  }
  
  // Progress-Bar Update (alle 100ms)
  if (currentState == SERVICE_ACTIVATING && millis() - lastProgressUpdate > 100) {
    unsigned long elapsed = millis() - touchStartTime;
    progressPercent = (elapsed * 100) / 20000;  // 20 Sekunden = 100%
    
    if (progressPercent > 100) progressPercent = 100;
    
    drawProgressBar(progressPercent);
    lastProgressUpdate = millis();
    
    #if DB_INFO == 1
      if (progressPercent % 10 == 0) {  // Jede 10%
        Serial.print("DEBUG: Service-Aktivierung ");
        Serial.print(progressPercent);
        Serial.println("%");
      }
    #endif
  }
}

void ServiceManager::handleTouch(int x, int y, bool touched) {
  switch (currentState) {
    case SERVICE_INACTIVE:
      // 20-Sekunden Touch-Detection starten
      if (touched) {
        if (!longTouchActive) {
          touchStartTime = millis();
          longTouchActive = true;
          currentState = SERVICE_ACTIVATING;
          progressPercent = 0;
          
          #if DB_INFO == 1
            Serial.println("DEBUG: Service-Aktivierung gestartet (20 Sekunden)");
          #endif
          
          // Progress-Bar anzeigen
          drawProgressBar(0);
        }
      } else {
        // Touch losgelassen - Aktivierung abbrechen
        if (longTouchActive) {
          longTouchActive = false;
          currentState = SERVICE_INACTIVE;
          
          #if DB_INFO == 1
            Serial.println("DEBUG: Service-Aktivierung abgebrochen");
          #endif
          
          // Zurück zum normalen Menü
          showMenu();
        }
      }
      break;
      
    case SERVICE_ACTIVATING:
      if (touched) {
        // Prüfen ob 20 Sekunden erreicht
        if (millis() - touchStartTime >= 20000) {
          enterServiceMode();
        }
      } else {
        // Touch losgelassen - Aktivierung abbrechen
        longTouchActive = false;
        currentState = SERVICE_INACTIVE;
        
        #if DB_INFO == 1
          Serial.println("DEBUG: Service-Aktivierung abgebrochen");
        #endif
        
        showMenu();
      }
      break;
      
    case SERVICE_ACTIVE:
      // Service-Menü Touch-Handling
      if (touched) {
        int buttonIndex = checkServiceButtonPress(x, y);
        if (buttonIndex >= 0) {
          handleServiceButtonPress(buttonIndex);
        }
      }
      break;
      
    case SERVICE_DEVICE_ID:
      // Device ID Editor Touch-Handling
      if (touched) {
        int buttonIndex = checkNumpadButtonPress(x, y);
        if (buttonIndex >= 0) {
          handleNumpadButtonPress(buttonIndex);
        }
      }
      break;
      
    case SERVICE_ORIENTATION:
      // Orientierungs-Preview Handling
      if (touched) {
        // Zurück zum Service-Menü
        currentState = SERVICE_ACTIVE;
        drawServiceMenu();
      }
      break;
      
    default:
      break;
  }
}

bool ServiceManager::isServiceMode() {
  return (currentState != SERVICE_INACTIVE);
}

void ServiceManager::enterServiceMode() {
  currentState = SERVICE_ACTIVE;
  longTouchActive = false;
  
  // *** KORRIGIERT: Original-Orientierung basierend auf aktueller TFT-Rotation setzen ***
  int currentTftRotation = tft.getRotation();
  originalOrientation = (currentTftRotation == 0 || currentTftRotation == 2) ? PORTRAIT : LANDSCAPE;
  currentOrientation = originalOrientation;  // Aktuelle = Original beim Start
  orientationChanged = false;
  
  #if DB_INFO == 1
    Serial.print("DEBUG: Service-Modus aktiviert - TFT-Rotation: ");
    Serial.print(currentTftRotation);
    Serial.print(" → Original-Orientierung: ");
    Serial.println(originalOrientation == LANDSCAPE ? "LANDSCAPE" : "PORTRAIT");
  #endif
  
  // Service-Menü anzeigen
  drawServiceMenu();
}

void ServiceManager::exitServiceMode() {
  if (configChanged) {
    saveConfig();
    
    #if DB_INFO == 1
      Serial.println("DEBUG: Konfiguration gespeichert");
    #endif
    
    // *** KORRIGIERT: Orientierung nur bei tatsächlicher Änderung anwenden ***
    if (orientationChanged && (currentOrientation != originalOrientation)) {
      #if DB_INFO == 1
        Serial.print("DEBUG: Orientierung wurde geändert - Wende neue Orientierung an: ");
        Serial.println(currentOrientation == LANDSCAPE ? "LANDSCAPE" : "PORTRAIT");
      #endif
      
      // *** SOFORTIGE Orientierungs-Anwendung ***
      setOrientation(currentOrientation);
      
      // *** WICHTIG: Pause für UI-Stabilisierung ***
      delay(500);
    } else {
      #if DB_INFO == 1
        Serial.println("DEBUG: Orientierung unverändert - keine Anwendung nötig");
      #endif
    }
  }
  
  currentState = SERVICE_INACTIVE;
  configChanged = false;
  orientationChanged = false;
  
  #if DB_INFO == 1
    Serial.println("DEBUG: Service-Modus verlassen");
  #endif
  
  // *** KORRIGIERT: Zurück zum Hauptmenü (mit korrekter Orientierung) ***
  showMenu();
}


void ServiceManager::cancelServiceMode() {
  // Konfiguration nicht speichern
  currentState = SERVICE_INACTIVE;
  configChanged = false;
  
  #if DB_INFO == 1
    Serial.println("DEBUG: Service-Modus abgebrochen (ohne speichern)");
  #endif
  
  // Zurück zum Hauptmenü
  showMenu();
}

void ServiceManager::drawServiceMenu() {
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  
  // Header
  tft.drawCentreString("SERVICE MODUS - V1.60", SCREEN_WIDTH/2, 10, 2);
  tft.drawLine(0, 30, SCREEN_WIDTH, 30, TFT_BLACK);
  
  // *** KORRIGIERTE Konfiguration anzeigen ***
  tft.setTextSize(1);
  String configText = "Device ID: " + currentDeviceID;
  tft.drawCentreString(configText, SCREEN_WIDTH/2, 40, 1);
  
  // *** KORREKTUR: Aktuelle TFT-Rotation anzeigen ***
  int actualTftRotation = tft.getRotation();
  String orientText = "TFT-Rotation: " + String(actualTftRotation) + " (" + 
                     String((actualTftRotation == 0 || actualTftRotation == 2) ? "PORTRAIT" : "LANDSCAPE") + ")";
  tft.drawCentreString(orientText, SCREEN_WIDTH/2, 50, 1);
  
  // Bildschirm-Dimensionen anzeigen
  String dimText = "Display: " + String(SCREEN_WIDTH) + "x" + String(SCREEN_HEIGHT);
  tft.drawCentreString(dimText, SCREEN_WIDTH/2 + 80, 40, 1);
  
  // WiFi-Status anzeigen
  if (wifiActive) {
    tft.setTextColor(TFT_DARKGREEN, TFT_WHITE);
    String wifiText = "WiFi: " + WiFi.softAPIP().toString();
    tft.drawCentreString(wifiText, SCREEN_WIDTH/2 - 80, 40, 1);
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
  }
  
  // Geändert-Indikator
  if (configChanged) {
    tft.setTextColor(TFT_RED, TFT_WHITE);
    tft.drawCentreString("* GEÄNDERT *", SCREEN_WIDTH/2 - 80, 50, 1);
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
  }
  
  // Service-Buttons zeichnen
  for (int i = 0; i < NUM_SERVICE_BUTTONS; i++) {
    if (serviceButtons[i].enabled) {
      // Button-Hintergrund
      tft.fillRect(serviceButtons[i].x, serviceButtons[i].y, 
                   serviceButtons[i].w, serviceButtons[i].h, serviceButtons[i].color);
      
      // Button-Rahmen
      tft.drawRect(serviceButtons[i].x, serviceButtons[i].y, 
                   serviceButtons[i].w, serviceButtons[i].h, TFT_BLACK);
      
      // Button-Text zentrieren
      tft.setTextColor(serviceButtons[i].textColor);
      int textX = serviceButtons[i].x + serviceButtons[i].w/2;
      int textY = serviceButtons[i].y + serviceButtons[i].h/2 - 8;
      tft.drawCentreString(serviceButtons[i].label, textX, textY, 2);
    } else {
      // Deaktivierte Buttons grau zeichnen
      tft.fillRect(serviceButtons[i].x, serviceButtons[i].y, 
                   serviceButtons[i].w, serviceButtons[i].h, TFT_LIGHTGREY);
      tft.drawRect(serviceButtons[i].x, serviceButtons[i].y, 
                   serviceButtons[i].w, serviceButtons[i].h, TFT_DARKGREY);
      
      tft.setTextColor(TFT_DARKGREY);
      int textX = serviceButtons[i].x + serviceButtons[i].w/2;
      int textY = serviceButtons[i].y + serviceButtons[i].h/2 - 8;
      tft.drawCentreString(serviceButtons[i].label, textX, textY, 2);
    }
  }
}

void ServiceManager::redrawServiceMenu() {
  // Service-Buttons für neue Orientierung neu berechnen
  initServiceButtons();
  
  // Menü neu zeichnen
  drawServiceMenu();
}

void ServiceManager::drawProgressBar(int percent) {
  // Progress-Bar am unteren Bildschirmrand
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
  
  // Prozent-Text
  String percentText = String(percent) + "%";
  tft.drawCentreString(percentText, SCREEN_WIDTH/2, barY + 5, 1);
}

int ServiceManager::checkServiceButtonPress(int x, int y) {
  for (int i = 0; i < NUM_SERVICE_BUTTONS; i++) {
    if (serviceButtons[i].enabled && 
        x >= serviceButtons[i].x && x <= serviceButtons[i].x + serviceButtons[i].w &&
        y >= serviceButtons[i].y && y <= serviceButtons[i].y + serviceButtons[i].h) {
      return i;
    }
  }
  return -1;
}

void ServiceManager::handleServiceButtonPress(int buttonIndex) {
  #if DB_INFO == 1
    Serial.print("DEBUG: Service-Button gedrückt: ");
    Serial.println(serviceButtons[buttonIndex].label);
  #endif
  
  switch (buttonIndex) {
    case 0: onEditDeviceID(); break;
    case 1: onToggleOrientation(); break;
    case 2: onTestFunction(); break;        // *** NEU: Test-Button ***
    case 3: onWiFiToggle(); break;
    case 4: onWebConfig(); break;
    case 5: /* Platzhalter */ break;
    case 6: onSaveAndExit(); break;
    case 7: onCancel(); break;
  }
}

void ServiceManager::onTestFunction() {
  #if DB_INFO == 1
    Serial.println("DEBUG: Test-Funktion ausgeführt");
  #endif
  
  // Test-Screen anzeigen
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString("TEST FUNKTIONEN", SCREEN_WIDTH/2, 10, 2);
  tft.drawLine(0, 30, SCREEN_WIDTH, 30, TFT_BLACK);
  
  // Test-Informationen anzeigen
  tft.drawString("Device ID: " + currentDeviceID, 10, 40, 1);
  tft.drawString("Orientierung: " + String(currentOrientation == LANDSCAPE ? "LANDSCAPE" : "PORTRAIT"), 10, 55, 1);
  tft.drawString("WiFi Status: " + String(wifiActive ? "AKTIV" : "INAKTIV"), 10, 70, 1);
  tft.drawString("Freier Speicher: " + String(ESP.getFreeHeap()) + " Bytes", 10, 85, 1);
  tft.drawString("Uptime: " + String(millis() / 1000) + " Sekunden", 10, 100, 1);
  
  // Display-Dimensionen
  tft.drawString("Display: " + String(SCREEN_WIDTH) + "x" + String(SCREEN_HEIGHT), 10, 115, 1);
  
  // Touch-Test-Bereich
  tft.fillRect(10, 130, SCREEN_WIDTH-20, 80, TFT_LIGHTGREY);
  tft.drawRect(10, 130, SCREEN_WIDTH-20, 80, TFT_BLACK);
  tft.setTextColor(TFT_BLACK);
  tft.drawCentreString("TOUCH-TEST BEREICH", SCREEN_WIDTH/2, 140, 1);
  tft.drawCentreString("Berühren Sie hier zum Testen", SCREEN_WIDTH/2, 155, 1);
  tft.drawCentreString("Koordinaten werden angezeigt", SCREEN_WIDTH/2, 170, 1);
  
  // Zurück-Button
  tft.fillRect(10, SCREEN_HEIGHT-40, SCREEN_WIDTH-20, 30, TFT_BLUE);
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString("TOUCH ZUM ZURÜCKKEHREN", SCREEN_WIDTH/2, SCREEN_HEIGHT-30, 1);
  
  // Warten auf Touch und Touch-Koordinaten anzeigen
  // Warten auf Touch
  bool exitTest = false;
  while (!exitTest) {
    if (touchscreen.tirqTouched() && touchscreen.touched()) {
      delay(50);
      
      int x, y;
      getTouchPoint(&x, &y);
      
      // Touch-Kalibrierung
      if (x >= 10 && x <= 160 && y >= 50 && y <= 90) {
        touchCalibrationWizard();
        exitTest = true;
      }
      // Button-Test  
      else if (x >= 170 && x <= 320 && y >= 50 && y <= 90) {
        testTouch();
        exitTest = true;
      }
      // Zurück
      else if (y >= SCREEN_HEIGHT-50) {
        exitTest = true;
      }
      // Warten bis losgelassen
      while (touchscreen.touched()) {
        delay(10);
      }
    }
    delay(50);
  }
  
  // Zurück zum Service-Menü
  drawServiceMenu();
}

void ServiceManager::onEditDeviceID() {
  currentState = SERVICE_DEVICE_ID;
  editDeviceID = currentDeviceID;
  editPosition = 0;
  
  // Sicherstellen, dass Device ID 4 Zeichen hat
  while (editDeviceID.length() < 4) {
    editDeviceID += "0";
  }
  editDeviceID = editDeviceID.substring(0, 4);  // Auf 4 Zeichen begrenzen
  
  #if DB_INFO == 1
    Serial.print("DEBUG: Device ID Editor geöffnet - aktuelle ID: ");
    Serial.println(editDeviceID);
    Serial.print("DEBUG: Orientierung: ");
    Serial.println(currentOrientation == LANDSCAPE ? "LANDSCAPE" : "PORTRAIT");
  #endif
  
  // *** WICHTIG: Numpad für aktuelle Orientierung neu initialisieren ***
  initNumpadButtons();
  
  // Device ID Editor anzeigen
  drawDeviceIDEditor();
}

void ServiceManager::onToggleOrientation() {
  #if DB_INFO == 1
    Serial.print("DEBUG: onToggleOrientation() - Aktuelle TFT-Rotation: ");
    Serial.print(tft.getRotation());
    Serial.print(" → Umschalten auf: ");
  #endif
  
  // *** KORREKTUR: Basierend auf aktueller TFT-Rotation umschalten ***
  int currentTftRotation = tft.getRotation();
  
  if (currentTftRotation == 0 || currentTftRotation == 2) {
    // Aktuell Portrait → wechseln zu Landscape
    currentOrientation = LANDSCAPE;
    #if DB_INFO == 1
      Serial.println("LANDSCAPE");
    #endif
  } else {
    // Aktuell Landscape → wechseln zu Portrait  
    currentOrientation = PORTRAIT;
    #if DB_INFO == 1
      Serial.println("PORTRAIT");
    #endif
  }
  
  configChanged = true;
  orientationChanged = true;
  
  // *** Bestätigungsmeldung anzeigen (ohne Orientierung zu ändern) ***
  tft.fillRect(0, SCREEN_HEIGHT - 40, SCREEN_WIDTH, 40, TFT_DARKGREEN);
  tft.setTextColor(TFT_WHITE);
  String orientText = (currentOrientation == LANDSCAPE) ? "LANDSCAPE" : "PORTRAIT";
  tft.drawCentreString("Neue Orientierung: " + orientText, SCREEN_WIDTH/2, SCREEN_HEIGHT - 25, 2);
  tft.drawCentreString("Wird beim SAVE & EXIT angewendet", SCREEN_WIDTH/2, SCREEN_HEIGHT - 10, 1);
  
  delay(2000);
  
  // *** Service-Button Label aktualisieren ***
  initServiceButtons();  // Button-Labels neu setzen
  
  // Service-Menü neu zeichnen
  drawServiceMenu();
}

void ServiceManager::toggleOrientation() {
  // Alternative Implementation für Orientierungs-Umschaltung
  onToggleOrientation();
}

void ServiceManager::applyOrientation(int rotation) {
  rotation %= 4;
  tft.setRotation(rotation);
  currentOrientation = rotation;
  configManager.device.orientation = rotation;
  configChanged = true;

  initButtons();
  drawButtons();
  drawHeader();
}

void ServiceManager::showOrientationPreview() {
  currentState = SERVICE_ORIENTATION;

  // Neue Orientierung anwenden
  applyOrientation(currentOrientation);  // ✅ Nur aufrufen, nicht definieren

  // Preview-Screen anzeigen
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);

  String orientationName = (currentOrientation == LANDSCAPE) ? "LANDSCAPE" : "PORTRAIT";
  tft.drawCentreString("ORIENTIERUNG: " + orientationName, SCREEN_WIDTH/2, 10, 2);
  tft.drawLine(0, 30, SCREEN_WIDTH, 30, TFT_BLACK);

  String dimText = "Auflösung: " + String(SCREEN_WIDTH) + " x " + String(SCREEN_HEIGHT);
  tft.drawCentreString(dimText, SCREEN_WIDTH/2, 50, 2);

  // Testrechtecke
  tft.fillRect(10, 70, 50, 30, TFT_RED);
  tft.fillRect(SCREEN_WIDTH - 60, 70, 50, 30, TFT_GREEN);
  tft.fillRect(10, SCREEN_HEIGHT - 50, 50, 30, TFT_BLUE);
  tft.fillRect(SCREEN_WIDTH - 60, SCREEN_HEIGHT - 50, 50, 30, TFT_YELLOW);

  drawServiceMenu();  // zurück zur Menüanzeige
}


void ServiceManager::onWiFiToggle() {
  if (wifiActive) {
    stopWiFi();
  } else {
    startWiFiAP();
  }
  
  // Buttons neu initialisieren (WiFi-Status hat sich geändert)
  initServiceButtons();
  drawServiceMenu();
}

void ServiceManager::onWebConfig() {
    if (!wifiActive) {
        // WiFi erst aktivieren
        startWiFiAP();
        initServiceButtons();
        drawServiceMenu();
        return;
    }
    
    if (!webServerActive) {
        startWebServer();
    }
    
    // Web-Config Anzeige
    tft.fillScreen(TFT_WHITE);
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    tft.drawCentreString("WEB-KONFIGURATION", SCREEN_WIDTH/2, 10, 2);
    tft.drawLine(0, 30, SCREEN_WIDTH, 30, TFT_BLACK);
    
    if (wifiActive && webServerActive) {
        IPAddress ip = WiFi.softAPIP();
        String ipText = "IP: " + ip.toString();
        tft.drawCentreString(ipText, SCREEN_WIDTH/2, 50, 2);
        tft.drawCentreString("http://" + ip.toString(), SCREEN_WIDTH/2, 80, 2);
        
        tft.setTextColor(TFT_DARKGREEN, TFT_WHITE);
        tft.drawCentreString("Web-Server AKTIV", SCREEN_WIDTH/2, 110, 2);
        
        tft.setTextColor(TFT_BLACK, TFT_WHITE);
        tft.drawCentreString("SSID: " + wifiSSID, SCREEN_WIDTH/2, 140, 1);
        tft.drawCentreString("Passwort: " + wifiPassword, SCREEN_WIDTH/2, 155, 1);
        
        tft.drawCentreString("Konfiguration über Browser", SCREEN_WIDTH/2, 180, 1);
        tft.drawCentreString("möglich", SCREEN_WIDTH/2, 195, 1);
        
        // SPIFFS-Status anzeigen
        size_t totalBytes = SPIFFS.totalBytes();
        size_t usedBytes = SPIFFS.usedBytes();
        float usage = (float)usedBytes / totalBytes * 100.0;
        
        tft.setTextColor(TFT_BLUE, TFT_WHITE);
        tft.drawCentreString("SPIFFS: " + String(usage, 1) + "% belegt", SCREEN_WIDTH/2, 215, 1);
        
        tft.setTextColor(TFT_BLACK, TFT_WHITE);
        tft.drawCentreString("Touch zum Zurückkehren", SCREEN_WIDTH/2, 235, 1);
    } else {
        tft.setTextColor(TFT_RED, TFT_WHITE);
        tft.drawCentreString("Web-Server nicht verfügbar!", SCREEN_WIDTH/2, 50, 2);
        tft.setTextColor(TFT_BLACK, TFT_WHITE);
        tft.drawCentreString("Aktivieren Sie zuerst WiFi", SCREEN_WIDTH/2, 80, 1);
    }
    
    // Warten auf Touch
    unsigned long startTime = millis();
    bool touched = false;
    
    while (!touched && (millis() - startTime < 10000)) {  // 10 Sekunden Timeout
        if (touchscreen.tirqTouched() && touchscreen.touched()) {
            touched = true;
            delay(200);  // Entprellung
        }
        delay(50);
    }
    
    // Zurück zum Service-Menü
    drawServiceMenu();
}

void ServiceManager::onSaveAndExit() {
  exitServiceMode();
}

void ServiceManager::onCancel() {
  cancelServiceMode();
}

void ServiceManager::startWiFiAP() {
  #if DB_INFO == 1
    Serial.println("DEBUG: Starte WiFi Access Point...");
  #endif
  
  // Access Point starten
  WiFi.mode(WIFI_AP);
  bool success = WiFi.softAP(wifiSSID.c_str(), wifiPassword.c_str());
  
  if (success) {
    wifiActive = true;
    IPAddress IP = WiFi.softAPIP();
    
    #if DB_INFO == 1
      Serial.print("DEBUG: WiFi AP gestartet - SSID: ");
      Serial.println(wifiSSID);
      Serial.print("DEBUG: IP-Adresse: ");
      Serial.println(IP);
    #endif
    
    // Automatisch Web-Server starten
    startWebServer();
  } else {
    #if DB_INFO == 1
      Serial.println("ERROR: WiFi AP konnte nicht gestartet werden");
    #endif
  }
}

void ServiceManager::stopWiFi() {
  #if DB_INFO == 1
    Serial.println("DEBUG: Stoppe WiFi...");
  #endif
  
  // Zuerst Web-Server stoppen
  if (webServerActive) {
    stopWebServer();
  }
  
  // WiFi stoppen
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  wifiActive = false;
  
  #if DB_INFO == 1
    Serial.println("DEBUG: WiFi gestoppt");
  #endif
}

void ServiceManager::startWebServer() {
    if (!webServerManager.isRunning()) {
        webServerManager.begin();
    }
}

void ServiceManager::stopWebServer() {
    if (webServerManager.isRunning()) {
        webServerManager.stop();
    }
}

void ServiceManager::handleServiceTelegram(String action, String params) {
  if (action == "SERVICE") {
    int value = params.toInt();
    if (value == 1) {
      enterServiceMode();
      #if DB_INFO == 1
        Serial.println("DEBUG: Service-Modus per Telegramm aktiviert");
      #endif
    } else {
      exitServiceMode();
      #if DB_INFO == 1
        Serial.println("DEBUG: Service-Modus per Telegramm deaktiviert");
      #endif
    }
  } else if (action == "WIFI") {
    // WiFi-Steuerung per Telegramm
    int value = params.toInt();
    if (value == 1) {
      // WiFi aktivieren
      if (!wifiActive) {
        startWiFiAP();
        #if DB_INFO == 1
          Serial.println("DEBUG: WiFi per Telegramm aktiviert");
        #endif
      }
    } else {
      // WiFi deaktivieren
      if (wifiActive) {
        stopWiFi();
        #if DB_INFO == 1
          Serial.println("DEBUG: WiFi per Telegramm deaktiviert");
        #endif
      }
    }
  } else if (action == "WEBSERVER") {
    // Web-Server-Steuerung per Telegramm
    int value = params.toInt();
    if (value == 1 && wifiActive) {
      if (!webServerActive) {
        startWebServer();
        #if DB_INFO == 1
          Serial.println("DEBUG: Web-Server per Telegramm aktiviert");
        #endif
      }
    } else {
      if (webServerActive) {
        stopWebServer();
        #if DB_INFO == 1
          Serial.println("DEBUG: Web-Server per Telegramm deaktiviert");
        #endif
      }
    }
  } else if (action == "DEVICE_ID") {
    // Device ID per Telegramm ändern
    if (params.length() == 4) {
      bool isValid = true;
      for (int i = 0; i < 4; i++) {
        if (!isDigit(params.charAt(i))) {
          isValid = false;
          break;
        }
      }
      
      if (isValid) {
        currentDeviceID = params;
        configChanged = true;
        
        #if DB_INFO == 1
          Serial.print("DEBUG: Device ID per Telegramm geändert auf: ");
          Serial.println(currentDeviceID);
        #endif
      } else {
        #if DB_INFO == 1
          Serial.println("ERROR: Ungültige Device ID per Telegramm (muss 4 Ziffern sein)");
        #endif
      }
    }
  } else if (action == "ORIENTATION") {
    // Orientierung per Telegramm ändern
    int value = params.toInt();
    if (value == 0 || value == 1) {
      currentOrientation = value;
      configChanged = true;
      
      // Sofort anwenden
      applyOrientation(currentOrientation);
      
      #if DB_INFO == 1
        Serial.print("DEBUG: Orientierung per Telegramm geändert auf: ");
        Serial.println(currentOrientation == LANDSCAPE ? "LANDSCAPE" : "PORTRAIT");
      #endif
    }
  } else if (action == "RESET") {
    // System-Reset per Telegramm
    #if DB_INFO == 1
      Serial.println("DEBUG: SYSTEM RESET per Telegramm empfangen!");
      Serial.println("DEBUG: ESP32 wird in 2 Sekunden neu gestartet...");
      Serial.flush();
    #endif
    
    delay(2000);
    ESP.restart();
  }
}

void ServiceManager::loadConfig() {
  ServiceConfig config;
  EEPROM.get(CONFIG_EEPROM_ADDR, config);
  
  // Konfiguration validieren
  if (config.magic == CONFIG_MAGIC_BYTE) {
    // Checksum prüfen
    uint8_t checksum = config.magic + config.orientation;
    for (int i = 0; i < 4; i++) {
      checksum += config.deviceID[i];
    }
    
    if (checksum == config.checksum) {
      currentDeviceID = String(config.deviceID);
      currentOrientation = config.orientation;
      
      #if DB_INFO == 1
        Serial.print("DEBUG: Konfiguration geladen - Device ID: ");
        Serial.print(currentDeviceID);
        Serial.print(", Orientation: ");
        Serial.println(currentOrientation == LANDSCAPE ? "LANDSCAPE" : "PORTRAIT");
      #endif
      return;
    }
  }
  
  #if DB_INFO == 1
    Serial.println("DEBUG: Keine gültige Konfiguration gefunden, verwende Standard-Werte");
  #endif
}

void ServiceManager::saveConfig() {
  ServiceConfig config;
  config.magic = CONFIG_MAGIC_BYTE;
  strncpy(config.deviceID, currentDeviceID.c_str(), 4);
  config.deviceID[4] = '\0';
  config.orientation = currentOrientation;
  
  // Checksum berechnen
  config.checksum = config.magic + config.orientation;
  for (int i = 0; i < 4; i++) {
    config.checksum += config.deviceID[i];
  }
  
  EEPROM.put(CONFIG_EEPROM_ADDR, config);
  EEPROM.commit();
  
  #if DB_INFO == 1
    Serial.print("DEBUG: Konfiguration gespeichert - Device ID: ");
    Serial.print(currentDeviceID);
    Serial.print(", Orientation: ");
    Serial.println(currentOrientation == LANDSCAPE ? "LANDSCAPE" : "PORTRAIT");
  #endif
}

String ServiceManager::getDeviceID() {
  return currentDeviceID;
}

void ServiceManager::setDeviceID(String newID) {
  if (newID.length() == 4) {
    bool isValid = true;
    for (int i = 0; i < 4; i++) {
      if (!isDigit(newID.charAt(i))) {
        isValid = false;
        break;
      }
    }
    
    if (isValid) {
      currentDeviceID = newID;
      configChanged = true;
      
      #if DB_INFO == 1
        Serial.print("DEBUG: Device ID geändert auf: ");
        Serial.println(currentDeviceID);
      #endif
    } else {
      #if DB_INFO == 1
        Serial.println("ERROR: Ungültige Device ID (muss 4 Ziffern sein)");
      #endif
    }
  }
}

int ServiceManager::getOrientation() {
  // *** KORREKTUR: tft.getRotation() direkt verwenden ***
  int tftRotation = tft.getRotation();
  
  #if DB_INFO == 1
    Serial.print("DEBUG: TFT Rotation ist: ");
    Serial.print(tftRotation);
    Serial.print(" → Orientierung: ");
    Serial.println((tftRotation == 0 || tftRotation == 2) ? "PORTRAIT" : "LANDSCAPE");
  #endif
  
  // TFT-Rotation zu unserer Orientierung mappem:
  // 0 oder 2 = Portrait, 1 oder 3 = Landscape
  return (tftRotation == 0 || tftRotation == 2) ? PORTRAIT : LANDSCAPE;
}

void ServiceManager::setOrientation(int orientation) {
  #if DB_INFO == 1
    Serial.print("DEBUG: setOrientation() aufgerufen mit: ");
    Serial.println(orientation == LANDSCAPE ? "LANDSCAPE" : "PORTRAIT");
  #endif
  
  // *** KORREKTUR: Sofortige TFT-Anwendung ***
  if (orientation == PORTRAIT) {
    tft.setRotation(ROTATION_0);        // Portrait
    currentOrientation = PORTRAIT;
  } else {
    tft.setRotation(ROTATION_270);      // Landscape USB rechts (Standard)
    currentOrientation = LANDSCAPE;
  }
  
  configChanged = true;
  orientationChanged = true;
  
  #if DB_INFO == 1
    Serial.print("DEBUG: TFT Rotation gesetzt auf: ");
    Serial.print(tft.getRotation());
    Serial.print(", currentOrientation: ");
    Serial.println(currentOrientation == LANDSCAPE ? "LANDSCAPE" : "PORTRAIT");
  #endif
  
  // *** UI SOFORT NEU AUFBAUEN ***
  // Buttons neu initialisieren für neue Orientierung
  initButtons();
  
  // Header neu initialisieren für neue Orientierung  
  if (currentState == SERVICE_ACTIVE) {
    // Im Service-Modus: Service-Buttons neu initialisieren
    initServiceButtons();
    drawServiceMenu();
  } else {
    // Im Normal-Modus: Hauptmenü neu zeichnen
    showMenu();
  }
}

bool ServiceManager::isWiFiActive() {
  return wifiActive;
}

bool ServiceManager::isWebServerActive() {
  return webServerActive;
}

// *** DEVICE ID EDITOR IMPLEMENTATION ***

void ServiceManager::drawDeviceIDEditor() {
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  
  // Header
  tft.drawCentreString("DEVICE ID EDITOR", SCREEN_WIDTH/2, SCREEN_HEIGHT - 10, 1);
  
  // Device ID Anzeige
  drawDeviceIDDisplay();
  
  // Numpad
  drawNumpad();
}

void ServiceManager::drawDeviceIDDisplay() {
  int displayY = 5;  // Direkt im oberen Bereich (Headerbereich)

  tft.setTextColor(TFT_BLACK, TFT_WHITE);

  // ID-Zeichen einzeln zeichnen - ORIENTIERUNGSABHÄNGIG
  int charWidth = 40;
  int spacing = 5;
  int totalWidth = 4 * charWidth + 3 * spacing;
  int startX = (SCREEN_WIDTH - totalWidth) / 2;
  int charY = displayY;

  for (int i = 0; i < 4; i++) {
    int x = startX + i * (charWidth + spacing);
    uint16_t bgColor = (i == editPosition) ? TFT_YELLOW : TFT_LIGHTGREY;
    uint16_t textColor = TFT_BLACK;

    tft.fillRect(x, charY, charWidth, 30, bgColor);
    tft.drawRect(x, charY, charWidth, 30, TFT_BLACK);

    String digit = String(editDeviceID.charAt(i));
    tft.setTextColor(textColor);
    tft.drawCentreString(digit, x + charWidth/2, charY + 8, 2);
  }

  // Position-Indikator
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  String posText = "Pos: " + String(editPosition + 1) + "/4";
  tft.drawCentreString(posText, SCREEN_WIDTH/2, charY + 35, 1);
}  

void ServiceManager::drawNumpad() {
  for (int i = 0; i < NUM_NUMPAD_BUTTONS; i++) {
    // Button-Hintergrund
    tft.fillRect(numpadButtons[i].x, numpadButtons[i].y, 
                 numpadButtons[i].w, numpadButtons[i].h, numpadButtons[i].color);
    
    // Button-Rahmen
    tft.drawRect(numpadButtons[i].x, numpadButtons[i].y, 
                 numpadButtons[i].w, numpadButtons[i].h, TFT_BLACK);
    
    // Button-Text
    tft.setTextColor(numpadButtons[i].textColor);
    int textX = numpadButtons[i].x + numpadButtons[i].w/2;
    int textY = numpadButtons[i].y + numpadButtons[i].h/2 - 8;
    tft.drawCentreString(numpadButtons[i].label, textX, textY, 2);
  }
}

int ServiceManager::checkNumpadButtonPress(int x, int y) {
  for (int i = 0; i < NUM_NUMPAD_BUTTONS; i++) {
    if (x >= numpadButtons[i].x && x <= numpadButtons[i].x + numpadButtons[i].w &&
        y >= numpadButtons[i].y && y <= numpadButtons[i].y + numpadButtons[i].h) {
      return i;
    }
  }
  return -1;
}

void ServiceManager::handleNumpadButtonPress(int buttonIndex) {
  char value = numpadButtons[buttonIndex].value;
  
  #if DB_INFO == 1
    Serial.print("DEBUG: Numpad-Button gedrückt: ");
    Serial.print(numpadButtons[buttonIndex].label);
    Serial.print(" (");
    Serial.print(value);
    Serial.println(")");
  #endif
  
  switch (value) {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      updateDeviceIDDigit(value);
      break;
      
    case '+':
      incrementCurrentDigit();
      break;
      
    case '-':
      decrementCurrentDigit();
      break;
      
    case '<':
      moveEditPosition(-1);
      break;
      
    case '>':
      moveEditPosition(1);
      break;
      
    case 'O':  // OK
      confirmDeviceIDEdit();
      break;
      
    case 'C':  // CANCEL
      cancelDeviceIDEdit();
      break;
  }
  
  // Display aktualisieren (außer bei OK/CANCEL)
  if (value != 'O' && value != 'C') {
    drawDeviceIDDisplay();
  }
}

void ServiceManager::updateDeviceIDDigit(char digit) {
  if (editPosition >= 0 && editPosition < 4) {
    editDeviceID.setCharAt(editPosition, digit);
    
    #if DB_INFO == 1
      Serial.print("DEBUG: Ziffer an Position ");
      Serial.print(editPosition);
      Serial.print(" auf ");
      Serial.print(digit);
      Serial.print(" gesetzt. Neue ID: ");
      Serial.println(editDeviceID);
    #endif
  }
}

void ServiceManager::moveEditPosition(int direction) {
  editPosition += direction;
  editPosition = constrain(editPosition, 0, 3);
  
  #if DB_INFO == 1
    Serial.print("DEBUG: Edit-Position auf ");
    Serial.println(editPosition);
  #endif
}

void ServiceManager::incrementCurrentDigit() {
  if (editPosition >= 0 && editPosition < 4) {
    char currentChar = editDeviceID.charAt(editPosition);
    int digit = currentChar - '0';
    digit = (digit + 1) % 10;  // 0-9 Zyklus
    editDeviceID.setCharAt(editPosition, '0' + digit);
    
    #if DB_INFO == 1
      Serial.print("DEBUG: Ziffer incrementiert auf ");
      Serial.println(digit);
    #endif
  }
}

void ServiceManager::decrementCurrentDigit() {
  if (editPosition >= 0 && editPosition < 4) {
    char currentChar = editDeviceID.charAt(editPosition);
    int digit = currentChar - '0';
    digit = (digit - 1 + 10) % 10;  // 0-9 Zyklus (rückwärts)
    editDeviceID.setCharAt(editPosition, '0' + digit);
    
    #if DB_INFO == 1
      Serial.print("DEBUG: Ziffer decrementiert auf ");
      Serial.println(digit);
    #endif
  }
}

void ServiceManager::confirmDeviceIDEdit() {
  // Validierung der neuen Device ID
  bool isValid = true;
  for (int i = 0; i < 4; i++) {
    if (!isDigit(editDeviceID.charAt(i))) {
      isValid = false;
      break;
    }
  }
  
  if (isValid) {
    currentDeviceID = editDeviceID;
    configChanged = true;
    
    #if DB_INFO == 1
      Serial.print("DEBUG: Device ID bestätigt: ");
      Serial.println(currentDeviceID);
    #endif
    
    // Erfolgsmeldung anzeigen
    tft.fillRect(0, SCREEN_HEIGHT - 50, SCREEN_WIDTH, 50, TFT_DARKGREEN);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("Device ID erfolgreich geändert!", SCREEN_WIDTH/2, SCREEN_HEIGHT - 35, 2);
    tft.drawCentreString("Neue ID: " + currentDeviceID, SCREEN_WIDTH/2, SCREEN_HEIGHT - 15, 1);
    
    delay(2000);
  } else {
    // Fehlermeldung
    tft.fillRect(0, SCREEN_HEIGHT - 50, SCREEN_WIDTH, 50, TFT_RED);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("Ungültige Device ID!", SCREEN_WIDTH/2, SCREEN_HEIGHT - 35, 2);
    tft.drawCentreString("Nur Ziffern 0-9 erlaubt", SCREEN_WIDTH/2, SCREEN_HEIGHT - 15, 1);
    
    delay(2000);
    
    // Zurück zum Editor
    drawDeviceIDEditor();
    return;
  }
  
  // Zurück zum Service-Menü
  currentState = SERVICE_ACTIVE;
  drawServiceMenu();
}

void ServiceManager::cancelDeviceIDEdit() {
  #if DB_INFO == 1
    Serial.println("DEBUG: Device ID Bearbeitung abgebrochen");
  #endif
  
  // Abbruch-Meldung anzeigen
  tft.fillRect(0, SCREEN_HEIGHT - 30, SCREEN_WIDTH, 30, TFT_ORANGE);
  tft.setTextColor(TFT_BLACK);
  tft.drawCentreString("Device ID Bearbeitung abgebrochen", SCREEN_WIDTH/2, SCREEN_HEIGHT - 20, 1);
  
  delay(1000);
  
  // Zurück zum Service-Menü ohne Änderungen
  currentState = SERVICE_ACTIVE;
  drawServiceMenu();
}

// *** GLOBALE HILFSFUNKTIONEN ***

void setupServiceManager() {
  EEPROM.begin(512);  // EEPROM initialisieren
  serviceManager.loadConfig();
  
  #if DB_INFO == 1
    Serial.println("DEBUG: ServiceManager initialisiert - Version 1.50");
    Serial.print("DEBUG: Device ID: ");
    Serial.println(serviceManager.getDeviceID());
    Serial.print("DEBUG: Orientierung: ");
    Serial.println(serviceManager.getOrientation() == LANDSCAPE ? "LANDSCAPE" : "PORTRAIT");
  #endif
}

void updateServiceManager() {
  serviceManager.update();
}

void handleServiceTouch(int x, int y, bool touched) {
  serviceManager.handleTouch(x, y, touched);
}

void handleWebServerLoop() {
  serviceManager.handleWebServer();
  }