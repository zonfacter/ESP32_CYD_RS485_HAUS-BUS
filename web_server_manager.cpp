/**
 * web_server_manager.cpp - SPIFFS-basierte Version
 * 
 * Verwendet SPIFFS für HTML-Dateien:
 * - Separate HTML/CSS/JS-Dateien in /data/ Verzeichnis
 * - Saubere Trennung von Code und Web-Content
 * - Einfache Wartung und Erweiterung
 */

#include "web_server_manager.h"
#include <SPIFFS.h>

WebServerManager webServerManager;

WebServerManager::WebServerManager() : server(80), serverRunning(false) {
}

void WebServerManager::begin() {
    if (serverRunning) return;
    
    // SPIFFS initialisieren
    if (!SPIFFS.begin(true)) {
        Serial.println("ERROR: SPIFFS Mount fehlgeschlagen");
        return;
    }
    
    Serial.println("DEBUG: SPIFFS erfolgreich initialisiert");
    
    setupRoutes();
    server.begin();
    serverRunning = true;
    
    Serial.println("DEBUG: SPIFFS Web-Server gestartet auf Port 80");
    Serial.println("DEBUG: Verfügbare Seiten:");
    Serial.println("  http://192.168.4.1/          - Dashboard");
    Serial.println("  http://192.168.4.1/config    - Konfiguration");
    Serial.println("  http://192.168.4.1/backup    - Backup/Restore");
    Serial.println("  http://192.168.4.1/files     - Datei-Manager");
    Serial.println("  http://192.168.4.1/logs      - System-Logs");
}

void WebServerManager::stop() {
    if (!serverRunning) return;
    
    server.stop();
    serverRunning = false;
    
    Serial.println("DEBUG: Web-Server gestoppt");
}

void WebServerManager::handleClient() {
    if (serverRunning) {
        server.handleClient();
    }
}

bool WebServerManager::isRunning() {
    return serverRunning;
}

void WebServerManager::setupRoutes() {
    // Statische Dateien aus SPIFFS servieren
    server.serveStatic("/", SPIFFS, "/www/");
    
    // Explizite HTML-Seiten
    server.on("/", [this]() { serveFile("/www/index.html"); });
    server.on("/index.html", [this]() { serveFile("/www/index.html"); });
    server.on("/config.html", [this]() { serveFile("/www/config.html"); });
    server.on("/backup.html", [this]() { serveFile("/www/backup.html"); });
    server.on("/files.html", [this]() { serveFile("/www/files.html"); });
    server.on("/logs.html", [this]() { serveFile("/www/logs.html"); });
    
    // CSS und JavaScript
    server.on("/style.css", [this]() { serveFile("/www/style.css", "text/css"); });
    server.on("/app.js", [this]() { serveFile("/www/app.js", "application/javascript"); });
    
    // API-Endpunkte für AJAX-Requests
    server.on("/api/status", HTTP_GET, [this]() { handleAPIStatus(); });
    server.on("/api/config", HTTP_GET, [this]() { handleAPIGetConfig(); });
    server.on("/api/config", HTTP_POST, [this]() { handleAPISaveConfig(); });
    server.on("/api/brightness", HTTP_POST, [this]() { handleAPISetBrightness(); });
    server.on("/api/orientation", HTTP_POST, [this]() { handleAPISetOrientation(); });
    server.on("/api/device-id", HTTP_POST, [this]() { handleAPISetDeviceID(); });
    server.on("/api/button", HTTP_POST, [this]() { handleAPIButtonControl(); });
    server.on("/api/backup/create", HTTP_POST, [this]() { handleAPICreateBackup(); });
    server.on("/api/backup/list", HTTP_GET, [this]() { handleAPIListBackups(); });
    server.on("/api/system/reboot", HTTP_POST, [this]() { handleAPIReboot(); });
    server.on("/api/system/reset", HTTP_POST, [this]() { handleAPIFactoryReset(); });
    
    // 404 Handler
    server.onNotFound([this]() { handleNotFound(); });
}

void WebServerManager::serveFile(const String& path, const String& contentType) {
    if (SPIFFS.exists(path)) {
        File file = SPIFFS.open(path, "r");
        if (file) {
            // Content-Type automatisch bestimmen wenn nicht angegeben
            String type = contentType;
            if (type.isEmpty()) {
                if (path.endsWith(".html")) type = "text/html";
                else if (path.endsWith(".css")) type = "text/css";
                else if (path.endsWith(".js")) type = "application/javascript";
                else if (path.endsWith(".json")) type = "application/json";
                else type = "text/plain";
            }
            
            server.streamFile(file, type);
            file.close();
            return;
        }
    }
    
    // Fallback: Einfache HTML-Seite generieren
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'><title>ESP32 Touch Panel</title>";
    html += "<style>body{font-family:Arial;margin:40px;background:#f0f0f0;}";
    html += ".card{background:white;padding:20px;border-radius:10px;margin:20px 0;}</style>";
    html += "</head><body>";
    html += "<div class='card'><h1>ESP32 Touch Panel</h1>";
    html += "<p><strong>Datei nicht gefunden:</strong> " + path + "</p>";
    html += "<p>SPIFFS-Dateien müssen erst hochgeladen werden.</p>";
    html += "<p><a href='/api/status'>API Status</a> | ";
    html += "<a href='/api/config'>API Config</a></p></div>";
    html += "</body></html>";
    
    server.send(404, "text/html", html);
}

void WebServerManager::handleNotFound() {
    String message = "Datei nicht gefunden\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET ? "GET" : "POST");
    message += "\nArguments: ";
    message += String(server.args());
    message += "\n";
    
    for (uint8_t i = 0; i < server.args(); i++) {
        message += " ";
        message += server.argName(i);
        message += ": ";
        message += server.arg(i);
        message += "\n";
    }
    
    server.send(404, "text/plain", message);
}

// *** API-HANDLER FÜR AJAX-REQUESTS ***

void WebServerManager::handleAPIStatus() {
    JsonDocument doc;
    
    // System-Informationen
    doc["system"]["uptime"] = millis() / 1000;
    doc["system"]["freeHeap"] = ESP.getFreeHeap();
    doc["system"]["chipModel"] = ESP.getChipModel();
    doc["system"]["chipRevision"] = ESP.getChipRevision();
    doc["system"]["flashSize"] = ESP.getFlashChipSize();
    
    // Device-Konfiguration
    doc["device"]["id"] = DEVICE_ID;
    doc["device"]["brightness"] = currentBacklight;
    doc["device"]["orientation"] = serviceManager.getOrientation();
    
    // WiFi-Status
    doc["wifi"]["mode"] = "AP";
    doc["wifi"]["ssid"] = "ESP32-ServiceMode";
    doc["wifi"]["ip"] = WiFi.softAPIP().toString();
    doc["wifi"]["clients"] = WiFi.softAPgetStationNum();
    
    // CSMA/CD-Statistiken (falls verfügbar)
    extern unsigned long totalSent;
    extern unsigned long totalCollisions;
    extern unsigned long totalRetries;
    
    doc["communication"]["totalSent"] = totalSent;
    doc["communication"]["totalCollisions"] = totalCollisions;
    doc["communication"]["totalRetries"] = totalRetries;
    
    sendJSON(doc, 200);
}

void WebServerManager::handleAPIGetConfig() {
    JsonDocument doc;
    
    doc["deviceID"] = serviceManager.getDeviceID();
    doc["orientation"] = serviceManager.getOrientation();
    doc["brightness"] = currentBacklight;
    doc["debugMode"] = (DB_INFO == 1);
    
    sendJSON(doc, 200);
}

void WebServerManager::handleAPISaveConfig() {
    if (server.hasArg("deviceID")) {
        String newID = server.arg("deviceID");
        if (newID.length() == 4) {
            serviceManager.setDeviceID(newID);
        }
    }
    
    if (server.hasArg("orientation")) {
        int orientation = server.arg("orientation").toInt();
        if (orientation == 0 || orientation == 1) {
            serviceManager.setOrientation(orientation);
        }
    }
    
    // Konfiguration speichern
    serviceManager.saveConfig();
    
    sendSuccess("Konfiguration gespeichert");
}

void WebServerManager::handleAPISetBrightness() {
    if (server.hasArg("value")) {
        int brightness = server.arg("value").toInt();
        brightness = constrain(brightness, 0, 100);
        
        setBacklight(brightness);
        sendSuccess("Helligkeit auf " + String(brightness) + "% gesetzt");
    } else {
        sendError("Parameter 'value' fehlt");
    }
}

void WebServerManager::handleAPISetOrientation() {
    if (server.hasArg("value")) {
        int orientation = server.arg("value").toInt();
        if (orientation == 0 || orientation == 1) {
            serviceManager.setOrientation(orientation);
            serviceManager.saveConfig();
            
            // Orientierung sofort anwenden
            if (orientation == LANDSCAPE) {
                tft.setRotation(1);
                touchscreen.setRotation(1);
            } else {
                tft.setRotation(0);
                touchscreen.setRotation(0);
            }
            
            sendSuccess("Orientierung geändert auf " + String(orientation == 0 ? "Portrait" : "Landscape"));
        } else {
            sendError("Ungültiger Orientierungswert (0 oder 1 erwartet)");
        }
    } else {
        sendError("Parameter 'value' fehlt");
    }
}

void WebServerManager::handleAPISetDeviceID() {
    if (server.hasArg("value")) {
        String newID = server.arg("value");
        if (newID.length() == 4) {
            serviceManager.setDeviceID(newID);
            serviceManager.saveConfig();
            sendSuccess("Device ID auf " + newID + " gesetzt");
        } else {
            sendError("Device ID muss 4-stellig sein");
        }
    } else {
        sendError("Parameter 'value' fehlt");
    }
}

void WebServerManager::handleAPIButtonControl() {
    if (server.hasArg("button") && server.hasArg("state")) {
        int buttonNum = server.arg("button").toInt();
        int state = server.arg("state").toInt();
        
        if (buttonNum >= 1 && buttonNum <= 6) {
            // LED-ID berechnen (Button 1-6 → LED 49-54)
            int ledID = 48 + buttonNum;
            
            // Telegramm senden
            if (state == 1) {
                sendTelegram("LED", String(ledID), "ON", "100");
            } else {
                sendTelegram("LED", String(ledID), "ON", "0");
            }
            
            sendSuccess("Button " + String(buttonNum) + " " + (state ? "aktiviert" : "deaktiviert"));
        } else {
            sendError("Ungültige Button-Nummer (1-6 erwartet)");
        }
    } else {
        sendError("Parameter 'button' und 'state' erforderlich");
    }
}

void WebServerManager::handleAPICreateBackup() {
    // Vereinfachtes Backup als JSON
    JsonDocument backup;
    
    backup["metadata"]["timestamp"] = millis();
    backup["metadata"]["version"] = "1.50";
    backup["metadata"]["type"] = "manual";
    
    backup["device"]["id"] = serviceManager.getDeviceID();
    backup["device"]["orientation"] = serviceManager.getOrientation();
    backup["device"]["brightness"] = currentBacklight;
    
    String filename = "/backup_" + String(millis()) + ".json";
    
    File backupFile = SPIFFS.open(filename, "w");
    if (backupFile) {
        serializeJson(backup, backupFile);
        backupFile.close();
        
        sendSuccess("Backup erstellt: " + filename);
    } else {
        sendError("Backup konnte nicht erstellt werden");
    }
}

void WebServerManager::handleAPIListBackups() {
    JsonDocument doc;
    JsonArray backups = doc["backups"].to<JsonArray>();
    
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    
    while (file) {
        String fileName = file.name();
        if (fileName.startsWith("/backup_") && fileName.endsWith(".json")) {
            JsonObject backup = backups.add<JsonObject>();
            backup["name"] = fileName;
            backup["size"] = file.size();
        }
        file = root.openNextFile();
    }
    
    sendJSON(doc, 200);
}

void WebServerManager::handleAPIReboot() {
    sendSuccess("System wird neu gestartet...");
    delay(1000);
    ESP.restart();
}

void WebServerManager::handleAPIFactoryReset() {
    // SPIFFS formatieren
    SPIFFS.format();
    
    // Service-Manager auf Defaults zurücksetzen
    serviceManager.setDeviceID("5999");
    serviceManager.setOrientation(LANDSCAPE);
    setBacklight(100);
    
    sendSuccess("Factory Reset durchgeführt. System wird neu gestartet...");
    delay(2000);
    ESP.restart();
}

// *** JSON-HILFSFUNKTIONEN ***

void WebServerManager::sendJSON(JsonDocument& doc, int httpCode) {
    String response;
    serializeJson(doc, response);
    server.send(httpCode, "application/json", response);
}

void WebServerManager::sendError(const String& message, int httpCode) {
    JsonDocument doc;
    doc["success"] = false;
    doc["message"] = message;
    sendJSON(doc, httpCode);
}

void WebServerManager::sendSuccess(const String& message) {
    JsonDocument doc;
    doc["success"] = true;
    doc["message"] = message;
    sendJSON(doc, 200);
}