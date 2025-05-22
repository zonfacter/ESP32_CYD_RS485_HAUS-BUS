/**
 * service_manager.cpp - Version 1.50
 * 
 * Implementation des Service-Menüs für ESP32 Touch-Interface
 */

#include "service_manager.h"
#include "menu.h"
#include "communication.h"
#include "touch.h"  // Für getTouchPoint()
#include <EEPROM.h>
#include <WiFi.h>

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
  // Button-Layout für Service-Menü - DYNAMISCH je nach Orientierung
  int buttonW, buttonH, spacing, startY;
  
  if (currentOrientation == LANDSCAPE) {
    // Landscape: 3x2 Layout
    buttonW = (SCREEN_WIDTH - 30) / 2;  // 2 Spalten
    buttonH = 40;
    spacing = 10;
    startY = 60;
  } else {
    // Portrait: 2x3 Layout  
    buttonW = (SCREEN_WIDTH - 30) / 2;  // 2 Spalten
    buttonH = 35;  // Etwas kleiner für Portrait
    spacing = 8;
    startY = 70;
  }
  
  // Button 0: Edit Device ID
  serviceButtons[0] = {10, startY, buttonW, buttonH, "Device ID", TFT_BLUE, TFT_WHITE, true};
  
  // Button 1: Toggle Orientation  
  String orientLabel = (currentOrientation == LANDSCAPE) ? "→ Portrait" : "→ Landscape";
  serviceButtons[1] = {20 + buttonW, startY, buttonW, buttonH, orientLabel, TFT_GREEN, TFT_BLACK, true};
  
  // Button 2: WiFi Toggle - AKTIVIERT
  String wifiLabel = wifiActive ? "WiFi OFF" : "WiFi ON";
  uint16_t wifiColor = wifiActive ? TFT_RED : TFT_ORANGE;
  serviceButtons[2] = {10, startY + buttonH + spacing, buttonW, buttonH, wifiLabel, wifiColor, TFT_BLACK, true};
  
  // Button 3: Web Config - AKTIVIERT wenn WiFi an
  serviceButtons[3] = {20 + buttonW, startY + buttonH + spacing, buttonW, buttonH, "Web Config", TFT_PURPLE, TFT_WHITE, wifiActive};
  
  // Button 4: Save & Exit
  serviceButtons[4] = {10, startY + 2*(buttonH + spacing), buttonW, buttonH, "SAVE & EXIT", TFT_DARKGREEN, TFT_WHITE, true};
  
  // Button 5: Cancel
  serviceButtons[5] = {20 + buttonW, startY + 2*(buttonH + spacing), buttonW, buttonH, "CANCEL", TFT_RED, TFT_WHITE, true};
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
  
  #if DB_INFO == 1
    Serial.println("DEBUG: Service-Modus aktiviert");
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
    
    // Orientierung beim Verlassen anwenden - DIREKT ohne applyOrientation
    if (currentOrientation == LANDSCAPE) {
      tft.setRotation(1);
      touchscreen.setRotation(1);
    } else {
      tft.setRotation(0);  
      touchscreen.setRotation(0);
    }
  }
  
  currentState = SERVICE_INACTIVE;
  configChanged = false;
  
  #if DB_INFO == 1
    Serial.println("DEBUG: Service-Modus verlassen");
  #endif
  
  // Zurück zum Hauptmenü in neuer Orientierung
  delay(500);  // Kurze Pause für Orientierungs-Umschaltung
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
  tft.drawCentreString("SERVICE MODUS - V1.50", SCREEN_WIDTH/2, 10, 2);
  tft.drawLine(0, 30, SCREEN_WIDTH, 30, TFT_BLACK);
  
  // Aktuelle Konfiguration anzeigen - ERWEITERT
  tft.setTextSize(1);
  String configText = "Device ID: " + currentDeviceID;
  tft.drawCentreString(configText, SCREEN_WIDTH/2, 40, 1);
  
  String orientText = "Orientierung: " + String(currentOrientation == LANDSCAPE ? "LANDSCAPE" : "PORTRAIT");
  tft.drawCentreString(orientText, SCREEN_WIDTH/2, 50, 1);
  
  // Bildschirm-Dimensionen anzeigen
  String dimText = "Display: " + String(SCREEN_WIDTH) + "x" + String(SCREEN_HEIGHT);
  tft.drawCentreString(dimText, SCREEN_WIDTH/2 + 80, 40, 1);
  
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
    }
  }
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
    case 2: onWiFiToggle(); break;      // Für später
    case 3: onWebConfig(); break;       // Für später  
    case 4: onSaveAndExit(); break;
    case 5: onCancel(); break;
  }
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
  #endif
  
  // Device ID Editor anzeigen
  drawDeviceIDEditor();
}

void ServiceManager::onToggleOrientation() {
  // Orientierung umschalten
  if (currentOrientation == LANDSCAPE) {
    currentOrientation = PORTRAIT;
  } else {
    currentOrientation = LANDSCAPE;
  }
  
  configChanged = true;
  
  #if DB_INFO == 1
    Serial.print("DEBUG: Orientierung umgeschaltet auf: ");
    Serial.println(currentOrientation == LANDSCAPE ? "LANDSCAPE" : "PORTRAIT");
  #endif
  
  // Sofortige Anwendung der neuen Orientierung - DIREKT ohne applyOrientation
  if (currentOrientation == LANDSCAPE) {
    tft.setRotation(1);
    touchscreen.setRotation(1);
  } else {
    tft.setRotation(0);
    touchscreen.setRotation(0);
  }
  
  // Kurze Verzögerung für visuelles Feedback
  delay(500);
  
  // Service-Menü in neuer Orientierung neu zeichnen
  initServiceButtons();  // Buttons für neue Orientierung neu berechnen
  drawServiceMenu();
}

void ServiceManager::onWiFiToggle() {
  if (wifiActive) {
    // WiFi stoppen - DIREKT implementiert
    #if DB_INFO == 1
      Serial.println("DEBUG: Stoppe WiFi...");
    #endif
    
    // Zuerst Web-Server stoppen
    if (webServerActive) {
      webServerActive = false;
      #if DB_INFO == 1
        Serial.println("DEBUG: Web-Server gestoppt");
      #endif
    }
    
    // WiFi stoppen
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    wifiActive = false;
    
    #if DB_INFO == 1
      Serial.println("DEBUG: WiFi gestoppt");
    #endif
  } else {
    // WiFi starten - DIREKT implementiert
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
      if (!webServerActive) {
        webServerActive = true;
        #if DB_INFO == 1
          Serial.println("DEBUG: Web-Server gestartet (vereinfacht)");
        #endif
      }
    } else {
      #if DB_INFO == 1
        Serial.println("ERROR: WiFi AP konnte nicht gestartet werden");
      #endif
    }
  }
  
  // Buttons neu initialisieren (WiFi-Status hat sich geändert)
  initServiceButtons();
  drawServiceMenu();
}

void ServiceManager::onWebConfig() {
  if (wifiActive && !webServerActive) {
    startWebServer();
  }
  
  // Web-Config Anzeige - VEREINFACHT ohne getTouchPoint Loop
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString("WEB-KONFIGURATION", SCREEN_WIDTH/2, 10, 2);
  tft.drawLine(0, 30, SCREEN_WIDTH, 30, TFT_BLACK);
  
  if (wifiActive) {
    String ipText = "IP: " + WiFi.softAPIP().toString();
    tft.drawCentreString(ipText, SCREEN_WIDTH/2, 50, 2);
    tft.drawCentreString("http://192.168.4.1", SCREEN_WIDTH/2, 80, 2);
    
    tft.setTextColor(TFT_DARKGREEN, TFT_WHITE);
    tft.drawCentreString("Web-Server AKTIV", SCREEN_WIDTH/2, 110, 2);
    
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    tft.drawCentreString("Konfiguration über Browser", SCREEN_WIDTH/2, 140, 1);
    tft.drawCentreString("möglich", SCREEN_WIDTH/2, 155, 1);
    
    tft.drawCentreString("Touch zum Zurückkehren", SCREEN_WIDTH/2, 180, 1);
  } else {
    tft.setTextColor(TFT_RED, TFT_WHITE);
    tft.drawCentreString("WiFi nicht aktiv!", SCREEN_WIDTH/2, 50, 2);
  }
  
  // Automatischer Rücksprung nach 5 Sekunden
  delay(5000);
  
  // Zurück zum Service-Menü
  drawServiceMenu();
}

void ServiceManager::onSaveAndExit() {
  exitServiceMode();
}

void ServiceManager::onCancel() {
  cancelServiceMode();
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
    // WiFi-Steuerung per Telegramm - DIREKT implementiert
    int value = params.toInt();
    if (value == 1) {
      // WiFi aktivieren
      if (!wifiActive) {
        #if DB_INFO == 1
          Serial.println("DEBUG: Starte WiFi per Telegramm...");
        #endif
        
        WiFi.mode(WIFI_AP);
        bool success = WiFi.softAP(wifiSSID.c_str(), wifiPassword.c_str());
        
        if (success) {
          wifiActive = true;
          webServerActive = true;  // Automatisch mit starten
          
          #if DB_INFO == 1
            Serial.println("DEBUG: WiFi per Telegramm aktiviert");
          #endif
        }
      }
    } else {
      // WiFi deaktivieren
      if (wifiActive) {
        webServerActive = false;
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_OFF);
        wifiActive = false;
        
        #if DB_INFO == 1
          Serial.println("DEBUG: WiFi per Telegramm deaktiviert");
        #endif
      }
    }
  } else if (action == "WEBSERVER") {
    // Web-Server-Steuerung per Telegramm - DIREKT implementiert
    int value = params.toInt();
    if (value == 1 && wifiActive) {
      webServerActive = true;
      #if DB_INFO == 1
        Serial.println("DEBUG: Web-Server per Telegramm aktiviert");
      #endif
    } else {
      webServerActive = false;
      #if DB_INFO == 1
        Serial.println("DEBUG: Web-Server per Telegramm deaktiviert");
      #endif
    }
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
  currentDeviceID = newID;
  configChanged = true;
}

int ServiceManager::getOrientation() {
  return currentOrientation;
}

void ServiceManager::setOrientation(int orientation) {
  currentOrientation = orientation;
  configChanged = true;
}

// Globale Hilfsfunktionen
void setupServiceManager() {
  EEPROM.begin(512);  // EEPROM initialisieren
  serviceManager.loadConfig();
  
  #if DB_INFO == 1
    Serial.println("DEBUG: ServiceManager initialisiert - Version 1.50");
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

// *** NEU: Public Interface Funktionen ***

bool ServiceManager::isWiFiActive() {
  return wifiActive;
}

bool ServiceManager::isWebServerActive() {
  return webServerActive;
}

void ServiceManager::handleWebServer() {
  // Vereinfacht - kein WebServer.handleClient() für jetzt
  if (webServerActive) {
    // Placeholder für Web-Server Handling
  }
}

// *** NEU: Device ID Editor Implementation ***

void ServiceManager::initNumpadButtons() {
  // Numpad Layout: 4x4 Grid
  // [1] [2] [3] [+]
  // [4] [5] [6] [-]  
  // [7] [8] [9] [<]
  // [0] [  ] [  ] [>]
  // [OK]    [CANCEL]
  
  int buttonW = 60;
  int buttonH = 40;
  int spacing = 5;
  int startX = (SCREEN_WIDTH - (4 * buttonW + 3 * spacing)) / 2;
  int startY = 80;
  
  // Zahlen 1-3
  numpadButtons[0] = {startX, startY, buttonW, buttonH, "1", '1', TFT_LIGHTGREY, TFT_BLACK};
  numpadButtons[1] = {startX + (buttonW + spacing), startY, buttonW, buttonH, "2", '2', TFT_LIGHTGREY, TFT_BLACK};
  numpadButtons[2] = {startX + 2*(buttonW + spacing), startY, buttonW, buttonH, "3", '3', TFT_LIGHTGREY, TFT_BLACK};
  
  // Zahlen 4-6
  numpadButtons[3] = {startX, startY + (buttonH + spacing), buttonW, buttonH, "4", '4', TFT_LIGHTGREY, TFT_BLACK};
  numpadButtons[4] = {startX + (buttonW + spacing), startY + (buttonH + spacing), buttonW, buttonH, "5", '5', TFT_LIGHTGREY, TFT_BLACK};
  numpadButtons[5] = {startX + 2*(buttonW + spacing), startY + (buttonH + spacing), buttonW, buttonH, "6", '6', TFT_LIGHTGREY, TFT_BLACK};
  
  // Zahlen 7-9
  numpadButtons[6] = {startX, startY + 2*(buttonH + spacing), buttonW, buttonH, "7", '7', TFT_LIGHTGREY, TFT_BLACK};
  numpadButtons[7] = {startX + (buttonW + spacing), startY + 2*(buttonH + spacing), buttonW, buttonH, "8", '8', TFT_LIGHTGREY, TFT_BLACK};
  numpadButtons[8] = {startX + 2*(buttonW + spacing), startY + 2*(buttonH + spacing), buttonW, buttonH, "9", '9', TFT_LIGHTGREY, TFT_BLACK};
  
  // Zahl 0
  numpadButtons[9] = {startX, startY + 3*(buttonH + spacing), buttonW, buttonH, "0", '0', TFT_LIGHTGREY, TFT_BLACK};
  
  // Funktions-Buttons (rechte Spalte)
  numpadButtons[10] = {startX + 3*(buttonW + spacing), startY, buttonW, buttonH, "+", '+', TFT_GREEN, TFT_BLACK};
  numpadButtons[11] = {startX + 3*(buttonW + spacing), startY + (buttonH + spacing), buttonW, buttonH, "-", '-', TFT_RED, TFT_WHITE};
  numpadButtons[12] = {startX + 3*(buttonW + spacing), startY + 2*(buttonH + spacing), buttonW, buttonH, "<", '<', TFT_BLUE, TFT_WHITE};
  numpadButtons[13] = {startX + 3*(buttonW + spacing), startY + 3*(buttonH + spacing), buttonW, buttonH, ">", '>', TFT_BLUE, TFT_WHITE};
  
  // OK und CANCEL (breite Buttons unten)
  int wideButtonW = 2*buttonW + spacing;
  numpadButtons[14] = {startX, startY + 4*(buttonH + spacing) + 10, wideButtonW, buttonH, "OK", 'O', TFT_DARKGREEN, TFT_WHITE};
  numpadButtons[15] = {startX + wideButtonW + spacing, startY + 4*(buttonH + spacing) + 10, wideButtonW, buttonH, "CANCEL", 'C', TFT_RED, TFT_WHITE};
}

void ServiceManager::drawDeviceIDEditor() {
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  
  // Header
  tft.drawCentreString("DEVICE ID EDITOR", SCREEN_WIDTH/2, 10, 2);
  tft.drawLine(0, 30, SCREEN_WIDTH, 30, TFT_BLACK);
  
  // Device ID Anzeige
  drawDeviceIDDisplay();
  
  // Numpad
  drawNumpad();
}

void ServiceManager::drawDeviceIDDisplay() {
  // Device ID groß anzeigen mit Cursor
  int displayY = 45;
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString("Current ID:", SCREEN_WIDTH/2, displayY, 1);
  
  // ID-Zeichen einzeln zeichnen
  int charWidth = 40;
  int spacing = 5;
  int totalWidth = 4 * charWidth + 3 * spacing;
  int startX = (SCREEN_WIDTH - totalWidth) / 2;
  int charY = displayY + 15;
  
  for (int i = 0; i < 4; i++) {
    int x = startX + i * (charWidth + spacing);
    
    // Hintergrund-Farbe je nach Position
    uint16_t bgColor = (i == editPosition) ? TFT_YELLOW : TFT_LIGHTGREY;
    uint16_t textColor = TFT_BLACK;
    
    // Zeichen-Box zeichnen
    tft.fillRect(x, charY, charWidth, 30, bgColor);
    tft.drawRect(x, charY, charWidth, 30, TFT_BLACK);
    
    // Zeichen zentriert zeichnen
    String digit = String(editDeviceID.charAt(i));
    tft.setTextColor(textColor);
    tft.drawCentreString(digit, x + charWidth/2, charY + 8, 2);
  }
  
  // Position-Indikator
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  String posText = "Position: " + String(editPosition + 1) + "/4";
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
    
    // Automatisch zur nächsten Position
    if (editPosition < 3) {
      editPosition++;
    }
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
  currentDeviceID = editDeviceID;
  configChanged = true;
  
  #if DB_INFO == 1
    Serial.print("DEBUG: Device ID bestätigt: ");
    Serial.println(currentDeviceID);
  #endif
  
  // Zurück zum Service-Menü
  currentState = SERVICE_ACTIVE;
  drawServiceMenu();
}

void ServiceManager::cancelDeviceIDEdit() {
  #if DB_INFO == 1
    Serial.println("DEBUG: Device ID Bearbeitung abgebrochen");
  #endif
  
  // Zurück zum Service-Menü ohne Änderungen
  currentState = SERVICE_ACTIVE;
  drawServiceMenu();
}