#ifndef WEB_SERVER_MANAGER_H
#define WEB_SERVER_MANAGER_H

#include "Arduino.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include "service_manager.h"
#include "communication.h"
#include "backlight.h"
#include "menu.h"
#include <ESPAsyncWebServer.h>

using namespace fs;

// Forward-Deklarationen
extern int currentBacklight;
extern TFT_eSPI tft;
extern XPT2046_Touchscreen touchscreen;
extern ServiceManager serviceManager;

class WebServerManager {
public:
    WebServerManager();

    void begin();
    void stop();
    bool isRunning() const;
    void setupRoutes();
    void serveFile(AsyncWebServerRequest *request, const String& path, const String& type = "");
    
    // Korrigierte Handler-Funktionen mit AsyncWebServerRequest Parameter
    void handleNotFound(AsyncWebServerRequest *request);
    void handleAPIStatus(AsyncWebServerRequest *request);
    void handleAPIGetConfig(AsyncWebServerRequest *request);
    void handleAPISaveConfig(AsyncWebServerRequest *request);
    void handleAPISetBrightness(AsyncWebServerRequest *request);
    void handleAPISetOrientation(AsyncWebServerRequest *request);
    void handleAPISetDeviceID(AsyncWebServerRequest *request);
    void handleAPIButtonControl(AsyncWebServerRequest *request);
    void handleAPICreateBackup(AsyncWebServerRequest *request);
    void handleAPIListBackups(AsyncWebServerRequest *request);
    void handleAPIReboot(AsyncWebServerRequest *request);
    void handleAPIFactoryReset(AsyncWebServerRequest *request);

    // Button-Konfiguration Handler
    void handleGetButtonConfig(AsyncWebServerRequest *request);
    void handleSaveButtonConfig(AsyncWebServerRequest *request);
    void handleGetTemplates(AsyncWebServerRequest *request);
    void handleApplyTemplate(AsyncWebServerRequest *request);
    void handleSaveTemplate(AsyncWebServerRequest *request);
    void handleTestButton(AsyncWebServerRequest *request);
    void handleExportConfig(AsyncWebServerRequest *request);
    void handleImportConfig(AsyncWebServerRequest *request);
    void handleCreateBackup(AsyncWebServerRequest *request);

    void sendJSON(AsyncWebServerRequest *request, JsonDocument& doc, int httpCode);
    void sendError(AsyncWebServerRequest *request, const String& message, int httpCode);
    void sendSuccess(AsyncWebServerRequest *request, const String& message);

    // WebInterface
    void handleAPIGetTime(AsyncWebServerRequest *request);
    void handleAPISetTime(AsyncWebServerRequest *request);
    void handleAPIGetDate(AsyncWebServerRequest *request);
    void handleAPISetDate(AsyncWebServerRequest *request);
    void handleAPISetDateTime(AsyncWebServerRequest *request);

private:
    AsyncWebServer server;
    bool serverRunning;
    void syncButtonsToDisplay();
    void loadButtonConfigFromSPIFFS();
    void saveButtonConfigToSPIFFS();
    void createDefaultButtonConfig();
    uint16_t hexToColor565(String hexColor);
    String color565ToHex(uint16_t color565);
    String getIconTypeFromLabel(String label);
    String getIconForType(String iconType);
    void updateButtonFromJson(int buttonId, JsonObject btn);
    void updateButtonFromParams(int buttonId, AsyncWebServerRequest *request);
    void updateButtonFromConfigurator(int buttonId, AsyncWebServerRequest *request);
};

extern WebServerManager webServerManager;

#endif // WEB_SERVER_MANAGER_H