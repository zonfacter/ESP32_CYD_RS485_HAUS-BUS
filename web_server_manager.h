/**
 * web_server_manager.h - SPIFFS-basierte Version
 * 
 * Saubere Header-Datei für SPIFFS Web-Server
 */

#ifndef WEB_SERVER_MANAGER_H
#define WEB_SERVER_MANAGER_H

#include "Arduino.h"
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "service_manager.h"
#include "communication.h"
#include "backlight.h"

// Forward-Deklarationen
extern int currentBacklight;
extern TFT_eSPI tft;
extern XPT2046_Touchscreen touchscreen;
extern ServiceManager serviceManager;

class WebServerManager {
private:
    WebServer server;
    bool serverRunning;

    // Setup-Funktionen
    void setupRoutes();
    void serveFile(const String& path, const String& contentType = "");

    // Handler-Funktionen
    void handleNotFound();

    // API-Handler
    void handleAPIStatus();
    void handleAPIGetConfig();
    void handleAPISaveConfig();
    void handleAPISetBrightness();
    void handleAPISetOrientation();
    void handleAPISetDeviceID();
    void handleAPIButtonControl();
    void handleAPICreateBackup();
    void handleAPIListBackups();
    void handleAPIReboot();
    void handleAPIFactoryReset();

    // JSON-Hilfsfunktionen
    void sendJSON(JsonDocument& doc, int httpCode = 200);
    void sendError(const String& message, int httpCode = 400);
    void sendSuccess(const String& message);

public:
    WebServerManager();
    
    // Hauptfunktionen
    void begin();
    void stop();
    void handleClient();
    bool isRunning();
};

// Globale Instanz
extern WebServerManager webServerManager;

#endif // WEB_SERVER_MANAGER_H